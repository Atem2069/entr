#include"PPU.h"

void PPU::rebuildPageTables()
{
	/*
	* 	uint8_t* LCDCPageTable[0x29];
	uint8_t* ABGPageTable[32];
	uint8_t* AObjPageTable[16];
	uint8_t* BBGPageTable[8];
	uint8_t* BObjPageTable[8];
	uint8_t* ARM7VRAMPageTable[2];	//2 128KB regions
	*/

	/*
	* A=00000-1FFFF
	* B=20000-3FFFF
	* C=40000-5FFFF
	* D=60000-7FFFF
	* E=80000-8FFFF
	* F=90000-93FFF
	* G=94000-97FFF
	* H=98000-9FFFF
	* I=A0000-A3FFF
	*/
	uint32_t BOffs = 0x20000;
	uint32_t COffs = 0x40000;
	uint32_t DOffs = 0x60000;
	uint32_t EOffs = 0x80000;
	uint32_t FOffs = 0x90000;
	uint32_t GOffs = 0x94000;
	uint32_t HOffs = 0x98000;
	uint32_t IOffs = 0xA0000;

	m_mem->VRAM_C_ARM7 = false;
	m_mem->VRAM_D_ARM7 = false;

	//flush page tables
	memset(m_mem->LCDCPageTable, 0, 0x29 * sizeof(uint8_t*));
	memset(m_mem->ABGPageTable, 0, 32 * sizeof(uint8_t*));
	memset(m_mem->AObjPageTable, 0, 16 * sizeof(uint8_t*));
	memset(m_mem->BBGPageTable, 0, 8 * sizeof(uint8_t*));
	memset(m_mem->BObjPageTable, 0, 8 * sizeof(uint8_t*));
	memset(m_mem->ARM7VRAMPageTable, 0, 2 * sizeof(uint8_t*));

	//start to refill by checking each mst/ofs
	uint8_t MST = VRAMCNT_A & 0b11;
	uint8_t OFS = (VRAMCNT_A >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		for (int i = 0; i < 8; i++)
			m_mem->LCDCPageTable[i] = m_mem->VRAM + (16384 * i);
		break;
	case 1:
		for (int i = 0; i < 8; i++)
			m_mem->ABGPageTable[(OFS << 3) + i] = m_mem->VRAM + (16384 * i);
		break;
	case 2:
		for (int i = 0; i < 8; i++)
			m_mem->AObjPageTable[((OFS & 0b1) << 3) + i] = m_mem->VRAM + (16384 * i);
		break;
	case 3:
		m_mem->TexturePageTable[OFS] = m_mem->VRAM;
		break;
	}

	MST = VRAMCNT_B & 0b11;
	OFS = (VRAMCNT_B >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		for (int i = 0; i < 8; i++)
			m_mem->LCDCPageTable[i + 0x8] = m_mem->VRAM + BOffs + (16384 * i);
		break;
	case 1:
		for (int i = 0; i < 8; i++)
			m_mem->ABGPageTable[(OFS << 3) + i] = m_mem->VRAM + BOffs + (16384 * i);
		break;
	case 2:
		for (int i = 0; i < 8; i++)
			m_mem->AObjPageTable[((OFS & 0b1) << 3) + i] = m_mem->VRAM + BOffs + (16384 * i);
		break;
	case 3:
		m_mem->TexturePageTable[OFS] = m_mem->VRAM + BOffs;
		break;
	}

	MST = VRAMCNT_C & 0b111;
	OFS = (VRAMCNT_C >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		for (int i = 0; i < 8; i++)
			m_mem->LCDCPageTable[i + 0x10] = m_mem->VRAM + COffs + (16384 * i);
		break;
	case 1:
		for (int i = 0; i < 8; i++)
			m_mem->ABGPageTable[(OFS << 3) + i] = m_mem->VRAM + COffs + (16384 * i);
		break;
	case 2:
		m_mem->VRAM_C_ARM7 = true;
		m_mem->ARM7VRAMPageTable[(OFS & 0b1)] = m_mem->VRAM + COffs;
		break;
	case 3:
		m_mem->TexturePageTable[OFS] = m_mem->VRAM + COffs;
		break;
	case 4:
		for (int i = 0; i < 8; i++)
			m_mem->BBGPageTable[i] = m_mem->VRAM + COffs + (16384 * i);
	}

	MST = VRAMCNT_D & 0b111;
	OFS = (VRAMCNT_D >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		for (int i = 0; i < 8; i++)
			m_mem->LCDCPageTable[i + 0x18] = m_mem->VRAM + DOffs + (16384 * i);
		break;
	case 1:
		for (int i = 0; i < 8; i++)
			m_mem->ABGPageTable[(OFS << 3) + i] = m_mem->VRAM + DOffs + (16384 * i);
		break;
	case 2:
		m_mem->VRAM_D_ARM7 = true;
		m_mem->ARM7VRAMPageTable[(OFS & 0b1)] = m_mem->VRAM + DOffs;
		break;
	case 3:
		m_mem->TexturePageTable[OFS] = m_mem->VRAM + DOffs;
		break;
	case 4:
		for (int i = 0; i < 8; i++)
			m_mem->BObjPageTable[i] = m_mem->VRAM + DOffs + (16384 * i);
		break;
	}

	MST = VRAMCNT_E & 0b111;
	OFS = (VRAMCNT_E >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		for (int i = 0; i < 4; i++)
			m_mem->LCDCPageTable[i + 0x20] = m_mem->VRAM + EOffs + (16384 * i);
		break;
	case 1:
		for (int i = 0; i < 4; i++)
			m_mem->ABGPageTable[i] = m_mem->VRAM + EOffs + (16384 * i);
		break;
	case 2:
		for (int i = 0; i < 4; i++)
			m_mem->AObjPageTable[i] = m_mem->VRAM + EOffs + (16384 * i);
		break;
	case 3:
		for (int i = 0; i < 4; i++)
			m_mem->TexPalettePageTable[i] = m_mem->VRAM + EOffs + (16384 * i);
		break;
	case 4:
		m_mem->ABGExtPalPageTable[0] = m_mem->VRAM + EOffs;
		m_mem->ABGExtPalPageTable[1] = m_mem->VRAM + EOffs + 16384;
		break;
	}

	MST = VRAMCNT_F & 0b111;
	OFS = (VRAMCNT_F >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		m_mem->LCDCPageTable[0x24] = m_mem->VRAM + FOffs;
		break;
	case 1:
		m_mem->ABGPageTable[(OFS & 1) | (OFS & 2) << 1] = m_mem->VRAM + FOffs;		//mirrored
		m_mem->ABGPageTable[(OFS & 1) | (OFS & 2) << 1 | 2] = m_mem->VRAM + FOffs;
		break;
	case 2:
		m_mem->AObjPageTable[(OFS & 1) | (OFS & 2) << 1] = m_mem->VRAM + FOffs;
		m_mem->AObjPageTable[(OFS & 1) | (OFS & 2) << 1 | 2] = m_mem->VRAM + FOffs;	//also mirrored
		break;
	case 3:
		m_mem->TexPalettePageTable[(OFS & 1) + ((OFS & 2) << 1)] = m_mem->VRAM + FOffs;
		break;
	case 4:
		m_mem->ABGExtPalPageTable[(OFS & 0b1)] = m_mem->VRAM + FOffs;
		break;
	case 5:
		m_mem->AObjExtPalPageTable = m_mem->VRAM + FOffs;
		break;
	}

	MST = VRAMCNT_G & 0b111;
	OFS = (VRAMCNT_G >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		m_mem->LCDCPageTable[0x25] = m_mem->VRAM + GOffs;
		break;
	case 1:
		m_mem->ABGPageTable[(OFS & 1) | (OFS & 2) << 1] = m_mem->VRAM + GOffs;		//mirrored
		m_mem->ABGPageTable[(OFS & 1) | (OFS & 2) << 1 | 2] = m_mem->VRAM + GOffs;
		break;
	case 2:
		m_mem->AObjPageTable[(OFS & 1) | (OFS & 2) << 1] = m_mem->VRAM + GOffs;
		m_mem->AObjPageTable[(OFS & 1) | (OFS & 2) << 1 | 2] = m_mem->VRAM + GOffs;	//also mirrored
		break;
	case 3:
		m_mem->TexPalettePageTable[(OFS & 1) + ((OFS & 2) << 1)] = m_mem->VRAM + GOffs;
		break;
	case 4:
		m_mem->ABGExtPalPageTable[(OFS & 0b1)] = m_mem->VRAM + GOffs;
		break;
	case 5:
		m_mem->AObjExtPalPageTable = m_mem->VRAM + GOffs;
		break;
	}

	MST = VRAMCNT_H & 0b11;
	OFS = (VRAMCNT_H >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		m_mem->LCDCPageTable[0x26] = m_mem->VRAM + HOffs;
		m_mem->LCDCPageTable[0x27] = m_mem->VRAM + HOffs + 16384;
		break;
	case 1:
		m_mem->BBGPageTable[0] = m_mem->VRAM + HOffs;
		m_mem->BBGPageTable[1] = m_mem->VRAM + HOffs + 16384;
		m_mem->BBGPageTable[4] = m_mem->VRAM + HOffs;
		m_mem->BBGPageTable[5] = m_mem->VRAM + HOffs + 16384;
		break;
	case 2:
		m_mem->BBGExtPalPageTable[0] = m_mem->VRAM + HOffs;
		m_mem->BBGExtPalPageTable[1] = m_mem->VRAM + HOffs + 16384;
		break;
	}

	MST = VRAMCNT_I & 0b11;
	OFS = (VRAMCNT_I >> 3) & 0b11;
	switch (MST)
	{
	case 0:
		m_mem->LCDCPageTable[0x28] = m_mem->VRAM + IOffs;
		break;
	case 1:
		m_mem->BBGPageTable[2] = m_mem->VRAM + IOffs;	//todo: check mirroring for this
		break;
	case 2:
		for (int i = 0; i < 8; i++)
			m_mem->BObjPageTable[i] = m_mem->VRAM + IOffs;	//mirrored across 8 16kb pages
		break;
	case 3:
		m_mem->BObjExtPalPageTable = m_mem->VRAM + IOffs;
	}
}

