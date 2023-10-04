#include"PPU.h"

PPU::PPU()
{

}

void PPU::init(InterruptManager* interruptManager, Scheduler* scheduler)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;

	memset(m_renderBuffer, 0, 256 * 384 * 8);
	memset(m_safeDisplayBuffer, 0, 256 * 384 * 4);

	VCOUNT = 0;
	m_state = PPUState::HDraw;

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, 1606);
}

PPU::~PPU()
{

}

void PPU::updateDisplayOutput()
{
	memcpy(m_safeDisplayBuffer, m_renderBuffer[!pageIdx], 256 * 384 * sizeof(uint32_t));
}

void PPU::registerMemory(NDSMem* mem)
{
	Logger::msg(LoggerSeverity::Info, "Registered memory with PPU!");
	m_mem = mem;
}

void PPU::eventHandler()
{
	switch (m_state)
	{
	case PPUState::HDraw:
		HDraw();
		break;
	case PPUState::HBlank:
		HBlank();
		break;
	case PPUState::VBlank:
		VBlank();
		break;
	}
}

void PPU::HDraw()
{
	//this is a hack, should fix!
	//renderLCDCMode();

	uint8_t displayMode = (m_engineARegisters.DISPCNT >> 16) & 0b11;
	if (displayMode == 2)
		renderLCDCMode();
	else if (displayMode == 1)
	{
		uint8_t mode = m_engineARegisters.DISPCNT & 0b111;
		switch (mode)
		{
		case 0:
			renderMode0<Engine::A>();
			break;
		case 1:
			renderMode1<Engine::A>();
			break;
		case 2:
			renderMode2<Engine::A>();
			break;
		case 3:
			renderMode3<Engine::A>();
			break;
		case 4:
			renderMode4<Engine::A>();
			break;
		case 5:
			renderMode5<Engine::A>();
			break;
		}
		composeLayers<Engine::A>();
	}
	else
		composeLayers<Engine::A>();		//not a fan of this approach. could check for switch to display mode 0, then just clear framebuffer white..

	displayMode = (m_engineBRegisters.DISPCNT >> 16) & 0b11;
	if (displayMode == 1)
	{
		uint8_t mode = m_engineBRegisters.DISPCNT & 0b111;
		switch (mode)
		{
		case 0:
			renderMode0<Engine::B>();
			break;
		case 1:
			renderMode1<Engine::B>();
			break;
		case 2:
			renderMode2<Engine::B>();
			break;
		case 3:
			renderMode3<Engine::B>();
			break;
		case 4:
			renderMode4<Engine::B>();
			break;
		case 5:
			renderMode5<Engine::B>();
			break;
			//default:
			//	Logger::msg(LoggerSeverity::Error, std::format("Unknown mode {}",(int)mode));
		}
		composeLayers<Engine::B>();
	}
	else
		composeLayers<Engine::B>();

	//line=2130 cycles. hdraw=1606 cycles? hblank=524 cycles
	setHBlankFlag(true);
	m_state = PPUState::HBlank;
	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);

	if (((DISPSTAT >> 4) & 0b1))
		m_interruptManager->NDS9_requestInterrupt(InterruptType::HBlank);
	if (((NDS7_DISPSTAT >> 4) & 0b1))
		m_interruptManager->NDS7_requestInterrupt(InterruptType::HBlank);

	//check dmas
	NDS9_HBlankDMACallback(callbackContext);
}

void PPU::HBlank()
{
	setHBlankFlag(false);
	VCOUNT++;
	checkVCOUNTInterrupt();

	checkAffineRegsDirty();				//if affine reference points updated during frame, they're re-latched. not sure exactly when, but hblank seems reasonable

	if (VCOUNT == 192)
	{
		//set vblank flag here, request vblank interrupt

		if (((DISPSTAT >> 3) & 0b1))
			m_interruptManager->NDS9_requestInterrupt(InterruptType::VBlank);
		if (((NDS7_DISPSTAT >> 3) & 0b1))
			m_interruptManager->NDS7_requestInterrupt(InterruptType::VBlank);

		setVBlankFlag(true);
		m_state = PPUState::VBlank;

		pageIdx = !pageIdx;

		NDS9_VBlankDMACallback(callbackContext);

		m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
		return;
	}
	else
		m_state = PPUState::HDraw;

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

void PPU::VBlank()
{
	if (!vblank_setHblankBit)
	{
		setHBlankFlag(true);
		vblank_setHblankBit = true;
		m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);
		return;
	}
	vblank_setHblankBit = false;
	setHBlankFlag(false);
	VCOUNT++;
	if (VCOUNT == 262)
	{
		setVBlankFlag(false);
		VCOUNT = 0;
		checkVCOUNTInterrupt();
		m_state = PPUState::HDraw;

		//end of vblank: latch affine ref points for start of first scanline
		m_engineARegisters.BG2X = m_engineARegisters.reg_BG2X;
		m_engineARegisters.BG2Y = m_engineARegisters.reg_BG2Y;
		m_engineARegisters.BG3X = m_engineARegisters.reg_BG3X;
		m_engineARegisters.BG3Y = m_engineARegisters.reg_BG3Y;
		m_engineARegisters.BG2X_dirty = false;
		m_engineARegisters.BG2Y_dirty = false;
		m_engineARegisters.BG3X_dirty = false;
		m_engineARegisters.BG3Y_dirty = false;

		m_engineBRegisters.BG2X = m_engineBRegisters.reg_BG2X;
		m_engineBRegisters.BG2Y = m_engineBRegisters.reg_BG2Y;
		m_engineBRegisters.BG3X = m_engineBRegisters.reg_BG3X;
		m_engineBRegisters.BG3Y = m_engineBRegisters.reg_BG3Y;
		m_engineBRegisters.BG2X_dirty = false;
		m_engineBRegisters.BG2Y_dirty = false;
		m_engineBRegisters.BG3X_dirty = false;
		m_engineBRegisters.BG3Y_dirty = false;
	}
	else
	{
		if (VCOUNT == 261)
			setVBlankFlag(false);
		checkVCOUNTInterrupt();
	}
	//todo: weird behaviour of last line in vblank (same as in gba)

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

void PPU::renderLCDCMode()
{
	for (int i = 0; i < 256; i++)
	{
		uint8_t VRAMBlock = ((m_engineARegisters.DISPCNT >> 18) & 0b11);
		uint8_t* basePtr = m_mem->VRAM + (VRAMBlock * 0x20000);
		uint32_t address = (VCOUNT * (256 * 2)) + (i * 2);
		//uint32_t address = (VCOUNT * (256 * 2)) + (i * 2);
		//uint8_t* vramPtr = mapAddressToVRAM(0x06800000 + address);
		uint8_t colLow = basePtr[address];
		uint8_t colHigh = basePtr[address + 1];
		uint16_t col = ((colHigh << 8) | colLow);
		m_renderBuffer[pageIdx][((VCOUNT * 256) + i)] = col16to32(col);
	}
}

template<Engine engine>void PPU::renderMode0()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	if (engine == Engine::B)
		m_backgroundLayers = m_engineBBgLayers;

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
		renderBackground<engine, 2>();
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
		renderBackground<engine, 3>();

}

template<Engine engine> void PPU::renderMode1()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	if (engine == Engine::B)
		m_backgroundLayers = m_engineBBgLayers;

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
		renderBackground<engine, 2>();
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
		renderAffineBackground<engine, 3, false>();
}

template<Engine engine> void PPU::renderMode2()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	if (engine == Engine::B)
		m_backgroundLayers = m_engineBBgLayers;

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
		renderAffineBackground<engine, 2, false>();
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
		renderAffineBackground<engine, 3, false>();
}

template<Engine engine> void PPU::renderMode3()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	PPURegisters* m_registers = &m_engineARegisters;
	if (engine == Engine::B)
	{
		m_registers = &m_engineBRegisters;
		m_backgroundLayers = m_engineBBgLayers;
	}

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
		renderBackground<engine, 2>();
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
	{
		if (((m_registers->BG3CNT >> 7) & 0b1))
		{
			switch ((m_registers->BG3CNT >> 2) & 0b1)
			{
			case 0:
				render256ColorBitmap<engine, 3>();
				break;
			case 1:
				renderDirectColorBitmap<engine, 3>();
				break;
			}
		}
		else
			renderAffineBackground<engine, 3, true>();
	}
}

