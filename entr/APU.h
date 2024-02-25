#pragma once

#include"Logger.h"
#include"Scheduler.h"

#include<SDL.h>
#undef main			//wtf sdl?

struct APUChannel
{
	uint32_t control;
	uint32_t srcAddress;
	uint16_t timer;
	uint16_t loopStart;
	uint32_t length;

	uint32_t cycleCount;
	uint32_t curLength;
	
	int16_t sample;
};

class Bus;

class APU
{
public:
	APU();
	~APU();
	void init(Bus* bus, Scheduler* scheduler);

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value);
	static void onSampleEvent(void* ctx);
private:
	void tickChannel(int channel);
	void sampleChannels();
	Scheduler* m_scheduler;
	Bus* m_bus;
	uint16_t SOUNDCNT = {};
	uint16_t SOUNDBIAS = {};

	APUChannel m_channels[16] = {};

	SDL_AudioDeviceID m_audioDevice;
	float m_sampleBuffer[4096] = {};
	int sampleIndex = 0;

	static constexpr int cyclesPerSample = 512;
};