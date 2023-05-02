#include"Bus.h"

Bus::Bus()
{

}

void Bus::init(std::vector<uint8_t> NDS7_BIOS, std::vector<uint8_t> NDS9_BIOS, Cartridge* cartridge, Scheduler* scheduler, InterruptManager* interruptManager, PPU* ppu, Input* input)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;
	m_ppu = ppu;
	m_input = input;
	m_cartridge = cartridge;

	m_mem = new NDSMem;
	m_ipc.init(m_interruptManager);
	m_NDS9Timer.init(true, m_interruptManager, m_scheduler);
	m_NDS7Timer.init(false, m_interruptManager, m_scheduler);
	m_spi.init(m_interruptManager);

	m_ppu->registerMemory(m_mem);
	m_ppu->registerDMACallbacks((callbackFn)&Bus::NDS9_HBlankDMACallback, (callbackFn)&Bus::NDS9_VBlankDMACallback, (void*)this);
	m_cartridge->registerDMACallbacks((callbackFn)&Bus::NDS7_CartridgeDMACallback, (callbackFn)&Bus::NDS9_CartridgeDMACallback, (void*)this);

	//i don't know if this initial state is accurate, but oh well..
	m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
	m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
	m_mem->NDS7_sharedWRAMPtrs[0] = nullptr;
	m_mem->NDS7_sharedWRAMPtrs[1] = nullptr;

	memcpy(m_mem->NDS7_BIOS, &NDS7_BIOS[0], NDS7_BIOS.size());
	memcpy(m_mem->NDS9_BIOS, &NDS9_BIOS[0], NDS9_BIOS.size());
}

Bus::~Bus()
{
	delete m_mem;
}


uint8_t Bus::NDS7_read8(uint32_t address)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address > 0x3FFF)
			return 0;
		return m_mem->NDS7_BIOS[address];
	case 2:
		return m_mem->RAM[address & 0x3FFFFF];
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			return m_mem->ARM7_WRAM[address & 0xFFFF];
		return bank[address & 0x3FFF];
	}
	case 4:
		return NDS7_readIO8(address);
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		return addr[0];
	}
	case 8: case 9:
		return ((address >> 1) & 0xFFFF) >> ((address & 0b1)<<3);
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
	}

	return 0;
}

void Bus::NDS7_write8(uint32_t address, uint8_t value)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		m_mem->RAM[address & 0x3FFFFF] = value;
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			m_mem->ARM7_WRAM[address & 0xFFFF] = value;
		else
			bank[address & 0x3FFF] = value;
		break;
	}
	case 4:
		NDS7_writeIO8(address,value);
		break;
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		addr[0] = value;
		break;
	}
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
	}
}

uint16_t Bus::NDS7_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address > 0x3FFF)
			return 0;
		return getValue16(m_mem->NDS7_BIOS, address, 0x3FFF);
	case 2:
		return getValue16(m_mem->RAM,address & 0x3FFFFF,0x3FFFFF);
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			return getValue16(m_mem->ARM7_WRAM, address & 0xFFFF, 0xFFFF);
		return getValue16(bank, address & 0x3FFF, 0x3FFF);
	}
	case 4:
		return NDS7_readIO16(address);
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		return getValue16(addr, 0, 0xFFFF);
	}
	case 8: case 9:
		return (address >> 1) & 0xFFFF;
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
	}

	return 0;
}

void Bus::NDS7_write16(uint32_t address, uint16_t value)
{
	address &= 0xFFFFFFFE;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		setValue16(m_mem->RAM,address & 0x3FFFFF,0x3FFFFF, value);
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			setValue16(m_mem->ARM7_WRAM, address & 0xFFFF, 0xFFFF, value);
		else
			setValue16(bank, address & 0x3FFF, 0x3FFF, value);
		break;
	}
	case 4:
		NDS7_writeIO16(address, value);
		break;
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		setValue16(addr, 0, 0xFFFF, value);
		break;
	}
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
	}
}

uint32_t Bus::NDS7_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0:
		if (address > 0x3FFF)
			return 0;
		return getValue32(m_mem->NDS7_BIOS, address, 0x3FFF);
	case 2:
		return getValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF);
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			return getValue32(m_mem->ARM7_WRAM, address & 0xFFFF, 0xFFFF);
		return getValue32(bank, address & 0x3FFF, 0x3FFF);
	}
	case 4:
		return NDS7_readIO32(address);
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		return getValue32(addr, 0, 0xFFFF);
	}
	case 8: case 9:
		return ((address >> 1) & 0xFFFF) | ((((address + 2) >> 1) & 0xFFFF) << 16);
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
	}

	return 0;
}

void Bus::NDS7_write32(uint32_t address, uint32_t value)
{
	address &= 0xFFFFFFFC;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		setValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF, value);
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS7_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank || address >= 0x03800000)
			setValue32(m_mem->ARM7_WRAM, address & 0xFFFF, 0xFFFF, value);
		else
			setValue32(bank, address & 0x3FFF, 0x3FFF, value);
		break;
	}
	case 4:
		NDS7_writeIO32(address, value);
		break;
	case 6:
	{
		uint8_t* addr = m_ppu->mapARM7Address(address);
		setValue32(addr, 0, 0xFFFF, value);
		break;
	}
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
	}
}

