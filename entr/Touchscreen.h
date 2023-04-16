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

	static int screenX;
	static int screenY;
	static bool pressed;
private:
	uint16_t m_data;
	int adcx1 = 0, adcy1 = 0, scrx1 = 0, scry1 = 0, adcx2 = 0xFF0, adcy2 = 0xBF0, scrx2 = 0xFF, scry2 = 0xBF;	//values ripped from firmware. should pull them automatically :)
	uint16_t adcy = 0, adcx = 0;
};