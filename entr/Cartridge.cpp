#include"Cartridge.h"

Cartridge::Cartridge()
{

}

void Cartridge::init(std::vector<uint8_t> cartData, InterruptManager* interruptManager)
{
	m_cartData = cartData;
	m_interruptManager = interruptManager;
	m_backup = new EEPROM;
}

Cartridge::~Cartridge()
{
	delete m_backup;
}

void Cartridge::encryptSecureArea(uint8_t* keyBuf)
{
	memcpy(storedKeyBuffer, keyBuf, 0x1048);
	cartId = m_cartData[0xC] | (m_cartData[0xD] << 8) | (m_cartData[0xE] << 16) | (m_cartData[0xF] << 24);
	uint32_t secureAreaId = *(uint32_t*)&m_cartData[0x4000];
	if (secureAreaId != 0xE7FFDEFF)							//secure area already encrypted, or no secure area exists
		return;

	//write 'encryObj' to secure area ID
	char encryObj[8] = { 'e','n','c','r','y','O','b','j' };
	for (int i = 0; i < 8; i++)
		m_cartData[0x4000 + i] = encryObj[i];

	//level 3 encrypt first 2KB
	KEY1_InitKeyCode(cartId, 3, 2);
	for (int i = 0x4000; i < 0x4800; i += 8)
	{
		KEY1_encrypt((uint32_t*)&m_cartData[i]);
	}

	//level 2 encrypt secure area id once more
	KEY1_InitKeyCode(cartId, 2, 2);
	KEY1_encrypt((uint32_t*)&m_cartData[0x4000]);
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
			endTransfer();
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

	switch (m_encryptionState)
	{
	case CartEncryptionState::Unencrypted:
		decodeUnencryptedCmd();
		break;
	case CartEncryptionState::KEY1:
	{
		uint32_t temp[2] = { cartCommand & 0xFFFFFFFF,(cartCommand >> 32) & 0xFFFFFFFF };
		KEY1_decrypt(temp);
		cartCommand = temp[0] | ((uint64_t)(temp[1]) << 32);
		Logger::msg(LoggerSeverity::Info, std::format("Decrypted KEY1 command: {:#x}", cartCommand));
		decodeKEY1Cmd();
		break;
	}
	case CartEncryptionState::KEY2:
		decodeKEY2Cmd();
		break;
	}

	if (transferInProgress)
	{
		switch (NDS7HasAccess)
		{
		case 0:NDS9_DMACallback(DMAContext); break;
		case 1:NDS7_DMACallback(DMAContext); break;
		}
	}
}

void Cartridge::decodeUnencryptedCmd()
{
	uint8_t commandId = ((cartCommand >> 56) & 0xFF);
	switch (commandId)
	{
	case 0x9F:								//read dummy bytes
		memset(readBuffer, 0xFF, 0x2000);
		transferLength = 0x2000;
		break;
	case 0x00:								//read header
		memcpy(readBuffer, &m_cartData[0], 0x200);
		transferLength = 0x200;
		break;
	case 0x90:								//chip id
	{
		// 0x00001FC2
		readBuffer[0] = 0xC2;
		readBuffer[1] = 0x1F;
		readBuffer[2] = 0;
		readBuffer[3] = 0;
		break;
	}
	case 0x3c:								//activate KEY1
		Logger::msg(LoggerSeverity::Info, "Entering KEY1 mode..");
		m_encryptionState = CartEncryptionState::KEY1;
		endTransfer();
		KEY1_InitKeyCode(cartId, 2, 2);
		break;
	}
}

void Cartridge::decodeKEY1Cmd()
{
	uint8_t commandId = ((cartCommand >> 60) & 0xF);

	switch (commandId)
	{
	case 0x4:								//activate KEY2
	{
		Logger::msg(LoggerSeverity::Info, "Enable KEY2..");
		endTransfer();
		break;
	}
	case 0x1:								//chip id
	{
		// 0x00001FC2
		readBuffer[0] = 0xC2;
		readBuffer[1] = 0x1F;
		readBuffer[2] = 0;
		readBuffer[3] = 0;
		break;
	}
	case 0x2:								//read secure area
	{
		uint64_t baseBlock = ((cartCommand >> 44) & 0xF);
		uint32_t baseAddr = baseBlock << 12;
		memcpy(readBuffer, &m_cartData[baseAddr], 0x1000);
		//transferLength = 0x1000;
		break;
	}
	case 0xa:								//enter main data mode
	{
		Logger::msg(LoggerSeverity::Info, "Enter main data..");
		m_encryptionState = CartEncryptionState::KEY2;
		endTransfer();
		break;
	}
	default:
		endTransfer();
		Logger::msg(LoggerSeverity::Error, std::format("Unknown KEY1 command {:#x}", commandId));
		break;
	}

}

