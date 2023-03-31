#pragma once

#include"Logger.h"

struct IPCFIFO
{
	uint32_t lastWord = 0;
	int front = 0, back = 0;
	int size = 0;
	uint32_t buffer[16];

	void push(uint32_t word)
	{
		if (size == 16)
			error = true;
		buffer[back++] = word;
		back &= 0xF;	//restrict to [0..15] (circular queue)
		size++;
	}

	uint32_t pop()
	{
		if (size == 0)
		{
			error = true;
			return lastWord;
		}

		lastWord = buffer[front++];
		front &= 0xF;	//restrict to [0..15]
		size--;
		return lastWord;
	}

	void clear()
	{
		front = 0;
		back = 0;
		size = 0;
		lastWord = 0;
	}

	bool error = false;
};

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