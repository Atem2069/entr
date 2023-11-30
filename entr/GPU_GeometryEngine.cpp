#include "GPU.h"

void GPU::cmd_setMatrixMode(uint32_t* params)
{

}

void GPU::cmd_pushMatrix()
{

}

void GPU::cmd_popMatrix(uint32_t* params)
{

}

void GPU::cmd_storeMatrix(uint32_t* params)
{

}

void GPU::cmd_restoreMatrix(uint32_t* params)
{

}

void GPU::cmd_loadIdentity()
{

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