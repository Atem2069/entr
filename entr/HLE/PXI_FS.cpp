#include"PXI_FS.h"
#include"../Bus.h"

PXIFS::PXIFS()
{
	initialized = false;
}

PXIFS::~PXIFS()
{

}

void PXIFS::init(Bus* bus)
{
	m_bus = bus;
}

void PXIFS::processPXICmd(uint32_t cmd)
{
	//awful. rewrite this
	if (!initialized)
	{
		switch (cmd)
		{
		case 0:
			initialized = true;
			waitForArgPtr = true;
			Logger::msg(LoggerSeverity::Info, "FS: init");
			break;
		default:
			Logger::msg(LoggerSeverity::Warn, std::format("FS: wtf? {:#x}", cmd));
		}
	}
	else if (waitForArgPtr)
	{
		argPtr = cmd;
		waitForArgPtr = 0;
		Logger::msg(LoggerSeverity::Info, std::format("FS: grabbed ptr to args: {:#x}", argPtr));
		
		//+4 offset should be right?? I hope so..
		uint32_t cardBackupType = m_bus->NDS7_read32(argPtr + 4);
		uint8_t backupTypeId = cardBackupType & 0xFF;
		uint8_t backupSizeBit = (cardBackupType >> 8) & 0xFF;
		Logger::msg(LoggerSeverity::Info, std::format("FS: backup type={} size={} bytes", backupTypeNames[backupTypeId&0b11], (1 << backupSizeBit)));
	}
	else
	{

		uint32_t cardReadSrc = m_bus->NDS7_read32(argPtr + 12);
		uint32_t cardReadDst = m_bus->NDS7_read32(argPtr + 16);
		uint32_t cardReadLen = m_bus->NDS7_read32(argPtr + 20);

		switch (cmd & 0xF)
		{
		case 6:
			Logger::msg(LoggerSeverity::Info, std::format("FS: read backup: src={:#x} dst={:#x} len={:#x}", cardReadSrc, cardReadDst, cardReadLen));
			break;
		default:
			Logger::msg(LoggerSeverity::Warn, std::format("FS: unhandled {}", cardCmdNames[cmd & 0xF]));
		}
	}
}