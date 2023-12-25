#include"Bus.h"

Bus::Bus()
{

}

void Bus::init(std::vector<uint8_t> NDS7_BIOS, std::vector<uint8_t> NDS9_BIOS, Cartridge* cartridge, Scheduler* scheduler, InterruptManager* interruptManager, PPU* ppu, GPU* gpu, Input* input)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;
	m_ppu = ppu;
	m_gpu = gpu;
	m_input = input;
	m_cartridge = cartridge;

	m_mem = new NDSMem;
	m_ipc.init(m_interruptManager);
	m_NDS9Timer.init(true, m_interruptManager, m_scheduler);
	m_NDS7Timer.init(false, m_interruptManager, m_scheduler);
	m_spi.init(m_interruptManager);

	m_ppu->registerMemory(m_mem);
	m_ppu->registerDMACallbacks((callbackFn)&Bus::NDS9_HBlankDMACallback, (callbackFn)&Bus::NDS9_VBlankDMACallback, (void*)this);
	m_ppu->registerGPUCallback((callbackFn)&GPU::VBlankEventHandler, (void*)gpu);
	m_gpu->registerMemory(m_mem);
	m_gpu->registerCallbacks((callbackFn)&Bus::NDS9_GXFIFODMACallback, (void*)this);
	m_cartridge->registerDMACallbacks((callbackFn)&Bus::NDS7_CartridgeDMACallback, (callbackFn)&Bus::NDS9_CartridgeDMACallback, (void*)this);

	//i don't know if this initial state is accurate, but oh well..
	m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
	m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
	m_mem->NDS7_sharedWRAMPtrs[0] = nullptr;
	m_mem->NDS7_sharedWRAMPtrs[1] = nullptr;

	memcpy(m_mem->NDS7_BIOS, &NDS7_BIOS[0], NDS7_BIOS.size());
	memcpy(m_mem->NDS9_BIOS, &NDS9_BIOS[0], NDS9_BIOS.size());
	m_ARM9PageTable = new uint8_t * [0x40000];
	m_ARM7PageTable = new uint8_t * [0x40000];
	memset(m_ARM9PageTable, 0, 0x40000);
	memset(m_ARM7PageTable, 0, 0x40000);
	//build beginning of page tables
	for (uint32_t i = 0; i < 0x01000000; i += 16384)
	{
		uint32_t page = addressToPage(i + 0x02000000);
		m_ARM9PageTable[page] = m_mem->RAM + (i & 0x3FFFFF);
		m_ARM7PageTable[page] = m_mem->RAM + (i & 0x3FFFFF);
	}
	for (uint32_t i = 0; i < addressToPage(0x00800000); i += 4)
	{
		m_ARM7PageTable[addressToPage(0x03800000) + i] = m_mem->ARM7_WRAM;
		m_ARM7PageTable[addressToPage(0x03800000) + i + 1] = m_mem->ARM7_WRAM + 16384;
		m_ARM7PageTable[addressToPage(0x03800000) + i + 2] = m_mem->ARM7_WRAM + 32768;
		m_ARM7PageTable[addressToPage(0x03800000) + i + 3] = m_mem->ARM7_WRAM + 49152;
	}

	m_ARM9PageTable[addressToPage(0xFFFF0000)] = m_mem->NDS9_BIOS;		//might have to keep these on slowmem, because they're not writeable.
	//m_ARM7PageTable[0] = m_mem->NDS7_BIOS;
	
}

Bus::~Bus()
{
	delete m_mem;
	delete[] m_ARM9PageTable;
	delete[] m_ARM7PageTable;
}

