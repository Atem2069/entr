#include "GPU.h"
#include <algorithm>

void GPU::onVBlank()
{
	if (!swapBuffersPending)
		return;

	swapBuffersPending = false;

	//sort polygons into opaque/translucent, so translucent polygons are rendered last.
	//todo: sort by y too.
	//opaque polygons seem to always be sorted by y, and then translucent ones are sorted depending on SWAP_BUFFERS.0
	auto translucencyCriteria = [](const Poly& p) {return (p.attribs.alpha && p.attribs.alpha < 31)
		|| ((p.texParams.format == 1 || p.texParams.format == 6) && p.attribs.mode == 0); };
	std::stable_sort(m_polygonRAM[bufIdx], m_polygonRAM[bufIdx] + m_polygonCount, [translucencyCriteria](const Poly& a, const Poly& b) {return !translucencyCriteria(a) && translucencyCriteria(b); });

	bufIdx = !bufIdx;
	m_renderPolygonCount = m_polygonCount;

	if (Config::NDS.numGPUThreads != numThreads)
	{
		destroyWorkerThreads();

		numThreads = Config::NDS.numGPUThreads;
		linesPerThread = 192 / numThreads;

		createWorkerThreads();
	}


	for (int i = 0; i < numThreads; i++)
	{
		m_workerThreads[i].rendering = true;
		m_workerThreads[i].cv.notify_one();
	}

	renderInProgress = true;

	m_vertexCount = 0;
	m_polygonCount = 0;
	m_runningVtxCount = 0;
	m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, m_scheduler->getCurrentTimestamp() + 1);
}

void GPU::onSync(int threadId)
{
	if (!renderInProgress || threadId >= numThreads)
		return;

	while (m_workerThreads[threadId].rendering) {}

	uint32_t start = (threadId * linesPerThread) * 256;
	//memcpy(&output[start], &renderBuffer[start], 256 * linesPerThread * sizeof(uint16_t));
	memcpy(&output.output[start], &renderBuffer.output[start], 256 * linesPerThread * sizeof(uint16_t));
	memcpy(&output.alpha[start], &renderBuffer.alpha[start], 256 * linesPerThread * sizeof(uint16_t));

	if (threadId == (numThreads - 1))
		renderInProgress = false;
}

void GPU::render(int yMin, int yMax)
{
	uint16_t clearCol = clearColor | 0x8000;
	uint16_t clearAlpha = (clearColor >> 16) & 0x1F;
	RenderAttribute clearAttr = { 0,0x00FFFFFF,0 };
	std::fill(renderBuffer.output+(256*yMin), renderBuffer.output + (256 * (yMax+1)), clearCol);
	std::fill(renderBuffer.alpha + (256 * yMin), renderBuffer.alpha + (256 * (yMax + 1)), clearAlpha);
	std::fill(attributeBuffer + (256 * yMin), attributeBuffer + (256 * (yMax + 1)), clearAttr);
	std::fill(stencilBuffer + (256 * yMin), stencilBuffer + (256 * (yMax + 1)), 0);

	for (uint32_t i = 0; i < m_renderPolygonCount; i++)
	{
		Poly p = m_polygonRAM[!bufIdx][i];

		//skip render if poly is completely out of thread's 'render area'
		bool skipRender = (p.yTop < yMin&& p.yBottom < yMin) || (p.yTop > yMax && p.yBottom > yMax);
		if (!p.drawable || skipRender)
			continue;

		rasterizePolygon(p, yMin, yMax);
	}
}

