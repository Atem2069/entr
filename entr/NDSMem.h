#pragma once

#include<iostream>

struct NDSMem
{
	uint8_t RAM[4194304];
	uint8_t WRAM[2][16384];
	uint8_t VRAM[671744];			//<---this is bad, just to get lcdc mode working..
	uint8_t PAL[2048];
	uint8_t OAM[2048];

	uint8_t ARM7_WRAM[65536];
};