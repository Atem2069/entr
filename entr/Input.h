#pragma once

#include<iostream>
#include"Logger.h"
#include"Scheduler.h"
#include"InterruptManager.h"

union InputState
{
	uint16_t reg;
	struct
	{
		bool A : 1;
		bool B : 1;
		bool Select : 1;
		bool Start : 1;
		bool Right : 1;
		bool Left : 1;
		bool Up : 1;
		bool Down : 1;
		bool R : 1;
		bool L : 1;
		bool unused10 : 1;
		bool unused11 : 1;
		bool unused12 : 1;
		bool unused13 : 1;
		bool unused14 : 1;
		bool unused15 : 1;
	};
};

union ExtInputState
{
	uint8_t reg;
	struct
	{
		bool X : 1;
		bool Y : 1;
		bool unused2 : 1;
		bool unused3 : 1;
		bool unused4 : 1;
		bool unused5 : 1;
		bool penDown : 1;
		bool unused7 : 1;
	};
};

class Input
{
public:
	Input();
	~Input();
	void registerInterrupts(InterruptManager* interruptManager);

	uint8_t readIORegister(uint32_t address);
	void writeIORegister(uint32_t address, uint8_t value);
	void tick();
	bool getIRQConditionsMet();

	static InputState inputState;
	static ExtInputState extInputState;
private:
	void checkIRQ();

	InterruptManager* m_interruptManager;
	uint64_t lastEventTime = 0;
	uint16_t keyInput = 0;
	uint16_t KEYCNT = 0;
	uint8_t extKeyInput = 0;
	bool irqActive = false;
};