void Bus::mapVRAMPages()
{
	for (uint32_t i = 0; i < addressToPage(0x00200000); i++)
	{
		m_ARM9PageTable[addressToPage(0x06000000) + i] = m_mem->ABGPageTable[i & 31];
		m_ARM9PageTable[addressToPage(0x06200000) + i] = m_mem->BBGPageTable[i & 7];
		m_ARM9PageTable[addressToPage(0x06400000) + i] = m_mem->AObjPageTable[i & 15];
		m_ARM9PageTable[addressToPage(0x06600000) + i] = m_mem->BObjPageTable[i & 7];
	}
	for (uint32_t i = 0; i < addressToPage(0x00800000); i++)
	{
		m_ARM9PageTable[addressToPage(0x06800000) + i] = m_mem->LCDCPageTable[i % 41];		//might not be right.
	}

	//somewhat messy.
	for (uint32_t i = 0; i < 64; i++)
	{
		uint32_t baseAddr = 0x06000000 + (i * 262144);
		uint32_t upperBase = baseAddr + 131072;
		for (uint32_t j = 0; j < 8; j++)
		{
			m_ARM7PageTable[addressToPage(baseAddr)+j] = nullptr;
			m_ARM7PageTable[addressToPage(upperBase)+j] = nullptr;
			if(m_mem->ARM7VRAMPageTable[0])
				m_ARM7PageTable[addressToPage(baseAddr) + j] =  m_mem->ARM7VRAMPageTable[0] + (j * 16384);
			if(m_mem->ARM7VRAMPageTable[1])
				m_ARM7PageTable[addressToPage(upperBase) + j] = m_mem->ARM7VRAMPageTable[1] + (j * 16384);
		}
	}
}


uint8_t Bus::NDS7_read8(uint32_t address)
{
	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
		return ptr[addressToOffset(address)];

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address <= 0x3FFF)
			return m_mem->NDS7_BIOS[address];
		return 0;
	case 4:
		return NDS7_readIO8(address);
	case 8: case 9:
		return ((address >> 1) & 0xFFFF) >> ((address & 0b1)<<3);
	}

	return 0;
}

void Bus::NDS7_write8(uint32_t address, uint8_t value)
{
	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
	{
		ptr[addressToOffset(address)] = value;
		return;
	}
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS7_writeIO8(address,value);
		break;
	}
}

uint16_t Bus::NDS7_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;

	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
		return getValue16(ptr, addressToOffset(address), 0xFFFFFFFF);

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address <= 0x3FFF)
			return getValue16(m_mem->NDS7_BIOS, address, 0xFFFFFFFF);
		return 0;
	case 4:
		return NDS7_readIO16(address);
	case 8: case 9:
		return (address >> 1) & 0xFFFF;
	}

	return 0;
}

void Bus::NDS7_write16(uint32_t address, uint16_t value)
{
	address &= 0xFFFFFFFE;

	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
	{
		setValue16(ptr, addressToOffset(address), 0xFFFFFFFF, value);
		return;
	}

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS7_writeIO16(address, value);
		break;
	}
}

uint32_t Bus::NDS7_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;

	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
		return getValue32(ptr, addressToOffset(address), 0xFFFFFFFF);

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address <= 0x3FFF)
			return getValue32(m_mem->NDS7_BIOS, address, 0xFFFFFFFF);
		return 0;
	case 4:
		return NDS7_readIO32(address);
	case 8: case 9:
		return ((address >> 1) & 0xFFFF) | ((((address + 2) >> 1) & 0xFFFF) << 16);
	}

	return 0;
}

void Bus::NDS7_write32(uint32_t address, uint32_t value)
{
	address &= 0xFFFFFFFC;

	uint8_t* ptr = m_ARM7PageTable[addressToPage(address)];
	if (ptr)
	{
		setValue32(ptr, addressToOffset(address), 0xFFFFFFFF,value);
		return;
	}

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS7_writeIO32(address, value);
		break;
	}
}

uint8_t Bus::NDS9_read8(uint32_t address)
{
	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
		return ptr[addressToOffset(address)];
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		return NDS9_readIO8(address);
	case 5:
		return m_mem->PAL[address & 0x7FF];
	case 7:
		return m_mem->OAM[address & 0x7FF];
	case 8: case 9: case 0xA:
		return 0xFF;
	default:
		return 0;
	}

	return 0;
}

void Bus::NDS9_write8(uint32_t address, uint8_t value)
{
	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
	{
		ptr[addressToOffset(address)] = value;
		return;
	}
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS9_writeIO8(address,value);
		break;
	case 5:
		m_mem->PAL[address & 0x7FF] = value;
		break;
	case 7:
		m_mem->OAM[address & 0x7FF] = value;
		break;
	case 8: case 9: case 0xA:
		break;
	default:
		break;
	}

}

