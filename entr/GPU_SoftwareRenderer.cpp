#include "GPU.h"

void GPU::onVBlank()
{
	if (!swapBuffersPending)
		return;

	swapBuffersPending = false;
	render();

	memcpy(output, renderBuffer, 256 * 192 * sizeof(uint16_t));

	m_vertexCount = 0;
	m_polygonCount = 0;
	m_runningVtxCount = 0;
	m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, m_scheduler->getCurrentTimestamp() + 1);
}

void GPU::render()
{
	//can we speed up this loop somehow?
	//X=(X*200h)+((X+1)/8000h)*1FFh".
	uint32_t depth = clearDepth & 0x7FFF;
	depth = (depth * 0x200) + ((depth + 1) / 0x8000) * 0x1ff;
	uint16_t clearCol = clearColor & 0x7FFF;
	for (int y = 0; y < 192; y++)
	{
		for (int x = 0; x < 256; x++)
		{
			renderBuffer[(y * 256) + x] = 0x8000;	//todo: figure out alpha
			depthBuffer[(y * 256) + x] = 0xFFFFFFFFFFFFFFFF;
		}
	}

	for (int i = 0; i < m_polygonCount; i++)
	{
		Poly p = m_polygonRAM[i];
		bool draw = true;
		for (int j = 0; j < p.numVertices; j++)
		{
			Vertex cur = p.m_vertices[j];
			if (cur.v[3] > 0)					//only transform if vtx w component is 0.
			{
				int64_t screenX = (((cur.v[0] + cur.v[3]) * ((viewportX2-viewportX1)+1)) / (cur.v[3] << 1)) + viewportX1;
				int64_t screenY = (((-cur.v[1] + cur.v[3]) * ((viewportY2-viewportY1)+1)) / (cur.v[3] << 1)) + viewportY1;

				uint64_t z = ((((uint64_t)cur.v[2] << 14) / (int64_t)cur.v[3]) + 0x3FFF) << 9;

				Vertex v = {};
				v.v[0] = screenX;
				v.v[1] = screenY;
				v.v[2] = z;
				v.v[3] = cur.v[3];
				v.color = cur.color;
				v.texcoord[0] = cur.texcoord[0];
				v.texcoord[1] = cur.texcoord[1];
				p.m_vertices[j] = v;
			}
			else
				draw = false;
		}
		if (!draw || (p.attribs.mode==3))
			continue;
		rasterizePolygon(p);
		/*
		for (int i = 0; i < p.numVertices; i++)
		{
			Vertex v0 = p.m_vertices[i];
			Vertex v1 = p.m_vertices[(i + 1) % p.numVertices];
			debug_drawLine(v0.v[0], v0.v[1], v1.v[0], v1.v[1]);
		}
		*/
	}
}

