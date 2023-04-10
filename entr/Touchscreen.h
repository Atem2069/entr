#pragma once

#include"Logger.h"
#include"SPIDevice.h"

class Touchscreen : public SPIDevice
{
public:
	Touchscreen();
	~Touchscreen();

	uint8_t sendCommand(uint8_t value);
	void deselect();
};