#pragma once

#include"NDSMem.h"
#include"Logger.h"
#include"PPU.h"
#include"GPU.h"
#include"Input.h"
#include"IPC.h"
#include"Math.h"
#include"Timer.h"
#include"Cartridge.h"
#include"SPI.h"
#include"RTC.h"
#include"WifiStub.h"
#include"APU.h"

#include<format>

struct DMAChannel
{
	//vals CPU writes to dma registers
	uint32_t src;
	uint32_t dest;
	uint32_t wordCount;

	//latched regs at dma start or repeat
	uint32_t internalSrc;
	uint32_t internalDest;
	uint32_t internalWordCount;

	uint16_t control;

	//only for NDS9 DMAs
	uint32_t scratchpad;
};

class Bus
{
public:
	Bus();
	void init(std::vector<uint8_t> NDS7_BIOS, std::vector<uint8_t> NDS9_BIOS, Cartridge* cartridge, Scheduler* scheduler, InterruptManager* interruptManager, PPU* ppu, GPU* gpu, Input* input);
	~Bus();

	void mapVRAMPages();

	//NDS7 read/write handlers
	uint8_t NDS7_read8(uint32_t address);
	void NDS7_write8(uint32_t address, uint8_t value);

	uint16_t NDS7_read16(uint32_t address);
	void NDS7_write16(uint32_t address, uint16_t value);

	uint32_t NDS7_read32(uint32_t address);
	void NDS7_write32(uint32_t address, uint32_t value);

	//NDS9 read/write handlers
	uint8_t NDS9_read8(uint32_t address);
	void NDS9_write8(uint32_t address, uint8_t value);

	uint16_t NDS9_read16(uint32_t address);
	void NDS9_write16(uint32_t address, uint16_t value);

	uint32_t NDS9_read32(uint32_t address);
	void NDS9_write32(uint32_t address, uint32_t value);

	//Handle NDS7 IO
	uint8_t NDS7_readIO8(uint32_t address);
	void NDS7_writeIO8(uint32_t address, uint8_t value);

	uint16_t NDS7_readIO16(uint32_t address);
	void NDS7_writeIO16(uint32_t address, uint16_t value);

	uint32_t NDS7_readIO32(uint32_t address);
	void NDS7_writeIO32(uint32_t address, uint32_t value);

	//Handle NDS9 IO
	uint8_t NDS9_readIO8(uint32_t address);
	void NDS9_writeIO8(uint32_t address, uint8_t value);

	uint16_t NDS9_readIO16(uint32_t address);
	void NDS9_writeIO16(uint32_t address, uint16_t value);

	uint32_t NDS9_readIO32(uint32_t address);
	void NDS9_writeIO32(uint32_t address, uint32_t value);

	//helper functions for reading/writing wide values etc.
	static inline uint16_t getValue16(uint8_t* arr, int base, int mask)
	{
		return *(uint16_t*)(arr + base);
	}

	static inline void setValue16(uint8_t* arr, int base, int mask, uint16_t val)
	{
		*(uint16_t*)(arr + base) = val;
	}

	static inline uint32_t getValue32(uint8_t* arr, int base, int mask)
	{
		return *(uint32_t*)(arr + base);
	}

	static inline void setValue32(uint8_t* arr, int base, int mask, uint32_t val)
	{
		*(uint32_t*)(arr + base) = val;
	}

	inline uint8_t* getVRAMAddr(uint32_t address)
	{
		uint8_t* addr = nullptr;
		switch ((address >> 20) & 0xF)
		{
		case 0: case 1:
			addr = m_ppu->mapABgAddress(address);
			break;
		case 2: case 3:
			addr = m_ppu->mapBBgAddress(address);
			break;
		case 4: case 5:
			addr = m_ppu->mapAObjAddress(address);
			break;
		case 6: case 7:
			addr = m_ppu->mapBObjAddress(address);
			break;
		case 8: case 9: case 0xA: case 0xB: case 0xC: case 0xD: case 0xE: case 0xF:
			addr = m_ppu->mapLCDCAddress(address);
			break;
		}
		return addr;
	}

	static void NDS9_HBlankDMACallback(void* context);
	static void NDS9_VBlankDMACallback(void* context);
	static void NDS9_GXFIFODMACallback(void* context);
	static void NDS9_CartridgeDMACallback(void* context);
	static void NDS7_CartridgeDMACallback(void* context);

	bool ARM7_halt = false;
private:
	NDSMem* m_mem;
	Scheduler* m_scheduler;
	InterruptManager* m_interruptManager;
	PPU* m_ppu;
	GPU* m_gpu;
	Input* m_input;
	IPC m_ipc;
	DSMath m_math;
	Timer m_NDS7Timer;
	Timer m_NDS9Timer;
	Cartridge* m_cartridge;
	SPI m_spi;
	RTC m_rtc;
	WifiStub m_wifi;
	APU m_apu;

	DMAChannel m_NDS7Channels[4] = {};
	DMAChannel m_NDS9Channels[4] = {};

	uint8_t NDS7_readDMAReg(uint32_t address);
	void NDS7_writeDMAReg(uint32_t address, uint8_t value);
	void NDS7_checkDMAChannel(int channel);
	void NDS7_doDMATransfer(int channel);
	void NDS7DMA_onTrigger(int trigger);

	uint8_t NDS9_readDMAReg(uint32_t address);
	void NDS9_writeDMAReg(uint32_t address, uint8_t value);
	void NDS9_checkDMAChannel(int channel);
	void NDS9_doDMATransfer(int channel);
	void NDS9DMA_onTrigger(int trigger);

	uint8_t WRAMCNT = 0;
	uint16_t EXMEMCNT = {};
	uint8_t NDS7_POSTFLG = 0;
	uint8_t NDS9_POSTFLG = 0;
	uint16_t hack_soundBias = 0;

	inline void setByteInWord(uint32_t* word, uint8_t byte, int pos);
	inline void setByteInHalfword(uint16_t* halfword, uint8_t byte, int pos);

	inline uint32_t addressToPage(uint32_t address) { return address >> 14; }
	inline uint32_t addressToOffset(uint32_t address) { return address & 0x3FFF; }

	uint8_t** m_ARM9PageTable;
	uint8_t** m_ARM7PageTable;
};