#pragma once

#include<iostream>

struct NDSMem
{
	uint8_t NDS7_BIOS[16384];
	uint8_t NDS9_BIOS[32768];	//<--in reality, only like 3-4k of this is used

	uint8_t RAM[4194304];
	uint8_t WRAM[2][16384];
	uint8_t VRAM[671744];		
	uint8_t PAL[2048];
	uint8_t OAM[2048];

	uint8_t ARM7_WRAM[65536];

	uint8_t* NDS7_sharedWRAMPtrs[2];		//dealing with shared wram banking
	uint8_t* NDS9_sharedWRAMPtrs[2];

	uint8_t* LCDCPageTable[0x29];
	uint8_t* ABGPageTable[32];
	uint8_t* AObjPageTable[16];
	uint8_t* BBGPageTable[8];
	uint8_t* BObjPageTable[8];
	uint8_t* ARM7VRAMPageTable[2];	//2 128KB regions

	//extended palettes (not mapped to CPU-visible address space)
	uint8_t* ABGExtPalPageTable[2];
	uint8_t* AObjExtPalPageTable;	//single 8KB region
	uint8_t* BBGExtPalPageTable[2];
	uint8_t* BObjExtPalPageTable;	//same as engine a, 1 8KB region

	bool VRAM_C_ARM7 = false;
	bool VRAM_D_ARM7 = false;
};