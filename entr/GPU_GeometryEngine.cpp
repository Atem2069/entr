#include "GPU.h"

void GPU::cmd_setMatrixMode(uint32_t* params)
{
	uint32_t mode = params[0] & 0b11;
	m_matrixMode = mode;
}

void GPU::cmd_pushMatrix()
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionStack = m_projectionMatrix;
		break;
	case 1: case 2:
		m_coordinateStack[m_coordinateStackPointer&0x1F] = m_coordinateMatrix;
		m_directionalStack[m_coordinateStackPointer&0x1F] = m_directionalMatrix;
		m_coordinateStackPointer++;
		GXSTAT &= 0xFFFFE0FF;
		GXSTAT |= (m_coordinateStackPointer & 0x1F) << 8;
		break;
	}
}

void GPU::cmd_popMatrix(uint32_t* params)
{
	int32_t offs = params[0] & 0x3F;
	if ((offs >> 5) & 0b1)
		offs |= 0xFFFFFFC0;		//sign extend

	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_projectionStack;
		break;
	case 1: case 2:
		m_coordinateStackPointer -= offs;
		if (m_coordinateStackPointer < 0 || m_coordinateStackPointer > 63)	//games might be silly enough to trigger this.
		{
			m_coordinateStackPointer = 0;
			GXSTAT |= (1 << 15);		
			Logger::msg(LoggerSeverity::Error, std::format("gpu: exploded.. Coordinate Matrix Stack Pointer is OOB: {}, {}", m_coordinateStackPointer, offs));
		}
		m_coordinateMatrix = m_coordinateStack[m_coordinateStackPointer&0x1F];
		m_directionalMatrix = m_directionalStack[m_coordinateStackPointer&0x1F];
		GXSTAT &= 0xFFFFE0FF;
		GXSTAT |= (m_coordinateStackPointer & 0x1F) << 8;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_storeMatrix(uint32_t* params)
{
	uint32_t addr = params[0] & 0x1F;

	switch (m_matrixMode)
	{
	case 0:
		m_projectionStack = m_projectionMatrix;
		break;
	case 1: case 2:
		m_coordinateStack[addr] = m_coordinateMatrix;
		m_directionalStack[addr] = m_directionalMatrix;
		break;
	}
}

void GPU::cmd_restoreMatrix(uint32_t* params)
{
	uint32_t addr = params[0] & 0x1F;

	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_projectionStack;
		break;
	case 1: case 2:
		m_coordinateMatrix = m_coordinateStack[addr];
		m_directionalMatrix = m_directionalStack[addr];
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_loadIdentity()
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_identityMatrix;
		break;
	case 1:
		m_coordinateMatrix = m_identityMatrix;
		break;
	case 2:
		m_coordinateMatrix = m_identityMatrix;
		m_directionalMatrix = m_identityMatrix;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_load4x4Matrix(uint32_t* params)
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = gen4x4Matrix(params);
		break;
	case 1:
		m_coordinateMatrix = gen4x4Matrix(params);
		break;
	case 2:
		m_coordinateMatrix = gen4x4Matrix(params);
		m_directionalMatrix = m_coordinateMatrix;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_load4x3Matrix(uint32_t* params)
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = gen4x3Matrix(params);
		break;
	case 1:
		m_coordinateMatrix = gen4x3Matrix(params);
		break;
	case 2:
		m_coordinateMatrix = gen4x3Matrix(params);
		m_directionalMatrix = m_coordinateMatrix;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply4x4Matrix(uint32_t* params)
{
	Matrix m = gen4x4Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply4x3Matrix(uint32_t* params)
{
	Matrix m = gen4x3Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply3x3Matrix(uint32_t* params)
{
	Matrix m = gen3x3Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiplyByScale(uint32_t* params)
{
	Matrix m = {};
	m.m[0] = params[0];
	m.m[5] = params[1];
	m.m[10] = params[2];
	m.m[15] = (1 << 12);

	//directional/vector matrix (why does gbatek use two names) doesn't get updated by MTX_SCALE
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1: case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiplyByTrans(uint32_t* params)
{
	Matrix m = m_identityMatrix;
	m.m[12] = params[0];
	m.m[13] = params[1];
	m.m[14] = params[2];

	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_beginVertexList(uint32_t* params)
{
	//just so I don't forget.
	//triangle strips emit triangles as:
	//0,1,2 2,1,3 2,3,4 ...
	uint32_t primitiveType = params[0] & 0b11;		//should do something with this soon when emitting polys
	m_primitiveType = primitiveType;
	m_runningVtxCount = 0;							//reset running vertex count - primitive type may have changed so we construct new polys with the new vtx list being sent
}

//i don't like the code duplication here, should ideally clean this up.
void GPU::cmd_vertex16Bit(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;
	int16_t z = params[1] & 0xFFFF;


	Vector point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_vertex10Bit(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t x = (params[0] & 0x3FF) << 6;
	int16_t y = ((params[0] >> 10) & 0x3FF) << 6;
	int16_t z = ((params[0] >> 20) & 0x3FF) << 6;

	Vector point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_vertexXY(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;

	Vector point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = m_lastVertex.v[2]; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_vertexXZ(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t x = params[0] & 0xFFFF;
	int16_t z = (params[0] >> 16) & 0xFFFF;

	Vector point = {};
	point.v[0] = x; point.v[1] = m_lastVertex.v[1]; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_vertexYZ(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t y = params[0] & 0xFFFF;
	int16_t z = (params[0] >> 16) & 0xFFFF;

	Vector point = {};
	point.v[0] = m_lastVertex.v[0]; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_vertexDiff(uint32_t* params)
{
	if (m_vertexCount >= 6144)
		return;
	int16_t x = params[0] & 0x3FF;
	int16_t y = (params[0] >> 10) & 0x3FF;
	int16_t z = (params[0] >> 20) & 0x3FF;
	if ((x >> 9) & 0b1)
		x |= 0xFC00;
	if ((y >> 9) & 0b1)
		y |= 0xFC00;
	if ((z >> 9) & 0b1)
		z |= 0xFC00;
	
	x >>= 3; y >>= 3; z >>= 3;

	Vector point = m_lastVertex;
	point.v[0] += x; point.v[1] += y; point.v[2] += z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = point;
}

void GPU::cmd_endVertexList()
{
	//this seems to be useless on real hardware.
}

void GPU::cmd_swapBuffers()
{
	//Logger::msg(LoggerSeverity::Info, std::format("gpu: SwapBuffers"));
	debug_render();

	memcpy(output, renderBuffer, 256 * 192 * sizeof(uint16_t));
	memset(renderBuffer, 0xFF, 256 * 192 * sizeof(uint16_t));
	
	m_vertexCount = 0;
	m_polygonCount = 0;
	m_runningVtxCount = 0;
}

void GPU::submitPolygon()
{
	if (m_polygonCount == 2048)
		return;
	switch (m_primitiveType)
	{
	case 0:
		if (m_runningVtxCount && ((m_runningVtxCount % 3) == 0))
		{
			Poly p = {};
			p.numVertices = 3;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			m_polygonRAM[m_polygonCount++] = p;
		}
		break;
	case 1:
		if (m_runningVtxCount && ((m_runningVtxCount % 4) == 0))
		{
			Poly p = {};
			p.numVertices = 4;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 4];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[3] = m_vertexRAM[m_vertexCount - 1];
			m_polygonRAM[m_polygonCount++] = p;
		}
		break;
	case 2:
		if (m_runningVtxCount >= 3)
		{
			//supposedly: ds generates alternating cw/ccw
			//num vtxs odd: n-3,n-2,n-1
			//num vtxs even: n-2,n-3,n-1
			Poly p = {};
			p.numVertices = 3;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			m_polygonRAM[m_polygonCount++] = p;
		}
		break;
	case 3:
		if (m_runningVtxCount >= 4 && (!(m_runningVtxCount & 0b1)))
		{
			//i think this is right, not sure... but it seems polys are 0123 2345 4567 ...
			Poly p = {};
			p.numVertices = 4;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 4];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			p.m_vertices[3] = m_vertexRAM[m_vertexCount - 2];
			m_polygonRAM[m_polygonCount++] = p;
		}
		break;
	}
}

//this stuff shouldn't be in the geometry engine. going to move when i'm happy with polygon generation and all :)
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
		Vector v0 = p.m_vertices[0];
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
		}
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