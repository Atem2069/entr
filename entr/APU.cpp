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
	switch (address)
	{
	case 0x04000500:
		return SOUNDCNT & 0xFF;
	case 0x04000501:
		return (SOUNDCNT >> 8) & 0xFF;
	case 0x04000502: case 0x04000503:
		return 0;
	case 0x04000504:
		return SOUNDBIAS & 0xFF;
	case 0x04000505:
		return (SOUNDBIAS >> 8) & 0xFF;
	}

	uint32_t chan = (address >> 4) & 0xF;
	uint32_t reg = address & 0xF;
	Logger::msg(LoggerSeverity::Info, std::format("apu: read chan {:#x} reg {}", chan, reg));
	return 0;
}

void APU::writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000500:
		SOUNDCNT &= 0xFF00; SOUNDCNT |= value;
		return;
	case 0x04000501:
		SOUNDCNT &= 0xFF; SOUNDCNT |= (value << 8);
		return;
	case 0x04000502: case 0x04000503:
		return;
	case 0x04000504:
		SOUNDBIAS &= 0xFF00; SOUNDBIAS |= value;
		return;
	case 0x04000505:
		SOUNDBIAS &= 0xFF; SOUNDBIAS |= ((value & 0b11) << 8);
		return;
	}

	uint32_t chan = (address >> 4) & 0xF;
	uint32_t reg = address & 0xF;
	Logger::msg(LoggerSeverity::Info, std::format("apu: write chan {:#x} reg {}", chan, reg));
}