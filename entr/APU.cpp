#include"APU.h"

APU::APU()
{

}

APU::~APU()
{

}

void APU::init(Scheduler* scheduler)
{
	m_scheduler = scheduler;
	//sdl init,...
}

uint8_t APU::readIO(uint32_t address)
{
	//todo
	return 0;
}

void APU::writeIO(uint32_t address, uint8_t value)
{
	//todo
}