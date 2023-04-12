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

class NDS
{
public:
	NDS();
	~NDS();

	void run();
	void notifyDetach();	
	void* getPPUData();
	static void onEvent(void* context);
private:
	std::shared_ptr<ARM946E> ARM9;
	std::shared_ptr<ARM7TDMI> ARM7;
	std::shared_ptr<Bus> m_bus;
	std::shared_ptr<PPU> m_ppu;
	std::shared_ptr<InterruptManager> m_interruptManager;
	std::shared_ptr<Input> m_input;
	std::shared_ptr<InputState> m_inputState;
	std::shared_ptr<Scheduler> m_scheduler;
	std::shared_ptr<Cartridge> m_cartridge;

	void m_initialise();
	void m_destroy();

	void frameEventHandler();
	bool m_shouldStop = false;

	std::vector<uint8_t> readFile(const char* name);
};