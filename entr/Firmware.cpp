#include"Firmware.h"

Firmware::Firmware()
{
	m_data = readFile("rom\\firmware.bin");
}

Firmware::~Firmware()
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Saving firmware data..");
	std::ofstream fout("rom\\firmware.bin", std::ios::out | std::ios::binary);
	fout.write((char*)&m_data[0], m_data.size());
	fout.close();
}

uint8_t Firmware::sendCommand(uint8_t value)
{
	switch (m_state)
	{
	case FirmwareState::AwaitCommand:
		decodeCommand(value);
		return 0;
	case FirmwareState::WriteAddress:
	{
		int writeIdx = 16 - (addressProgress << 3);
		m_readAddress &= ~(0xFF << writeIdx);
		m_readAddress |= (value << writeIdx);
		addressProgress++;
		if (addressProgress == 3)
		{
			Logger::getInstance()->msg(LoggerSeverity::Info, std::format("Access to address {:#x}", m_readAddress));
			switch (m_command)
			{
			case 0x03:
				m_state = FirmwareState::ReadData; break;
			case 0x0A:
				m_state = FirmwareState::WriteData; break;
			case 0x02:
				m_state = FirmwareState::ProgramData; break;
			}
		}
		return 0;
	}
	case FirmwareState::ReadData:
	{
		return m_data[m_readAddress++];
	}
	case FirmwareState::WriteData:						//write data/program data shouldn't cross page boundaries.. should take this into account at some point
	{
		m_data[m_readAddress++] = value;
		return 0;
	}
	case FirmwareState::ProgramData:
	{
		uint8_t curData = m_data[m_readAddress];
		m_data[m_readAddress++] = curData & value;
		return 0;
	}
	case FirmwareState::ReadStatus:
		return m_statusReg;
	}
}

void Firmware::deselect()
{
	m_state = FirmwareState::AwaitCommand;
}

void Firmware::decodeCommand(uint8_t value)
{
	switch (value)
	{
	case 0x03: case 0x0A: case 0x02:
		m_state = FirmwareState::WriteAddress;
		m_readAddress = 0;
		addressProgress = 0;
		break;
	case 0x04:
		m_statusReg &= ~(1 << 1);	//clear write enable bit
		m_state = FirmwareState::AwaitCommand;
		break;
	case 0x05:
		m_state = FirmwareState::ReadStatus;
		break;
	case 0x06:
		m_statusReg |= (1 << 1);
		m_state = FirmwareState::AwaitCommand;
		break;

	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unknown firmware command {:#x}", value));
	}

	m_command = value;
}