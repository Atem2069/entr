#include"PPU.h"

PPU::PPU(std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<Scheduler> scheduler)
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

void PPU::registerMemory(std::shared_ptr<NDSMem> mem)
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Registered memory with PPU!");
	m_mem = mem;

	//setup vram banks

	//128k banks
	m_mem->VRAM_A.VRAMBase = m_mem->VRAM;
	m_mem->VRAM_A.size = 0x20000;
	m_mem->VRAM_B.VRAMBase = m_mem->VRAM_A.VRAMBase + 0x20000;
	m_mem->VRAM_B.size = 0x20000;
	m_mem->VRAM_C.VRAMBase = m_mem->VRAM_B.VRAMBase + 0x20000;
	m_mem->VRAM_C.size = 0x20000;
	m_mem->VRAM_D.VRAMBase = m_mem->VRAM_C.VRAMBase + 0x20000;
	m_mem->VRAM_D.size = 0x20000;

	//64k
	m_mem->VRAM_E.VRAMBase = m_mem->VRAM_D.VRAMBase + 0x20000;
	m_mem->VRAM_E.size = 0x10000;

	//16k
	m_mem->VRAM_F.VRAMBase = m_mem->VRAM_E.VRAMBase + 0x10000;
	m_mem->VRAM_F.size = 0x4000;
	m_mem->VRAM_G.VRAMBase = m_mem->VRAM_F.VRAMBase + 0x4000;
	m_mem->VRAM_G.size = 0x4000;

	//32k
	m_mem->VRAM_H.VRAMBase = m_mem->VRAM_G.VRAMBase + 0x4000;
	m_mem->VRAM_H.size = 0x8000;

	//16k
	m_mem->VRAM_I.VRAMBase = m_mem->VRAM_H.VRAMBase + 0x8000;
	m_mem->VRAM_I.size = 0x4000;
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

	uint8_t displayMode = (DISPCNT >> 16) & 0b11;
	if (displayMode == 2)
		renderLCDCMode();
	else
	{
		uint8_t mode = DISPCNT & 0b111;
		switch (mode)
		{
		case 0:
			renderMode0();
			break;
		}
	}

	//line=2130 cycles. hdraw=1606 cycles? hblank=524 cycles
	setHBlankFlag(true);
	m_state = PPUState::HBlank;
	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);

	if (((DISPSTAT >> 4) & 0b1))
		m_interruptManager->NDS9_requestInterrupt(InterruptType::HBlank);
	if (((NDS7_DISPSTAT >> 4) & 0b1))
		m_interruptManager->NDS7_requestInterrupt(InterruptType::HBlank);
}

void PPU::HBlank()
{
	setHBlankFlag(false);
	VCOUNT++;
	//check vcount interrupt

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
		m_state = PPUState::HDraw;
	}
	//todo: weird behaviour of last line in vblank (same as in gba)

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

void PPU::renderLCDCMode()
{
	for (int i = 0; i < 256; i++)
	{
		uint8_t VRAMBlock = ((DISPCNT >> 18) & 0b11);
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

void PPU::renderMode0()
{
	uint16_t ctrlReg = m_engineARegisters.BG0CNT;
	uint32_t xScroll = 0, yScroll = 0;
	int mosaicHorizontal = 0;// (MOSAIC & 0xF) + 1;
	int mosaicVertical = 0;// ((MOSAIC >> 4) & 0xF) + 1;

	bool mosaicEnabled = false;//  ((ctrlReg >> 6) & 0b1);

	uint8_t bgPriority = ctrlReg & 0b11;
	uint32_t tileDataBaseBlock = ((ctrlReg >> 2) & 0b11);
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
		uint32_t bgMapBaseAddress = ((bgMapBaseBlock + baseBlockOffset) * 2048) + bgMapYIdx;
		bgMapBaseAddress += ((normalizedTileFetchIdx >> 3) * 2);

		uint8_t tileLower = m_mem->VRAM[bgMapBaseAddress];
		uint8_t tileHigher = m_mem->VRAM[bgMapBaseAddress + 1];
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

		uint32_t tileMapBaseAddress = (tileDataBaseBlock * 16384) + (tileNumber * tileByteSizeLUT[hiColor]); //find correct tile based on just the tile number
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
				int paletteEntry = m_mem->VRAM[tileMapBaseAddress + pixelOffset];
				uint32_t paletteMemoryAddr = paletteEntry << 1;

				//if (paletteEntry)
				{
					uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
					uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];
					col = ((colHigh << 8) | colLow) & 0x7FFF;
				}
			}
			else
			{
				uint8_t tileData = m_mem->VRAM[tileMapBaseAddress + (pixelOffset >> 1)];
				int stepTile = ((pixelOffset & 0b1)) << 2;
				int colorId = ((tileData >> stepTile) & 0xf);
				if (colorId)
				{
					uint32_t paletteMemoryAddr = paletteNumber * 32;
					paletteMemoryAddr += (colorId * 2);
					uint8_t colLow = m_mem->PAL[paletteMemoryAddr];
					uint8_t colHigh = m_mem->PAL[paletteMemoryAddr + 1];
					col = ((colHigh << 8) | colLow) & 0x7FFF;
				}
			}

			if (screenX < 256)
				m_renderBuffer[pageIdx][(256 * VCOUNT) + screenX] = col16to32(col);
				//m_backgroundLayers[bg].lineBuffer[screenX] = col;
			screenX++;

			if (!mosaicEnabled)
				tileFetchIdx++;
			else
				tileFetchIdx = ((screenX / mosaicHorizontal) * mosaicHorizontal) + xScroll;		//if mosaic enabled, align to mosaic grid instead of just moving to next pixel

			renderTileIdx = (tileFetchIdx >> 3) & 0xFF;
		}
	}
}

