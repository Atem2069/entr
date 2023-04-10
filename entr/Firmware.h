#pragma once

#include"Logger.h"

enum class FirmwareState
{
	AwaitCommand,
	WriteAddress,
	ReadStatus,
	TransferData
};

class Firmware
{
public:
	Firmware();
	~Firmware();

	uint8_t sendCommand(uint8_t value);
	void deselect();

private:
	std::vector<uint8_t> m_data;
	uint32_t m_readAddress = 0;
	uint32_t addressProgress = 0;
	uint8_t m_statusReg = 0;

	FirmwareState m_state = FirmwareState::AwaitCommand;

	void decodeCommand(uint8_t value);

	//don't like this. will move it out
	std::vector<uint8_t> readFile(const char* name)
	{
		// open the file:
		std::ifstream file(name, std::ios::binary);

		// Stop eating new lines in binary mode!!!
		file.unsetf(std::ios::skipws);

		// get its size:
		std::streampos fileSize;

		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		// reserve capacity
		std::vector<uint8_t> vec;
		vec.reserve(fileSize);

		// read the data:
		vec.insert(vec.begin(),
			std::istream_iterator<uint8_t>(file),
			std::istream_iterator<uint8_t>());

		file.close();

		return vec;
	}
};