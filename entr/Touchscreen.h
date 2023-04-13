#pragma once

#include"Logger.h"
#include"SPIDevice.h"

enum class TouchscreenState
{
	AwaitCommand,
	ReadPositionUpper,
	ReadPositionLower,
	Dummy
};

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
	TouchscreenState m_state = TouchscreenState::AwaitCommand;
	uint16_t m_data;
	int adcx1 = 0, adcx2 = 0, adcy1 = 0, adcy2 = 0, scrx1 = 0, scrx2 = 0, scry1 = 0, scry2 = 0;	//todo: fill out
	uint16_t adcy = 0, adcx = 0;
};