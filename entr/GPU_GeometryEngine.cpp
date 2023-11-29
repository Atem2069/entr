#include "GPU.h"

void GPU::cmd_beginVertexList(uint32_t* params)
{
	uint32_t primitiveType = params[0] & 0b11;
	Logger::msg(LoggerSeverity::Info, std::format("gpu: Begin vertices, type {}", primitiveType));
}

void GPU::cmd_vertex16Bit(uint32_t* params)
{
	
}

void GPU::cmd_vertex10Bit(uint32_t* params)
{

}

void GPU::cmd_endVertexList()
{
	Logger::msg(LoggerSeverity::Info, std::format("gpu: End vertices"));
}

void GPU::cmd_swapBuffers()
{
	Logger::msg(LoggerSeverity::Info, std::format("gpu: SwapBuffers"));
}