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
	bool start = ((value >> 7) & 0b1);
	uint8_t result = (m_data >> 8) & 0xFF;
	m_data <<= 8;

	if (start)			//update output data
	{
		adcy = 0xFFF8;
		adcx = 0;
		if (pressed)
		{
			adcy = ((screenY - scry1 + 1) * (adcy2 - adcy1) / (scry2 - scry1) + adcy1) << 3;
			adcx = ((screenX - scrx1 + 1) * (adcx2 - adcx1) / (scrx2 - scrx1) + adcx1) << 3;
		}

		uint8_t channelSelect = ((value >> 4) & 0b111);
		switch (channelSelect)
		{
		case 1:			//y
			m_data = adcy;
			break;
		case 5:			//x
			m_data = adcx;
			break;
		default:
			m_data = 0;
		}
	}
	return result;
}

void Touchscreen::deselect()
{
	m_data = 0;	//not sure if the data latch or whatever is really cleared whenever tsc deselected
}

int Touchscreen::screenX = 0;
int Touchscreen::screenY = 0;
bool Touchscreen::pressed = false;