#pragma once

#include"SPIDevice.h"

class EEPROM : public SPIDevice
{
public:
	EEPROM();
	~EEPROM();

	uint8_t sendCommand(uint8_t value);
	void deselect();
};