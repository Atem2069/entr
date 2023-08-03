#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"

class GPU
{
public:
	GPU();
	~GPU();

	void init(InterruptManager* interruptManager, Scheduler* scheduler);

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	void writeGXFIFO(uint32_t value);
	void writeCmdPort(uint32_t address, uint32_t value);

private:
	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;
};