void Cartridge::decodeKEY2Cmd()
{
	uint8_t commandId = ((cartCommand >> 56) & 0xFF);
	switch (commandId)
	{
	case 0xB7:								//read data
	{
		uint32_t baseAddr = ((cartCommand >> 24) & 0xFFFFFFFF);
		memcpy(readBuffer, &m_cartData[baseAddr], transferLength);
		break;
	}
	case 0xB8: case 0x90:					//chip id
	{
		// 0x00001FC2
		readBuffer[0] = 0xC2;
		readBuffer[1] = 0x1F;
		readBuffer[2] = 0;
		readBuffer[3] = 0;
		break;
	}
	}
}

void Cartridge::endTransfer()
{
	ROMCTRL &= ~(1 << 23);
	ROMCTRL &= 0x7FFFFFFF;
	if ((AUXSPICNT >> 14) & 0b1)
	{
		if (NDS7HasAccess)
			m_interruptManager->NDS7_requestInterrupt(InterruptType::GamecardTransferComplete);
		else
			m_interruptManager->NDS9_requestInterrupt(InterruptType::GamecardTransferComplete);
	}
	transferInProgress = false;
}

void Cartridge::KEY1_InitKeyCode(uint32_t idcode, int level, int mod)
{
	memcpy(KEY1_KeyBuffer, storedKeyBuffer, 0x1048);
	uint32_t keyCode[3] = { idcode, idcode >> 1, idcode << 1 };
	if (level >= 1)
		KEY1_ApplyKeyCode(keyCode, mod);
	if (level >= 2)
		KEY1_ApplyKeyCode(keyCode, mod);
	keyCode[1] <<= 1;
	keyCode[2] >>= 1;
	if (level >= 3)
		KEY1_ApplyKeyCode(keyCode, mod);
}

void Cartridge::KEY1_ApplyKeyCode(uint32_t* keycode, int mod)
{
	KEY1_encrypt(&keycode[1]);
	KEY1_encrypt(&keycode[0]);

	uint32_t scratch[2] = {0,0};

	for (int i = 0; i <= 0x11; i++)
		KEY1_KeyBuffer[i] = KEY1_KeyBuffer[i] ^ KEY1_ByteSwap(keycode[i % mod]);

	for (int i = 0; i <= 0x410; i += 2)
	{
		KEY1_encrypt(scratch);
		KEY1_KeyBuffer[i] = scratch[1];
		KEY1_KeyBuffer[i + 1] = scratch[0];
	}
}

void Cartridge::KEY1_encrypt(uint32_t* data)
{
	uint32_t y = data[0], x = data[1], z = 0;
	for (int i = 0; i <= 0xF; i++)
	{
		z = KEY1_KeyBuffer[i] ^ x;
		x = KEY1_KeyBuffer[0x012 + (z >> 24)];
		x += KEY1_KeyBuffer[0x112 + ((z >> 16) & 0xFF)];
		x ^= KEY1_KeyBuffer[0x212 + ((z >> 8) & 0xFF)];
		x += KEY1_KeyBuffer[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	data[0] = x ^ KEY1_KeyBuffer[0x10];
	data[1] = y ^ KEY1_KeyBuffer[0x11];
}

void Cartridge::KEY1_decrypt(uint32_t* data)
{
	uint32_t y = data[0], x = data[1], z = 0;

	for (int i = 0x11; i >= 0x02; i--)
	{
		z = KEY1_KeyBuffer[i] ^ x;
		x = KEY1_KeyBuffer[0x012 + (z >> 24)];
		x += KEY1_KeyBuffer[0x112 + ((z >> 16) & 0xFF)];
		x ^= KEY1_KeyBuffer[0x212 + ((z >> 8) & 0xFF)];
		x += KEY1_KeyBuffer[0x312 + (z & 0xFF)];
		x ^= y;
		y = z;
	}

	data[0] = x ^ KEY1_KeyBuffer[1];
	data[1] = y ^ KEY1_KeyBuffer[0];
}

uint32_t Cartridge::KEY1_ByteSwap(uint32_t in)
{
	uint32_t a = in & 0xFF;
	uint32_t b = (in >> 8) & 0xFF;
	uint32_t c = (in >> 16) & 0xFF;
	uint32_t d = (in >> 24) & 0xFF;

	return d | (c << 8) | (b << 16) | (a << 24);
}