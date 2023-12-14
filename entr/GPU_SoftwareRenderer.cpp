#include "GPU.h"

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
			depthBuffer[(y * 256) + x] = depth;
		}
	}

	for (int i = 0; i < m_polygonCount; i++)
	{
		Poly p = m_polygonRAM[i];
		bool draw = true;
		for (int j = 0; j < p.numVertices; j++)
		{
			Vector cur = p.m_vertices[j];
			Vector vec = cur;
			if (cur.v[3] > 0)					//hack to prevent div by 0, also cull polygon if w less than 0.
			{
				int64_t screenX = fixedPointMul((cur.v[0] + cur.v[3]), 256 << 12);
				int64_t screenY = fixedPointMul((cur.v[1] + cur.v[3]), 192 << 12);
				screenX = fixedPointDiv(screenX, (cur.v[3] << 1)) >> 12;
				screenY = 192 - (fixedPointDiv(screenY, (cur.v[3] << 1)) >> 12);

				// (((Z * 0x4000 / W) + 0x3FFF) * 0x200) & 0xFFFFFF
				int64_t z = (((cur.v[2] * 0x4000 / cur.v[3]) + 0x3FFF) * 0x200) & 0xFFFFFF;

				Vector v = {};
				v.v[0] = screenX;
				v.v[1] = screenY;
				v.v[2] = z;
				v.color = cur.color;
				p.m_vertices[j] = v;
			}
			else
				draw = false;
		}
		if (!draw)
			continue;
		rasterizePolygon(p);
		/*
		for (int i = 0; i < p.numVertices; i++)
		{
			Vector v0 = p.m_vertices[i];
			Vector v1 = p.m_vertices[(i + 1) % p.numVertices];
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

	//hacky
	//if (smallY < 0 || smallY >= 192 || largeY < 0 || largeY >= 192)
	//	return;

	Vector l1 = {}, l2 = {};
	Vector r1 = {}, r2 = {};

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
	
	//clamp ymin,ymax so we don't draw insane polys
	//we probably have clipping bugs so this is necessary
	largeY = std::min(largeY, 192);
	y = std::max(0, y);
	while (y < largeY)
	{
		//rasterize
		//same for xmin,xmax
		xMin = std::max(xMin, 0);
		xMax = std::min(xMax, 255);
		for (int x = xMin; x <= xMax; x++)
		{
			//interpolate color in y, then x
			uint16_t lcol = interpolateColor(y, l1.color, l2.color, l1.v[1], l2.v[1]);
			uint16_t rcol = interpolateColor(y, r1.color, r2.color, r1.v[1], r2.v[1]);
			//interpolate in x
			uint16_t col = interpolateColor(x, lcol, rcol, xMin, xMax);

			//is interpolating z fine? or should we interpolate 1/z?
			int32_t zl = linearInterpolate(y, l1.v[2], l2.v[2], l1.v[1], l2.v[1]);
			int32_t zr = linearInterpolate(y, r1.v[2], r2.v[2], r1.v[1], r2.v[1]);
			int32_t z = linearInterpolate(x, zl, zr, xMin, xMax) & 0xFFFFF;
			if (y >= 0 && y < 192 && x>=0 && x < 256)
			{
				if ((uint32_t)z < depthBuffer[(y * 256) + x])
				{
					depthBuffer[(y * 256) + x] = z;
					renderBuffer[(y * 256) + x] = col & 0x7FFF;	//lol. do proper interpolation
				}
			}
		}

		//advance next scanline
		y++;

		//this is in dire need of a cleanup
		if (y >= l2.v[1])	//reached end of left slope
		{
			l1 = l2;
			l2Idx = (l2Idx + leftStep) % p.numVertices;
			l2 = p.m_vertices[l2Idx];
			xMin = l1.v[0];
			continue;
		}
		if (y >= r2.v[1])
		{
			r1 = r2;
			r2Idx = (r2Idx + rightStep) % p.numVertices;
			r2 = p.m_vertices[r2Idx];
			xMax = r1.v[0];
			continue;
		}

		//xMin = l1.v[0] + ((y - l1.v[1]) * (l2.v[0] - l1.v[0])) / (l2.v[1] - l1.v[1]);
		//xMax = r1.v[0] + ((y - r1.v[1]) * (r2.v[0] - r1.v[0])) / (r2.v[1] - r1.v[1]);
		xMin = linearInterpolate(y, l1.v[0], l2.v[0], l1.v[1], l2.v[1]);
		xMax = linearInterpolate(y, r1.v[0], r2.v[0], r1.v[1], r2.v[1]);
	}
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