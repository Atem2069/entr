#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Firmware.h"

class SPI
{
public:
	SPI(std::shared_ptr<InterruptManager> interruptManager);
	~SPI();

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

private:
	std::shared_ptr<InterruptManager> m_interruptManager;
	std::shared_ptr<Firmware> m_firmware;

	void writeSPIData(uint8_t value);
	uint16_t SPICNT = 0;
	uint8_t m_SPIData = 0;

	bool chipSelectHold = false;
	uint8_t deviceSelect = 0;
	bool enabled = false;
	bool irq = false;
};