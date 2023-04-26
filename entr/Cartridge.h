#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"
#include"Flash.h"
#include"EEPROM.h"

enum class CartEncryptionState
{
	Unencrypted,
	KEY1,
	KEY2
};

class Cartridge
{
public:
	Cartridge();
	void init(std::vector<uint8_t> cartData, InterruptManager* interruptManager);
	~Cartridge();

	void encryptSecureArea(uint8_t* keyBuf);
	void directBoot() { m_encryptionState = CartEncryptionState::KEY2; }	//directly enter KEY2/main data mode if direct booting

	void registerDMACallbacks(callbackFn NDS7Callback, callbackFn NDS9Callback, void* ctx)
	{
		NDS7_DMACallback = NDS7Callback;
		NDS9_DMACallback = NDS9Callback;
		DMAContext = ctx;
	}

	uint8_t NDS7_read(uint32_t address);
	void NDS7_write(uint32_t address, uint8_t value);
	uint32_t NDS7_readGamecard();

	uint8_t NDS9_read(uint32_t address);
	void NDS9_write(uint32_t address, uint8_t value);
	uint32_t NDS9_readGamecard();

	void setNDS7AccessRights(bool val);
	uint32_t getWordsLeft() { return (transferLength - bytesTransferred) / 4; }

private:
	void startTransfer();
	void decodeUnencryptedCmd();
	void decodeKEY1Cmd();
	void decodeKEY2Cmd();

	void endTransfer();

	CartEncryptionState m_encryptionState = CartEncryptionState::Unencrypted;
	std::vector<uint8_t> m_cartData;	//vector probably sucks, but oh well
	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	SPIDevice* m_backup;

	callbackFn NDS9_DMACallback;
	callbackFn NDS7_DMACallback;
	void* DMAContext;

	//KEY1 stuff
	uint32_t storedKeyBuffer[0x412];
	uint32_t KEY1_KeyBuffer[0x412];	
	uint32_t cartId = 0;
	uint32_t m_chipID = 0x00001FC2;
	void KEY1_InitKeyCode(uint32_t idcode, int level, int mod);
	void KEY1_ApplyKeyCode(uint32_t* keycode, int mod);
	void KEY1_encrypt(uint32_t* data);
	void KEY1_decrypt(uint32_t* data);
	uint32_t KEY1_ByteSwap(uint32_t in);

	uint32_t readGamecard();

	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;
	uint16_t AUXSPICNT = {};
	uint32_t ROMCTRL = {};

	uint8_t readBuffer[0x2000];
	int transferLength = 0, bytesTransferred = 0;
	bool transferInProgress = false;
	bool NDS7HasAccess = false;
	uint64_t cartCommand = 0;

	bool chipSelectHold = false;
	uint8_t SPIData = 0;
};