#pragma once

#include<iostream>
#include<vector>

#include"Logger.h"
#include"Bus.h"
#include"ARM7/ARM7TDMI.h"
#include"ARM9/ARM946E.h"
#include"PPU.h"
#include"InterruptManager.h"
#include"Input.h"
#include"Scheduler.h"
#include"Cartridge.h"
#include"Config.h"

class NDS
{
public:
	NDS();
	~NDS();

	bool initialise();

	void run();
	void notifyDetach();	
	static void onEvent(void* context);
private:
	ARM946E ARM9;
	ARM7TDMI ARM7;
	Bus m_bus;
	PPU m_ppu;
	InterruptManager m_interruptManager;
	Input m_input;
	InputState m_inputState;
	Scheduler m_scheduler;
	Cartridge m_cartridge;

	void m_destroy();

	void frameEventHandler();
	bool m_shouldStop = false;

	bool readFile(std::vector<uint8_t>& vec, const char* name);
};