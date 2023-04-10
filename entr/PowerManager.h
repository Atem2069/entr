#pragma once

#include"Logger.h"
#include"SPIDevice.h"

class PowerManager : public SPIDevice
{
public:
	PowerManager();
	~PowerManager();

	uint8_t sendCommand(uint8_t value);
	void deselect();
};