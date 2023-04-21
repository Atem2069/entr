#pragma once

#include<iostream>

class SPIDevice
{
public:
	SPIDevice() {};
	virtual ~SPIDevice() {};

	virtual uint8_t sendCommand(uint8_t value) { return 0; };
	virtual void deselect() {};
};