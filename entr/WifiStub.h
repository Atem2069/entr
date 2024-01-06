#pragma once

#include<iostream>
#include"Logger.h"

class WifiStub
{
public:
	WifiStub() {}
	~WifiStub() {}

	uint8_t read(uint32_t address)
	{
		if (address >= 0x04808000 && address <= 0x04808FFF)
		{
			switch (address)
			{
			//W_POWERSTATE.9 forced to 1 (disabled)
			case 0x0480803d:
				return 2;
			case 0x0480815C:
				return W_BB_READ & 0xFF;
			//idk if necessary
			case 0x0480815E:
				return 0;
			}
			return IO[address & 0xFFF];
		}

		if (address >= 0x04804000 && address <= 0x04805FFF)
		{
			return WifiRAM[address & 0x1FFF];
		}
	}

	void write(uint32_t address, uint8_t value)
	{
		if (address >= 0x04808000 && address <= 0x04808FFF)
		{
			switch (address)
			{
			case 0x04808158:
				W_BB_CNT &= 0xFF00; W_BB_CNT |= value;
				break;
			case 0x04808159:
			{
				W_BB_CNT &= 0xFF; W_BB_CNT |= (value << 8);
				uint8_t direction = (W_BB_CNT >> 12) & 0xF;
				if (direction == 5)
				{
					BBRAM[W_BB_CNT & 0xFF] = W_BB_WRITE & 0xFF;
				}
				if (direction == 6)
				{
					W_BB_READ = BBRAM[W_BB_CNT & 0xFF];
				}
				break;
			}
			case 0x0480815A:
				W_BB_WRITE = value;
			break;
			}
			IO[address & 0xFFF] = value;
		}

		if (address >= 0x04804000 && address <= 0x04805FFF)
		{
			WifiRAM[address & 0x1FFF] = value;
		}
	}
private:
	uint8_t IO[0x1000] = {};
	uint8_t WifiRAM[0x2000] = {};

	uint16_t W_BB_CNT = {};
	uint16_t W_BB_READ = {};
	uint16_t W_BB_WRITE = {};
	uint8_t BBRAM[256] = {};
};