#include"PowerManager.h"

PowerManager::PowerManager()
{

}

PowerManager::~PowerManager()
{

}

uint8_t PowerManager::sendCommand(uint8_t value)
{
	switch (m_state)
	{
	case PowerManagerState::Command: return decodeCommand(value);
	case PowerManagerState::ReadRegister: return readRegister();
	case PowerManagerState::WriteRegister: return writeRegister(value);
	}
}

void PowerManager::deselect()
{
	m_state = PowerManagerState::Command;
}

uint8_t PowerManager::decodeCommand(uint8_t val)
{
	registerIdx = val & 0x7F;
	bool readWrite = (val >> 7) & 0b1;
	if (readWrite)
		m_state = PowerManagerState::ReadRegister;
	else
		m_state = PowerManagerState::WriteRegister;
	return 0;
}

uint8_t PowerManager::readRegister()
{
	switch (registerIdx)
	{
	case 0: return m_controlReg;
	case 1: return m_batteryStatus;
	case 4: return m_backlightControl;
	default: return 0;
	}
}

uint8_t PowerManager::writeRegister(uint8_t val)
{
	switch (registerIdx)
	{
	case 0:
	{
		m_controlReg = val;
		if ((val >> 6) & 0b1)
		{
			Logger::msg(LoggerSeverity::Warn, "Unhandled system shutdown");
			//should actually shutdown here.. not sure of best approach that isn't hacky.
		}
		break;
	}
	case 4: m_backlightControl = val; break;
	}
	return 0;
}