uint8_t PPU::readIO(uint32_t address)
{
	switch (address)
	{
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
		return DISPCNT & 0xFF;
	case 0x04000001:
		return ((DISPCNT >> 8) & 0xFF);
	case 0x04000002:
		return ((DISPCNT >> 16) & 0xFF);
	case 0x04000003:
		return ((DISPCNT >> 24) & 0xFF);
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
	}
	Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO read! addr={:#x}", address));
	return 0;
}

void PPU::writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
		//vram banking.. aaa
	case 0x04000240:
	{
		VRAMCNT_A = value;
		uint8_t MST = value & 0b11;
		uint8_t OFS = (value >> 3) & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_A.memBase = 0x06800000;
			break;
		case 1:
			m_mem->VRAM_A.memBase = 0x06000000 + (0x20000 * OFS);
			break;
		case 2:
			m_mem->VRAM_A.memBase = 0x06400000 + (0x20000 * (OFS & 0b1));
			break;
		}
		break;
	}
	case 0x04000241:
	{
		VRAMCNT_B = value;
		uint8_t MST = value & 0b11;
		uint8_t OFS = (value >> 3) & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_B.memBase = 0x06820000;
			break;
		case 1:
			m_mem->VRAM_B.memBase = 0x06000000 + (0x20000 * OFS);
			break;
		case 2:
			m_mem->VRAM_B.memBase = 0x06400000 + (0x20000 * (OFS & 0b1));
			break;
		}
		break;
	}
	case 0x04000242:
	{
		VRAMCNT_C = value;
		uint8_t MST = value & 0b111;
		uint8_t OFS = (value >> 3) & 0b11;
		m_mem->VRAM_C.ARM7 = false;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_C.memBase = 0x06840000;
			break;
		case 1:
			m_mem->VRAM_C.memBase = 0x06000000 + (0x20000 * OFS);
			break;
		case 2:
			m_mem->VRAM_C.memBase = 0x06000000 + (0x20000 * (OFS&0b1));
			m_mem->VRAM_C.ARM7 = true;
			break;
		case 4:
			m_mem->VRAM_C.memBase = 0x06200000;
			break;
		}
		break;
	}
	case 0x04000243:
	{
		VRAMCNT_D = value;
		uint8_t MST = value & 0b111;
		uint8_t OFS = (value >> 3) & 0b11;
		m_mem->VRAM_D.ARM7 = false;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_D.memBase = 0x06860000;
			break;
		case 1:
			m_mem->VRAM_D.memBase = 0x06000000 + (0x20000 * OFS);
			break;
		case 2:
			m_mem->VRAM_D.memBase = 0x06000000 + (0x20000 * (OFS & 0b1));
			m_mem->VRAM_D.ARM7 = true;
			break;
		case 4:
			m_mem->VRAM_D.memBase = 0x06600000;
			break;
		}
		break;
	}
	case 0x04000244:
	{
		VRAMCNT_E = value;
		uint8_t MST = value & 0b111;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_E.memBase = 0x06880000;
			break;
		case 1:
			m_mem->VRAM_E.memBase = 0x06000000;
			break;
		case 2:
			m_mem->VRAM_E.memBase = 0x06400000;
			break;
		}
		break;
	}
	case 0x04000245:
	{
		VRAMCNT_F = value;
		uint8_t MST = value & 0b111;
		uint8_t OFS = (value >> 3) & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_F.memBase = 0x06890000;
			break;
		case 1:
			m_mem->VRAM_F.memBase = 0x06000000 + (0x4000 * (OFS & 0b1)) + (0x10000 * ((OFS >> 1) & 0b1));
			break;
		case 2:
			m_mem->VRAM_F.memBase = 0x06400000 + (0x4000 * (OFS & 0b1)) + (0x10000 * ((OFS >> 1) & 0b1));
			break;
		}
		break;
	}
	case 0x04000246:
	{
		VRAMCNT_G = value;
		uint8_t MST = value & 0b111;
		uint8_t OFS = (value >> 3) & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_G.memBase = 0x06894000;
			break;
		case 1:
			m_mem->VRAM_G.memBase = 0x06000000 + (0x4000 * (OFS & 0b1)) + (0x10000 * ((OFS >> 1) & 0b1));
			break;
		case 2:
			m_mem->VRAM_G.memBase = 0x06400000 + (0x4000 * (OFS & 0b1)) + (0x10000 * ((OFS >> 1) & 0b1));
			break;
		}
		break;
	}
	case 0x04000248:
	{
		VRAMCNT_H = value;
		uint8_t MST = value & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_H.memBase = 0x06898000;
			break;
		case 1:
			m_mem->VRAM_H.memBase = 0x06200000;
			break;
		}
		break;
	}
	case 0x04000249:
	{
		VRAMCNT_I = value;
		uint8_t MST = value & 0b11;
		switch (MST)
		{
		case 0:
			m_mem->VRAM_I.memBase = 0x068A0000;
			break;
		case 1:
			m_mem->VRAM_I.memBase = 0x06208000;
			break;
		case 2:
			m_mem->VRAM_I.memBase = 0x06600000;
			break;
		}
		break;
	}
	case 0x04000000:
		DISPCNT &= 0xFFFFFF00; DISPCNT |= value;
		break;
	case 0x04000001:
		DISPCNT &= 0xFFFF00FF; DISPCNT |= (value << 8);
		break;
	case 0x04000002:
		DISPCNT &= 0xFF00FFFF; DISPCNT |= (value << 16);
		break;
	case 0x04000003:
		DISPCNT &= 0x00FFFFFF; DISPCNT |= (value << 24);
		break;
	case 0x04000004:
		DISPSTAT &= 0xFF07; DISPSTAT |= (value & 0xF8);
		break;
	case 0x04000005:
		DISPSTAT &= 0x00FF; DISPSTAT |= (value << 8);
		break;
	case 0x04000008:
		m_engineARegisters.BG0CNT &= 0xFF00; m_engineARegisters.BG0CNT |= value;
		break;
	case 0x04000009:
		m_engineARegisters.BG0CNT &= 0x00FF; m_engineARegisters.BG0CNT |= (value << 8);
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO write! addr={:#x} val={:#x}", address, value));
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
		break;
	}
}