template<Engine engine> void PPU::renderMode4()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	PPURegisters* m_registers = &m_engineARegisters;
	if (engine == Engine::B)
	{
		m_registers = &m_engineBRegisters;
		m_backgroundLayers = m_engineBBgLayers;
	}

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
		renderAffineBackground<engine, 2,false>();
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
	{
		if (((m_registers->BG3CNT >> 7) & 0b1))
		{
			switch ((m_registers->BG3CNT >> 2) & 0b1)
			{
			case 0:
				render256ColorBitmap<engine, 3>();
				break;
			case 1:
				renderDirectColorBitmap<engine, 3>();
				break;
			}
		}
		else
			renderAffineBackground<engine, 3, true>();
	}
}

template<Engine engine> void PPU::renderMode5()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	PPURegisters* m_registers = &m_engineARegisters;
	if (engine == Engine::B)
	{
		m_registers = &m_engineBRegisters;
		m_backgroundLayers = m_engineBBgLayers;
	}

	renderSprites<engine>();
	//bit messy, don't like too much..
	m_backgroundLayers[0].priority = 255;
	if (m_backgroundLayers[0].enabled)
		renderBackground<engine, 0>();
	m_backgroundLayers[1].priority = 255;
	if (m_backgroundLayers[1].enabled)
		renderBackground<engine, 1>();
	m_backgroundLayers[2].priority = 255;
	if (m_backgroundLayers[2].enabled)
	{
		if (((m_registers->BG2CNT >> 7) & 0b1))
		{
			switch ((m_registers->BG2CNT >> 2) & 0b1)
			{
			case 0:
				render256ColorBitmap<engine, 2>();
				break;
			case 1:
				renderDirectColorBitmap<engine, 2>();
				break;
			}
		}
		else
			renderAffineBackground<engine, 2, true>();
	}
	m_backgroundLayers[3].priority = 255;
	if (m_backgroundLayers[3].enabled)
	{
		if (((m_registers->BG3CNT >> 7) & 0b1))
		{
			switch ((m_registers->BG3CNT >> 2) & 0b1)
			{
			case 0:
				render256ColorBitmap<engine, 3>();
				break;
			case 1:
				renderDirectColorBitmap<engine, 3>();
				break;
			}
		}
		else
			renderAffineBackground<engine, 3, true>();
	}
}


template<Engine engine> void PPU::composeLayers()
{
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	uint16_t* spriteLineBuffer = m_engineASpriteLineBuffer;
	SpriteAttribute* spriteAttributeBuffer = m_engineASpriteAttribBuffer;
	PPURegisters* m_regs = &m_engineARegisters;
	if constexpr (engine == Engine::B)
	{
		m_regs = &m_engineBRegisters;
		m_backgroundLayers = m_engineBBgLayers;
		spriteLineBuffer = m_engineBSpriteLineBuffer;
		spriteAttributeBuffer = m_engineBSpriteAttribBuffer;
	}

	uint16_t backdrop = (engine == Engine::A) ? *(uint16_t*)m_mem->PAL : *(uint16_t*)(m_mem->PAL + 0x400);

	for (int i = 0; i < 256; i++)
	{
		//offset into framebuffer to render into.
		//each engine is assigned a screen (top/bottom) to render to - the framebuffer consists of both screens (bottom screen rendered directly below top)
		uint32_t renderBase = (engine == Engine::A) ? EngineA_RenderBase : EngineB_RenderBase;

		//no display if either force blank bit set, or display mode==0
		bool noDisplay = ((m_regs->DISPCNT >> 7) & 0b1) || (((m_regs->DISPCNT >> 16) & 0b11) == 0);
		if (noDisplay)		
		{
			m_renderBuffer[pageIdx][renderBase + (256 * VCOUNT) + i] = 0xFFFFFFFF;
			continue;
		}
		
		uint16_t finalCol = backdrop;
		uint8_t bestPriority = 255;
		for (int j = 3; j >= 0; j--)
		{
			if (m_backgroundLayers[j].enabled)
			{
				uint16_t colAtLayer = m_backgroundLayers[j].lineBuffer[i];
				if ((!(colAtLayer >> 15)) && m_backgroundLayers[j].priority <= bestPriority)
				{
					bestPriority = m_backgroundLayers[j].priority;
					finalCol = colAtLayer;
				}
			}
		}

		//check if sprites can really be drawn, lol
		uint16_t spritePixel = spriteLineBuffer[i];
		if ((!(spritePixel >> 15)) && spriteAttributeBuffer[i].priority <= bestPriority)
			finalCol = spritePixel;

		m_renderBuffer[pageIdx][renderBase + (256 * VCOUNT) + i] = col16to32(finalCol);
	}
}