uint16_t Bus::NDS9_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;

	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
		return getValue16(ptr, addressToOffset(address), 0xFFFFFFFF);

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		return NDS9_readIO16(address);
	case 5:
		return getValue16(m_mem->PAL,address & 0x7FF,0x7FF);
	case 7:
		return getValue16(m_mem->OAM,address & 0x7FF,0x7FF);
	case 8: case 9: case 0xA:
		return 0xFFFF;
	default:
		return 0;
	}

	return 0;
}

void Bus::NDS9_write16(uint32_t address, uint16_t value)
{
	address &= 0xFFFFFFFE;

	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
	{
		setValue16(ptr, addressToOffset(address), 0xFFFFFFFF, value);
		return;
	}

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS9_writeIO16(address, value);
		break;
	case 5:
		setValue16(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 7:
		setValue16(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		break;
	default:
		break;
	}

}

uint32_t Bus::NDS9_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;

	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
		return getValue32(ptr, addressToOffset(address), 0xFFFFFFFF);

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		return NDS9_readIO32(address);
	case 5:
		return getValue32(m_mem->PAL, address & 0x7FF, 0x7FF);
	case 7:
		return getValue32(m_mem->OAM, address & 0x7FF, 0x7FF);
	case 8: case 9: case 0xA:
		return 0xFFFFFFFF;
	default:
		return 0;
	}

	return 0;
}

