#include "GPU.h"

void GPU::debug_render()
{
	for (int i = 0; i < m_polygonCount; i++)
	{
		Poly p = m_polygonRAM[i];
		bool draw = true;
		for (int j = 0; j < p.numVertices; j++)
		{
			Vector cur = p.m_vertices[j];
			if (cur.v[3] > 0)					//hack to prevent div by 0, also cull polygon if w less than 0.
			{
				int64_t screenX = fixedPointMul((cur.v[0] + cur.v[3]), 256 << 12);
				int64_t screenY = fixedPointMul((cur.v[1] + cur.v[3]), 192 << 12);
				screenX = fixedPointDiv(screenX, (cur.v[3] << 1)) >> 12;
				screenY = 192 - (fixedPointDiv(screenY, (cur.v[3] << 1)) >> 12);
				Vector v = {};
				v.v[0] = screenX;
				v.v[1] = screenY;
				p.m_vertices[j] = v;
			}
			else
				draw = false;
		}
		if (!draw)
			continue;
		rasterizePolygon(p);
		/*Vector v0 = p.m_vertices[0];
		Vector v1 = p.m_vertices[1];
		Vector v2 = p.m_vertices[2];
		Vector v3 = p.m_vertices[3];
		if (p.numVertices == 3)
		{
			//0,1, 0,2, 1,2

			debug_drawLine(v0.v[0], v0.v[1], v1.v[0], v1.v[1]);
			debug_drawLine(v0.v[0], v0.v[1], v2.v[0], v2.v[1]);
			debug_drawLine(v1.v[0], v1.v[1], v2.v[0], v2.v[1]);
		}
		else
		{
			//0,1, 1,2, 2,3, 3,0

			debug_drawLine(v0.v[0], v0.v[1], v1.v[0], v1.v[1]);
			debug_drawLine(v1.v[0], v1.v[1], v2.v[0], v2.v[1]);
			debug_drawLine(v2.v[0], v2.v[1], v3.v[0], v3.v[1]);
			debug_drawLine(v3.v[0], v3.v[1], v0.v[0], v0.v[1]);
		}*/
	}
}

int dumbMod(int a, int b)
{
	int rem = a % b;
	if (rem < 0)
		rem += b;
	return rem;
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
	if (smallY < 0 || smallY >= 192 || largeY < 0 || largeY >= 192)
		return;

	Vector l1 = {}, l2 = {};
	Vector r1 = {}, r2 = {};

	l1 = p.m_vertices[topVtxIdx];
	l2 = p.m_vertices[(topVtxIdx + 1) % (p.numVertices)];
	r1 = p.m_vertices[topVtxIdx];
	r2 = p.m_vertices[dumbMod(topVtxIdx-1,p.numVertices)];

	int l2Idx = (topVtxIdx + 1) % (p.numVertices);
	int r2Idx = dumbMod(topVtxIdx - 1, p.numVertices);

	int xMin = l1.v[0]; int xMax = r1.v[0];
	
	int y = smallY;
	while (y <= largeY)
	{
		//rasterize
		for (int x = xMin; x <= xMax; x++)
		{
			if (y > 0 && y < 192 && x>0 && x < 256)
				renderBuffer[(y * 256) + x] = 0x001F;
		}
		y++;
		if (y >= l2.v[1])	//reached end of left slope
		{
			l1 = l2;
			l2Idx = (l2Idx + 1) % p.numVertices;
			l2 = p.m_vertices[l2Idx];
			xMin = l1.v[0];
			continue;
		}
		if (y >= r2.v[1])
		{
			r1 = r2;
			r2Idx = dumbMod(r2Idx - 1, p.numVertices);
			r2 = p.m_vertices[r2Idx];
			xMax = r1.v[0];
			continue;
		}
		xMin = l1.v[0] + ((y - l1.v[1]) * (l2.v[0] - l1.v[0])) / (l2.v[1] - l1.v[1]);
		xMax = r1.v[0] + ((y - r1.v[1]) * (r2.v[0] - r1.v[0])) / (r2.v[1] - r1.v[1]);
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
		if ((x > 0 && x < 256) && (y > 0 && y < 192))
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
		if ((x > 0 && x < 256) && (y > 0 && y < 192))
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