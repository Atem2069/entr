#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"

class Cartridge
{
public:
	Cartridge(std::vector<uint8_t> cartData, std::shared_ptr<InterruptManager> interruptManager);
	~Cartridge();

	void registerDMACallbacks(callbackFn NDS9Callback, void* ctx)
	{
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
	std::vector<uint8_t> m_cartData;	//vector probably sucks, but oh well
	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	callbackFn NDS9_DMACallback;
	void* DMAContext;

	uint32_t readGamecard();

	std::shared_ptr<InterruptManager> m_interruptManager;
	uint16_t AUXSPICNT = {};
	uint32_t ROMCTRL = {};

	uint8_t readBuffer[0x2000];
	int transferLength = 0, bytesTransferred = 0;
	bool transferInProgress = false;
	bool NDS7HasAccess = false;
	uint64_t cartCommand = 0;
};