template<Engine engine, int bg> void PPU::renderBackground()
{
	PPURegisters* m_regs = {};
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	uint32_t screenBase = 0, charBase = 0;;
	if (engine == Engine::A)
	{
		screenBase = (((m_engineARegisters.DISPCNT >> 27) & 0b111) * 65536);
		charBase = (((m_engineARegisters.DISPCNT >> 24) & 0b111) * 65536);
		m_regs = &m_engineARegisters;
		if (((m_regs->DISPCNT >> 3) & 0b1) && bg==0)		//do not render BG0 if 3d bit set
			return;
	}
	else
	{
		m_backgroundLayers = m_engineBBgLayers;
		m_regs = &m_engineBRegisters;
	}
	uint16_t ctrlReg = 0;
	uint32_t xScroll = 0, yScroll = 0;
	int extPalBlock = 0;
	switch (bg)
	{
	case 0:
		ctrlReg = m_regs->BG0CNT;
		xScroll = m_regs->BG0HOFS;
		yScroll = m_regs->BG0VOFS;
		extPalBlock = ((ctrlReg >> 13) & 0b1) << 1;	//0=slot 0,1=slot 2
		break;
	case 1:
		ctrlReg = m_regs->BG1CNT;
		xScroll = m_regs->BG1HOFS;
		yScroll = m_regs->BG1VOFS;
		extPalBlock = (((ctrlReg >> 13) & 0b1) << 1) + 1;	//0=slot 1, 1=slot 3
		break;
	case 2:
		ctrlReg = m_regs->BG2CNT;
		xScroll = m_regs->BG2HOFS;
		yScroll = m_regs->BG2VOFS;
		extPalBlock = 2;
		break;
	case 3:
		ctrlReg = m_regs->BG3CNT;
		xScroll = m_regs->BG3HOFS;
		yScroll = m_regs->BG3VOFS;
		extPalBlock = 3;
		break;
	}

	bool extendedPalette = ((m_regs->DISPCNT >> 30) & 0b1);
	int mosaicHorizontal = 0;// (MOSAIC & 0xF) + 1;
	int mosaicVertical = 0;// ((MOSAIC >> 4) & 0xF) + 1;

	bool mosaicEnabled = false;//  ((ctrlReg >> 6) & 0b1);

	uint8_t bgPriority = ctrlReg & 0b11;
	m_backgroundLayers[bg].priority = bgPriority;
	uint32_t tileDataBaseBlock = ((ctrlReg >> 2) & 0b1111);
	bool hiColor = ((ctrlReg >> 7) & 0b1);
	uint32_t bgMapBaseBlock = ((ctrlReg >> 8) & 0x1F);
	int screenSize = ((ctrlReg >> 14) & 0b11);
	static constexpr int xSizeLut[4] = { 255,511,255,511 };
	static constexpr int ySizeLut[4] = { 255,255,511,511 };
	int xWrap = xSizeLut[screenSize];
	int yWrap = ySizeLut[screenSize];

	int y = VCOUNT;
	if (mosaicEnabled)
		y = (y / mosaicVertical) * mosaicVertical;

	uint32_t fetcherY = ((y + yScroll) & yWrap);
	if (screenSize && (fetcherY > 255))					//messy, fix!
	{
		fetcherY -= 256;
		bgMapBaseBlock += 1;
		if (screenSize == 3)	//not completely sure
			bgMapBaseBlock += 1;
	}

	uint32_t bgMapYIdx = ((fetcherY / 8) * 32) * 2; //each row is 32 chars - each char is 2 bytes


	int screenX = 0;
	int tileFetchIdx = xScroll;
	while (screenX < 256)
	{
		int baseBlockOffset = 0;
		int normalizedTileFetchIdx = tileFetchIdx & xWrap;
		if (screenSize && normalizedTileFetchIdx > 255)		//not sure how/why this works anymore, but i'll roll with it /shrug
		{
			normalizedTileFetchIdx -= 256;
			baseBlockOffset++;
		}
		uint32_t bgMapBaseAddress = screenBase + ((bgMapBaseBlock + baseBlockOffset) * 2048) + bgMapYIdx;
		bgMapBaseAddress += ((normalizedTileFetchIdx >> 3) * 2);

		uint8_t tileLower = ppuReadBg<engine>(bgMapBaseAddress);
		uint8_t tileHigher = ppuReadBg<engine>(bgMapBaseAddress + 1);
		uint16_t tile = ((uint16_t)tileHigher << 8) | tileLower;

		uint32_t tileNumber = tile & 0x3FF;
		uint32_t paletteNumber = ((tile >> 12) & 0xF);
		bool verticalFlip = ((tile >> 11) & 0b1);
		bool horizontalFlip = ((tile >> 10) & 0b1);

		int yMod8 = ((fetcherY & 7));
		if (verticalFlip)
			yMod8 = 7 - yMod8;

		static constexpr uint32_t tileByteSizeLUT[2] = { 32,64 };
		static constexpr uint32_t tileRowPitchLUT[2] = { 4,8 };

		uint32_t tileMapBaseAddress = charBase + (tileDataBaseBlock * 16384) + (tileNumber * tileByteSizeLUT[hiColor]); //find correct tile based on just the tile number
		tileMapBaseAddress += (yMod8 * tileRowPitchLUT[hiColor]);									//then extract correct row of tile info, row pitch says how large each row is in bytes

		//todo: clean this bit up
		int initialTileIdx = ((tileFetchIdx >> 3) & 0xFF);
		int renderTileIdx = initialTileIdx;
		while (initialTileIdx == renderTileIdx)	//keep rendering pixels from this tile until next tile reached (i.e. we're done)
		{
			int pixelOffset = tileFetchIdx & 7;
			if (horizontalFlip)
				pixelOffset = 7 - pixelOffset;
			uint16_t col = 0x8000;
			if (hiColor)
			{
				int paletteEntry = ppuReadBg<engine>(tileMapBaseAddress + pixelOffset);
				uint32_t paletteMemoryAddr = paletteEntry << 1;
				if (engine == Engine::B)
					paletteMemoryAddr += 0x400;

				if (paletteEntry)
				{
					if (!extendedPalette)
					{
						uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
						uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];
						col = ((colHigh << 8) | colLow) & 0x7FFF;
					}
					else
					{
						paletteMemoryAddr = paletteNumber * 512;
						paletteMemoryAddr += (paletteEntry * 2);
						uint8_t colLow = ppuReadBgExtPal<engine>(extPalBlock, paletteMemoryAddr);
						uint8_t colHigh = ppuReadBgExtPal<engine>(extPalBlock, paletteMemoryAddr + 1);
						col = ((colHigh << 8) | colLow) & 0x7FFF;
					}
				}
			}
			else
			{
				uint8_t tileData = ppuReadBg<engine>(tileMapBaseAddress + (pixelOffset >> 1));
				int stepTile = ((pixelOffset & 0b1)) << 2;
				int colorId = ((tileData >> stepTile) & 0xf);
				if (colorId)
				{
					uint32_t paletteMemoryAddr = paletteNumber * 32;
					paletteMemoryAddr += (colorId * 2);
					if (engine == Engine::B)
						paletteMemoryAddr += 0x400;
					uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
					uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];
					col = ((colHigh << 8) | colLow) & 0x7FFF;
				}
			}

			if (screenX < 256)
				m_backgroundLayers[bg].lineBuffer[screenX] = col;
			screenX++;

			if (!mosaicEnabled)
				tileFetchIdx++;
			else
				tileFetchIdx = ((screenX / mosaicHorizontal) * mosaicHorizontal) + xScroll;		//if mosaic enabled, align to mosaic grid instead of just moving to next pixel

			renderTileIdx = (tileFetchIdx >> 3) & 0xFF;
		}
	}
}

template<Engine engine, int bg, bool extended> void PPU::renderAffineBackground()
{
	PPURegisters* m_regs = nullptr;
	BackgroundLayer* m_backgroundLayers = nullptr;
	uint32_t screenBase = 0, charBase=0;
	if constexpr (engine == Engine::A)
	{
		m_regs = &m_engineARegisters;
		m_backgroundLayers = m_engineABgLayers;
		screenBase = (((m_regs->DISPCNT) >> 27) & 0b111) * 65536;
		charBase = (((m_regs->DISPCNT) >> 24) & 0b111) * 65536;
	}
	else
	{
		m_regs = &m_engineBRegisters;
		m_backgroundLayers = m_engineBBgLayers;
	}

	uint16_t ctrlReg = {};
	int16_t dx = {}, dy = {}, dmx = {}, dmy = {};
	int32_t xRef = {}, yRef = {};

	switch (bg)
	{
	case 2:
		ctrlReg = m_regs->BG2CNT;
		xRef = m_regs->BG2X&0x0FFFFFFF;
		yRef = m_regs->BG2Y&0x0FFFFFFF;

		if ((xRef >> 27) & 0b1)
			xRef |= 0xF0000000;
		if ((yRef >> 27) & 0b1)
			yRef |= 0xF0000000;

		dx = m_regs->BG2PA;
		dmx = m_regs->BG2PB;
		dy = m_regs->BG2PC;
		dmy = m_regs->BG2PD;

		//hacky: doesn't account for mosaic
		m_regs->BG2X = (xRef+dmx) & 0x0FFFFFFF;
		m_regs->BG2Y = (yRef+dmy) & 0x0FFFFFFF;
		break;
	case 3:
		ctrlReg = m_regs->BG3CNT;
		xRef = m_regs->BG3X&0x0FFFFFFF;
		yRef = m_regs->BG3Y&0x0FFFFFFF;

		if ((xRef >> 27) & 0b1)
			xRef |= 0xF0000000;
		if ((yRef >> 27) & 0b1)
			yRef |= 0xF0000000;

		dx = m_regs->BG3PA;
		dmx = m_regs->BG3PB;
		dy = m_regs->BG3PC;
		dmy = m_regs->BG3PD;

		m_regs->BG3X = (xRef+dmx) & 0x0FFFFFFF;
		m_regs->BG3Y = (yRef+dmy) & 0x0FFFFFFF;
		break;
	}

	bool extendedPalette = ((m_regs->DISPCNT >> 30) & 0b1);

	uint8_t priority = ctrlReg & 0b11;
	m_backgroundLayers[bg].priority = priority;

	uint32_t bgMapBaseBlock = (ctrlReg >> 8) & 0x1F;
	uint32_t tileDataBaseBlock = (ctrlReg >> 2) & 0xF;
	uint8_t screenSizeIdx = (ctrlReg >> 14) & 0b11;

	static constexpr uint32_t wrapLUT[4] = { 128,256,512,1024 };
	uint32_t xWrap = wrapLUT[screenSizeIdx];
	uint32_t yWrap = wrapLUT[screenSizeIdx];
	bool overflowWrap = ((ctrlReg >> 13) & 0b1);

	for (int x = 0; x < 256; x++, calcAffineCoords(xRef, yRef, dx, dy))
	{
		m_backgroundLayers[bg].lineBuffer[x] = 0x8000;
		uint32_t fetcherY = (uint32_t)((int32_t)yRef >> 8);
		if ((fetcherY >= yWrap) && !overflowWrap)
			continue;
		fetcherY &= (yWrap - 1);							

		uint32_t fetcherX = (uint32_t)((int32_t)xRef >> 8);
		if ((fetcherX >= xWrap) && !overflowWrap)
			continue;
		fetcherX &= (xWrap - 1);	

		uint32_t bgMapAddr = (fetcherY >> 3) * (xWrap >> 3);
		bgMapAddr += (fetcherX >> 3);

		uint32_t tileIdx = 0;

		if constexpr (extended)
		{
			bgMapAddr <<= 1;
			uint8_t tileLow = ppuReadBg<engine>(screenBase + (bgMapBaseBlock * 2048) + bgMapAddr);
			uint8_t tileHigh = ppuReadBg<engine>(screenBase + (bgMapBaseBlock * 2048) + bgMapAddr + 1);
			uint16_t tile = (tileHigh << 8) | tileLow;
			tileIdx = tile & 0x3FF;
			bool flipHorizontal = ((tile >> 10) & 0b1)<< 3;
			bool flipVertical = ((tile >> 11) & 0b1) << 3;
			uint32_t palette = (tile >> 12) & 0xF;

			uint32_t tileDataAddr = (tileDataBaseBlock * 16384) + (tileIdx * 64);

			uint32_t xOffs = flipHorizontal ? (7-(fetcherX & 7)) : (fetcherX & 7);
			uint32_t yOffs = flipVertical ? (7 - (fetcherY & 7)) : (fetcherY & 7);

			tileDataAddr += yOffs << 3;		//Y*8 - each row of tile is 8 bytes.
			tileDataAddr += xOffs;

			uint32_t paletteEntry = ppuReadBg<engine>(charBase + tileDataAddr);
			if (paletteEntry)
			{
				if (extendedPalette)
				{
					uint32_t palAddr = palette * 512;
					palAddr += (paletteEntry << 1);
					uint8_t colLow = ppuReadBgExtPal<engine>(bg, palAddr);
					uint8_t colHigh = ppuReadBgExtPal<engine>(bg, palAddr + 1);
					m_backgroundLayers[bg].lineBuffer[x] = ((colHigh << 8) | colLow) & 0x7FFF;
				}
				else
				{
					uint32_t palAddr = 0;
					if constexpr (engine == Engine::B)
						palAddr = 0x400;
					palAddr += (paletteEntry << 1);
					uint8_t colLow = m_mem->PAL[palAddr];
					uint8_t colHigh = m_mem->PAL[palAddr + 1];
					m_backgroundLayers[bg].lineBuffer[x] = ((colHigh << 8) | colLow) & 0x7FFF;
				}
			}
		}
		else
		{
			tileIdx = ppuReadBg<engine>(screenBase + (bgMapBaseBlock * 2048) + bgMapAddr);

			uint32_t tileDataAddr = (tileDataBaseBlock * 16384) + (tileIdx * 64);	//64 bytes: each pixel in 8x8 tile encodes palette entry
			tileDataAddr += ((fetcherY & 7) * 8);
			tileDataAddr += (fetcherX & 7);

			uint32_t palAddr = ppuReadBg<engine>(charBase + tileDataAddr) << 1;
			if (engine == Engine::B)
				palAddr += 0x400;
			uint16_t col = m_mem->PAL[palAddr] | (m_mem->PAL[palAddr + 1] << 8);
			if (palAddr)
				m_backgroundLayers[bg].lineBuffer[x] = col & 0x7FFF;
		}
	}
}

