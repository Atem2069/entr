#pragma once

#include"Logger.h"
#include"Scheduler.h"

struct APUChannel
{
	uint32_t control;
	uint32_t srcAddress;
	uint16_t loopStart;
	uint32_t length;

	//todo: extra stuff (e.g. actual sample data,...)
};

class APU
{
public:
	APU();
	~APU();
	void init(Scheduler* scheduler);

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value);
private:
	Scheduler* m_scheduler;
	uint16_t SOUNDCNT = {};
	uint8_t SOUNDBIAS = {};

	APUChannel m_channels[16] = {};
};