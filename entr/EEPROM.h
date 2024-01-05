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
	EEPROM(std::string filename);
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

	std::string m_fileName;
	int m_saveSize = {};

	//don't like this. will move it out
	bool readFile(std::vector<uint8_t> &vec, const char* name)
	{
		// open the file:
		std::ifstream file(name, std::ios::binary);
		if (!file)
			return false;

		// Stop eating new lines in binary mode!!!
		file.unsetf(std::ios::skipws);

		// get its size:
		std::streampos fileSize;

		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		// reserve capacity
		vec.reserve(fileSize);

		// read the data:
		vec.insert(vec.begin(),
			std::istream_iterator<uint8_t>(file),
			std::istream_iterator<uint8_t>());

		file.close();

		return true;
	}
};