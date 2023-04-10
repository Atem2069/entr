#include"Firmware.h"

Firmware::Firmware()
{
	m_data = readFile("rom\\firmware.bin");
}

Firmware::~Firmware()
{

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
			m_state = FirmwareState::TransferData;
		}
		return 0;
	}
	case FirmwareState::TransferData:
	{
		return m_data[m_readAddress++];
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
	case 0x03:
		Logger::getInstance()->msg(LoggerSeverity::Info, "Firmware read command!");
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
}