void GPU::rasterizePolygon(Poly p)
{

	//find top and bottom points
	//while scanline count < bottom
	//left slope = top->top+1        (initially)
	//right slope = top->top-1       (initially)
	//xspan = xr-xl, draw scanline
	//if we reached end of any slope, find new slope
	//otherwise: interpolate x for each slope
	
	int smallY = 0x7FFFFFFF;
	int smallX = 0x7FFFFFFF;
	int topVtxIdx = 0;
	int largeY = 0xFFFFFFFF;
	int largeX = 0xFFFFFFFF;
	int bottomVtxIdx = 0;

	for (int i = 0; i < p.numVertices; i++)
	{
		if (p.m_vertices[i].v[1] <= smallY)
		{
			if (p.m_vertices[i].v[1] == smallY && p.m_vertices[i].v[0] > smallX)
				continue;
			smallX = p.m_vertices[i].v[0];
			smallY = p.m_vertices[i].v[1];
			topVtxIdx = i;
		}
		if (p.m_vertices[i].v[1] >= largeY)
		{
			if (p.m_vertices[i].v[1] == largeY && (int)p.m_vertices[i].v[0] < largeX)
				continue;
			largeY = p.m_vertices[i].v[1];
			largeX = p.m_vertices[i].v[0];
			bottomVtxIdx = i;
		}
	}

	Vertex l1 = {}, l2 = {};
	Vertex r1 = {}, r2 = {};

	int leftStep = 1;
	int rightStep = p.numVertices - 1;
	if (p.cw)
	{
		leftStep = p.numVertices - 1;
		rightStep = 1;
	}

	//todo: fix this up. assumes CCW winding order, could calculate winding order to determine order to walk edges on left/right
	int l2Idx = (topVtxIdx + leftStep) % p.numVertices;
	int r2Idx = (topVtxIdx + rightStep) % p.numVertices;

	l1 = p.m_vertices[topVtxIdx];
	l2 = p.m_vertices[l2Idx];
	r1 = p.m_vertices[topVtxIdx];
	r2 = p.m_vertices[r2Idx];

	int xMin = l1.v[0]; int xMax = r1.v[0];
	int y = smallY;

	//THIS CODE IS HORRIBLE
	//probs should doublecheck if this is okay, could cause some seams in rasterization?
	if (y >= l2.v[1])	//reached end of left slope
	{
		l1 = l2;
		l2Idx = (l2Idx + leftStep) % p.numVertices;
		l2 = p.m_vertices[l2Idx];
		xMin = l1.v[0];
	}
	if (y >= r2.v[1])
	{
		r1 = r2;
		r2Idx = (r2Idx + rightStep) % p.numVertices;
		r2 = p.m_vertices[r2Idx];
		xMax = r1.v[0];
	}

	//determine whether initial edges are x-major
	bool lEdgeXMajor = abs(l2.v[0] - l1.v[0]) > (l2.v[1] - l1.v[1]);
	bool rEdgeXMajor = abs(r2.v[0] - r1.v[0]) > (r2.v[1] - r1.v[1]);
	
	//clamp ymin,ymax so we don't draw insane polys
	//we probably have clipping bugs so this is necessary
	largeY = std::min(largeY, 192);
	y = std::max(0, y);
	while (y <= largeY)
	{
		int64_t wl = {}, wr = {};
		int32_t ul = {}, ur = {}, vl = {}, vr = {};
		uint16_t lcol = {}, rcol = {};

		//interpolate linearly if w values equal
		if (l1.v[3] == l2.v[3])
		{
			wl = linearInterpolate(y, l1.v[3], l2.v[3], l1.v[1], l2.v[1]);
			ul = linearInterpolate(y, l1.texcoord[0], l2.texcoord[0], l1.v[1], l2.v[1]);
			vl = linearInterpolate(y, l1.texcoord[1], l2.texcoord[1], l1.v[1], l2.v[1]);
			lcol = interpolateColor(y, l1.color, l2.color, l1.v[1], l2.v[1]);
		}
		else
		{
			uint64_t x1 = l1.v[1], x2 = l2.v[1], x = y;
			if (lEdgeXMajor)
			{
				x1 = l1.v[0]; x2 = l2.v[0]; x = xMin;
				if (x1 > x2)
				{
					uint64_t tmp = x1;
					x1 = x2;
					x2 = tmp;
					x = (x2 - x) + x1;
				}
			}
			uint64_t factorL = calcFactor((x2-x1) + 1, (x - x1), l1.v[3], l2.v[3], 9);
			wl = interpolatePerspectiveCorrect(factorL, 9, l1.v[3], l2.v[3]);
			ul = interpolatePerspectiveCorrect(factorL, 9, l1.texcoord[0], l2.texcoord[0]);
			vl = interpolatePerspectiveCorrect(factorL, 9, l1.texcoord[1], l2.texcoord[1]);
			lcol = interpolateColorPerspectiveCorrect(factorL, 9, l1.color, l2.color);
		}

		if (r1.v[3] == r2.v[3])
		{
			wr = linearInterpolate(y, r1.v[3], r2.v[3], r1.v[1], r2.v[1]);
			ur = linearInterpolate(y, r1.texcoord[0], r2.texcoord[0], r1.v[1], r2.v[1]);
			vr = linearInterpolate(y, r1.texcoord[1], r2.texcoord[1], r1.v[1], r2.v[1]);
			rcol = interpolateColor(y, r1.color, r2.color, r1.v[1], r2.v[1]);
		}
		else
		{
			uint64_t x1 = r1.v[1], x2 = r2.v[1], x = y;
			if (rEdgeXMajor)
			{
				x1 = r1.v[0], x2 = r2.v[0], x = xMax;
				if (x1 > x2)
				{
					uint64_t tmp = x1;
					x1 = x2;
					x2 = tmp;
					x = (x2 - x) + x1;
				}
			}
			uint64_t factorR = calcFactor((x2-x1) + 1, (x-x1), r1.v[3], r2.v[3], 9);
			wr = interpolatePerspectiveCorrect(factorR, 9, r1.v[3], r2.v[3]);
			ur = interpolatePerspectiveCorrect(factorR, 9, r1.texcoord[0], r2.texcoord[0]);
			vr = interpolatePerspectiveCorrect(factorR, 9, r1.texcoord[1], r2.texcoord[1]);
			rcol = interpolateColorPerspectiveCorrect(factorR, 9, r1.color, r2.color);
		}

		int64_t zl = linearInterpolate(y, l1.v[2], l2.v[2], l1.v[1], l2.v[1]);
		int64_t zr = linearInterpolate(y, r1.v[2], r2.v[2], r1.v[1], r2.v[1]);

		for (int x = std::max(xMin,0); x <= std::min(xMax,255); x++)
		{
			uint16_t col = {};
			//is interpolating z fine? or should we interpolate 1/z?
			int64_t z = linearInterpolate(x, zl, zr, xMin, xMax);
			int64_t w = {}, u = {}, v = {};
			if (wl == wr)
			{
				w = linearInterpolate(x, wl, wr, xMin, xMax);
				u = linearInterpolate(x, ul, ur, xMin, xMax);
				v = linearInterpolate(x, vl, vr, xMin, xMax);
				col = interpolateColor(x, lcol, rcol, xMin, xMax);
			}
			else
			{
				uint64_t factor = calcFactor((xMax - xMin) + 1, x - xMin, wl, wr, 8);
				w = interpolatePerspectiveCorrect(factor, 8, wl, wr);
				u = interpolatePerspectiveCorrect(factor, 8, ul, ur);
				v = interpolatePerspectiveCorrect(factor, 8, vl, vr);
				col = interpolateColorPerspectiveCorrect(factor, 8, lcol, rcol);
			}

			int64_t depth = (wBuffer) ? w : z;
			uint16_t texCol = decodeTexture(u, v, p.texParams);

			//should perhaps put this into 'decodeTexture'
			//should also account for different modes, e.g. this assumes modulation
			if (!(texCol >> 15))
			{
				/*
				*   R = ((Rt+1)*(Rv+1)-1)/64
					G = ((Gt+1)*(Gv+1)-1)/64
					B = ((Bt+1)*(Bv+1)-1)/64
					A = ((At+1)*(Av+1)-1)/64

				*/
				uint32_t r1 = (texCol & 0x1F), g1 = (texCol >> 5) & 0x1F, b1 = (texCol >> 10) & 0x1F;
				uint32_t r2 = (col & 0x1F), g2 = (col >> 5) & 0x1F, b2 = (col >> 10) & 0x1F;
				uint32_t R = ((r1 + 1) * (r2 + 1) - 1) / 32;
				uint32_t G = ((g1 + 1) * (g2 + 1) - 1) / 32;
				uint32_t B = ((b1 + 1) * (b2 + 1) - 1) / 32;
				col = (R & 0x1F) | ((G & 0x1F) << 5) | ((B & 0x1F) << 10);
			}
			//bad bad hack. need to do proper alpha blending
			else
				col = 0x8000;

			if (y >= 0 && y < 192 && x>=0 && x < 256 && !(col>>15))
			{
				if (depth < depthBuffer[(y * 256) + x])
				{
					depthBuffer[(y * 256) + x] = depth;
					renderBuffer[(y * 256) + x] = col & 0x7FFF;	
				}
			}
		}

		//advance next scanline
		y++;

		//this is in dire need of a cleanup
		//todo: proper x-major interpolation for poly edges
		if (y >= l2.v[1])	//reached end of left slope
		{
			l1 = l2;
			l2Idx = (l2Idx + leftStep) % p.numVertices;
			l2 = p.m_vertices[l2Idx];
			xMin = l1.v[0];
			
			lEdgeXMajor = abs(l2.v[0] - l1.v[0]) > (l2.v[1] - l1.v[1]);
		}
		else
			xMin = linearInterpolate(y, l1.v[0], l2.v[0], l1.v[1], l2.v[1]);
		if (y >= r2.v[1])
		{
			r1 = r2;
			r2Idx = (r2Idx + rightStep) % p.numVertices;
			r2 = p.m_vertices[r2Idx];
			xMax = r1.v[0];

			rEdgeXMajor = abs(r2.v[0] - r1.v[0]) > (r2.v[1] - r1.v[1]);
		}
		else
			xMax = linearInterpolate(y, r1.v[0], r2.v[0], r1.v[1], r2.v[1]);
	}
}

