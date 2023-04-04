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

struct PPURegisters
{
	uint32_t DISPCNT;
	uint16_t BG0CNT;
};

enum class Engine
{
	A=0,
	B=1
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

	uint8_t NDS7_readIO(uint32_t address);
	void NDS7_writeIO(uint32_t address, uint8_t value);

	uint8_t* mapLCDCAddress(uint32_t address);
	uint8_t* mapABgAddress(uint32_t address);
	uint8_t* mapAObjAddress(uint32_t address);
	uint8_t* mapBBgAddress(uint32_t address);
	uint8_t* mapBObjAddress(uint32_t address);
	uint8_t* mapARM7Address(uint32_t address);

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

	void renderLCDCMode();
	template <Engine engine> void renderMode0();

	template<Engine engine, int bg> void renderBackground();

	template<Engine engine> uint8_t ppuReadBg(uint32_t address)
	{
		if (engine == Engine::A)
			return *mapABgAddress(address);
		return *mapBBgAddress(address);
	}

	void setVBlankFlag(bool value);
	void setHBlankFlag(bool value);
	void setVCounterFlag(bool value);

	uint32_t col16to32(uint16_t col);

	uint16_t DISPSTAT = {};
	uint16_t NDS7_DISPSTAT = {};
	uint16_t VCOUNT = {};

	PPURegisters m_engineARegisters = {};
	PPURegisters m_engineBRegisters = {};


	//dumb vram stuff
	uint8_t VRAMCNT_A = {};
	uint8_t VRAMCNT_B = {};
	uint8_t VRAMCNT_C = {};
	uint8_t VRAMCNT_D = {};
	uint8_t VRAMCNT_E = {};
	uint8_t VRAMCNT_F = {};
	uint8_t VRAMCNT_G = {};
	uint8_t VRAMCNT_H = {};
	uint8_t VRAMCNT_I = {};

	void rebuildPageTables();
};