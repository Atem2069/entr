#pragma once

#include"Logger.h"

enum class InterruptType
{
	VBlank = (1 << 0),
	HBlank = (1 << 1),
	VCount = (1 << 2),
	Timer0 = (1 << 3),
	Timer1 = (1 << 4),
	Timer2 = (1 << 5),
	Timer3 = (1 << 6),
	SIO = (1 << 7),
	DMA0 = (1 << 8),
	DMA1 = (1 << 9),
	DMA2 = (1 << 10),
	DMA3 = (1 << 11),
	Keypad = (1 << 12),
	GBASlot = (1 << 13),
	IPCSync = (1 << 16),
	IPCSend = (1 << 17),
	IPCReceive = (1 << 18)
	//todo: more interrupt sources
};

class InterruptManager
{
public:
	InterruptManager();
	~InterruptManager();

	void NDS7_requestInterrupt(InterruptType type);
	bool NDS7_getInterruptsEnabled() { return NDS7_IME & 0b1; }
	bool NDS7_getInterrupt();

	uint8_t NDS7_readIO(uint32_t address);
	void NDS7_writeIO(uint32_t address, uint8_t value);

	void NDS9_requestInterrupt(InterruptType type);
	bool NDS9_getInterruptsEnabled() { return NDS9_IME & 0b1; }
	bool NDS9_getInterrupt();

	uint8_t NDS9_readIO(uint32_t address);
	void NDS9_writeIO(uint32_t address, uint8_t value);

private:
	uint8_t NDS7_IME = {};
	uint32_t NDS7_IE = {};
	uint32_t NDS7_IF = {};

	uint8_t NDS9_IME = {};
	uint32_t NDS9_IE = {};
	uint32_t NDS9_IF = {};
};