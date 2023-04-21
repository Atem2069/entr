#pragma once

#include"Logger.h"
#include"NDSMem.h"
#include"InterruptManager.h"
#include"Scheduler.h"

enum class PPUState
{
	HDraw,
	HBlank,
	VBlank
};

struct PPURegisters
{
	uint32_t DISPCNT;
	uint16_t BG0CNT;
	uint16_t BG1CNT;
	uint16_t BG2CNT;
	uint16_t BG3CNT;
	uint16_t BG0HOFS;
	uint16_t BG0VOFS;
	uint16_t BG1HOFS;
	uint16_t BG1VOFS;
	uint16_t BG2HOFS;
	uint16_t BG2VOFS;
	uint16_t BG3HOFS;
	uint16_t BG3VOFS;
};

enum class Engine
{
	A=0,
	B=1
};

struct BackgroundLayer
{
	uint8_t priority;
	bool enabled;
	uint16_t lineBuffer[256];
};

union SpriteAttribute
{
	uint8_t attr;
	struct
	{
		uint8_t priority : 5;
		bool objWindow : 1;
		bool transparent : 1;
		bool mosaic : 1;
	};
};

union OAMEntry
{
	uint64_t data;
	struct
	{
		//attribute 0
		unsigned yCoord : 8;
		unsigned rotateScale : 1;
		unsigned disableObj : 1;    //depends on mode. regular sprites use this as 'disable' flag, affine use it as doublesize flag 
		unsigned objMode : 2;
		unsigned mosaic : 1;
		unsigned bitDepth : 1;
		unsigned shape : 2;
		//attribute 1
		unsigned xCoord : 9;
		unsigned unused : 3;		//in rot/scale bits 9-13 of attr1 are actually the parameter selection
		unsigned xFlip : 1;
		unsigned yFlip : 1;
		unsigned size : 2;
		//attribute 2
		unsigned charName : 10;
		unsigned priority : 2;
		unsigned paletteNumber : 4;
	};
};

class PPU
{
public:
	PPU();
	void init(InterruptManager* interruptManager, Scheduler* scheduler);
	~PPU();

	void registerDMACallbacks(callbackFn NDS9HBlank, callbackFn NDS9VBlank, void* ctx)
	{
		NDS9_HBlankDMACallback = NDS9HBlank;
		NDS9_VBlankDMACallback = NDS9VBlank;
		callbackContext = ctx;
	}

	void updateDisplayOutput();

	void registerMemory(NDSMem* mem);

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value); 

	uint8_t NDS7_readIO(uint32_t address);
	void NDS7_writeIO(uint32_t address, uint8_t value);

	uint8_t* mapLCDCAddress(uint32_t address);
	uint8_t* mapABgAddress(uint32_t address);
	uint8_t* mapAObjAddress(uint32_t address);
	uint8_t* mapBBgAddress(uint32_t address);
	uint8_t* mapBObjAddress(uint32_t address);
	uint8_t* mapARM7Address(uint32_t address);

	static void onSchedulerEvent(void* context);

	static uint32_t m_safeDisplayBuffer[256 * 384];
private:
	PPUState m_state = {};
	bool registered = false;
	NDSMem* m_mem;
	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;

	callbackFn NDS9_HBlankDMACallback;
	callbackFn NDS9_VBlankDMACallback;
	void* callbackContext = nullptr;

	BackgroundLayer m_engineABgLayers[4] = {};
	BackgroundLayer m_engineBBgLayers[4] = {};
	SpriteAttribute m_engineASpriteAttribBuffer[256] = {};
	SpriteAttribute m_engineBSpriteAttribBuffer[256] = {};
	uint16_t m_engineASpriteLineBuffer[256] = {};
	uint16_t m_engineBSpriteLineBuffer[256] = {};

	uint32_t m_renderBuffer[2][256 * 384];
	bool pageIdx = false;

	void eventHandler();
	void HDraw();
	void HBlank();
	void VBlank();
	bool vblank_setHblankBit = false;

	void renderLCDCMode();
	template <Engine engine> void renderMode0();
	template <Engine engine> void renderMode1();
	template <Engine engine> void renderMode2();
	template <Engine engine> void renderMode3();
	template <Engine engine> void renderMode4();
	template <Engine engine> void renderMode5();

	template<Engine engine, int bg> void renderBackground();
	template<Engine engine> void renderSprites();
	template<Engine engine> void composeLayers();

	template<Engine engine> uint16_t extractColorFromTile(uint32_t tileBase, uint32_t xOffset, bool hiColor, bool sprite, uint32_t palette);

	template<Engine engine> uint8_t ppuReadBg(uint32_t address)
	{
		uint8_t* ptr = nullptr;
		if constexpr (engine == Engine::A)
			ptr = mapABgAddress(address);
		else
			ptr = mapBBgAddress(address);
		if (ptr < m_mem->VRAM)
			return 0;
		return *ptr;
	}

	template<Engine engine> uint8_t ppuReadObj(uint32_t address)
	{
		uint8_t* ptr = nullptr;
		if constexpr (engine == Engine::A)
			ptr = mapAObjAddress(address);
		else
			ptr = mapBObjAddress(address);
		if (ptr < m_mem->VRAM)
			return 0;
		return *ptr;
	}

	template<Engine engine> uint8_t ppuReadBgExtPal(int block, uint32_t address)
	{
		uint8_t* ptr = nullptr;
		uint32_t lookupAddr = (block * 8192) + address;
		uint8_t page = (lookupAddr >> 14);
		uint32_t offset = lookupAddr & 0x3FFF;
		if constexpr (engine == Engine::A)
			ptr = m_mem->ABGExtPalPageTable[page];
		else
			ptr = m_mem->BBGExtPalPageTable[page];
		if (ptr)
			return ptr[offset];
		return 0xFF;
	}

	template<Engine engine> uint8_t ppuReadObjExtPal(uint32_t address)
	{
		uint8_t* ptr = nullptr;
		if constexpr (engine == Engine::A)
			ptr = m_mem->AObjExtPalPageTable;
		else
			ptr = m_mem->BObjExtPalPageTable;
		if (ptr)
			return ptr[address];
		return 0;
	}

	void setVBlankFlag(bool value);
	void setHBlankFlag(bool value);
	void NDS7_setVCounterFlag(bool value);
	void NDS9_setVCounterFlag(bool value);

	void checkVCOUNTInterrupt();
	bool nds7_vcountIRQLine = false;
	bool nds9_vcountIRQLine = false;

	uint32_t col16to32(uint16_t col);

	uint16_t DISPSTAT = {};
	uint16_t NDS7_DISPSTAT = {};
	uint16_t VCOUNT = {};

	PPURegisters m_engineARegisters = {};
	PPURegisters m_engineBRegisters = {};

	uint16_t POWCNT1 = {};
	uint32_t EngineA_RenderBase = 0;
	uint32_t EngineB_RenderBase = 256 * 192;

	//dumb vram stuff
	uint8_t VRAMCNT_A = {};
	uint8_t VRAMCNT_B = {};
	uint8_t VRAMCNT_C = {};
	uint8_t VRAMCNT_D = {};
	uint8_t VRAMCNT_E = {};
	uint8_t VRAMCNT_F = {};
	uint8_t VRAMCNT_G = {};
	uint8_t VRAMCNT_H = {};
	uint8_t VRAMCNT_I = {};

	void rebuildPageTables();
};