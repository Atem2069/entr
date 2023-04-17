#include"EEPROM.h"

EEPROM::EEPROM()
{
	memset(m_data, 0xFF, 65536);
	m_addressLatch = 0;
}

EEPROM::~EEPROM()
{

}

uint8_t EEPROM::sendCommand(uint8_t value)
{
	switch (m_state)
	{
	case EEPROMState::AwaitCommand:
		decodeCommand(value);
		break;
	case EEPROMState::WriteAddress:
	{
		uint32_t shiftAmount = 8 - (m_addressProgress * 8);
		m_addressLatch &= ~(0xFF << shiftAmount);
		m_addressLatch |= (value << shiftAmount);
		m_addressProgress++;
		if (m_addressProgress == 2)
		{
			m_addressProgress = 0;
			m_state = m_nextState;
		}
		break;
	}
	case EEPROMState::ReadData:
		return m_data[m_addressLatch++];
	case EEPROMState::WriteData:
		m_data[m_addressLatch++] = value;	//<--todo: write cannot cross page boundary
		break;
	case EEPROMState::ReadStatus:
		return  m_statusRegister;
	case EEPROMState::WriteStatus:
	{
		m_statusRegister &= 0b00000011;				//preserve lower 2 bits
		m_statusRegister |= (value & 0b10001100);
		break;
	}
	}
	return 0;
}

void EEPROM::deselect()
{
	m_state = EEPROMState::AwaitCommand;
}

void EEPROM::decodeCommand(uint8_t value)
{
	switch (value)
	{
	case 0x06:						//write enable
		m_statusRegister |= 0b10;
		break;
	case 0x04:						//write disable
		m_statusRegister &= ~0b10;
		break;
	case 0x05:						//read status register
		m_state = EEPROMState::ReadStatus;
		break;
	case 0x01:						//write status register
		m_state = EEPROMState::WriteStatus;
		break;
	case 0x9F:						//chip id (not supported on eeprom? gbatek says returns $FF)
		Logger::msg(LoggerSeverity::Error, "Tried to read chip ID from EEPROM..");
		break;
	case 0x03:						//read data
		m_state = EEPROMState::WriteAddress;
		m_nextState = EEPROMState::ReadData;
		break;
	case 0x02:						//write data
		m_state = EEPROMState::WriteAddress;
		m_nextState = EEPROMState::WriteData;
		break;
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unknown EEPROM command {:#x}", value));
		break;
	}
}