template<Engine engine, int bg> void PPU::renderDirectColorBitmap()
{
	PPURegisters* m_regs = {};
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	if constexpr (engine == Engine::A)
		m_regs = &m_engineARegisters;
	else
	{
		m_backgroundLayers = m_engineBBgLayers;
		m_regs = &m_engineBRegisters;
	}

	uint16_t ctrlReg = {};
	switch (bg)
	{
	case 2:
		ctrlReg = m_regs->BG2CNT;
		break;
	case 3:
		ctrlReg = m_regs->BG3CNT;
		break;
	}

	uint32_t screenBase = ((ctrlReg >> 8) & 0x1F) * 16384;
	uint8_t bgPriority = ctrlReg & 0b11;
	m_backgroundLayers[bg].priority = bgPriority;

	for (int x = 0; x < 256; x++)		//todo: account for affine
	{
		uint32_t pixelBase = ((256 * VCOUNT * 2) + (x << 1));
		uint8_t colLow = ppuReadBg<engine>(screenBase+pixelBase);
		uint8_t colHigh = ppuReadBg<engine>(screenBase+pixelBase+1);
		uint16_t res = (colHigh << 8) | colLow;
		m_backgroundLayers[bg].lineBuffer[x] = res & 0x7FFF;		//bit 15 used as alpha flag for bitmap modes in nds, should handle at some point.
	}
}

template<Engine engine, int bg> void PPU::render256ColorBitmap()
{
	PPURegisters* m_regs = {};
	BackgroundLayer* m_backgroundLayers = m_engineABgLayers;
	uint32_t paletteBase = 0;
	if constexpr (engine == Engine::A)
		m_regs = &m_engineARegisters;
	else
	{
		paletteBase = 0x400;
		m_backgroundLayers = m_engineBBgLayers;
		m_regs = &m_engineBRegisters;
	}

	uint16_t ctrlReg = {};
	switch (bg)
	{
	case 2:
		ctrlReg = m_regs->BG2CNT;
		break;
	case 3:
		ctrlReg = m_regs->BG3CNT;
		break;
	}
	uint32_t screenBase = ((ctrlReg >> 8) & 0x1F) * 16384;
	uint8_t bgPriority = ctrlReg & 0b11;
	m_backgroundLayers[bg].priority = bgPriority;

	for (int x = 0; x < 256; x++)
	{
		uint32_t pixelBase = ((256 * VCOUNT) + x);
		uint8_t paletteId = ppuReadBg<engine>(screenBase+pixelBase);
		uint16_t col = 0x8000;
		if (paletteId)
		{
			uint8_t colLow = m_mem->PAL[paletteBase + ((uint32_t)(paletteId) << 1)];
			uint8_t colHigh = m_mem->PAL[paletteBase + ((uint32_t)(paletteId) << 1) + 1];
			col = (colHigh << 8) | colLow;
		}
		m_backgroundLayers[bg].lineBuffer[x] = col & 0x7FFF;
	}
}

