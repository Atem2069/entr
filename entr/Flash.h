#pragma once

#include"Logger.h"
#include"SPIDevice.h"

enum class FlashState
{
	AwaitCommand,
	WriteAddress,
	ReadStatus,
	ReadData,
	WriteData,
	ProgramData
};

class Flash : public SPIDevice
{
public:
	Flash(std::string fileName);
	~Flash();

	uint8_t sendCommand(uint8_t value);
	void deselect();

private:
	uint8_t m_data[1048576];
	uint32_t m_readAddress = 0;
	uint32_t addressProgress = 0;
	uint8_t m_statusReg = 0;

	FlashState m_state = FlashState::AwaitCommand;
	FlashState m_nextState = FlashState::AwaitCommand;
	uint8_t m_command = 0;

	void decodeCommand(uint8_t value);

	std::string m_fileName;
	int m_saveSize = {};

	//don't like this. will move it out
	bool readFile(std::vector<uint8_t>& vec, const char* name)
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