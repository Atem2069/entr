#pragma once

#include"NDSMem.h"
#include"Logger.h"
#include"PPU.h"
#include"Input.h"
#include"IPC.h"

#include<format>

class Bus
{
public:
	Bus(std::shared_ptr<PPU> ppu, std::shared_ptr<Input> input);
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
	static inline uint16_t getValue16(uint8_t* arr, int base, int mask);
	static inline void setValue16(uint8_t* arr, int base, int mask, uint16_t val);

	static inline uint32_t getValue32(uint8_t* arr, int base, int mask);
	static inline void setValue32(uint8_t* arr, int base, int mask, uint32_t val);

private:
	std::shared_ptr<NDSMem> m_mem;
	std::shared_ptr<PPU> m_ppu;
	std::shared_ptr<Input> m_input;
	std::shared_ptr<IPC> m_ipc;

	void setByteInWord(uint32_t* word, uint8_t byte, int pos);
	void setByteInHalfword(uint16_t* halfword, uint8_t byte, int pos);
};