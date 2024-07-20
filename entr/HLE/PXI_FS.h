#pragma once

#include"../Logger.h"

class Bus;

//stub for PXI FS subsystem.
class PXIFS
{
public:
	PXIFS();
	~PXIFS();

	void init(Bus* bus);

	void processPXICmd(uint32_t cmd);

private:
	Bus* m_bus;
	bool initialized = false;
	bool waitForArgPtr = false;

	uint32_t argPtr = {};

	std::string cardCmdNames[16] = { "CARD_REQ_INIT", "CARD_REQ_ACK", "CARD_REQ_IDENTIFY", "CARD_REQ_READ_ID", "CARD_REQ_READ_ROM", "CARD_REQ_WRITE_ROM", "CARD_REQ_READ_BACKUP", "CARD_REQ_WRITE_BACKUP",
									"CARD_REQ_PROGRAM_BACKUP", "CARD_REQ_VERIFY_BACKUP", "CARD_REQ_ERASE_PAGE_BACKUP", "CARD_REQ_ERASE_SECTOR_BACKUP", "CARD_REQ_ERASE_CHIP_BACKUP", "unk", "unk", "unk"};
	std::string backupTypeNames[4] = { "NONE", "EEPROM", "FLASH", "FRAM" };
};
