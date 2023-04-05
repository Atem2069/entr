#pragma once

#include"NDSMem.h"
#include"Logger.h"
#include"PPU.h"
#include"Input.h"
#include"IPC.h"
#include"Math.h"
#include"Timer.h"
#include"Cartridge.h"

#include<format>

struct DMAChannel
{
	//vals CPU writes to dma registers
	uint32_t src;
	uint32_t dest;
	uint16_t wordCount;

	//latched regs at dma start or repeat
	uint32_t internalSrc;
	uint32_t internalDest;
	uint16_t internalWordCount;

	uint16_t control;

	//only for NDS9 DMAs
	uint32_t scratchpad;
};

class Bus
{
public:
	Bus(std::vector<uint8_t> NDS7_BIOS, std::vector<uint8_t> NDS9_BIOS, std::shared_ptr<Scheduler> scheduler, std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<PPU> ppu, std::shared_ptr<Input> input);
	~Bus();

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
		return (uint16_t)arr[base] | ((arr[(base + 1) & mask]) << 8);
	}

	static inline void setValue16(uint8_t* arr, int base, int mask, uint16_t val)
	{
		arr[base] = val & 0xFF;
		arr[(base + 1) & mask] = ((val >> 8) & 0xFF);
	}

	static inline uint32_t getValue32(uint8_t* arr, int base, int mask)
	{
		return (uint32_t)arr[base] | ((arr[(base + 1) & mask]) << 8) | ((arr[(base + 2) & mask]) << 16) | ((arr[(base + 3) & mask]) << 24);
	}

	static inline void setValue32(uint8_t* arr, int base, int mask, uint32_t val)
	{
		arr[base] = val & 0xFF;
		arr[(base + 1) & mask] = ((val >> 8) & 0xFF);
		arr[(base + 2) & mask] = ((val >> 16) & 0xFF);
		arr[(base + 3) & mask] = ((val >> 24) & 0xFF);
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

private:
	std::shared_ptr<NDSMem> m_mem;
	std::shared_ptr<Scheduler> m_scheduler;
	std::shared_ptr<InterruptManager> m_interruptManager;
	std::shared_ptr<PPU> m_ppu;
	std::shared_ptr<Input> m_input;
	std::shared_ptr<IPC> m_ipc;
	std::shared_ptr<DSMath> m_math;
	std::shared_ptr<Timer> m_NDS7Timer;
	std::shared_ptr<Timer> m_NDS9Timer;
	std::shared_ptr<Cartridge> m_cartridge;

	DMAChannel m_NDS7Channels[4] = {};
	DMAChannel m_NDS9Channels[4] = {};

	uint8_t NDS7_readDMAReg(uint32_t address);
	void NDS7_writeDMAReg(uint32_t address, uint8_t value);
	void NDS7_checkDMAChannel(int channel);
	void NDS7_doDMATransfer(int channel);

	uint8_t NDS9_readDMAReg(uint32_t address);
	void NDS9_writeDMAReg(uint32_t address, uint8_t value);
	void NDS9_checkDMAChannel(int channel);
	void NDS9_doDMATransfer(int channel);

	uint8_t WRAMCNT = 0;
	uint16_t EXMEMCNT = {};

	void setByteInWord(uint32_t* word, uint8_t byte, int pos);
	void setByteInHalfword(uint16_t* halfword, uint8_t byte, int pos);
};