template<Engine engine> void PPU::renderSprites()
{
	uint32_t m_OAMBase = 0;
	SpriteAttribute* m_spriteAttrBuffer = nullptr;
	uint16_t* m_spriteLineBuffer = nullptr;
	PPURegisters* m_registers = {};
	switch (engine)
	{
	case Engine::A:
		m_registers = &m_engineARegisters;
		m_spriteAttrBuffer = m_engineASpriteAttribBuffer;
		m_spriteLineBuffer = m_engineASpriteLineBuffer;
		break;
	case Engine::B:
		m_registers = &m_engineBRegisters;
		m_spriteAttrBuffer = m_engineBSpriteAttribBuffer;
		m_spriteLineBuffer = m_engineBSpriteLineBuffer;
		m_OAMBase = 0x400;
		break;
	}
	memset(m_spriteAttrBuffer, 0b00011111, 256);
	memset(m_spriteLineBuffer, 0x80, 512);

	if (!((m_registers->DISPCNT >> 12) & 0b1))							
		return;
	bool extendedPalettes = ((m_registers->DISPCNT >> 31) & 0b1);
	bool oneDimensionalMapping = ((m_registers->DISPCNT >> 4) & 0b1);	

	int mosaicHorizontal = 1;// ((MOSAIC >> 8) & 0xF) + 1;
	int mosaicVertical = 1;// ((MOSAIC >> 12) & 0xF) + 1;

	bool limitSpriteCycles = ((m_registers->DISPCNT >> 5) & 0b1);
	int maxAllowedSpriteCycles = (limitSpriteCycles) ? 954 : 1210;	//with h-blank interval free set, then less cycles can be spent rendering sprites

	for (int i = 0; i < 128; i++)
	{
		uint32_t spriteBase = i * 8;	//each OAM entry is 8 bytes

		OAMEntry* curSpriteEntry = (OAMEntry*)(m_mem->OAM + m_OAMBase + spriteBase);
		if (curSpriteEntry->objMode == 3)
			continue;
		if (curSpriteEntry->rotateScale)	
		{
			renderAffineSprite<engine>(curSpriteEntry);
			continue;
		}

		if (curSpriteEntry->disableObj)
			continue;

		bool isObjWindow = curSpriteEntry->objMode == 2;
		//bool mosaic = (curSpriteEntry->attr0 >> 12) & 0b1;

		int renderY = VCOUNT;
		if (curSpriteEntry->mosaic)
			renderY = (renderY / mosaicVertical) * mosaicVertical;

		int spriteTop = curSpriteEntry->yCoord;
		if (spriteTop >= 192)							//bit of a dumb hack to accommodate for when sprites are offscreen
			spriteTop -= 256;
		int spriteLeft = curSpriteEntry->xCoord;
		if (spriteLeft >= 256)
			spriteLeft -= 512;
		if (spriteTop > renderY)	//nope. sprite is offscreen or too low
			continue;
		int spriteBottom = 0, spriteRight = 0;
		int rowPitch = 1;	//find out how many lines we have to 'cross' to get to next row (in 1d mapping)
		//need to find out dimensions first to figure out whether to ignore this object
		int spritePriority = curSpriteEntry->priority;

		int spriteBoundsLookupId = (curSpriteEntry->shape << 2) | curSpriteEntry->size;
		static constexpr int spriteXBoundsLUT[12] = { 8,16,32,64,16,32,32,64,8,8,16,32 };
		static constexpr int spriteYBoundsLUT[12] = { 8,16,32,64,8,8,16,32,16,32,32,64 };
		static constexpr int xPitchLUT[12] = { 1,2,4,8,2,4,4,8,1,1,2,4 };

		spriteRight = spriteLeft + spriteXBoundsLUT[spriteBoundsLookupId];
		spriteBottom = spriteTop + spriteYBoundsLUT[spriteBoundsLookupId];
		rowPitch = xPitchLUT[spriteBoundsLookupId];
		if (VCOUNT >= spriteBottom)	//nope, we're past it.
			continue;

		bool flipHorizontal = curSpriteEntry->xFlip;
		bool flipVertical = curSpriteEntry->yFlip;

		int spriteYSize = (spriteBottom - spriteTop);	//find out how big sprite is
		int yOffsetIntoSprite = renderY - spriteTop;
		if (flipVertical)
			yOffsetIntoSprite = (spriteYSize - 1) - yOffsetIntoSprite;//flip y coord we're considering

		uint32_t tileId = curSpriteEntry->charName;
		bool hiColor = curSpriteEntry->bitDepth;
		if (hiColor)
			rowPitch *= 2;

		uint32_t tileObjBoundary = 32;
		int tileObjBoundarySelect = ((m_registers->DISPCNT >> 20) & 0b11);
		if (oneDimensionalMapping && tileObjBoundarySelect > 0)	//clean this up, kinda messy
		{
			static constexpr int tileObjBoundaryLUT[4] = { 32,64,128,256 };
			tileObjBoundary = tileObjBoundaryLUT[tileObjBoundarySelect];
		}

		//i hate all of this code. rewrite at some point...
		uint32_t objBase = tileId * tileObjBoundary;
		if (!hiColor)
			objBase += ((yOffsetIntoSprite&7) * 4);
		else
			objBase += ((yOffsetIntoSprite&7) * 8);

		while (yOffsetIntoSprite >= 8)	
		{
			yOffsetIntoSprite -= 8;
			if (!oneDimensionalMapping)
				objBase += 32 * 32;
			else
				objBase += (rowPitch * 32);
		}

		int numXTilesToRender = (spriteRight - spriteLeft) / 8;
		for (int xSpanTile = 0; xSpanTile < numXTilesToRender; xSpanTile++)
		{
			int curXSpanTile = xSpanTile;
			if (flipHorizontal)
				curXSpanTile = ((numXTilesToRender - 1) - curXSpanTile);	//flip render order if horizontal flip !!
			uint32_t tileMapLookupAddr = objBase + (curXSpanTile * ((hiColor) ? 64 : 32));

			for (int x = 0; x < 8; x++)
			{
				int baseX = x;
				if (flipHorizontal)
					baseX = 7 - baseX;

				int plotCoord = (xSpanTile * 8) + x + spriteLeft;
				if (plotCoord > 255 || plotCoord < 0)
					continue;

				uint16_t col = extractColorFromTile<engine>(tileMapLookupAddr, baseX, hiColor, curSpriteEntry->paletteNumber);

				if (isObjWindow)
				{
					if (!(col >> 15))
						m_spriteAttrBuffer[plotCoord].objWindow = 1;
					continue;
				}

				uint8_t priorityAtPixel = m_spriteAttrBuffer[plotCoord].priority;
				bool renderedPixelTransparent = m_spriteLineBuffer[plotCoord] >> 15;
				bool currentPixelTransparent = col >> 15;
				if ((spritePriority >= priorityAtPixel) && (!renderedPixelTransparent || currentPixelTransparent))	//keep rendering if lower priority, BUT last pixel transparent
					continue;

				if (!currentPixelTransparent)
				{
					m_spriteAttrBuffer[plotCoord].priority = spritePriority & 0b11111;
					m_spriteAttrBuffer[plotCoord].transparent = (curSpriteEntry->objMode == 1);
					m_spriteAttrBuffer[plotCoord].mosaic = curSpriteEntry->mosaic;
					m_spriteLineBuffer[plotCoord] = col;
				}
			}
		}

	}
}