uint8_t* PPU::mapLCDCAddress(uint32_t address)
{
	address &= 0xFFFFF;
	uint8_t page = (address >> 14);
	uint32_t offset = address & 0x3FFF;
	//return m_mem->LCDCPageTable[page] + offset;
	uint8_t* pagePtr = m_mem->LCDCPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;
}

uint8_t* PPU::mapABgAddress(uint32_t address)
{
	address &= 0x7FFFF;
	uint8_t page = (address >> 14);
	uint32_t offset = address & 0x3FFF;
	uint8_t* pagePtr = m_mem->ABGPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;
}

uint8_t* PPU::mapAObjAddress(uint32_t address)
{
	address &= 0x3FFFF;
	uint8_t page = (address >> 14);
	uint32_t offset = address & 0x3FFF;
	uint8_t* pagePtr = m_mem->AObjPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;

}

uint8_t* PPU::mapBBgAddress(uint32_t address)
{
	address &= 0x1FFFF;
	uint8_t page = (address >> 14);
	uint32_t offset = address & 0x3FFF;
	uint8_t* pagePtr = m_mem->BBGPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;
}

uint8_t* PPU::mapBObjAddress(uint32_t address)
{
	address &= 0x1FFFF;
	uint8_t page = (address >> 14);
	uint32_t offset = address & 0x3FFF;
	uint8_t* pagePtr = m_mem->BObjPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;
}

uint8_t* PPU::mapARM7Address(uint32_t address)
{
	address &= 0x3ffff;
	uint8_t page = (address >> 17);
	uint32_t offset = address & 0x1FFFF;
	uint8_t* pagePtr = m_mem->ARM7VRAMPageTable[page];
	if (!pagePtr)
		return m_mem->VRAM;
	return pagePtr + offset;
}