void Bus::NDS9_write32(uint32_t address, uint32_t value)
{
	address &= 0xFFFFFFFC;

	uint8_t* ptr = m_ARM9PageTable[addressToPage(address)];
	if (ptr)
	{
		setValue32(ptr, addressToOffset(address), 0xFFFFFFFF, value);
		return;
	}

	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 4:
		NDS9_writeIO32(address, value);
		break;
	case 5:
		setValue32(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 7:
		setValue32(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		break;
	default:
		break;
	}
}

//Handle NDS7 IO
uint8_t Bus::NDS7_readIO8(uint32_t address)
{
	switch (address)
	{
	case 0x04000004: case 0x04000005: case 0x04000006: case 0x04000007:
		return m_ppu->NDS7_readIO(address);
	case 0x040000B0: case 0x040000B1: case 0x040000B2: case 0x040000B3: case 0x040000B4: case 0x040000B5: case 0x040000B6: case 0x040000B7:
	case 0x040000B8: case 0x040000B9: case 0x040000BA: case 0x040000BB: case 0x040000BC: case 0x040000BD: case 0x040000BE: case 0x040000BF:
	case 0x040000C0: case 0x040000C1: case 0x040000C2: case 0x040000C3: case 0x040000C4: case 0x040000C5: case 0x040000C6: case 0x040000C7:
	case 0x040000C8: case 0x040000C9: case 0x040000CA: case 0x040000CB: case 0x040000CC: case 0x040000CD: case 0x040000CE: case 0x040000CF:
	case 0x040000D0: case 0x040000D1: case 0x040000D2: case 0x040000D3: case 0x040000D4: case 0x040000D5: case 0x040000D6: case 0x040000D7:
	case 0x040000D8: case 0x040000D9: case 0x040000DA: case 0x040000DB: case 0x040000DC: case 0x040000DD: case 0x040000DE: case 0x040000DF:
		return NDS7_readDMAReg(address);
	case 0x04000100: case 0x04000101: case 0x04000102: case 0x04000103: case 0x04000104: case 0x04000105: case 0x04000106: case 0x04000107:
	case 0x04000108: case 0x04000109: case 0x0400010A: case 0x0400010B: case 0x0400010C: case 0x0400010D: case 0x0400010E: case 0x0400010F:
		return m_NDS7Timer.readIO(address);
	case 0x04000130: case 0x04000131: case 0x04000132: case 0x04000133: case 0x04000136: case 0x04000137:
		return m_input->readIORegister(address);
	case 0x04000138: case 0x04000139:	//4000139 to shut up console
		return m_rtc.read(address);
	case 0x04000180: case 0x04000181: case 0x04000182: case 0x04000183: case 0x04000184: case 0x04000185:
		return m_ipc.NDS7_read8(address);
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		return m_interruptManager->NDS7_readIO(address);
	case 0x040001A0: case 0x040001A1: case 0x040001A2: case 0x040001A3: case 0x040001A4: case 0x040001A5: case 0x040001A6: case 0x040001A7: case 0x040001A8: case 0x040001A9:
	case 0x040001AA: case 0x040001AB: case 0x040001AC: case 0x040001AD: case 0x040001AE: case 0x040001AF: case 0x04100010: case 0x04100011: case 0x04100012: case 0x04100013:
		return m_cartridge->NDS7_read(address);
	case 0x040001C0: case 0x040001C1: case 0x040001C2: case 0x040001C3:
		return m_spi.read(address);
	case 0x04000204:
		return (EXMEMCNT & 0xFF);
	case 0x04000205:
		return ((EXMEMCNT >> 8) & 0xFF);
	case 0x04000240:
		return (m_mem->VRAM_C_ARM7) | (m_mem->VRAM_D_ARM7 << 1);
	case 0x04000241:
		return WRAMCNT;
	case 0x04000300:
		return NDS7_POSTFLG;
	case 0x04000504:
		return hack_soundBias & 0xFF;
	case 0x04000505:
		return ((hack_soundBias >> 8) & 0b11);
	//	return 1;
	}
	return 0;
}

void Bus::NDS7_writeIO8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000004: case 0x04000005: case 0x04000006: case 0x04000007:
		m_ppu->NDS7_writeIO(address, value);
		break;
	case 0x040000B0: case 0x040000B1: case 0x040000B2: case 0x040000B3: case 0x040000B4: case 0x040000B5: case 0x040000B6: case 0x040000B7:
	case 0x040000B8: case 0x040000B9: case 0x040000BA: case 0x040000BB: case 0x040000BC: case 0x040000BD: case 0x040000BE: case 0x040000BF:
	case 0x040000C0: case 0x040000C1: case 0x040000C2: case 0x040000C3: case 0x040000C4: case 0x040000C5: case 0x040000C6: case 0x040000C7:
	case 0x040000C8: case 0x040000C9: case 0x040000CA: case 0x040000CB: case 0x040000CC: case 0x040000CD: case 0x040000CE: case 0x040000CF:
	case 0x040000D0: case 0x040000D1: case 0x040000D2: case 0x040000D3: case 0x040000D4: case 0x040000D5: case 0x040000D6: case 0x040000D7:
	case 0x040000D8: case 0x040000D9: case 0x040000DA: case 0x040000DB: case 0x040000DC: case 0x040000DD: case 0x040000DE: case 0x040000DF:
		NDS7_writeDMAReg(address, value);
		break;
	case 0x04000100: case 0x04000101: case 0x04000102: case 0x04000103: case 0x04000104: case 0x04000105: case 0x04000106: case 0x04000107:
	case 0x04000108: case 0x04000109: case 0x0400010A: case 0x0400010B: case 0x0400010C: case 0x0400010D: case 0x0400010E: case 0x0400010F:
		m_NDS7Timer.writeIO(address, value);
		break;
	case 0x04000132: case 0x04000133:
		m_input->writeIORegister(address, value);
		break;
	case 0x04000138: case 0x04000139:	//4000139 to shut up console
		m_rtc.write(address, value);
		break;
	case 0x04000180: case 0x04000181: case 0x04000182: case 0x04000183: case 0x04000184: case 0x04000185:
		m_ipc.NDS7_write8(address, value);
		break;
	case 0x040001A0: case 0x040001A1: case 0x040001A2: case 0x040001A3: case 0x040001A4: case 0x040001A5: case 0x040001A6: case 0x040001A7: case 0x040001A8: case 0x040001A9:
	case 0x040001AA: case 0x040001AB: case 0x040001AC: case 0x040001AD: case 0x040001AE: case 0x040001AF: case 0x04100010: case 0x04100011: case 0x04100012: case 0x04100013:
		m_cartridge->NDS7_write(address, value);
		break;
	case 0x040001C0: case 0x040001C1: case 0x040001C2: case 0x040001C3:
		m_spi.write(address, value);
		break;
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		m_interruptManager->NDS7_writeIO(address,value);
		break;
	case 0x04000204:
		EXMEMCNT &= 0xFF80; EXMEMCNT |= value&0x7F;
		break;
	case 0x04000300:
		NDS7_POSTFLG = 1;
		break;
	case 0x04000301:
		ARM7_halt = ((value >> 6) & 0b11) == 2;
		break;
	case 0x04000504:
		hack_soundBias &= 0xFF00; hack_soundBias |= value;
		break;
	case 0x04000505:
		hack_soundBias &= 0x00FF; hack_soundBias |= ((value & 0b11) << 8);
		break;
	}
}