template<Engine engine> void PPU::renderAffineSprite(OAMEntry* curSpriteEntry)
{
	PPURegisters* m_registers = &m_engineARegisters;
	SpriteAttribute* m_spriteAttrBuffer = m_engineASpriteAttribBuffer;
	uint16_t* m_spriteLineBuffer = m_engineASpriteLineBuffer;
	uint32_t m_oamBase = 0;
	if constexpr (engine == Engine::B)
	{
		m_spriteAttrBuffer = m_engineBSpriteAttribBuffer;
		m_spriteLineBuffer = m_engineBSpriteLineBuffer;
		m_oamBase = 0x400;
		m_registers = &m_engineBRegisters;
	}
	bool oneDimensionalMapping = ((m_registers->DISPCNT >> 4) & 0b1);
	bool isObjWindow = (curSpriteEntry->objMode == 2);
	bool doubleSize = curSpriteEntry->disableObj;	//odd: bit 9 is 'double-size' flag with affine sprites

	int spriteTop = curSpriteEntry->yCoord;
	if (spriteTop >= 192)
		spriteTop -= 256;
	int spriteLeft = curSpriteEntry->xCoord;
	if (spriteLeft >= 256)
		spriteLeft -= 512;

	if (spriteTop > VCOUNT)
		return;

	int spriteBottom = 0, spriteRight = 0;
	int rowPitch = 1;	//find out how many lines we have to 'cross' to get to next row (in 1d mapping)

	//need to find out dimensions first to figure out whether to ignore this object
	int spriteBoundsLookupId = (curSpriteEntry->shape << 2) | curSpriteEntry->size;
	static constexpr int spriteXBoundsLUT[12] = { 8,16,32,64,16,32,32,64,8,8,16,32 };
	static constexpr int spriteYBoundsLUT[12] = { 8,16,32,64,8,8,16,32,16,32,32,64 };
	static constexpr int xPitchLUT[12] = { 1,2,4,8,2,4,4,8,1,1,2,4 };


	spriteRight = spriteLeft + spriteXBoundsLUT[spriteBoundsLookupId];
	spriteBottom = spriteTop + spriteYBoundsLUT[spriteBoundsLookupId];
	rowPitch = xPitchLUT[spriteBoundsLookupId];
	int doubleSizeOffset = ((spriteBottom - spriteTop)) * doubleSize;
	if (VCOUNT >= (spriteBottom + doubleSizeOffset))	//nope, we're past it.
		return;

	uint32_t tileId = curSpriteEntry->charName;
	bool hiColor = curSpriteEntry->bitDepth;
	if (hiColor)
		rowPitch *= 2;
	int yOffsetIntoSprite = VCOUNT - spriteTop;
	int xBase = 0;

	int halfWidth = (spriteRight - spriteLeft) / 2;
	int halfHeight = (spriteBottom - spriteTop) / 2;
	int spriteWidth = halfWidth * 2;
	int spriteHeight = halfHeight * 2;	//find out how big sprite is

	//get affine parameters
	uint32_t parameterSelection = (curSpriteEntry->data >> 25) & 0x1F;
	parameterSelection *= 0x20;
	parameterSelection += 6;
	int16_t PA = m_mem->OAM[m_oamBase + parameterSelection] | ((m_mem->OAM[m_oamBase + parameterSelection + 1]) << 8);
	int16_t PB = m_mem->OAM[m_oamBase + parameterSelection + 8] | ((m_mem->OAM[m_oamBase + parameterSelection + 9]) << 8);
	int16_t PC = m_mem->OAM[m_oamBase + parameterSelection + 16] | ((m_mem->OAM[m_oamBase + parameterSelection + 17]) << 8);
	int16_t PD = m_mem->OAM[m_oamBase + parameterSelection + 24] | ((m_mem->OAM[m_oamBase + parameterSelection + 25]) << 8);

	uint32_t tileObjBoundary = 32;
	int tileObjBoundarySelect = ((m_registers->DISPCNT >> 20) & 0b11);
	if (oneDimensionalMapping && tileObjBoundarySelect > 0)	//clean this up, kinda messy
	{
		static constexpr int tileObjBoundaryLUT[4] = { 32,64,128,256 };
		tileObjBoundary = tileObjBoundaryLUT[tileObjBoundarySelect];
	}

	uint32_t objBase = tileId * tileObjBoundary;

	for (int x = 0; x < spriteWidth * ((doubleSize) ? 2 : 1); x++)
	{
		int ix = (x - halfWidth);
		int iy = (yOffsetIntoSprite - halfHeight);
		if (doubleSize)
		{
			ix = (x - spriteWidth);
			iy = (yOffsetIntoSprite - spriteHeight);
		}

		uint32_t px = ((PA * ix + PB * iy) >> 8);
		uint32_t py = ((PC * ix + PD * iy) >> 8);
		px += halfWidth; py += halfHeight;
		if (py >= spriteHeight || px >= spriteWidth)
			continue;

		uint32_t objAddress = objBase;
		uint32_t yOffs = py;
		while (yOffs >= 8)
		{
			yOffs -= 8;
			if (!oneDimensionalMapping)
				objAddress += (32 * 32);
			else
				objAddress += (rowPitch * 32);
		}
		uint32_t yCorrection = ((py&7) * ((hiColor) ? 8 : 4));

		int curXSpanTile = px / 8;
		int xOffsIntoTile = px & 7;

		uint32_t tileMapLookupAddr = objAddress + yCorrection + (curXSpanTile * ((hiColor) ? 64 : 32));

		int plotCoord = x + spriteLeft;
		if (plotCoord > 255 || plotCoord < 0)
			continue;

		//uint16_t col = extractColorFromTile(tileMapLookupAddr, xOffsIntoTile, hiColor, true, curSpriteEntry->paletteNumber);
		uint16_t col = extractColorFromTile<engine>(tileMapLookupAddr, xOffsIntoTile, hiColor, curSpriteEntry->paletteNumber);
		if (isObjWindow)
		{
			if (!(col >> 15))
				m_spriteAttrBuffer[plotCoord].objWindow = 1;
			continue;
		}

		uint8_t priorityAtPixel = m_spriteAttrBuffer[plotCoord].priority;
		bool renderedPixelTransparent = m_spriteLineBuffer[plotCoord] >> 15;
		bool currentPixelTransparent = col >> 15;
		if ((curSpriteEntry->priority >= priorityAtPixel) && (!renderedPixelTransparent || currentPixelTransparent))	//same as for normal, only stop if we're transparent (and lower priority)
			continue;																						//...or last pixel isn't transparent
		if (!currentPixelTransparent)
		{
			m_spriteAttrBuffer[plotCoord].priority = curSpriteEntry->priority & 0b11111;
			m_spriteAttrBuffer[plotCoord].transparent = (curSpriteEntry->objMode == 1);
			m_spriteAttrBuffer[plotCoord].mosaic = curSpriteEntry->mosaic;
			m_spriteLineBuffer[plotCoord] = col;
		}
	}

}

template<Engine engine>uint16_t PPU::extractColorFromTile(uint32_t tileBase, uint32_t xOffset, bool hiColor, uint32_t palette)
{
	uint16_t col = 0;
	uint32_t paletteMemoryAddr = 0x200;
	PPURegisters* m_regs = &m_engineARegisters;
	if constexpr (engine == Engine::B)
	{
		m_regs = &m_engineBRegisters;
		paletteMemoryAddr = 0x600;
	}
	if (!hiColor)
	{
		tileBase += (xOffset / 2);

		uint8_t tileData = ppuReadObj<engine>(tileBase);
		int colorId = 0;
		int stepTile = ((xOffset & 0b1)) << 2;
		colorId = ((tileData >> stepTile) & 0xf);	//first (even) pixel - low nibble. second (odd) pixel - high nibble
		if (!colorId)
			return 0x8000;

		paletteMemoryAddr += palette * 32;
		paletteMemoryAddr += (colorId * 2);
		uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
		uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];

		col = (colHigh << 8) | colLow;
	}
	else
	{
		tileBase += xOffset;
		uint8_t tileData = ppuReadObj<engine>(tileBase);
		if (!tileData)
			return 0x8000;
		if ((m_regs->DISPCNT >> 31) & 0b1)
		{
			paletteMemoryAddr = palette * 512;
			paletteMemoryAddr += tileData * 2;
			uint8_t colLow = ppuReadObjExtPal<engine>(paletteMemoryAddr);
			uint8_t colHigh = ppuReadObjExtPal<engine>(paletteMemoryAddr + 1);
			col = (colHigh << 8) | colLow;
		}
		else
		{
			paletteMemoryAddr += (tileData * 2);
			uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
			uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];

			col = (colHigh << 8) | colLow;
		}
	}


	return col & 0x7FFF;
}

uint8_t PPU::readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000304:
		return POWCNT1 & 0xFF;
	case 0x04000305:
		return ((POWCNT1 >> 8) & 0xFF);
	case 0x04000240:
		return VRAMCNT_A;
	case 0x04000241:
		return VRAMCNT_B;
	case 0x04000242:
		return VRAMCNT_C;
	case 0x04000243:
		return VRAMCNT_D;
	case 0x04000244:
		return VRAMCNT_E;
	case 0x04000245:
		return VRAMCNT_F;
	case 0x04000246:
		return VRAMCNT_G;
	case 0x04000248:
		return VRAMCNT_H;
	case 0x04000249:
		return VRAMCNT_I;
	case 0x04000000:
		return m_engineARegisters.DISPCNT & 0xFF;
	case 0x04000001:
		return ((m_engineARegisters.DISPCNT >> 8) & 0xFF);
	case 0x04000002:
		return ((m_engineARegisters.DISPCNT >> 16) & 0xFF);
	case 0x04000003:
		return ((m_engineARegisters.DISPCNT >> 24) & 0xFF);
	case 0x04000004:
		return DISPSTAT & 0xFF;
	case 0x04000005:
		return ((DISPSTAT >> 8) & 0xFF);
	case 0x04000006:
		return VCOUNT & 0xFF;
	case 0x04000007:
		return ((VCOUNT >> 8) & 0xFF);
	case 0x04000008:
		return m_engineARegisters.BG0CNT & 0xFF;
	case 0x04000009:
		return ((m_engineARegisters.BG0CNT >> 8) & 0xFF);
	case 0x0400000A:
		return m_engineARegisters.BG1CNT & 0xFF;
	case 0x0400000B:
		return ((m_engineARegisters.BG1CNT >> 8) & 0xFF);
	case 0x0400000C:
		return m_engineARegisters.BG2CNT & 0xFF;
	case 0x0400000D:
		return ((m_engineARegisters.BG2CNT >> 8) & 0xFF);
	case 0x0400000E:
		return m_engineARegisters.BG3CNT & 0xFF;
	case 0x0400000F:
		return ((m_engineARegisters.BG3CNT >> 8) & 0xFF);
	case 0x04001000:
		return m_engineBRegisters.DISPCNT & 0xFF;
	case 0x04001001:
		return ((m_engineBRegisters.DISPCNT >> 8) & 0xFF);
	case 0x04001002:
		return ((m_engineBRegisters.DISPCNT >> 16) & 0xFF);
	case 0x04001003:
		return ((m_engineBRegisters.DISPCNT >> 24) & 0xFF);
	case 0x04001008:
		return m_engineBRegisters.BG0CNT & 0xFF;
	case 0x04001009:
		return ((m_engineBRegisters.BG0CNT >> 8) & 0xFF);
	case 0x0400100A:
		return m_engineBRegisters.BG1CNT & 0xFF;
	case 0x0400100B:
		return ((m_engineBRegisters.BG1CNT >> 8) & 0xFF);
	case 0x0400100C:
		return m_engineBRegisters.BG2CNT & 0xFF;
	case 0x0400100D:
		return ((m_engineBRegisters.BG2CNT >> 8) & 0xFF);
	case 0x0400100E:
		return m_engineBRegisters.BG3CNT & 0xFF;
	case 0x0400100F:
		return ((m_engineBRegisters.BG3CNT >> 8) & 0xFF);
	}
	//Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO read! addr={:#x}", address));
	return 0;
}

