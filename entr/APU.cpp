#include"APU.h"
#include"Bus.h"

APU::APU()
{
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec desiredSpec = {}, obtainedSpec = {};
	desiredSpec.freq = sampleRate;
	desiredSpec.format = AUDIO_F32;
	desiredSpec.channels = 2;
	desiredSpec.silence = 0;
	desiredSpec.samples = sampleBufferSize;
	m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &obtainedSpec, 0);

	if (!m_audioDevice)
		Logger::msg(LoggerSeverity::Error, "Couldn't open audio device :(");

	SDL_PauseAudioDevice(m_audioDevice, 0);
}

APU::~APU()
{
	SDL_Quit();
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
		Logger::msg(LoggerSeverity::Info, std::format("SOUNDCNT: {:#x}", SOUNDCNT));
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
	{
		bool wasEnabled = (m_channels[chan].control >> 31);
		m_channels[chan].control &= 0x00FFFFFF; m_channels[chan].control |= (value << 24);
		if (!(m_channels[chan].control >> 31))
		{
			m_channels[chan].curSampleCount = 0;
			m_channels[chan].sample = 0;
		}
		if (!wasEnabled && (m_channels[chan].control >> 31))
		{
			m_channels[chan].curSrcAddress = m_channels[chan].srcAddress;
			m_channels[chan].lastCheckTimestamp = m_scheduler->getCurrentTimestamp();
			if (((m_channels[chan].control >> 29) & 0b11) == 2)
			{
				uint32_t adpcmHeader = m_bus->NDS7_read32(m_channels[chan].srcAddress);
				m_channels[chan].adpcm_initial = adpcmHeader & 0xFFFF;
				m_channels[chan].adpcm_tableIdx = (adpcmHeader >> 16) & 0x7F;
				m_channels[chan].adpcm_loop = m_channels[chan].adpcm_initial;
				m_channels[chan].adpcm_loopTableIdx = m_channels[chan].adpcm_tableIdx;
				m_channels[chan].curSrcAddress += 4;
				m_channels[chan].curSampleCount = 0;
				m_channels[chan].sample = 0;
			}
		}
		break;
	}
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
	{
		m_channels[channel].sample = 0;
		return;
	}
	int timer = (0x10000 - m_channels[channel].timer) << 1;

	int64_t timeDiff = m_scheduler->getEventTime() - m_channels[channel].lastCheckTimestamp;
	while (timeDiff >= timer)
	{
		timeDiff -= timer;

		uint8_t format = (m_channels[channel].control >> 29) & 0b11;
		switch (format)
		{
		case 0:
			m_channels[channel].sample = m_bus->NDS7_read8(m_channels[channel].curSrcAddress);
			m_channels[channel].sample <<= 8;
			m_channels[channel].curSrcAddress++;
			break;
		case 1:
			m_channels[channel].sample = m_bus->NDS7_read16(m_channels[channel].curSrcAddress);
			m_channels[channel].curSrcAddress += 2;
			break;
		case 2:
		{
			uint32_t adpcmSample = m_bus->NDS7_read8(m_channels[channel].curSrcAddress);
			if ((m_channels[channel].curSampleCount & 0b1))
			{
				adpcmSample >>= 4;
				m_channels[channel].curSrcAddress++;
			}
			adpcmSample &= 0xF;
			int diff = ((adpcmSample & 7) * 2 + 1) * m_adpcmTable[m_channels[channel].adpcm_tableIdx] / 8;
			if (!(adpcmSample & 8))
				m_channels[channel].sample = std::min(m_channels[channel].adpcm_initial + diff, 0x7FFF);
			else
				m_channels[channel].sample = std::max(m_channels[channel].adpcm_initial - diff, -0x7FFF);
			m_channels[channel].adpcm_initial = m_channels[channel].sample;
			m_channels[channel].adpcm_tableIdx = std::min(std::max(m_channels[channel].adpcm_tableIdx + m_indexTable[adpcmSample & 7], 0),88);
			if ((m_channels[channel].curSrcAddress-m_channels[channel].srcAddress == (m_channels[channel].loopStart << 2)) && (m_channels[channel].curSampleCount&0b1))
			{
				m_channels[channel].adpcm_loop = m_channels[channel].adpcm_initial;
				m_channels[channel].adpcm_loopTableIdx = m_channels[channel].adpcm_tableIdx;
			}
			break;
		}
		//default:
			//m_channels[channel].sample = 0;
			//m_channels[channel].curSrcAddress++;
		}

		m_channels[channel].curSampleCount++;

		//hacks
		uint32_t endOffset = m_channels[channel].srcAddress + ((m_channels[channel].length + m_channels[channel].loopStart) << 2);
		if (m_channels[channel].curSrcAddress >= endOffset)
		{
			//is this right?
			//wtf even is manual/one-shot??
			uint8_t repeatMode = (m_channels[channel].control >> 27) & 0b11;
			switch (repeatMode)
			{
			case 1:
				m_channels[channel].curSrcAddress = m_channels[channel].srcAddress + (((int)m_channels[channel].loopStart) << 2);
				m_channels[channel].adpcm_initial = m_channels[channel].adpcm_loop;
				m_channels[channel].adpcm_tableIdx = m_channels[channel].adpcm_loopTableIdx;
				break;
			case 2:
				//m_channels[channel].cycleCount = 0;
				m_channels[channel].curSampleCount = 0;
				m_channels[channel].control &= 0x7FFFFFFF;
				m_channels[channel].sample = 0;
				return;
			//default:
			//	std::cout << "wtf??" << '\n';
			}
			//doesn't really matter what it's set to, just for tracking adpcm really
			m_channels[channel].curSampleCount = 0;
		}
		m_channels[channel].lastCheckTimestamp = m_scheduler->getEventTime() - timeDiff;
	}
}

void APU::sampleChannels()
{
	int32_t finalSampleLeft = 0, finalSampleRight = 0;
	if ((SOUNDCNT >> 15))
	{
		for (int i = 0; i < 16; i++)
		{
			tickChannel(i);
			int channelPan = (m_channels[i].control >> 16) & 0x7F;
			int volume = (m_channels[i].control) & 0x7F;
			int32_t sample = m_channels[i].sample;
			sample = sample * volume / 128;

			finalSampleLeft += ((sample * (128 - channelPan)) / 128);
			finalSampleRight += ((sample * channelPan) / 128);
		}
	}

	float sampleOutLeft = ((float)finalSampleLeft) / 262144.f;
	float sampleOutRight = ((float)finalSampleRight) / 262144.f;
	m_sampleBuffer[sampleIndex << 1] = sampleOutLeft;
	m_sampleBuffer[(sampleIndex << 1) + 1] = sampleOutRight;
	sampleIndex++;
	if (sampleIndex == sampleBufferSize)
	{
		float m_finalSamples[sampleBufferSize * 2] = {};
		memcpy(m_finalSamples, m_sampleBuffer, sampleBufferSize * 8);
		sampleIndex = 0;
		SDL_QueueAudio(m_audioDevice, (void*)m_finalSamples, sampleBufferSize * 8);

		//rudimentary audio sync
		//while (SDL_GetQueuedAudioSize(m_audioDevice) > sampleBufferSize * 8);
	}

	m_scheduler->addEvent(Event::Sample, (callbackFn)&APU::onSampleEvent, (void*)this, m_scheduler->getEventTime() + cyclesPerSample);
}

void APU::onSampleEvent(void* ctx)
{
	APU* thisPtr = (APU*)ctx;
	thisPtr->sampleChannels();
}