#include"Cartridge.h"

Cartridge::Cartridge(std::vector<uint8_t> cartData, std::shared_ptr<InterruptManager> interruptManager)
{
	m_cartData = cartData;
	m_interruptManager = interruptManager;
	m_backup = std::make_shared<EEPROM>();
}

Cartridge::~Cartridge()
{

}

uint8_t Cartridge::NDS7_read(uint32_t address)
{
	if (!NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return read(address);
}

void Cartridge::NDS7_write(uint32_t address, uint8_t value)
{
	if (!NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to write gamecard registers without access!!");
		return;
	}
	write(address, value);
}

uint32_t Cartridge::NDS7_readGamecard()
{
	if (!NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return readGamecard();
}

uint8_t Cartridge::NDS9_read(uint32_t address)
{
	if (NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
		return 0;
	}
	return read(address);
}

void Cartridge::NDS9_write(uint32_t address, uint8_t value)
{
	if (NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to write gamecard registers without access!!");
		return;
	}
	write(address, value);
}

uint32_t Cartridge::NDS9_readGamecard()
{
	if (NDS7HasAccess)
	{
		Logger::msg(LoggerSeverity::Error, "Tried to read gamecard registers without access!!");
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
	case 0x040001A2:
		return SPIData;
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
		AUXSPICNT &= 0xFF00; AUXSPICNT |= value&0x7F;
		chipSelectHold = (AUXSPICNT >> 6) & 0b1;
		break;
	case 0x040001A1:
		AUXSPICNT &= 0xFF; AUXSPICNT |= (value << 8);
		if (!(AUXSPICNT >> 15))
			m_backup->deselect();
		Logger::msg(LoggerSeverity::Info, std::format("AUXSPICNT config. enable = {} chipselect hold = {} spi mode={}", (AUXSPICNT >> 15), chipSelectHold, ((AUXSPICNT >> 13) & 0b1)));
		break;
	case 0x040001A2:
		Logger::msg(LoggerSeverity::Info, std::format("AUXSPIDATA write: {:#x}", value));
		if (!(AUXSPICNT >> 15))
			break;
		SPIData = m_backup->sendCommand(value);
		if (!chipSelectHold)
			m_backup->deselect();
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
			Logger::msg(LoggerSeverity::Info, std::format("Cartridge command: {:#x}",cartCommand));
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
			Logger::msg(LoggerSeverity::Info, "Transfer completed!");
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
	transferInProgress = true;
	ROMCTRL |= (1 << 23);
	bytesTransferred = 0;
	uint8_t transferBlockSize = ((ROMCTRL >> 24) & 0b111);
	if (transferBlockSize == 7)
		transferLength = 4;
	else
		transferLength = 0x100 << transferBlockSize;

	Logger::msg(LoggerSeverity::Info, std::format("Gamecard transfer started. length={}", transferLength));

	uint8_t commandId = ((cartCommand >> 56) & 0xFF);
	switch (commandId)
	{
	case 0xB7:
	{
		uint32_t baseAddr = ((cartCommand >> 24) & 0xFFFFFFFF);
		memcpy(readBuffer, &m_cartData[baseAddr], transferLength);
		break;
	}
	case 0xB8: case 0x90:
	{
		// 0x00001FC2
		readBuffer[0] = 0xC2;
		readBuffer[1] = 0x1F;
		readBuffer[2] = 0;
		readBuffer[3] = 0;
		break;
	}
	case 0x3c:
		ROMCTRL &= ~(1 << 23);
		ROMCTRL &= 0x7FFF;
		break;
	default:
	{
		ROMCTRL &= ~(1 << 23);
		ROMCTRL &= 0x7FFF;
		if ((AUXSPICNT >> 14) & 0b1)
		{
			if (NDS7HasAccess)
				m_interruptManager->NDS7_requestInterrupt(InterruptType::GamecardTransferComplete);
			else
				m_interruptManager->NDS9_requestInterrupt(InterruptType::GamecardTransferComplete);
		}
		Logger::msg(LoggerSeverity::Error, std::format("Unknown cartridge command {:#x}",commandId));
		memset(readBuffer, 0xFF, transferLength);
		break;
	}
	}

	if (!NDS7HasAccess)
		NDS9_DMACallback(DMAContext);
}