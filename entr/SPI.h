#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"SPIDevice.h"
#include"Flash.h"
#include"PowerManager.h"
#include"Touchscreen.h"

class SPI
{
public:
	SPI();
	void init(InterruptManager* interruptManager);
	~SPI();

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

private:
	InterruptManager* m_interruptManager;
	std::unique_ptr<SPIDevice> m_SPIDevices[3];
	void writeSPIData(uint8_t value);
	uint16_t SPICNT = 0;
	uint8_t m_SPIData = 0;

	bool chipSelectHold = false;
	uint8_t deviceSelect = 0;
	bool enabled = false;
	bool irq = false;
};