void PPU::writeIO(uint32_t address, uint8_t value)
{
	PPURegisters* m_registers = nullptr;
	BackgroundLayer* m_bgLayers = nullptr;
	switch ((address >> 12) & 0xF)
	{
	case 0:
		m_bgLayers = m_engineABgLayers;
		m_registers = &m_engineARegisters;
		break;
	case 1:
		address &= ~0xF000;				//unset bits 12-15 (so addresses alias engine a regs)
		m_bgLayers = m_engineBBgLayers;		//
		m_registers = &m_engineBRegisters;	//writes modify engine b regs/layers instead
		break;
	}
	switch (address)
	{
		//POWCNT1
	case 0x04000304:
		POWCNT1 &= 0xFF00; POWCNT1 |= value;
		break;
	case 0x04000305:
		POWCNT1 &= 0x00FF; POWCNT1 |= (value << 8);
		if (!(POWCNT1 >> 15))
		{
			EngineA_RenderBase = 256 * 192;
			EngineB_RenderBase = 0;
		}
		else
		{
			EngineA_RenderBase = 0;
			EngineB_RenderBase = 256 * 192;
		}
		break;
		//vram banking.. aaa
	case 0x04000240:
		VRAMCNT_A = value;
		rebuildPageTables();
		break;
	case 0x04000241:
		VRAMCNT_B = value;
		rebuildPageTables();
		break;
	case 0x04000242:
		VRAMCNT_C = value;
		rebuildPageTables();
		break;
	case 0x04000243:
		VRAMCNT_D = value;
		rebuildPageTables();
		break;
	case 0x04000244:
		VRAMCNT_E = value;
		rebuildPageTables();
		break;
	case 0x04000245:
		VRAMCNT_F = value;
		rebuildPageTables();
		break;
	case 0x04000246:
		VRAMCNT_G = value;
		rebuildPageTables();
		break;
	case 0x04000248:
		VRAMCNT_H = value;
		rebuildPageTables();
		break;
	case 0x04000249:
		VRAMCNT_I = value;
		rebuildPageTables();
		break;
	case 0x04000000:
		m_registers->DISPCNT &= 0xFFFFFF00; m_registers->DISPCNT |= value;
		break;
	case 0x04000001:
		m_registers->DISPCNT &= 0xFFFF00FF; m_registers->DISPCNT |= (value << 8);
		for (int i = 8; i < 12; i++)
		{
			m_bgLayers[i-8].enabled = false;
			if ((m_registers->DISPCNT >> i) & 0b1)
				m_bgLayers[i-8].enabled = true;
		}
		break;
	case 0x04000002:
		m_registers->DISPCNT &= 0xFF00FFFF; m_registers->DISPCNT |= (value << 16);
		break;
	case 0x04000003:
		m_registers->DISPCNT &= 0x00FFFFFF; m_registers->DISPCNT |= (value << 24);
		break;
	case 0x04000004:
		DISPSTAT &= 0xFF07; DISPSTAT |= (value & 0xF8);
		break;
	case 0x04000005:
		DISPSTAT &= 0x00FF; DISPSTAT |= (value << 8);
		checkVCOUNTInterrupt();
		break;
	case 0x04000008:
		m_registers->BG0CNT &= 0xFF00; m_registers->BG0CNT |= value;
		break;
	case 0x04000009:
		m_registers->BG0CNT &= 0x00FF; m_registers->BG0CNT |= (value << 8);
		break;
	case 0x0400000A:
		m_registers->BG1CNT &= 0xFF00; m_registers->BG1CNT |= value;
		break;
	case 0x0400000B:
		m_registers->BG1CNT &= 0x00FF; m_registers->BG1CNT |= (value << 8);
		break;
	case 0x0400000C:
		m_registers->BG2CNT &= 0xFF00; m_registers->BG2CNT |= value;
		break;
	case 0x0400000D:
		m_registers->BG2CNT &= 0x00FF; m_registers->BG2CNT |= (value << 8);
		break;
	case 0x0400000E:
		m_registers->BG3CNT &= 0xFF00; m_registers->BG3CNT |= value;
		break;
	case 0x0400000F:
		m_registers->BG3CNT &= 0x00FF; m_registers->BG3CNT |= (value << 8);
		break;
	case 0x04000010:
		m_registers->BG0HOFS &= 0xFF00; m_registers->BG0HOFS |= value;
		break;
	case 0x04000011:
		m_registers->BG0HOFS &= 0x00FF; m_registers->BG0HOFS |= ((value & 0b1) << 8);
		break;
	case 0x04000012:
		m_registers->BG0VOFS &= 0xFF00; m_registers->BG0VOFS |= value;
		break;
	case 0x04000013:
		m_registers->BG0VOFS &= 0x00FF; m_registers->BG0VOFS |= ((value & 0b1) << 8);
		break;
	case 0x04000014:
		m_registers->BG1HOFS &= 0xFF00; m_registers->BG1HOFS |= value;
		break;
	case 0x04000015:
		m_registers->BG1HOFS &= 0x00FF; m_registers->BG1HOFS |= ((value & 0b1) << 8);
		break;
	case 0x04000016:
		m_registers->BG1VOFS &= 0xFF00; m_registers->BG1VOFS |= value;
		break;
	case 0x04000017:
		m_registers->BG1VOFS &= 0x00FF; m_registers->BG1VOFS |= ((value & 0b1) << 8);
		break;
	case 0x04000018:
		m_registers->BG2HOFS &= 0xFF00; m_registers->BG2HOFS |= value;
		break;
	case 0x04000019:
		m_registers->BG2HOFS &= 0x00FF; m_registers->BG2HOFS |= ((value & 0b1) << 8);
		break;
	case 0x0400001A:
		m_registers->BG2VOFS &= 0xFF00; m_registers->BG2VOFS |= value;
		break;
	case 0x0400001B:
		m_registers->BG2VOFS &= 0x00FF; m_registers->BG2VOFS |= ((value & 0b1) << 8);
		break;
	case 0x0400001C:
		m_registers->BG3HOFS &= 0xFF00; m_registers->BG3HOFS |= value;
		break;
	case 0x0400001D:
		m_registers->BG3HOFS &= 0x00FF; m_registers->BG3HOFS |= ((value & 0b1) << 8);
		break;
	case 0x0400001E:
		m_registers->BG3VOFS &= 0xFF00; m_registers->BG3VOFS |= value;
		break;
	case 0x0400001F:
		m_registers->BG3VOFS &= 0x00FF; m_registers->BG3VOFS |= ((value & 0b1) << 8);
		break;
	case 0x04000020:
		m_registers->BG2PA &= 0xFF00; m_registers->BG2PA |= value;
		break;
	case 0x04000021:
		m_registers->BG2PA &= 0x00FF; m_registers->BG2PA |= (value << 8);
		break;
	case 0x04000022:
		m_registers->BG2PB &= 0xFF00; m_registers->BG2PB |= value;
		break;
	case 0x04000023:
		m_registers->BG2PB &= 0x00FF; m_registers->BG2PB |= (value << 8);
		break;
	case 0x04000024:
		m_registers->BG2PC &= 0xFF00; m_registers->BG2PC |= value;
		break;
	case 0x04000025:
		m_registers->BG2PC &= 0x00FF; m_registers->BG2PC |= (value<<8);
		break;
	case 0x04000026:
		m_registers->BG2PD &= 0xFF00; m_registers->BG2PD |= value;
		break;
	case 0x04000027:
		m_registers->BG2PD &= 0x00FF; m_registers->BG2PD |= (value << 8);
		break;
	case 0x04000028:
		m_registers->reg_BG2X &= 0xFFFFFF00; m_registers->reg_BG2X |= value;
		m_registers->BG2X_dirty = true;
		break;
	case 0x04000029:
		m_registers->reg_BG2X &= 0xFFFF00FF; m_registers->reg_BG2X |= (value << 8);
		m_registers->BG2X_dirty = true;
		break;
	case 0x0400002A:
		m_registers->reg_BG2X &= 0xFF00FFFF; m_registers->reg_BG2X |= (value << 16);
		m_registers->BG2X_dirty = true;
		break;
	case 0x0400002B:
		m_registers->reg_BG2X &= 0x00FFFFFF; m_registers->reg_BG2X |= (value << 24);
		m_registers->BG2X_dirty = true;
		break;
	case 0x0400002C:
		m_registers->reg_BG2Y &= 0xFFFFFF00; m_registers->reg_BG2Y |= value;
		m_registers->BG2Y_dirty = true;
		break;
	case 0x0400002D:
		m_registers->reg_BG2Y &= 0xFFFF00FF; m_registers->reg_BG2Y |= (value << 8);
		m_registers->BG2Y_dirty = true;
		break;
	case 0x0400002E:
		m_registers->reg_BG2Y &= 0xFF00FFFF; m_registers->reg_BG2Y |= (value << 16);
		m_registers->BG2Y_dirty = true;
		break;
	case 0x0400002F:
		m_registers->reg_BG2Y &= 0x00FFFFFF; m_registers->reg_BG2Y |= (value << 24);
		m_registers->BG2Y_dirty = true;
		break;
	case 0x04000030:
		m_registers->BG3PA &= 0xFF00; m_registers->BG3PA |= value;
		break;
	case 0x04000031:
		m_registers->BG3PA &= 0x00FF; m_registers->BG3PA |= (value << 8);
		break;
	case 0x04000032:
		m_registers->BG3PB &= 0xFF00; m_registers->BG3PB |= value;
		break;
	case 0x04000033:
		m_registers->BG3PB &= 0x00FF; m_registers->BG3PB |= (value << 8);
		break;
	case 0x04000034:
		m_registers->BG3PC &= 0xFF00; m_registers->BG3PC |= value;
		break;
	case 0x04000035:
		m_registers->BG3PC &= 0x00FF; m_registers->BG3PC |= (value << 8);
		break;
	case 0x04000036:
		m_registers->BG3PD &= 0xFF00; m_registers->BG3PD |= value;
		break;
	case 0x04000037:
		m_registers->BG3PD &= 0x00FF; m_registers->BG3PD |= (value << 8);
		break;
	case 0x04000038:
		m_registers->reg_BG3X &= 0xFFFFFF00; m_registers->reg_BG3X |= value;
		m_registers->BG3X_dirty = true;
		break;
	case 0x04000039:
		m_registers->reg_BG3X &= 0xFFFF00FF; m_registers->reg_BG3X |= (value << 8);
		m_registers->BG3X_dirty = true;
		break;
	case 0x0400003A:
		m_registers->reg_BG3X &= 0xFF00FFFF; m_registers->reg_BG3X |= (value << 16);
		m_registers->BG3X_dirty = true;
		break;
	case 0x0400003B:
		m_registers->reg_BG3X &= 0x00FFFFFF; m_registers->reg_BG3X |= (value << 24);
		m_registers->BG3X_dirty = true;
		break;
	case 0x0400003C:
		m_registers->reg_BG3Y &= 0xFFFFFF00; m_registers->reg_BG3Y |= value;
		m_registers->BG3Y_dirty = true;
		break;
	case 0x0400003D:
		m_registers->reg_BG3Y &= 0xFFFF00FF; m_registers->reg_BG3Y |= (value << 8);
		m_registers->BG3Y_dirty = true;
		break;
	case 0x0400003E:
		m_registers->reg_BG3Y &= 0xFF00FFFF; m_registers->reg_BG3Y |= (value << 16);
		m_registers->BG3Y_dirty = true;
		break;
	case 0x0400003F:
		m_registers->reg_BG3Y &= 0x00FFFFFF; m_registers->reg_BG3Y |= (value << 24);
		m_registers->BG3Y_dirty = true;
		break;
		//default:
	//	Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO write! addr={:#x} val={:#x}", address, value));
	}
}

