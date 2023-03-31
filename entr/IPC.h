#pragma once

#include"Logger.h"

class IPC
{
public:
	IPC();
	~IPC();

	uint8_t NDS7_read8(uint32_t address);
	void NDS7_write8(uint32_t address, uint8_t value);

	uint8_t NDS9_read8(uint32_t address);
	void NDS9_write8(uint32_t address, uint8_t value);

	//todo: 16/32 bit accesses for IPCFIFO, as we'll have to special case it probably
private:
	uint8_t NDS7_IPCData;
	uint8_t NDS9_IPCData;
};