void GPU::rasterizePolygon(Poly p, int yMin, int yMax)
{
	int smallY = p.yTop, largeY = p.yBottom;
	int topVtxIdx = p.topVtxIdx;

	Vertex l1 = {}, l2 = {};
	Vertex r1 = {}, r2 = {};
	EdgeAttribs e[2] = {};

	int leftStep = 1;
	int rightStep = p.numVertices - 1;
	if (p.cw)
	{
		leftStep = p.numVertices - 1;
		rightStep = 1;
	}

	int l2Idx = (topVtxIdx + leftStep) % p.numVertices;
	int r2Idx = (topVtxIdx + rightStep) % p.numVertices;

	l1 = p.m_vertices[topVtxIdx];
	l2 = p.m_vertices[l2Idx];
	r1 = p.m_vertices[topVtxIdx];
	r2 = p.m_vertices[r2Idx];

	int64_t lEdgeMin = {}, lEdgeMax = {};
	int64_t rEdgeMin = {}, rEdgeMax = {};

	//determine whether initial edges are x-major
	bool lEdgeXMajor = abs(l2.v[0] - l1.v[0]) > (l2.v[1] - l1.v[1]);
	bool rEdgeXMajor = abs(r2.v[0] - r1.v[0]) > (r2.v[1] - r1.v[1]);
	
	int y = smallY;
	largeY = std::min(largeY, yMax+1);
	y = std::max(0, y);

	//hacky: allow one-pixel tall polys to render
	if (p.yTop == p.yBottom)
		largeY++;
	while (y < largeY)
	{
		if (y >= l2.v[1])
		{
			l1 = l2;
			l2Idx = (l2Idx + leftStep) % p.numVertices;
			l2 = p.m_vertices[l2Idx];
			lEdgeXMajor = abs(l2.v[0] - l1.v[0]) > (l2.v[1] - l1.v[1]);
			lEdgeMin = lEdgeMax = l1.v[0];
		}
		interpolateEdge(y, l1.v[0], l2.v[0], l1.v[1], l2.v[1], lEdgeMin, lEdgeMax);

		if (y >= r2.v[1])
		{
			r1 = r2;
			r2Idx = (r2Idx + rightStep) % p.numVertices;
			r2 = p.m_vertices[r2Idx];
			rEdgeXMajor = abs(r2.v[0] - r1.v[0]) > (r2.v[1] - r1.v[1]);
			rEdgeMin = rEdgeMax = r1.v[0];
		}
		interpolateEdge(y, r1.v[0], r2.v[0], r1.v[1], r2.v[1], rEdgeMin, rEdgeMax);

		//interpolate linearly if w values equal
		if (l1.v[3] == l2.v[3])
		{
			e[0].w = linearInterpolate(y, l1.v[3], l2.v[3], l1.v[1], l2.v[1]);
			e[0].u = linearInterpolate(y, l1.texcoord[0], l2.texcoord[0], l1.v[1], l2.v[1]);
			e[0].v = linearInterpolate(y, l1.texcoord[1], l2.texcoord[1], l1.v[1], l2.v[1]);
			e[0].col = interpolateColor(y, l1.color, l2.color, l1.v[1], l2.v[1]);
		}
		else
		{
			uint64_t x1 = l1.v[1], x2 = l2.v[1], x = y;
			if (lEdgeXMajor)
			{
				x1 = l1.v[0]; x2 = l2.v[0]; x = lEdgeMin;
				if (x1 > x2)
				{
					uint64_t tmp = x1;
					x1 = x2;
					x2 = tmp;
					x = (x2 - x) + x1;
				}
			}
			uint64_t factorL = calcFactor((x2-x1) + 1, (x - x1), l1.v[3], l2.v[3], 9);
			e[0].w = interpolatePerspectiveCorrect(factorL, 9, l1.v[3], l2.v[3]);
			e[0].u = interpolatePerspectiveCorrect(factorL, 9, l1.texcoord[0], l2.texcoord[0]);
			e[0].v = interpolatePerspectiveCorrect(factorL, 9, l1.texcoord[1], l2.texcoord[1]);
			e[0].col = interpolateColorPerspectiveCorrect(factorL, 9, l1.color, l2.color);
		}

		if (r1.v[3] == r2.v[3])
		{
			e[1].w = linearInterpolate(y, r1.v[3], r2.v[3], r1.v[1], r2.v[1]);
			e[1].u = linearInterpolate(y, r1.texcoord[0], r2.texcoord[0], r1.v[1], r2.v[1]);
			e[1].v = linearInterpolate(y, r1.texcoord[1], r2.texcoord[1], r1.v[1], r2.v[1]);
			e[1].col = interpolateColor(y, r1.color, r2.color, r1.v[1], r2.v[1]);
		}
		else
		{
			uint64_t x1 = r1.v[1], x2 = r2.v[1], x = y;
			if (rEdgeXMajor)
			{
				x1 = r1.v[0], x2 = r2.v[0], x = rEdgeMax;
				if (x1 > x2)
				{
					uint64_t tmp = x1;
					x1 = x2;
					x2 = tmp;
					x = (x2 - x) + x1;
				}
			}
			uint64_t factorR = calcFactor((x2-x1) + 1, (x-x1), r1.v[3], r2.v[3], 9);
			e[1].w = interpolatePerspectiveCorrect(factorR, 9, r1.v[3], r2.v[3]);
			e[1].u = interpolatePerspectiveCorrect(factorR, 9, r1.texcoord[0], r2.texcoord[0]);
			e[1].v = interpolatePerspectiveCorrect(factorR, 9, r1.texcoord[1], r2.texcoord[1]);
			e[1].col = interpolateColorPerspectiveCorrect(factorR, 9, r1.color, r2.color);
		}

		e[0].z = linearInterpolate(y, l1.v[2], l2.v[2], l1.v[1], l2.v[1]);
		e[1].z = linearInterpolate(y, r1.v[2], r2.v[2], r1.v[1], r2.v[1]);

		bool rEdgeVertical = (r1.v[0] == r2.v[0]);
		bool lEdgeNegative = (l1.v[0] > l2.v[0]);
		bool rEdgePositive = (r1.v[0] < r2.v[0]);
		bool lxm = lEdgeXMajor, rxm = rEdgeXMajor;
		//crossed poly: swap attribs
		if (lEdgeMin > rEdgeMin)
		{
			std::swap(lEdgeMin, rEdgeMin);
			std::swap(lEdgeMax, rEdgeMax);
			std::swap(e[0], e[1]);
			std::swap(lxm, rxm);
			rEdgeVertical = (l1.v[0] == l2.v[0]);
			lEdgeNegative = (r1.v[0] > r2.v[0]);
			rEdgePositive = (l1.v[0] < l2.v[0]);
		}

		//there are still some slightly buggy edges, but this should be fairly accurate (for opaque polygons)

		//not sure if this is accurate.
		bool forceFillEdge = (y == 191) || ((DISP3DCNT >> 4) & 0b1) || ((DISP3DCNT >> 5) & 0b1) || p.attribs.alpha<31;
		if (!lxm || lEdgeNegative || forceFillEdge)
		{
			renderSpan(p, lEdgeMin, lEdgeMax, y, yMin, true, lEdgeMin, rEdgeMax, e);
		}

		renderSpan(p, lEdgeMax + 1, rEdgeMin - 1, y, yMin, false, lEdgeMin, rEdgeMax, e);

		if ((rxm && rEdgePositive) || rEdgeVertical || forceFillEdge)
		{
			if (rEdgeVertical && r1.v[0] != 255)
				rEdgeMax--;
			renderSpan(p, rEdgeMin, rEdgeMax, y, yMin, true, lEdgeMin, rEdgeMax, e);
		}

		//advance next scanline
		y++;
	}
}

