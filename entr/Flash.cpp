#include"Flash.h"

Flash::Flash(std::string fileName)
{
	if (fileName.length() > 0)
	{
		writeback = true;
		m_data = readFile(fileName.c_str());
	}
	else
	{
		m_data.reserve(16777216);	//hack..
		for (int i = 0; i < m_data.size(); i++)
			m_data[i] = 0xFF;
	}
}

Flash::~Flash()
{
	if (writeback)
	{
		Logger::msg(LoggerSeverity::Info, "Saving flash data..");
		std::ofstream fout("rom\\firmware.bin", std::ios::out | std::ios::binary);
		fout.write((char*)&m_data[0], m_data.size());
		fout.close();
	}
}

uint8_t Flash::sendCommand(uint8_t value)
{
	switch (m_state)
	{
	case FlashState::AwaitCommand:
		decodeCommand(value);
		return 0;
	case FlashState::WriteAddress:
	{
		int writeIdx = 16 - (addressProgress << 3);
		m_readAddress &= ~(0xFF << writeIdx);
		m_readAddress |= (value << writeIdx);
		addressProgress++;
		if (addressProgress == 3)
		{
			Logger::msg(LoggerSeverity::Info, std::format("Access to address {:#x}", m_readAddress));
			switch (m_command)
			{
			case 0x03:
				m_state = FlashState::ReadData; break;
			case 0x0A:
				m_state = FlashState::WriteData; break;
			case 0x02:
				m_state = FlashState::ProgramData; break;
			}
		}
		return 0;
	}
	case FlashState::ReadData:
	{
		return m_data[m_readAddress++];
	}
	case FlashState::WriteData:						//write data/program data shouldn't cross page boundaries.. should take this into account at some point
	{
		m_data[m_readAddress++] = value;
		return 0;
	}
	case FlashState::ProgramData:
	{
		uint8_t curData = m_data[m_readAddress];
		m_data[m_readAddress++] = curData & value;
		return 0;
	}
	case FlashState::ReadStatus:
		return m_statusReg;
	}
}

void Flash::deselect()
{
	m_state = FlashState::AwaitCommand;
}

void Flash::decodeCommand(uint8_t value)
{
	switch (value)
	{
	case 0x03: case 0x0A: case 0x02:
		m_state = FlashState::WriteAddress;
		m_readAddress = 0;
		addressProgress = 0;
		break;
	case 0x04:
		m_statusReg &= ~(1 << 1);	//clear write enable bit
		m_state = FlashState::AwaitCommand;
		break;
	case 0x05:
		m_state = FlashState::ReadStatus;
		break;
	case 0x06:
		m_statusReg |= (1 << 1);
		m_state = FlashState::AwaitCommand;
		break;

	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unknown flash command {:#x}", value));
	}

	m_command = value;
}