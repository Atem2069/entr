#pragma once

#include"Logger.h"
#include"SPIDevice.h"
#include"Config.h"

enum class PowerManagerState
{
	Command,
	ReadRegister,
	WriteRegister
};

class PowerManager : public SPIDevice
{
public:
	PowerManager();
	~PowerManager();

	uint8_t sendCommand(uint8_t value);
	void deselect();

private:
	PowerManagerState m_state = PowerManagerState::Command;
	uint8_t registerIdx = {};

	uint8_t decodeCommand(uint8_t val);
	uint8_t readRegister();
	uint8_t writeRegister(uint8_t val);

	uint8_t m_controlReg = 0;
	uint8_t m_batteryStatus = 1;
	uint8_t m_backlightControl = 0;
};