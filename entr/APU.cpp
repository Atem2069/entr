#include"APU.h"
#include"Bus.h"

APU::APU()
{

}

APU::~APU()
{

}

void APU::init(Bus* bus, Scheduler* scheduler)
{
	m_bus = bus;
	m_scheduler = scheduler;
	//sdl init,...

	m_scheduler->addEvent(Event::Sample, (callbackFn)&APU::onSampleEvent, (void*)this, cyclesPerSample);
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
	switch (reg)
	{
	case 0:
		return m_channels[chan].control & 0xFF;
	case 1:
		return (m_channels[chan].control >> 8) & 0xFF;
	case 2:
		return (m_channels[chan].control >> 16) & 0xFF;
	case 3:
		return (m_channels[chan].control >> 24) & 0xFF;
	}

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
	switch (reg)
	{
	case 0:
		m_channels[chan].control &= 0xFFFFFF00; m_channels[chan].control |= value;
		break;
	case 1:
		m_channels[chan].control &= 0xFFFF00FF; m_channels[chan].control |= (value << 8);
		break;
	case 2:
		m_channels[chan].control &= 0xFF00FFFF; m_channels[chan].control |= (value << 16);
		break;
	case 3:
		m_channels[chan].control &= 0x00FFFFFF; m_channels[chan].control |= (value << 24);
		if (!(m_channels[chan].control >> 31))
		{
			m_channels[chan].curLength = 0;
			m_channels[chan].cycleCount = 0;
		}
		break;
	case 4:
		m_channels[chan].srcAddress &= 0xFFFFFF00; m_channels[chan].srcAddress |= value;
		break;
	case 5:
		m_channels[chan].srcAddress &= 0xFFFF00FF; m_channels[chan].srcAddress |= (value << 8);
		break;
	case 6:
		m_channels[chan].srcAddress &= 0xFF00FFFF; m_channels[chan].srcAddress |= (value << 16);
		break;
	case 7:
		m_channels[chan].srcAddress &= 0x00FFFFFF; m_channels[chan].srcAddress |= (value << 24);
		break;
	case 8:
		m_channels[chan].timer &= 0xFF00; m_channels[chan].timer |= value;
		break;
	case 9:
		m_channels[chan].timer &= 0xFF; m_channels[chan].timer |= (value << 8);
		break;
	case 10:
		m_channels[chan].loopStart &= 0xFF00; m_channels[chan].loopStart |= value;
		break;
	case 11:
		m_channels[chan].loopStart &= 0xFF; m_channels[chan].loopStart |= (value << 8);
		break;
	case 12:
		m_channels[chan].length &= 0xFFFFFF00; m_channels[chan].length |= value;
		break;
	case 13:
		m_channels[chan].length &= 0xFFFF00FF; m_channels[chan].length |= (value << 8);
		break;
	case 14:
		m_channels[chan].length &= 0xFF00FFFF; m_channels[chan].length |= (value << 16);
		break;
	case 15:
		m_channels[chan].length &= 0x00FFFFFF; m_channels[chan].length |= (value << 24);
		break;
	}
}

void APU::tickChannel(int channel)
{
	if (!(m_channels[channel].control >> 31))
		return;
	m_channels[channel].cycleCount += cyclesPerSample;
	int timer = (0x10000 - m_channels[channel].timer) << 1;
	while (m_channels[channel].cycleCount >= timer)
	{
		m_channels[channel].cycleCount -= timer;

		uint8_t format = (m_channels[channel].control >> 29) & 0b11;
		switch (format)
		{
		case 0:
			m_channels[channel].sample = m_bus->NDS7_read8(m_channels[channel].srcAddress + m_channels[channel].curLength);
			m_channels[channel].sample <<= 8;
			m_channels[channel].curLength++;
			break;
		case 1:
			m_channels[channel].sample = m_bus->NDS7_read16(m_channels[channel].srcAddress + m_channels[channel].curLength);
			m_channels[channel].curLength += 2;
			break;
		default:
			m_channels[channel].sample = 0;
			m_channels[channel].curLength++;
		}
		//hacks
		if (m_channels[channel].curLength == (m_channels[channel].length << 2))
		{
			m_channels[channel].curLength = 0;
			m_channels[channel].cycleCount = 0;
			m_channels[channel].control &= 0x7FFFFFFF;
		}
	}
}

void APU::sampleChannels()
{
	for (int i = 0; i < 16; i++)
		tickChannel(i);

	//todo: stuff with each channel
	m_scheduler->addEvent(Event::Sample, (callbackFn)&APU::onSampleEvent, (void*)this, m_scheduler->getEventTime() + cyclesPerSample);
}

void APU::onSampleEvent(void* ctx)
{
	APU* thisPtr = (APU*)ctx;
	thisPtr->sampleChannels();
}