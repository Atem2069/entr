#include"PXI_FS.h"
#include"../Bus.h"

PXIFS::PXIFS()
{
	initialized = false;

	std::size_t extensionPos = Config::NDS.RomName.find_last_of(".");
	std::string savePath = Config::NDS.RomName.substr(0, extensionPos) + ".sav";

	file = std::fstream(savePath, std::ios::in | std::ios::out | std::ios::binary);
}

PXIFS::~PXIFS()
{

}

void PXIFS::init(Bus* bus)
{
	m_bus = bus;
}

uint32_t PXIFS::processPXICmd(uint32_t cmd)
{
	uint32_t reply = 0;
	//awful. rewrite this
	if (!initialized)
	{
		switch (cmd)
		{
		case 0:
			initialized = true;
			waitForArgPtr = true;
			Logger::msg(LoggerSeverity::Info, "FS: init");
			reply = PXIREPLY(0xb, 1, 1);
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
		reply = PXIREPLY(0xb, 1, 1);
	}
	else
	{

		uint32_t src = m_bus->NDS7_read32(argPtr + 12);
		uint32_t dst = m_bus->NDS7_read32(argPtr + 16);
		uint32_t len = m_bus->NDS7_read32(argPtr + 20);

		switch (cmd & 0xF)
		{
		case CARD_REQ_IDENTIFY:
		{
			Logger::msg(LoggerSeverity::Warn, "FS: unimpl CARD_REQ_IDENTIFY");
			reply = PXIREPLY(0xb, 1, 1);
			break;
		}
		case CARD_REQ_READ_BACKUP:
		{
			Logger::msg(LoggerSeverity::Info, std::format("FS: read backup: src={:#x} dst={:#x} len={:#x}", src, dst, len));
			
			//allocating new array here on every read cmd is not ideal
			uint8_t* data = new uint8_t[len];
			file.seekg(src);
			file.read((char*)data, len);
			for (uint32_t i = 0; i < len; i++)
				m_bus->NDS7_write8(dst + i, data[i]);
			delete[] data;
			reply = PXIREPLY(0xb, 1, 1);
			break;
		}
		case CARD_REQ_WRITE_BACKUP:
		{
			Logger::msg(LoggerSeverity::Info, std::format("FS: write backup: src={:#x} dst={:#x} len={:#x}", src, dst, len));
			file.seekp(dst);

			uint8_t* data = new uint8_t[len];
			for (uint32_t i = 0; i < len; i++)
				data[i] = m_bus->NDS7_read8(src + i);
			file.write((char*)data, len);
			delete[] data;
			reply = PXIREPLY(0xb, 1, 1);
			break;
		}

		default:
			Logger::msg(LoggerSeverity::Warn, std::format("FS: unhandled {}", cardCmdNames[cmd & 0xF]));
			reply = PXIREPLY(0xb, 1, 1);
		}
	}

	//Logger::msg(LoggerSeverity::Info, std::format("PXI: tag={} err={}? data={:#x}", FIFOTagNames[value & 0x1F], ((value >> 5) & 0b1), (value >> 6)));
	return reply;
}