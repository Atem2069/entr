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
		m_coordinateStack[m_coordinateStackPointer] = m_coordinateMatrix;
		m_directionalStack[m_coordinateStackPointer] = m_directionalMatrix;
		m_coordinateStackPointer++;
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
		if (m_coordinateStackPointer < 0 || m_coordinateStackPointer > 29)
			std::cout << "fuck" << '\n';
		else
		{
			m_coordinateMatrix = m_coordinateStack[m_coordinateStackPointer];
			m_directionalMatrix = m_directionalStack[m_coordinateStackPointer];
		}
		break;
	}
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
}

void GPU::cmd_load4x4Matrix(uint32_t* params)
{

}

void GPU::cmd_load4x3Matrix(uint32_t* params)
{

}

void GPU::cmd_multiply4x4Matrix(uint32_t* params)
{

}

void GPU::cmd_multiply4x3Matrix(uint32_t* params)
{

}

void GPU::cmd_multiply3x3Matrix(uint32_t* params)
{

}

void GPU::cmd_multiplyByScale(uint32_t* params)
{

}

void GPU::cmd_multiplyByTrans(uint32_t* params)
{

}

void GPU::cmd_beginVertexList(uint32_t* params)
{
	//just so I don't forget.
	//triangle strips emit triangles as:
	//0,1,2 2,1,3 2,3,4 ...
	uint32_t primitiveType = params[0] & 0b11;
	//Logger::msg(LoggerSeverity::Info, std::format("gpu: Begin vertices, type {}", primitiveType));
}

void GPU::cmd_vertex16Bit(uint32_t* params)
{
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;
	int16_t z = params[1] & 0xFFFF;
	//double xf = debug_cvtVtx16(x);
	//double yf = -debug_cvtVtx16(y);

	//don't question this absolute insanity.
	int64_t screenX = (((x + (8 << 12)) * 0b000100000000) >> 12) * 256;
	int64_t screenY = (((y + (8 << 12)) * 0b000100000000) >> 12) * 192;
	screenX >>= 12;
	screenY >>= 12;
	screenY = 192 - screenY;


	//Logger::msg(LoggerSeverity::Info, std::format("gpu: vtx ({}, {}), screen=({}, {})", xf, yf, screenX, screenY));

	if ((screenX > 0 && screenX < 256) && (screenY > 0 && screenY < 192))
	{
		renderBuffer[(screenY * 256) + screenX] = 0x7FFF;
	}
}

void GPU::cmd_vertex10Bit(uint32_t* params)
{
	int16_t x = params[0] & 0x3FF;
	int16_t y = (params[0] >> 10) & 0x3FF;
	int16_t z = (params[0] >> 20) & 0x3FF;
	if ((x >> 9) & 0b1)
		x |= 0xFC00;
	if ((y >> 9) & 0b1)
		y |= 0xFC00;

	x <<= 6;
	y <<= 6;
	z <<= 6;

	//float xf = debug_cvtVtx16(x);
	//float yf = -debug_cvtVtx16(y);

	//don't question this absolute insanity.
	int64_t screenX = (((x + (8 << 12)) * 0b000100000000) >> 12) * 256;
	int64_t screenY = (((y + (8 << 12)) * 0b000100000000) >> 12) * 192;
	screenX >>= 12;
	screenY >>= 12;
	screenY = 192 - screenY;

	//Logger::msg(LoggerSeverity::Info, std::format("gpu: vtx ({}, {}), screen=({}, {})", xf, yf, screenX, screenY));

	if ((screenX > 0 && screenX < 256) && (screenY > 0 && screenY < 192))
	{
		renderBuffer[(screenY * 256) + screenX] = 0x7FFF;
	}
}

void GPU::cmd_endVertexList()
{
	//Logger::msg(LoggerSeverity::Info, std::format("gpu: End vertices"));
}

void GPU::cmd_swapBuffers()
{
	Logger::msg(LoggerSeverity::Info, std::format("gpu: SwapBuffers"));
	debug_renderDots();
}

void GPU::debug_renderDots()
{
	memcpy(output, renderBuffer, 256 * 192 * sizeof(uint16_t));
	memset(renderBuffer, 0xFF, 256 * 192 * sizeof(uint16_t));
	//todo: dots!!!
}