//woah this is a messy function call
void GPU::renderSpan(Poly& p, int xMin, int xMax, int y, int yMin, bool edge, int spanMin, int spanMax, EdgeAttribs* e)
{
	for (int x = std::max(xMin, 0); x <= std::min(xMax, 255); x++)
	{
		ColorRGBA5 col = {};
		//is interpolating z fine? or should we interpolate 1/z?
		int64_t z = linearInterpolate(x, e[0].z, e[1].z, spanMin, spanMax);
		int64_t w = {}, u = {}, v = {};
		if (e[0].w == e[1].w)
		{
			w = linearInterpolate(x, e[0].w, e[1].w, spanMin, spanMax);
			u = linearInterpolate(x, e[0].u, e[1].u, spanMin, spanMax);
			v = linearInterpolate(x, e[0].v, e[1].v, spanMin, spanMax);
			col = interpolateColor(x, e[0].col, e[1].col, spanMin, spanMax);
		}
		else
		{
			uint64_t factor = calcFactor((spanMax - spanMin) + 1, x - spanMin, e[0].w, e[1].w, 8);
			w = interpolatePerspectiveCorrect(factor, 8, e[0].w, e[1].w);
			u = interpolatePerspectiveCorrect(factor, 8, e[0].u, e[1].u);
			v = interpolatePerspectiveCorrect(factor, 8, e[0].v, e[1].v);
			col = interpolateColorPerspectiveCorrect(factor, 8, e[0].col, e[1].col);
		}

		int64_t depth = (wBuffer) ? w : z;
		ColorRGBA5 texCol = {};
		if (p.texParams.format)
			texCol = decodeTexture((int32_t)u, (int32_t)v, p.texParams);

		//this sort of wastes time. could just walk edges until we reach yMin and then start rendering
		if (y >= yMin)
		{
			if (edge)
				attributeBuffer[(y * 256) + x].flags |= PixelFlags_Edge;
			plotPixel(x, y, depth, col, texCol, p.attribs, p.texParams.format == 0);
		}
	}
}

