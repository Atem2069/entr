#include"Cartridge.h"

Cartridge::Cartridge(std::vector<uint8_t> cartData, std::shared_ptr<InterruptManager> interruptManager)
{
	m_cartData = cartData;
	m_interruptManager = interruptManager;
}

Cartridge::~Cartridge()
{

}

uint8_t Cartridge::NDS7_read(uint32_t address)
{
	if (!NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return read(address);
}

void Cartridge::NDS7_write(uint32_t address, uint8_t value)
{
	if (!NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to write gamecard registers without access!!");
		return;
	}
	write(address, value);
}

uint32_t Cartridge::NDS7_readGamecard()
{
	if (!NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return readGamecard();
}

uint8_t Cartridge::NDS9_read(uint32_t address)
{
	if (NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return read(address);
}

void Cartridge::NDS9_write(uint32_t address, uint8_t value)
{
	if (NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to write gamecard registers without access!!");
		return;
	}
	write(address, value);
}

uint32_t Cartridge::NDS9_readGamecard()
{
	if (NDS7HasAccess)
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return readGamecard();
}

void Cartridge::setNDS7AccessRights(bool val)
{
	NDS7HasAccess = val;
}

uint8_t Cartridge::read(uint32_t address)
{
	//std::cout << "read " << std::hex << address << '\n';
	switch (address)
	{
	case 0x040001A0:
		return AUXSPICNT & 0xFF;
	case 0x040001A1:
		return ((AUXSPICNT >> 8) & 0xFF);
	case 0x040001A4:
		return ROMCTRL & 0xFF;
	case 0x040001A5:
		return ((ROMCTRL >> 8) & 0xFF);
	case 0x040001A6:
		return ((ROMCTRL >> 16) & 0xFF);
	case 0x040001A7:
		return ((ROMCTRL >> 24) & 0xFF);
	}
}

void Cartridge::write(uint32_t address, uint8_t value)
{
	//std::cout << "write " << std::hex << address << " " << value << '\n';
	switch (address)
	{
	case 0x040001A0:
		AUXSPICNT &= 0xFF00; AUXSPICNT |= value;
		break;
	case 0x040001A1:
		AUXSPICNT &= 0xFF; AUXSPICNT |= (value << 8);
		break;
	case 0x040001A4:
		ROMCTRL &= 0xFFFFFF00; ROMCTRL |= value;
		break;
	case 0x040001A5:
		ROMCTRL &= 0xFFFF00FF; ROMCTRL |= (value << 8);
		break;
	case 0x040001A6:
		ROMCTRL &= 0xFF00FFFF; ROMCTRL |= ((value&0x7F) << 16);
		break;
	case 0x040001A7:
	{
		bool start = (!(ROMCTRL >> 31) && (value >> 7));
		ROMCTRL &= 0x00FFFFFF; ROMCTRL |= (value << 24);
		if (start)
			startTransfer();
		break;
	}
	case 0x040001A8: case 0x040001A9: case 0x040001AA: case 0x040001AB: case 0x040001AC: case 0x040001AD: case 0x040001AE: case 0x040001AF:
	{
		int shiftOffs = 56 - ((address & 0b111) << 3);
		cartCommand &= ~(((uint64_t)0xFF) << shiftOffs);
		cartCommand |= ((uint64_t)(value) << shiftOffs);
		if ((address & 0b111) == 7)
			Logger::getInstance()->msg(LoggerSeverity::Info, std::format("Cartridge command: {:#x}",cartCommand));
		break;
	}
	}
}

uint32_t Cartridge::readGamecard()
{
	uint32_t result = 0;
	if (transferInProgress)
	{
		result = readBuffer[bytesTransferred] | (readBuffer[bytesTransferred+1] << 8) | (readBuffer[bytesTransferred + 2] << 16) | (readBuffer[bytesTransferred + 3] << 24);
		bytesTransferred += 4;
		if (bytesTransferred == transferLength)
		{
			Logger::getInstance()->msg(LoggerSeverity::Info, "Transfer completed!");
			transferInProgress = false;
			ROMCTRL &= 0x7FFFFFFF;
			if ((AUXSPICNT >> 14) & 0b1)
			{
				if (NDS7HasAccess)
					m_interruptManager->NDS7_requestInterrupt(InterruptType::GamecardTransferComplete);
				else
					m_interruptManager->NDS9_requestInterrupt(InterruptType::GamecardTransferComplete);
			}
			ROMCTRL &= ~(1 << 23);
		}
	}
	return result;
	//return 0xFFFFFFFF;	//should send data according to command :)
}

void Cartridge::startTransfer()
{
	bytesTransferred = 0;
	uint8_t transferBlockSize = ((ROMCTRL >> 24) & 0b111);
	if (transferBlockSize == 7)
		transferLength = 4;
	else
		transferLength = 0x100 << transferBlockSize;

	uint8_t commandId = ((cartCommand >> 56) & 0xFF);
	switch (commandId)
	{
	case 0xB7:
	{
		uint32_t baseAddr = ((cartCommand >> 24) & 0xFFFFFFFF);
		for (int i = 0; i < transferLength; i++)
			readBuffer[i] = m_cartData[baseAddr + i];
		//memcpy(readBuffer, &m_cartData[baseAddr], transferLength);
		break;
	}
	case 0xB8:
	{
		// 0x00001FC2
		readBuffer[0] = 0xC2;
		readBuffer[1] = 0x1F;
		readBuffer[2] = 0;
		readBuffer[3] = 0;
		break;
	}
	default:
	{
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unknown cartridge command {:#x}",commandId));
		memset(readBuffer, 0xFF, transferLength);
		break;
	}
	}

	transferInProgress = true;
	Logger::getInstance()->msg(LoggerSeverity::Info, std::format("Gamecard transfer started. length={}", transferLength));
	ROMCTRL |= (1 << 23);
}