uint8_t* PPU::mapAddressToVRAM(uint32_t address)
{
	if (address >= m_mem->VRAM_A.memBase && address < (m_mem->VRAM_A.memBase + m_mem->VRAM_A.size))
	{
		address &= (m_mem->VRAM_A.size - 1);
		return m_mem->VRAM_A.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_B.memBase && address < (m_mem->VRAM_B.memBase + m_mem->VRAM_B.size))
	{
		address &= (m_mem->VRAM_B.size - 1);
		return m_mem->VRAM_B.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_C.memBase && address < (m_mem->VRAM_C.memBase + m_mem->VRAM_C.size) && !m_mem->VRAM_C.ARM7)
	{
		address &= (m_mem->VRAM_C.size - 1);
		return m_mem->VRAM_C.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_D.memBase && address < (m_mem->VRAM_D.memBase + m_mem->VRAM_D.size) && !m_mem->VRAM_D.ARM7)
	{
		address &= (m_mem->VRAM_D.size - 1);
		return m_mem->VRAM_D.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_E.memBase && address < (m_mem->VRAM_E.memBase + m_mem->VRAM_E.size))
	{
		address &= (m_mem->VRAM_E.size - 1);
		return m_mem->VRAM_E.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_F.memBase && address < (m_mem->VRAM_F.memBase + m_mem->VRAM_F.size))
	{
		address &= (m_mem->VRAM_F.size - 1);
		return m_mem->VRAM_F.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_G.memBase && address < (m_mem->VRAM_G.memBase + m_mem->VRAM_G.size))
	{
		address &= (m_mem->VRAM_G.size - 1);
		return m_mem->VRAM_G.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_H.memBase && address < (m_mem->VRAM_H.memBase + m_mem->VRAM_H.size))
	{
		address &= (m_mem->VRAM_H.size - 1);
		return m_mem->VRAM_H.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_I.memBase && address < (m_mem->VRAM_I.memBase + m_mem->VRAM_I.size))
	{
		address &= (m_mem->VRAM_I.size - 1);
		return m_mem->VRAM_I.VRAMBase + address;
	}
	Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Couldn't map address {:#x} into a VRAM bank.. ", address));
	return m_mem->VRAM;
}

uint8_t* PPU::NDS7_mapAddressToVRAM(uint32_t address)
{
	if (address >= m_mem->VRAM_C.memBase && address < (m_mem->VRAM_C.memBase + m_mem->VRAM_C.size) && m_mem->VRAM_C.ARM7)
	{
		address &= (m_mem->VRAM_C.size - 1);
		return m_mem->VRAM_C.VRAMBase + address;
	}
	if (address >= m_mem->VRAM_D.memBase && address < (m_mem->VRAM_D.memBase + m_mem->VRAM_D.size) && m_mem->VRAM_D.ARM7)
	{
		address &= (m_mem->VRAM_D.size - 1);
		return m_mem->VRAM_D.VRAMBase + address;
	}
	Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Couldn't map address {:#x} into a VRAM bank.. ", address));
	return m_mem->VRAM;
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

void PPU::setVCounterFlag(bool value)
{
	DISPSTAT &= ~0b100;
	DISPSTAT |= (value << 2);
	NDS7_DISPSTAT &= ~0b100;
	NDS7_DISPSTAT |= (value << 2);
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