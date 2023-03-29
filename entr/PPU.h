#pragma once

#include"Logger.h"
#include"NDSMem.h"
#include"InterruptManager.h"
#include"Scheduler.h"

enum class PPUState
{
	HDraw,
	HBlank,
	VBlank
};

class PPU
{
public:
	PPU(std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<Scheduler> scheduler);
	~PPU();

	void updateDisplayOutput();

	void registerMemory(std::shared_ptr<NDSMem> mem);

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value); 

	static void onSchedulerEvent(void* context);

	static uint32_t m_safeDisplayBuffer[256 * 384];
private:
	PPUState m_state = {};
	bool registered = false;
	std::shared_ptr<NDSMem> m_mem;
	std::shared_ptr<InterruptManager> m_interruptManager;
	std::shared_ptr<Scheduler> m_scheduler;

	uint32_t m_renderBuffer[2][256 * 384];
	bool pageIdx = false;

	void eventHandler();
	void HDraw();
	void HBlank();
	void VBlank();
	bool vblank_setHblankBit = false;

	void setVBlankFlag(bool value);
	void setHBlankFlag(bool value);
	void setVCounterFlag(bool value);

	uint32_t col16to32(uint16_t col);

	uint32_t DISPCNT = {};
	uint16_t DISPSTAT = {};
	uint16_t VCOUNT = {};
};