void GPU::plotPixel(int x, int y, uint64_t depth, ColorRGBA5 polyCol, ColorRGBA5 texCol, PolyAttributes& attributes, bool noTexture)
{
	ColorRGBA5 toonCol = {};
	toonCol.fromUint(m_toonTable[polyCol.r]);
	ColorRGBA5 output = {};
	if (noTexture)
	{
		output = polyCol;
		//toon/highlight blending with no texture
		if (attributes.mode == 2)
		{
			//highlight
			if ((DISP3DCNT >> 1) & 0b1)
			{
				output.r = std::min(polyCol.r + toonCol.r, 31);
				output.g = std::min(polyCol.g + toonCol.g, 31);
				output.b = std::min(polyCol.b + toonCol.b, 31);
				output.a = polyCol.a;
			}
			//toon
			else
			{
				output = toonCol;
				output.a = polyCol.a;
			}
		}
	}
	else
	{
		switch (attributes.mode)
		{
		case 0:
		{
			uint32_t R = (((uint32_t)texCol.r + 1) * ((uint32_t)polyCol.r + 1) - 1) / 32;
			uint32_t G = (((uint32_t)texCol.g + 1) * ((uint32_t)polyCol.g + 1) - 1) / 32;
			uint32_t B = (((uint32_t)texCol.b + 1) * ((uint32_t)polyCol.b + 1) - 1) / 32;
			uint32_t A = (((uint32_t)texCol.a + 1) * ((uint32_t)polyCol.a + 1) - 1) / 32;
			output.r = R & 0x1F;
			output.g = G & 0x1F;
			output.b = B & 0x1F;
			output.a = A & 0x1F;
			break;
		}
		case 1:
		{
			if (!texCol.a)
				output = polyCol;
			else if (texCol.a == 31)
				output = texCol;
			else
			{
				uint32_t R = (texCol.r * texCol.a + polyCol.r * (31 - texCol.a)) / 32;
				uint32_t G = (texCol.g * texCol.a + polyCol.g * (31 - texCol.a)) / 32;
				uint32_t B = (texCol.b * texCol.a + polyCol.b * (31 - texCol.a)) / 32;
				output.r = R & 0x1F;
				output.g = G & 0x1F;
				output.b = B & 0x1F;
				output.a = polyCol.a;
			}
			break;
		}
		case 2:
		{
			uint32_t R = {}, G = {}, B = {}, A = {};
			//highlight
			if ((DISP3DCNT >> 1) & 0b1)
			{
				if (texCol.a)
				{
					R = std::min((((texCol.r + 1) * (polyCol.r + 1) - 1) / 32) + toonCol.r, 31);
					G = std::min((((texCol.g + 1) * (polyCol.g + 1) - 1) / 32) + toonCol.g, 31);
					B = std::min((((texCol.b + 1) * (polyCol.b + 1) - 1) / 32) + toonCol.b, 31);
					A = ((texCol.a + 1) * (polyCol.a + 1) - 1) / 32;
				}
			}
			//toon
			else
			{
				R = ((texCol.r + 1) * (toonCol.r + 1) - 1) / 32;
				G = ((texCol.g + 1) * (toonCol.g + 1) - 1) / 32;
				B = ((texCol.b + 1) * (toonCol.b + 1) - 1) / 32;
				A = ((texCol.a + 1) * (polyCol.a + 1) - 1) / 32;
			}
			output.r = R & 0x1F;
			output.g = G & 0x1F;
			output.b = B & 0x1F;
			output.a = A & 0x1F;
			break;
		}
		}
	}

	RenderAttribute pixelAttribs = attributeBuffer[(y * 256) + x];
	uint16_t curCol = renderBuffer.output[(y * 256) + x];
	uint16_t curAlpha = renderBuffer.alpha[(y * 256) + x];
	ColorRGBA5 fbCol = { curCol & 0x1F, (curCol >> 5) & 0x1F, (curCol >> 10) & 0x1F, curAlpha&0x1F };

	if (output.a && output.a < 31)
	{
		//don't draw translucent pixel if fb.translucent and fb.polyid==curpixel.polyid
		if ((pixelAttribs.flags & PixelFlags_Translucent) && (pixelAttribs.polyID == attributes.polyID))
			return;
		pixelAttribs.flags |= PixelFlags_Translucent;
	}
	else
		pixelAttribs.flags &= ~PixelFlags_Translucent;

	if (((DISP3DCNT>>3)&0b1) && (output.a != 31 && output.a && fbCol.a))
	{
		output.r = ((output.r * (output.a + 1) + fbCol.r * (31 - output.a)) / 32) & 0x1F;
		output.g = ((output.g * (output.a + 1) + fbCol.g * (31 - output.a)) / 32) & 0x1F;
		output.b = ((output.b * (output.a + 1) + fbCol.b * (31 - output.a)) / 32) & 0x1F;
		output.a = std::max(output.a, fbCol.a);
	}

	if (!attributes.alpha && (pixelAttribs.flags & PixelFlags_Edge))
		output.a = 31;

	uint16_t res = output.toUint();
	if ((!(res >> 15)) && depth < pixelAttribs.depth)
	{
		//is this check true?
		if (attributes.mode == 3 && ((attributes.polyID && !stencilBuffer[(y * 256) + x]) || !attributes.polyID || attributes.polyID==pixelAttribs.polyID))
			return;

		//weird check, translucent pixels only update depth when POLYGON_ATTR.11 set
		if (((pixelAttribs.flags & PixelFlags_Translucent) && attributes.updateTranslucentDepth) || !(pixelAttribs.flags & PixelFlags_Translucent))
			pixelAttribs.depth = depth;
		pixelAttribs.polyID = attributes.polyID;
		attributeBuffer[(y * 256) + x] = pixelAttribs;

		stencilBuffer[(y * 256) + x] = 0;
		renderBuffer.output[(y * 256) + x] = res;
		renderBuffer.alpha[(y * 256) + x] = output.a;
	}

	//shadow poly with polyid.0 sets stencil bits when depth test fails
	if (depth >= pixelAttribs.depth && attributes.mode == 3 && !attributes.polyID)
		stencilBuffer[(y * 256) + x] = 1;
}

