#pragma once

#include<iostream>

struct VRAMBank
{
	bool ARM7;
	uint8_t* VRAMBase;	//where the bank resides in our big vram buffer
	uint32_t size;
	uint32_t memBase;	//base address in memory it's mapped to
};

struct NDSMem
{
	uint8_t NDS7_BIOS[16384];
	uint8_t NDS9_BIOS[32768];	//<--in reality, only like 3-4k of this is used

	uint8_t RAM[4194304];
	uint8_t WRAM[2][16384];
	uint8_t VRAM[671744];			//<---this is bad, just to get lcdc mode working..
	uint8_t PAL[2048];
	uint8_t OAM[2048];

	uint8_t ARM7_WRAM[65536];

	uint8_t* NDS7_sharedWRAMPtrs[2];		//dealing with shared wram banking
	uint8_t* NDS9_sharedWRAMPtrs[2];

	VRAMBank VRAM_A = {}, VRAM_B = {}, VRAM_C = {}, VRAM_D = {}, VRAM_E = {}, VRAM_F = {}, VRAM_G = {}, VRAM_H = {}, VRAM_I = {};
};