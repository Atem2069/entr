#include"Flash.h"

Flash::Flash(std::string fileName)
{

	std::vector<uint8_t> fileData;
	if (!readFile(fileData, fileName.c_str()))
	{
		m_saveSize = 256 * 1024;	//shouldn't be 256k automatically.
		memset(m_data, 0xFF, 1048576);
	}
	else
	{
		//todo: handle different EEPROM sizes
		std::copy(fileData.begin(), fileData.begin() + std::min((size_t)1048576, fileData.size()), m_data);
		m_saveSize = fileData.size();
	}
	m_fileName = fileName;
}

Flash::~Flash()
{
	Logger::msg(LoggerSeverity::Info, "Saving flash data..");
	std::ofstream fout(m_fileName, std::ios::out | std::ios::binary);
	fout.write((char*)&m_data[0], std::min(m_saveSize, 1048576));
	fout.close();
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
			addressProgress = 0;
			m_state = m_nextState;
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

	return 0;
}

void Flash::deselect()
{
	m_state = FlashState::AwaitCommand;
}

void Flash::decodeCommand(uint8_t value)
{
	switch (value)
	{
	case 0x03:
		m_state = FlashState::WriteAddress;
		m_nextState = FlashState::ReadData;
		break;
	case 0x0A:
		m_state = FlashState::WriteAddress;
		m_nextState = FlashState::WriteData;
		break;
	case 0x02:
		m_state = FlashState::WriteAddress;
		m_nextState = FlashState::ProgramData;
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