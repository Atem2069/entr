#pragma once

#include"Logger.h"
#include"InterruptManager.h"

class Cartridge
{
public:
	Cartridge(std::shared_ptr<InterruptManager> interruptManager);
	~Cartridge();

	uint8_t NDS7_read(uint32_t address);
	void NDS7_write(uint32_t address, uint8_t value);
	uint32_t NDS7_readGamecard();

	uint8_t NDS9_read(uint32_t address);
	void NDS9_write(uint32_t address, uint8_t value);
	uint32_t NDS9_readGamecard();

	void setNDS7AccessRights(bool val);
private:
	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	uint32_t readGamecard();

	std::shared_ptr<InterruptManager> m_interruptManager;
	uint16_t AUXSPICNT = {};
	uint32_t ROMCTRL = {};

	int transferLength = 0, bytesTransferred = 0;
	bool transferInProgress = false;
	bool NDS7HasAccess = false;
	uint64_t cartCommand = 0;
};