uint16_t Bus::NDS7_readIO16(uint32_t address)
{
	uint8_t lower = NDS7_readIO8(address);
	uint8_t upper = NDS7_readIO8(address + 1);
	return (upper << 8) | lower;
}

void Bus::NDS7_writeIO16(uint32_t address, uint16_t value)
{
	NDS7_writeIO8(address, value & 0xFF);
	NDS7_writeIO8(address + 1, (value >> 8) & 0xFF);
}

uint32_t Bus::NDS7_readIO32(uint32_t address)
{
	switch (address)
	{
	case 0x04100000:
		return m_ipc.NDS7_read32(address);
	case 0x04100010:
		return m_cartridge->NDS7_readGamecard();
	}
	uint16_t lower = NDS7_readIO16(address);
	uint16_t upper = NDS7_readIO16(address + 2);
	return (upper << 16) | lower;
}

void Bus::NDS7_writeIO32(uint32_t address, uint32_t value)
{
	//special case: write ipcfifo
	if (address == 0x04000188)
	{
		m_ipc.NDS7_write32(address, value);
		return;
	}

	NDS7_writeIO16(address, value & 0xFFFF);
	NDS7_writeIO16(address + 2, ((value >> 16) & 0xFFFF));
}

//Handle NDS9 IO
uint8_t Bus::NDS9_readIO8(uint32_t address)
{
	//this is horrible, need to move into switch statement.
	if (address==0x04000304 || address==0x04000305 || address >= 0x04000000 && address <= 0x04000058 || (address >= 0x04000240 && address <= 0x04000249 && address != 0x04000247) || (address >= 0x04001000 && address <= 0x04001058))
		return m_ppu->readIO(address);
	if (address==0x04000060 || address==0x04000061 || (address >= 0x04000320 && address <= 0x040006a3))
		return m_gpu->read(address);
	switch (address)
	{
	case 0x040000B0: case 0x040000B1: case 0x040000B2: case 0x040000B3: case 0x040000B4: case 0x040000B5: case 0x040000B6: case 0x040000B7:
	case 0x040000B8: case 0x040000B9: case 0x040000BA: case 0x040000BB: case 0x040000BC: case 0x040000BD: case 0x040000BE: case 0x040000BF:
	case 0x040000C0: case 0x040000C1: case 0x040000C2: case 0x040000C3: case 0x040000C4: case 0x040000C5: case 0x040000C6: case 0x040000C7:
	case 0x040000C8: case 0x040000C9: case 0x040000CA: case 0x040000CB: case 0x040000CC: case 0x040000CD: case 0x040000CE: case 0x040000CF:
	case 0x040000D0: case 0x040000D1: case 0x040000D2: case 0x040000D3: case 0x040000D4: case 0x040000D5: case 0x040000D6: case 0x040000D7:
	case 0x040000D8: case 0x040000D9: case 0x040000DA: case 0x040000DB: case 0x040000DC: case 0x040000DD: case 0x040000DE: case 0x040000DF:
	case 0x040000E0: case 0x040000E1: case 0x040000E2: case 0x040000E3: case 0x040000E4: case 0x040000E5: case 0x040000E6: case 0x040000E7:
	case 0x040000E8: case 0x040000E9: case 0x040000EA: case 0x040000EB: case 0x040000EC: case 0x040000ED: case 0x040000EE: case 0x040000EF:
		return NDS9_readDMAReg(address);
	case 0x04000100: case 0x04000101: case 0x04000102: case 0x04000103: case 0x04000104: case 0x04000105: case 0x04000106: case 0x04000107:
	case 0x04000108: case 0x04000109: case 0x0400010A: case 0x0400010B: case 0x0400010C: case 0x0400010D: case 0x0400010E: case 0x0400010F:
		return m_NDS9Timer.readIO(address);
	case 0x04000130: case 0x04000131: case 0x04000132: case 0x04000133:
		return m_input->readIORegister(address);
	case 0x04000180: case 0x04000181: case 0x04000182: case 0x04000183: case 0x04000184: case 0x04000185:
		return m_ipc.NDS9_read8(address);
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		return m_interruptManager->NDS9_readIO(address);
	case 0x04000247:
		return WRAMCNT;
	case 0x04000280: case 0x04000281: case 0x04000290: case 0x04000291: case 0x04000292: case 0x04000293: case 0x04000294: case 0x04000295:
	case 0x04000296: case 0x04000297: case 0x04000298: case 0x04000299: case 0x0400029A: case 0x0400029B: case 0x0400029C: case 0x0400029D:
	case 0x0400029E: case 0x0400029F: case 0x040002A0: case 0x040002A1: case 0x040002A2: case 0x040002A3: case 0x040002A4: case 0x040002A5:
	case 0x040002A6: case 0x040002A7: case 0x040002A8: case 0x040002A9: case 0x040002AA: case 0x040002AB: case 0x040002AC: case 0x040002AD:
	case 0x040002AE: case 0x040002AF: case 0x040002B0: case 0x040002B1: case 0x040002B2: case 0x040002B3: case 0x040002B4: case 0x040002B5:
	case 0x040002B6: case 0x040002B7: case 0x040002B8: case 0x040002B9: case 0x040002BA: case 0x040002BB: case 0x040002BC: case 0x040002BD:
	case 0x040002BE: case 0x040002BF:
		return m_math.readIO(address);
	case 0x040001A0: case 0x040001A1: case 0x040001A2: case 0x040001A3: case 0x040001A4: case 0x040001A5: case 0x040001A6: case 0x040001A7: case 0x040001A8: case 0x040001A9:
	case 0x040001AA: case 0x040001AB: case 0x040001AC: case 0x040001AD: case 0x040001AE: case 0x040001AF: case 0x04100010: case 0x04100011: case 0x04100012: case 0x04100013:
		return m_cartridge->NDS9_read(address);
	case 0x04000204:
		return EXMEMCNT & 0xFF;
	case 0x04000205:
		return ((EXMEMCNT >> 8) & 0xFF);
	case 0x04000300:
		return NDS9_POSTFLG;
		//ugh...
	case 0x04000064: case 0x04000065: case 0x04000066: case 0x04000067:
		return m_ppu->readIO(address);
	}
	return 0;
}

