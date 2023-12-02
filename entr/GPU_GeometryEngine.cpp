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
}

void GPU::cmd_vertex16Bit(uint32_t* params)
{
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;
	int16_t z = params[1] & 0xFFFF;


	Vector point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	if (clipPoint.v[3])															//hack to prevent divide-by-zero.
	{
		int64_t a = fixedPointMul((clipPoint.v[0] + clipPoint.v[3]), 256 << 12);
		a = fixedPointDiv(a, (2 * clipPoint.v[3]));
		int64_t b = fixedPointMul((clipPoint.v[1] + clipPoint.v[3]), 192 << 12);
		b = fixedPointDiv(b, (2 * clipPoint.v[3]));
		a >>= 12;
		b >>= 12;
		int64_t screenX = a, screenY = b;
		screenY = 192 - screenY;

		if ((screenX > 0 && screenX < 256) && (screenY > 0 && screenY < 192))
		{
			renderBuffer[(screenY * 256) + screenX] = 0x7FFF;
		}
	}
}

void GPU::cmd_vertex10Bit(uint32_t* params)
{
	int16_t x = (params[0] & 0x3FF) << 6;
	int16_t y = ((params[0] >> 10) & 0x3FF) << 6;
	int16_t z = ((params[0] >> 20) & 0x3FF) << 6;

	Vector point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	Vector clipPoint = multiplyVectorMatrix(point, m_clipMatrix);
	if (clipPoint.v[3])															//hack to prevent divide-by-zero.
	{
		int64_t a = fixedPointMul((clipPoint.v[0] + clipPoint.v[3]), 256 << 12);
		a = fixedPointDiv(a, (2 * clipPoint.v[3]));
		int64_t b = fixedPointMul((clipPoint.v[1] + clipPoint.v[3]), 192 << 12);
		b = fixedPointDiv(b, (2 * clipPoint.v[3]));
		a >>= 12;
		b >>= 12;
		int64_t screenX = a, screenY = b;
		screenY = 192 - screenY;

		if ((screenX > 0 && screenX < 256) && (screenY > 0 && screenY < 192))
		{
			renderBuffer[(screenY * 256) + screenX] = 0x7FFF;
		}
	}
}

void GPU::cmd_endVertexList()
{
	//Logger::msg(LoggerSeverity::Info, std::format("gpu: End vertices"));
}

void GPU::cmd_swapBuffers()
{
	//Logger::msg(LoggerSeverity::Info, std::format("gpu: SwapBuffers"));
	debug_renderDots();
}

void GPU::debug_renderDots()
{
	memcpy(output, renderBuffer, 256 * 192 * sizeof(uint16_t));
	memset(renderBuffer, 0xFF, 256 * 192 * sizeof(uint16_t));
	//todo: dots!!!
}