uint16_t GPU::decodeTexture(int32_t u, int32_t v, TextureParameters params)
{
	//fixed point -> integer
	u >>= 4; v >>= 4;
	
	if (params.repeatX)
	{
		if (params.flipX && (u & params.sizeX))
			u = -1 - u;

		u &= (params.sizeX - 1);
	}
	else
	{
		u = std::max(u, 0);
		u = std::min(u, (int32_t)params.sizeX-1);
	}
	if (params.repeatY)
	{
		if (params.flipY && (v & params.sizeY))
			v = -1 - v;

		v &= (params.sizeY - 1);
	}
	else
	{
		v = std::max(v, 0);
		v = std::min(v, (int32_t)params.sizeY-1);
	}

	uint16_t col = 0x7FFF;

	if (params.format == 2)
		params.paletteBase <<= 3;
	else
		params.paletteBase <<= 4;

	//lots of code duplication here, could cleanup
	switch (params.format)
	{
	case Tex_A3I5:
	{
		uint32_t offs = ((params.sizeX * v) + u);
		//todo: alpha
		uint32_t byte = gpuReadTex(params.VRAMOffs + offs);
		uint32_t palIndex = byte & 0x1F;
		if (!(byte & 0xE0))	//hacky, need proper alpha blending for these textures
			return 0x8000;
		uint32_t palAddr = (palIndex * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	//mode 2 seems broken, need to revisit (probs something silly)
	case Tex_Palette4Color:
	{
		uint32_t offs = ((params.sizeX * v) + u);
		uint32_t byte = gpuReadTex(params.VRAMOffs + (offs>>2));
		byte >>= (2 * (offs & 0b11));
		byte &= 0b11;
		if (params.col0Transparent && !byte)
			return 0x8000;
		uint32_t palAddr = (byte * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	case Tex_Palette16Color:
	{
		//get base addr for texel
		uint32_t offs = ((params.sizeX * v) + u);
		uint32_t byte = gpuReadTex(params.VRAMOffs + (offs>>1));
		if (offs & 0b1)
			byte >>= 4;
		byte &= 0xF;
		if (params.col0Transparent && !byte)
			return 0x8000;
		uint32_t palAddr = (byte * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	case Tex_Palette256Color:
	{
		uint32_t offs = (params.sizeX * v) + u;
		uint32_t byte = gpuReadTex(params.VRAMOffs + offs);
		if (params.col0Transparent && !byte)
			return 0x8000;
		uint32_t palAddr = (byte * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	case Tex_Compressed:
	{
		//get 4x4 tile offset
		uint32_t tileIdx = ((params.sizeX >> 2) * (v >> 2)) + (u >> 2);
		uint32_t tileAddr = (tileIdx << 2) + (v & 3);
		uint32_t byte = gpuReadTex(params.VRAMOffs + tileAddr);
		byte >>= ((u & 3) << 1);
		byte &= 0b11;

		uint32_t dataSlotIdx = (params.VRAMOffs + tileAddr) >> 18;
		uint32_t dataSlotOffs = (params.VRAMOffs + tileAddr) & 0x1FFFF;
		uint32_t palinfo = 0x20000 + (dataSlotOffs >> 1);
		if (dataSlotIdx == 1)
			palinfo += 0x10000;
		palinfo &= ~0b1;

		uint8_t palOffsLow = gpuReadTex(palinfo);
		uint8_t palOffsHigh = gpuReadTex(palinfo + 1);
		uint32_t palOffs = ((palOffsHigh << 8) | palOffsLow) & 0x3FFF;
		palOffs = (palOffs << 2);
		uint32_t palAddr = params.paletteBase + palOffs + (byte * 2);
		uint8_t mode = (palOffsHigh >> 6) & 0b11;
		switch (mode)
		{
		case 0:
		{
			if (byte == 3)
				return 0x8000;
			col = gpuReadPal16(palAddr);
			break;
		}
		case 1:
		{
			if (byte == 3)
				return 0x8000;
			if (byte == 2)
			{
				uint16_t col0 = gpuReadPal16(params.paletteBase + palOffs);
				uint16_t col1 = gpuReadPal16(params.paletteBase + palOffs + 2);
				uint32_t r0 = col0 & 0x1F, g0 = (col0 >> 5) & 0x1F, b0 = (col0 >> 10) & 0x1F;
				uint32_t r1 = col1 & 0x1F, g1 = (col1 >> 5) & 0x1F, b1 = (col1 >> 10) & 0x1F;
				uint32_t r = (r0 + r1) >> 1, g = (g0 + g1) >> 1, b = (b0 + b1) >> 1;
				col = (r & 0x1F) | ((g & 0x1F) << 5) | ((b & 0x1F) << 10);
			}
			else
				col = gpuReadPal16(palAddr);
			break;
		}
		case 2:
		{
			col = gpuReadPal16(palAddr);
			break;
		}
		case 3:
		{
			uint16_t col0 = gpuReadPal16(params.paletteBase + palOffs);
			uint16_t col1 = gpuReadPal16(params.paletteBase + palOffs + 2);
			uint32_t r0 = col0 & 0x1F, g0 = (col0 >> 5) & 0x1F, b0 = (col0 >> 10) & 0x1F;
			uint32_t r1 = col1 & 0x1F, g1 = (col1 >> 5) & 0x1F, b1 = (col1 >> 10) & 0x1F;
			if (byte == 2)
			{
				uint32_t r = ((r0 * 5) + (r1 * 3)) >> 3, g = ((g0 * 5) + (g1 * 3)) >> 3, b = ((b0 * 5) + (b1 * 3)) >> 3;
				col = (r & 0x1F) | ((g & 0x1F) << 5) | ((b & 0x1F) << 10);
			}
			else if (byte == 3)
			{
				uint32_t r = ((r0 * 3) + (r1 * 5)) >> 3, g = ((g0 * 3) + (g1 * 5)) >> 3, b = ((b0 * 3) + (b1 * 5)) >> 3;
				col = (r & 0x1F) | ((g & 0x1F) << 5) | ((b & 0x1F) << 10);
			}
			else
			{
				col = gpuReadPal16(palAddr);
			}
			break;
		}
		}
		break;
	}
	case Tex_A5I3:
	{
		uint32_t offs = ((params.sizeX * v) + u);
		//todo: alpha
		uint32_t byte = gpuReadTex(params.VRAMOffs + offs);
		uint8_t palIndex = byte & 0b111;
		if (!(byte & 0xF8))	//hacky, need to handle alpha properly.
			return 0x8000;
		uint32_t palAddr = (palIndex * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	case Tex_DirectColor:
	{
		uint32_t offs = (params.sizeX * v * 2) + u * 2;
		uint8_t colLow = gpuReadTex(params.VRAMOffs + offs);
		uint8_t colHigh = gpuReadTex(params.VRAMOffs + offs + 1);
		col = (colHigh << 8) | colLow;
		break;
	}
	}

	return col & 0x7FFF;
}

void GPU::debug_drawLine(int x0, int y0, int x1, int y1)
{
	if (abs(y1 - y0) < abs(x1 - x0))
	{
		if (x0 > x1)
			plotLow(x1, y1, x0, y0);
		else
			plotLow(x0, y0, x1, y1);
	}
	else
	{
		if (y0 > y1)
			plotHigh(x1, y1, x0, y0);
		else
			plotHigh(x0, y0, x1, y1);
	}
}

void GPU::plotLow(int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;
	if (dy < 0)
	{
		yi = -1;
		dy = -dy;
	}

	int D = (2 * dy) - dx;
	int y = y0;

	for (int x = x0; x < x1; x++)
	{
		if ((x >= 0 && x < 256) && (y >= 0 && y < 192))
			renderBuffer[(256 * y) + x] = 0x001F;
		if (D > 0)
		{
			y += yi;
			D = D + (2 * (dy - dx));
		}
		else
			D = D + (2 * dy);
	}
}

void GPU::plotHigh(int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;
	if (dx < 0)
	{
		xi = -1;
		dx = -dx;
	}
	int D = (2 * dx) - dy;
	int x = x0;

	for (int y = y0; y < y1; y++)
	{
		if ((x >= 0 && x < 256) && (y >= 0 && y < 192))
			renderBuffer[(256 * y) + x] = 0x001F;
		if (D > 0)
		{
			x += xi;
			D = D + (2 * (dx - dy));
		}
		else
			D = D + (2 * dx);
	}
}