uint8_t PPU::NDS7_readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000004:
		return NDS7_DISPSTAT & 0xFF;
	case 0x04000005:
		return ((NDS7_DISPSTAT >> 8) & 0xFF);
	case 0x04000006:
		return VCOUNT & 0xFF;
	case 0x04000007:
		return ((VCOUNT >> 8) & 0xFF);
	}
}

void PPU::NDS7_writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000004:
		NDS7_DISPSTAT &= 0xFF07; NDS7_DISPSTAT |= (value & 0xF8);
		break;
	case 0x04000005:
		NDS7_DISPSTAT &= 0x00FF; NDS7_DISPSTAT |= (value << 8);
		checkVCOUNTInterrupt();
		break;
	}
}

void PPU::setVBlankFlag(bool value)
{
	DISPSTAT &= ~0b1;
	DISPSTAT |= value;
	NDS7_DISPSTAT &= ~0b1;
	NDS7_DISPSTAT |= value;
}

void PPU::setHBlankFlag(bool value)
{
	DISPSTAT &= ~0b10;
	DISPSTAT |= (value << 1);
	NDS7_DISPSTAT &= ~0b10;
	NDS7_DISPSTAT |= (value << 1);
}

void PPU::NDS7_setVCounterFlag(bool value)
{
	NDS7_DISPSTAT &= ~0b100;
	NDS7_DISPSTAT |= (value << 2);
}

void PPU::NDS9_setVCounterFlag(bool value)
{
	DISPSTAT &= ~0b100;
	DISPSTAT |= (value << 2);
}

void PPU::checkVCOUNTInterrupt()
{
	uint16_t vcountCompare = ((DISPSTAT >> 8) & 0xFF) | (((DISPSTAT >> 7) & 0b1) << 8);
	bool matches = (vcountCompare == VCOUNT);
	NDS9_setVCounterFlag(matches);
	if (matches && ((DISPSTAT >> 5) & 0b1) && !nds9_vcountIRQLine)
		m_interruptManager->NDS9_requestInterrupt(InterruptType::VCount);
	nds9_vcountIRQLine = matches;

	vcountCompare = ((NDS7_DISPSTAT >> 8) & 0xFF) | (((NDS7_DISPSTAT >> 7) & 0b1) << 8);
	matches = (vcountCompare == VCOUNT);
	NDS7_setVCounterFlag(matches);
	if (matches && ((NDS7_DISPSTAT >> 5) & 0b1) && !nds7_vcountIRQLine)
		m_interruptManager->NDS7_requestInterrupt(InterruptType::VCount);
	nds7_vcountIRQLine = matches;
}

uint32_t PPU::col16to32(uint16_t col)
{
	int red = (col & 0b0000000000011111);
	red = (red << 3) | (red >> 2);
	int green = (col & 0b0000001111100000) >> 5;
	green = (green << 3) | (green >> 2);
	int blue = (col & 0b0111110000000000) >> 10;
	blue = (blue << 3) | (blue >> 2);

	return (red << 24) | (green << 16) | (blue << 8) | 0x000000FF;
}

void PPU::onSchedulerEvent(void* context)
{
	PPU* thisPtr = (PPU*)context;
	thisPtr->eventHandler();
}

uint32_t PPU::m_safeDisplayBuffer[256 * 384] = {};