void Bus::NDS9_writeIO8(uint32_t address, uint8_t value)
{
	//horrible...
	if (address==0x04000304 || address == 0x04000305 || (address >= 0x04000000 && address <= 0x04000058) || (address >= 0x04001000 && address <= 0x04001058))
	{
		m_ppu->writeIO(address, value);
		return;
	}
	if (address==0x04000060 || address==0x04000061 || (address >= 0x04000320 && address <= 0x040006a3))
	{
		m_gpu->write(address, value);
		return;
	}
	switch (address)
	{
	case 0x040000B0: case 0x040000B1: case 0x040000B2: case 0x040000B3: case 0x040000B4: case 0x040000B5: case 0x040000B6: case 0x040000B7:
	case 0x040000B8: case 0x040000B9: case 0x040000BA: case 0x040000BB: case 0x040000BC: case 0x040000BD: case 0x040000BE: case 0x040000BF:
	case 0x040000C0: case 0x040000C1: case 0x040000C2: case 0x040000C3: case 0x040000C4: case 0x040000C5: case 0x040000C6: case 0x040000C7:
	case 0x040000C8: case 0x040000C9: case 0x040000CA: case 0x040000CB: case 0x040000CC: case 0x040000CD: case 0x040000CE: case 0x040000CF:
	case 0x040000D0: case 0x040000D1: case 0x040000D2: case 0x040000D3: case 0x040000D4: case 0x040000D5: case 0x040000D6: case 0x040000D7:
	case 0x040000D8: case 0x040000D9: case 0x040000DA: case 0x040000DB: case 0x040000DC: case 0x040000DD: case 0x040000DE: case 0x040000DF:
	case 0x040000E0: case 0x040000E1: case 0x040000E2: case 0x040000E3: case 0x040000E4: case 0x040000E5: case 0x040000E6: case 0x040000E7:
	case 0x040000E8: case 0x040000E9: case 0x040000EA: case 0x040000EB: case 0x040000EC: case 0x040000ED: case 0x040000EE: case 0x040000EF:
		NDS9_writeDMAReg(address, value);
		break;
	case 0x04000100: case 0x04000101: case 0x04000102: case 0x04000103: case 0x04000104: case 0x04000105: case 0x04000106: case 0x04000107:
	case 0x04000108: case 0x04000109: case 0x0400010A: case 0x0400010B: case 0x0400010C: case 0x0400010D: case 0x0400010E: case 0x0400010F:
		m_NDS9Timer.writeIO(address, value);
		break;
	case 0x04000132: case 0x04000133:
		m_input->writeIORegister(address, value);
		break;
	case 0x04000180: case 0x04000181: case 0x04000182: case 0x04000183: case 0x04000184: case 0x04000185:
		m_ipc.NDS9_write8(address, value);
		break;
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		m_interruptManager->NDS9_writeIO(address,value);
		break;
	case 0x04000240: case 0x04000241: case 0x04000242: case 0x04000243: case 0x04000244: case 0x04000245: case 0x04000246: case 0x04000248: case 0x04000249:
		m_ppu->writeIO(address, value);
		mapVRAMPages();
		break;
	case 0x04000247:
	{
		WRAMCNT = value & 0b11;
		//nested switch, ugly!
		switch (WRAMCNT)
		{
		case 0:
		{
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x04000000); i += 2)
			{
				m_ARM9PageTable[i] = m_mem->WRAM[0];
				m_ARM9PageTable[i + 1] = m_mem->WRAM[1];
			}
			for (uint32_t i = 0; i < addressToPage(0x00800000); i += 4)
			{
				m_ARM7PageTable[addressToPage(0x03000000) + i] = m_mem->ARM7_WRAM;
				m_ARM7PageTable[addressToPage(0x03000000) + i + 1] = m_mem->ARM7_WRAM + 16384;
				m_ARM7PageTable[addressToPage(0x03000000) + i + 2] = m_mem->ARM7_WRAM + 32768;
				m_ARM7PageTable[addressToPage(0x03000000) + i + 3] = m_mem->ARM7_WRAM + 49152;
			}
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[0] = nullptr;
			m_mem->NDS7_sharedWRAMPtrs[1] = nullptr;
			break;
		}
		case 1:
		{
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x04000000); i++)
				m_ARM9PageTable[i] = m_mem->WRAM[1];
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x03800000); i++)
				m_ARM7PageTable[i] = m_mem->WRAM[0];
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[1];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[0];
			break;
		}
		case 2:
		{
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x04000000); i++)
				m_ARM9PageTable[i] = m_mem->WRAM[0];
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x03800000); i++)
				m_ARM7PageTable[i] = m_mem->WRAM[1];
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			break;
		}
		case 3:
		{
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x04000000); i += 2)
			{
				m_ARM9PageTable[i] = nullptr;
				m_ARM9PageTable[i + 1] = nullptr;
			}
			for (uint32_t i = addressToPage(0x03000000); i < addressToPage(0x03800000); i += 2)
			{
				m_ARM7PageTable[i] = m_mem->WRAM[0];
				m_ARM7PageTable[i + 1] = m_mem->WRAM[1];
			}
			m_mem->NDS9_sharedWRAMPtrs[0] = nullptr;
			m_mem->NDS9_sharedWRAMPtrs[1] = nullptr;
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			break;
		}
		}
		break;
	}
	case 0x04000280: case 0x04000281: case 0x04000290: case 0x04000291: case 0x04000292: case 0x04000293: case 0x04000294: case 0x04000295:
	case 0x04000296: case 0x04000297: case 0x04000298: case 0x04000299: case 0x0400029A: case 0x0400029B: case 0x0400029C: case 0x0400029D:
	case 0x0400029E: case 0x0400029F: case 0x040002A0: case 0x040002A1: case 0x040002A2: case 0x040002A3: case 0x040002A4: case 0x040002A5:
	case 0x040002A6: case 0x040002A7: case 0x040002A8: case 0x040002A9: case 0x040002AA: case 0x040002AB: case 0x040002AC: case 0x040002AD:
	case 0x040002AE: case 0x040002AF: case 0x040002B0: case 0x040002B1: case 0x040002B2: case 0x040002B3: case 0x040002B4: case 0x040002B5:
	case 0x040002B6: case 0x040002B7: case 0x040002B8: case 0x040002B9: case 0x040002BA: case 0x040002BB: case 0x040002BC: case 0x040002BD:
	case 0x040002BE: case 0x040002BF:
		m_math.writeIO(address, value);
		break;
	case 0x040001A0: case 0x040001A1: case 0x040001A2: case 0x040001A3: case 0x040001A4: case 0x040001A5: case 0x040001A6: case 0x040001A7: case 0x040001A8: case 0x040001A9:
	case 0x040001AA: case 0x040001AB: case 0x040001AC: case 0x040001AD: case 0x040001AE: case 0x040001AF: case 0x04100010: case 0x04100011: case 0x04100012: case 0x04100013:
		m_cartridge->NDS9_write(address, value);
		break;
	case 0x04000204:
		EXMEMCNT &= 0xFF00; EXMEMCNT |= value;
		break;
	case 0x04000205:
		EXMEMCNT &= 0xFF; EXMEMCNT |= (value << 8);
		m_cartridge->setNDS7AccessRights(((EXMEMCNT >> 11) & 0b1));
		break;
	case 0x04000300:
		NDS9_POSTFLG = 1;
		NDS9_POSTFLG |= (value & 0b10);
		break;
	case 0x04000064: case 0x04000065: case 0x04000066: case 0x04000067:
		m_ppu->writeIO(address, value);
		break;
	}
}

