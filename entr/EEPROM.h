#pragma once

#include"SPIDevice.h"
#include"Logger.h"

enum class EEPROMState
{
	AwaitCommand,
	WriteAddress,
	ReadData,
	WriteData,
	ReadStatus,
	WriteStatus
};

class EEPROM : public SPIDevice				//<--todo: distinguish between small eeprom (9 bit addresses) and larger eeprom chips (up to 16 bit addresses)
{
public:
	EEPROM();
	~EEPROM();

	uint8_t sendCommand(uint8_t value);
	void deselect();
private:
	EEPROMState m_state = EEPROMState::AwaitCommand;
	EEPROMState m_nextState = EEPROMState::AwaitCommand;
	uint8_t m_data[65536];			
	uint16_t m_addressLatch = 0;
	int m_addressProgress = 0;

	uint8_t m_statusRegister = 0;

	void decodeCommand(uint8_t value);
};