uint8_t Bus::NDS9_read8(uint32_t address)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		return m_mem->RAM[address & 0x3FFFFF];
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank)
			return 0;
		return bank[address & 0x3FFF];
	}
	case 4:
		return NDS9_readIO8(address);
	case 5:
		return m_mem->PAL[address & 0x7FF];
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		return addr[0];
	}
	case 7:
		return m_mem->OAM[address & 0x7FF];
	case 8: case 9: case 0xA:
		return 0xFF;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return m_mem->NDS9_BIOS[address & 0x7FFF];
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write8(uint32_t address, uint8_t value)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		m_mem->RAM[address & 0x3FFFFF] = value;
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (bank)
			bank[address & 0x3FFF] = value;
		break;
	}
	case 4:
		NDS9_writeIO8(address,value);
		break;
	case 5:
		m_mem->PAL[address & 0x7FF] = value;
		break;
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		addr[0] = value;
		break;
	}
	case 7:
		m_mem->OAM[address & 0x7FF] = value;
		break;
	case 8: case 9: case 0xA:
		break;
	case 0xFF:
		Logger::msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
		break;
	}

}

uint16_t Bus::NDS9_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		return getValue16(m_mem->RAM,address & 0x3FFFFF,0x3FFFFF);
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank)
			return 0;
		return getValue16(bank, address & 0x3FFF, 0x3FFF);
	}
	case 4:
		return NDS9_readIO16(address);
	case 5:
		return getValue16(m_mem->PAL,address & 0x7FF,0x7FF);
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		return getValue16(addr, 0, 0xFFFF);
	}
	case 7:
		return getValue16(m_mem->OAM,address & 0x7FF,0x7FF);
	case 8: case 9: case 0xA:
		return 0xFFFF;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return getValue16(m_mem->NDS9_BIOS, address & 0x7FFF, 0x7FFF);
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write16(uint32_t address, uint16_t value)
{
	address &= 0xFFFFFFFE;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		setValue16(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF, value);
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (bank)
			setValue16(bank, address & 0x3FFF, 0x3FFF, value);
		break;
	}
	case 4:
		NDS9_writeIO16(address, value);
		break;
	case 5:
		setValue16(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		setValue16(addr, 0, 0xFFFF,value);
		break;
	}
	case 7:
		setValue16(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		break;
	case 0xFF:
		Logger::msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
		break;
	}

}

uint32_t Bus::NDS9_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		return getValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF);
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (!bank)
			return 0;
		return getValue32(bank, address & 0x3FFF, 0x3FFF);
	}
	case 4:
		return NDS9_readIO32(address);
	case 5:
		return getValue32(m_mem->PAL, address & 0x7FF, 0x7FF);
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		return getValue32(addr, 0, 0xFFFF);
	}
	case 7:
		return getValue32(m_mem->OAM, address & 0x7FF, 0x7FF);
	case 8: case 9: case 0xA:
		return 0xFFFFFFFF;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return getValue32(m_mem->NDS9_BIOS, address & 0x7FFF, 0x7FFF);
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write32(uint32_t address, uint32_t value)
{
	address &= 0xFFFFFFFC;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 2:
		setValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF, value);
		break;
	case 3:
	{
		uint8_t* bank = m_mem->NDS9_sharedWRAMPtrs[(address >> 14) & 0b1];
		if (bank)
			setValue32(bank, address & 0x3FFF, 0x3FFF, value);
		break;
	}
	case 4:
		NDS9_writeIO32(address, value);
		break;
	case 5:
		setValue32(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 6:
	{
		uint8_t* addr = getVRAMAddr(address);
		setValue32(addr, 0, 0xFFFF,value);
		break;
	}
	case 7:
		setValue32(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		break;
	case 0xFF:
		Logger::msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
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
	case 0x04000130: case 0x04000131: case 0x04000132: case 0x04000133:
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
	case 0x04000136: case 0x04000137:
		return m_input->readIORegister(address);
	//	return 1;
	}
	Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented IO read! addr={:#x}", address));
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
	default:
		Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented IO write! addr={:#x} val={:#x}", address, value));
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
		return 0;
	}
	Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented IO read! addr={:#x}", address));
	return 0;
}

void Bus::NDS9_writeIO8(uint32_t address, uint8_t value)
{
	//horrible...
	if (address==0x04000304 || address == 0x04000305 || (address >= 0x04000000 && address <= 0x04000058) || (address >= 0x04000240 && address <= 0x04000249 && address!=0x04000247) || (address >= 0x04001000 && address <= 0x04001058))
	{
		m_ppu->writeIO(address, value);
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
	case 0x04000247:
	{
		WRAMCNT = value & 0b11;
		//nested switch, ugly!
		switch (WRAMCNT)
		{
		case 0:
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[0] = nullptr;
			m_mem->NDS7_sharedWRAMPtrs[1] = nullptr;
			break;
		case 1:
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[1];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[0];
			break;
		case 2:
			m_mem->NDS9_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS9_sharedWRAMPtrs[1] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[1];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[1];
			break;
		case 3:
			m_mem->NDS9_sharedWRAMPtrs[0] = nullptr;
			m_mem->NDS9_sharedWRAMPtrs[1] = nullptr;
			m_mem->NDS7_sharedWRAMPtrs[0] = m_mem->WRAM[0];
			m_mem->NDS7_sharedWRAMPtrs[1] = m_mem->WRAM[1];
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
	default:
		Logger::msg(LoggerSeverity::Warn, std::format("Unimplemented IO write! addr={:#x} val={:#x}", address, value));
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