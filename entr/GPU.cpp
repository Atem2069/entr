#include"GPU.h"

GPU::GPU()
{

}

GPU::~GPU()
{

}

void GPU::init(InterruptManager* interruptManager, Scheduler* scheduler)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;
}

uint8_t GPU::read(uint32_t address)
{
	Logger::msg(LoggerSeverity::Warn, std::format("Unhandled GPU read: {:#x}", address));
	return 0;
}

void GPU::write(uint32_t address, uint8_t value)
{
	Logger::msg(LoggerSeverity::Warn, std::format("Unhandled GPU write: {:#x}", address));
}

void GPU::writeGXFIFO(uint32_t value)
{
	std::cout << "Unhandled GXFIFO write: " << std::hex << value << std::endl;
}

void GPU::writeCmdPort(uint32_t address, uint32_t value)
{
	//todo: figure out port from address, some way to enqueue..
	//hopefully parameters are sent to the port too? that would probably make my life easier.
}
