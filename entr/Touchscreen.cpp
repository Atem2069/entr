#include"Touchscreen.h"

Touchscreen::Touchscreen()
{
	m_data = 0;
}

Touchscreen::~Touchscreen()
{

}

uint8_t Touchscreen::sendCommand(uint8_t value)
{
	uint8_t result = (m_data & 0xFF);	//janky
	m_data >>= 8;

	if ((value >> 7) & 0b1)			//update adc y, adc x
	{
		adcy = 0xFFF;
		adcx = 0;
		if (pressed)
		{
			adcy = ((screenY - scry1 + 1) * (adcy2 - adcy1) / (scry2 - scry1) + adcy1) << 3;
			adcx = ((screenX - scrx1 + 1) * (adcx2 - adcx1) / (scrx2 - scrx1) + adcx1) << 3;
		}
	}

	uint8_t channelSelect = ((value >> 4) & 0b111);
	switch (channelSelect)
	{
	case 1:			//y
		m_data = ((adcy >> 8) & 0xFF) | (adcy & 0xFF);
		break;
	case 5:			//x
		m_data = ((adcx >> 8) & 0xFF) | (adcx & 0xFF);
		break;
	}
	return result;
}

void Touchscreen::deselect()
{
	m_state = TouchscreenState::AwaitCommand;
}

int Touchscreen::screenX = 0;
int Touchscreen::screenY = 0;
bool Touchscreen::pressed = false;