ColorRGBA5 GPU::decodeTexture(int32_t u, int32_t v, TextureParameters params)
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
	uint16_t alpha = 31;

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
		uint32_t byte = gpuReadTex(params.VRAMOffs + offs);
		uint32_t palIndex = byte & 0x1F;
		alpha = (byte >> 5) & 0b111;
		alpha = (alpha << 2) + (alpha >> 1);
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
			return { 0,0,0,0 };
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
			return { 0,0,0,0 };
		uint32_t palAddr = (byte * 2) + params.paletteBase;
		col = gpuReadPal16(palAddr);
		break;
	}
	case Tex_Palette256Color:
	{
		uint32_t offs = (params.sizeX * v) + u;
		uint32_t byte = gpuReadTex(params.VRAMOffs + offs);
		if (params.col0Transparent && !byte)
			return { 0,0,0,0 };
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
				return { 0,0,0,0 };
			col = gpuReadPal16(palAddr);
			break;
		}
		case 1:
		{
			if (byte == 3)
				return { 0,0,0,0 };
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
		alpha = (byte >> 3) & 0x1F;
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

	ColorRGBA5 out = {};
	out.r = col & 0x1F;
	out.g = (col >> 5) & 0x1F;
	out.b = (col >> 10) & 0x1F;
	out.a = alpha;
	return out;
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
			renderBuffer.output[(256 * y) + x] = 0x001F;
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
			renderBuffer.output[(256 * y) + x] = 0x001F;
		if (D > 0)
		{
			x += xi;
			D = D + (2 * (dx - dy));
		}
		else
			D = D + (2 * dx);
	}
}