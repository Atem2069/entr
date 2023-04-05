#pragma once

#include"Logger.h"
#include"InterruptManager.h"

class Cartridge
{
public:
	Cartridge(std::shared_ptr<InterruptManager> interruptManager);
	~Cartridge();

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	uint32_t readGamecard();
private:
	std::shared_ptr<InterruptManager> m_interruptManager;
	uint16_t AUXSPICNT = {};
	uint32_t ROMCTRL = {};

	int transferLength = 0, bytesTransferred = 0;
	bool transferInProgress = false;
};