uint16_t Bus::NDS9_readIO16(uint32_t address)
{
	uint8_t lower = NDS9_readIO8(address);
	uint8_t upper = NDS9_readIO8(address + 1);
	return (upper << 8) | lower;
}

void Bus::NDS9_writeIO16(uint32_t address, uint16_t value)
{
	NDS9_writeIO8(address, value & 0xFF);
	NDS9_writeIO8(address + 1, ((value >> 8) & 0xFF));
}

uint32_t Bus::NDS9_readIO32(uint32_t address)
{
	//special cases
	switch (address)
	{
	case 0x04100000:
		return m_ipc.NDS9_read32(address);
	case 0x04100010:
		return m_cartridge->NDS9_readGamecard();
	}
	uint16_t lower = NDS9_readIO16(address);
	uint16_t upper = NDS9_readIO16(address + 2);
	return (upper << 16) | lower;
}

void Bus::NDS9_writeIO32(uint32_t address, uint32_t value)
{
	//special case: write ipcfifo
	if (address == 0x04000188)
	{
		m_ipc.NDS9_write32(address, value);
		return;
	}
	
	//special case: gxfifo
	if (address >= 0x04000400 && address <= 0x0400043f)	//gxfifo mirrored for 16 words?
	{
		m_gpu->writeGXFIFO(value);
		return;
	}
	if (address >= 0x04000440 && address <= 0x040005c8)
	{
		m_gpu->writeCmdPort(address, value);
		return;
	}
	NDS9_writeIO16(address, value & 0xFFFF);
	NDS9_writeIO16(address + 2, ((value >> 16) & 0xFFFF));
}

void Bus::setByteInWord(uint32_t* word, uint8_t byte, int pos)
{
	uint32_t tmp = *word;
	uint32_t mask = 0xFF;
	mask = ~(mask << (pos * 8));
	tmp &= mask;
	tmp |= (byte << (pos * 8));
	*word = tmp;
}

void Bus::setByteInHalfword(uint16_t* halfword, uint8_t byte, int pos)
{
	uint16_t tmp = *halfword;
	uint16_t mask = 0xFF;
	mask = ~(mask << (pos * 8));
	tmp &= mask;
	tmp |= (byte << (pos * 8));
	*halfword = tmp;
}