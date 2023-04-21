#pragma once

#include"Logger.h"
#include"InterruptManager.h"

struct IPCFIFO
{
	uint32_t lastWord = 0;
	int front = 0, back = 0;
	int size = 0;
	uint32_t buffer[16];

	void push(uint32_t word)
	{
		if (!enabled || (size==16))
			return;
		buffer[back++] = word;
		back &= 0xF;	//restrict to [0..15] (circular queue)
		size++;
	}

	uint32_t pop()
	{
		if (!enabled)
		{
			lastWord = buffer[front];
			return lastWord;
		}

		if (size == 0)
			return lastWord;

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
	bool enabled = false;
	bool IRQOnEmpty = false;		//triggers irq when send fifo is empty				(need to implement both of these)
	bool IRQOnReceive = false;		//triggers irq when receive fifo is not empty            ^^
};

class IPC
{
public:
	IPC();
	void init(InterruptManager* interruptManager);
	~IPC();

	uint8_t NDS7_read8(uint32_t address);
	void NDS7_write8(uint32_t address, uint8_t value);

	uint8_t NDS9_read8(uint32_t address);
	void NDS9_write8(uint32_t address, uint8_t value);

	//idk if we need to handle 16 bit writes for IPCFIFO, but..
	uint32_t NDS7_read32(uint32_t address);
	void NDS7_write32(uint32_t address, uint32_t value);

	uint32_t NDS9_read32(uint32_t address);
	void NDS9_write32(uint32_t address, uint32_t value);
private:
	InterruptManager* m_interruptManager;

	uint8_t NDS7_IPCData;
	uint8_t NDS9_IPCData;

	IPCFIFO NDS7_IPCFIFO;
	IPCFIFO NDS9_IPCFIFO;
};