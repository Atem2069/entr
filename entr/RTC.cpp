#include"RTC.h"

RTC::RTC()
{
	m_state = GPIOState::Ready;
	m_dataLatch = 0;
}

RTC::~RTC()
{

}

uint8_t RTC::read(uint32_t address)
{
	switch (address)
	{
	case 0x04000138:
		return RTCReg & (~dataDirectionMask) & 0xFF;
	}
}

void RTC::write(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000138:
		dataDirectionMask = ((value >> 4) & 0b1);
		m_writeDataRegister(value);
	}
}

void RTC::m_writeDataRegister(uint8_t value)
{
	uint8_t oldData = RTCReg;
	if (!dataDirectionMask)
	{
		value &= ~0b1;
		value |= (RTCReg & 0b1);
	}
	RTCReg = value;
	bool csRising = (~((oldData >> 2) & 0b1)) & ((RTCReg >> 2) & 0b1);	//CS low before, now high
	bool csFalling = ((oldData >> 2) & 0b1) & (~((RTCReg >> 2) & 0b1));	//CS high before, now low
	bool sckFalling = ((oldData >> 1) & 0b1) & (~((RTCReg >> 1) & 0b1));
	bool sckRising = (~(oldData >> 1) & 0b1) & (((RTCReg >> 1) & 0b1));
	bool sckLow = ~(((RTCReg >> 1)) & 0b1);
	switch (m_state)
	{
	case GPIOState::Ready:
	{
		if (csRising)	//not sure if SCK should be low :P
		{
			m_state = GPIOState::Command;
			m_shiftCount = 0;
			m_command = 0;
			m_dataLatch = 0;
			//Logger::msg(LoggerSeverity::Info, "GPIO transfer started..");
		}
		break;
	}

	case GPIOState::Command:
	{
		if (sckFalling)
		{
			uint8_t serialData = (RTCReg) & 0b1;
			m_command |= (serialData << m_shiftCount);
			m_shiftCount++;

			if (m_shiftCount == 8)
			{
				m_shiftCount = 0;
				if ((m_command & 0xF0) != 0b01100000)	//bits 4-7 should be 0110, otherwise flip
					m_command = m_reverseBits(m_command);

				m_updateRTCRegisters();

				//should decode bits 1-3 here, decide which register being read/written (important to determine byte size for r/w)...
				uint8_t rtcRegisterIdx = (m_command >> 1) & 0b111;

				//bit 0 denotes whether read/write command. 0=write,1=read
				if (m_command & 0b1)
				{
					switch (rtcRegisterIdx)
					{
					case 0:
						m_dataLatch = controlReg;
						break;
					case 2:
						m_dataLatch = ((uint64_t)timeReg << 32) | dateReg;
						break;
					case 3:
						m_dataLatch = timeReg;
						break;
					case 4:
						m_dataLatch = 0;
						break;
					}

					//Logger::msg(LoggerSeverity::Info, "Process GPIO read command..");
					m_state = GPIOState::Read;
				}
				else
				{
					//Logger::msg(LoggerSeverity::Info, "Process GPIO write command..");
					m_state = GPIOState::Write;
				}
			}
		}
		break;
	}

	case GPIOState::Read:
	{
		if (sckFalling)	//lmao, what??
		{
			//data &= ~0b010;
			if (!dataDirectionMask)
			{
				RTCReg &= ~0b1;
				uint64_t outBit = (m_dataLatch >> m_shiftCount) & 0b1;
				RTCReg |= outBit;
			}
			m_shiftCount++;
		}

		if (csFalling)
		{
			m_state = GPIOState::Ready;
			//Logger::msg(LoggerSeverity::Info, "GPIO transfer ended!");
		}
		break;
	}

	case GPIOState::Write:
	{
		//todo
		if (sckFalling)
		{
			uint64_t inBit = RTCReg & 0b1;
			m_dataLatch |= (inBit << m_shiftCount);
			m_shiftCount++;
		}

		if (csFalling)											//actually handle write when GPIO transfer ends? might not be accurate
		{
			uint8_t rtcRegisterIndex = (m_command >> 1) & 0b111;
			switch (rtcRegisterIndex)
			{
			case 0:
				controlReg = m_dataLatch & 0b00001110;
				//Logger::msg(LoggerSeverity::Info, std::format("RTC control write: {:#x}", m_dataLatch));
				break;
			case 2:
				//Logger::msg(LoggerSeverity::Warn, "Ignoring write to Date/Time RTC register...");
				break;
			case 3:
				//Logger::msg(LoggerSeverity::Warn, "Ignoring write to Time RTC register...");
				break;
			case 6:
				//Logger::msg(LoggerSeverity::Warn, "Write to IRQ RTC register ???");
				break;
			default:
				//Logger::msg(LoggerSeverity::Error, "Unsupported write to RTC register..");
				break;
			}
			m_state = GPIOState::Ready;
			//Logger::msg(LoggerSeverity::Info, "GPIO transfer ended!");
		}
		break;
	}

	}

}

void RTC::m_updateRTCRegisters()
{
	//update rtc registers to include current date/time in BCD.
	std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);

	uint8_t years = m_convertToBCD(now->tm_year - 100);
	uint8_t months = m_convertToBCD(now->tm_mon + 1);
	uint8_t days = m_convertToBCD(now->tm_mday);
	uint8_t dayOfWeek = now->tm_wday;
	if (dayOfWeek == 0)
		dayOfWeek = 6;

	dateReg = (dayOfWeek << 24) | (days << 16) | (months << 8) | years;

	int AMPMFlag = (now->tm_hour / 12) & 0b1;			//does this flag even exist? lol
	uint8_t hours = m_convertToBCD(now->tm_hour % (((controlReg >> 1) & 0b1) ? 24 : 12)) | (AMPMFlag << 7);
	uint8_t minutes = m_convertToBCD(now->tm_min);
	uint8_t seconds = m_convertToBCD(now->tm_sec);
	timeReg = (seconds << 16) | (minutes << 8) | hours;
}

uint8_t RTC::m_reverseBits(uint8_t a)
{
	uint8_t b = 0;
	//slow...
	for (int i = 0; i < 8; i++)
	{
		uint8_t bit = (a >> i) & 0b1;
		b |= (bit << (7 - i));
	}
	return b;
}

uint8_t RTC::m_convertToBCD(int val)
{
	int units = val % 10;
	int tens = val / 10;

	uint8_t res = (tens << 4) | units;
	return res;
}