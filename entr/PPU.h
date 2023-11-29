#pragma once

#include"Logger.h"
#include"NDSMem.h"
#include"InterruptManager.h"
#include"Scheduler.h"
#include"GPU.h"

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
	uint16_t WININ;
	uint16_t WINOUT;
	//reg_BGnX/Y is the value the CPU writes. BGnX/Y is the latched value the PPU reads.
	uint32_t reg_BG2X = {}, BG2X = {};
	uint32_t reg_BG2Y = {}, BG2Y = {};
	uint32_t reg_BG3X = {}, BG3X = {};
	uint32_t reg_BG3Y = {}, BG3Y = {};
	bool BG2X_dirty = false, BG2Y_dirty = false, BG3X_dirty = false, BG3Y_dirty = false;	//'dirty' flags to determine if reference points were written.

	//dx=1,dmy=1 might not be right. just using identity matrix for default vals
	uint16_t BG2PA = 1;
	uint16_t BG2PB = 0;
	uint16_t BG2PC = 0;
	uint16_t BG2PD = 1;

	uint16_t BG3PA = 1;
	uint16_t BG3PB = 0;
	uint16_t BG3PC = 0;
	uint16_t BG3PD = 1;
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

struct Window
{
	uint16_t x1, x2;
	uint16_t y1, y2;
	bool enabled;
	bool layerDrawable[4];
	bool objDrawable;
	bool blendable;
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

	Window m_engineAWindows[4];
	Window m_engineBWindows[4];
	Window m_defaultWindow = { 0,0,0,0,true,{true,true,true,true},true,true };

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
	template<Engine engine, int bg, bool extended> void renderAffineBackground();
	template<Engine engine, int bg> void renderDirectColorBitmap();
	template<Engine engine, int bg> void render256ColorBitmap();
	template<Engine engine> void renderSprites();
	template<Engine engine> void renderAffineSprite(OAMEntry* curSpriteEntry);
	template<Engine engine> void composeLayers();
	template<Engine engine> Window getPointAttributes(int x, int y);

	template<Engine engine> uint16_t extractColorFromTile(uint32_t tileBase, uint32_t xOffset, bool hiColor, uint32_t palette);

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

	inline void checkAffineRegsDirty()
	{
		//messy
		if (m_engineARegisters.BG2X_dirty)
			m_engineARegisters.BG2X = m_engineARegisters.reg_BG2X;
		if (m_engineARegisters.BG2Y_dirty)
			m_engineARegisters.BG2Y = m_engineARegisters.reg_BG2Y;
		if (m_engineARegisters.BG3X_dirty)
			m_engineARegisters.BG3X = m_engineARegisters.reg_BG3X;
		if (m_engineARegisters.BG3Y_dirty)
			m_engineARegisters.BG3Y = m_engineARegisters.reg_BG3Y;

		if (m_engineBRegisters.BG2X_dirty)
			m_engineBRegisters.BG2X = m_engineBRegisters.reg_BG2X;
		if (m_engineBRegisters.BG2Y_dirty)
			m_engineBRegisters.BG2Y = m_engineBRegisters.reg_BG2Y;
		if (m_engineBRegisters.BG3X_dirty)
			m_engineBRegisters.BG3X = m_engineBRegisters.reg_BG3X;
		if (m_engineBRegisters.BG3Y_dirty)
			m_engineBRegisters.BG3Y = m_engineBRegisters.reg_BG3Y;

		m_engineARegisters.BG2X_dirty = false;
		m_engineARegisters.BG2Y_dirty = false;
		m_engineARegisters.BG3X_dirty = false;
		m_engineARegisters.BG3Y_dirty = false;

		m_engineBRegisters.BG2X_dirty = false;
		m_engineBRegisters.BG2Y_dirty = false;
		m_engineBRegisters.BG3X_dirty = false;
		m_engineBRegisters.BG3Y_dirty = false;
	}

	inline void calcAffineCoords(int32_t& xRef, int32_t& yRef, int16_t dx, int16_t dy)
	{
		xRef += dx;
		yRef += dy;
	}

	inline void updateAffineRegisters(int bg)
	{

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