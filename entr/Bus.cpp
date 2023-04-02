#include"Bus.h"

Bus::Bus(std::vector<uint8_t> NDS7_BIOS, std::vector<uint8_t> NDS9_BIOS, std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<PPU> ppu, std::shared_ptr<Input> input)
{
	m_interruptManager = interruptManager;
	m_ppu = ppu;
	m_input = input;
	m_mem = std::make_shared<NDSMem>();
	m_ipc = std::make_shared<IPC>(m_interruptManager);
	m_math = std::make_shared<DSMath>();

	m_ppu->registerMemory(m_mem);

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
	//todo
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		return addr[0];
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		addr[0] = value;
		break;
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		return getValue16(addr, 0, 0xFFFF);
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		setValue16(addr, 0, 0xFFFF, value);
		break;
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		return getValue32(addr, 0, 0xFFFF);
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory read! addr={:#x}", address));
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
		uint8_t* addr = m_ppu->NDS7_mapAddressToVRAM(address);
		setValue16(addr, 0, 0xFFFF, value);
		break;
	}
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented/unmapped memory write! addr={:#x}", address));
	}
}

uint8_t Bus::NDS9_read8(uint32_t address)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		//return m_mem->ITCM[address & 0x7FFF];	//not sure about mirroring yet. would have to add CP15 first
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		return addr[0];
	}
	case 7:
		return m_mem->OAM[address & 0x7FF];
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return m_mem->NDS9_BIOS[address & 0x7FFF];
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write8(uint32_t address, uint8_t value)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		//m_mem->ITCM[address & 0x7FFF]=value;	//not sure about mirroring yet. would have to add CP15 first
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		addr[0] = value;
		break;
	}
	case 7:
		m_mem->OAM[address & 0x7FF] = value;
		break;
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space write! addr={:#x}", address));
		break;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
		break;
	}

}

uint16_t Bus::NDS9_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		//return getValue16(m_mem->ITCM,address & 0x7FFF,0x7FFF);	//not sure about mirroring yet. would have to add CP15 first
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		return getValue16(addr, 0, 0xFFFF);
	}
	case 7:
		return getValue16(m_mem->OAM,address & 0x7FF,0x7FF);
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return getValue16(m_mem->NDS9_BIOS, address & 0x7FFF, 0x7FFF);
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
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
	case 0: case 1:
		//setValue16(m_mem->ITCM, address & 0x7FFF, 0x7FFF, value);
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		setValue16(addr, 0, 0xFFFF,value);
		break;
	}
	case 7:
		setValue16(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space write! addr={:#x}", address));
		break;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
		break;
	}

}

uint32_t Bus::NDS9_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		//return getValue32(m_mem->ITCM, address & 0x7FFF, 0x7FFF);	//not sure about mirroring yet. would have to add CP15 first
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		return getValue32(addr, 0, 0xFFFF);
	}
	case 7:
		return getValue32(m_mem->OAM, address & 0x7FF, 0x7FF);
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		if ((address & 0xFFFF) > 0x7FFF)
			return 0;
		return getValue32(m_mem->NDS9_BIOS, address & 0x7FFF, 0x7FFF);
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
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
	case 0: case 1:
		//setValue32(m_mem->ITCM, address & 0x7FFF, 0x7FFF, value);
		break;
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
		uint8_t* addr = m_ppu->mapAddressToVRAM(address);
		setValue32(addr, 0, 0xFFFF,value);
	}
	case 7:
		setValue32(m_mem->OAM, address & 0x7FF, 0x7FF, value);
		break;
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space write! addr={:#x}", address));
		break;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS write!!");
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory write! addr={:#x}", address));
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
	case 0x04000130: case 0x04000131: case 0x04000132: case 0x04000133:
		return m_input->readIORegister(address);
	case 0x04000180: case 0x04000181: case 0x04000184: case 0x04000185:
		return m_ipc->NDS7_read8(address);
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		return m_interruptManager->NDS7_readIO(address);
	case 0x04000240:
		return (m_mem->VRAM_C.ARM7) | (m_mem->VRAM_D.ARM7 << 1);
	case 0x04000241:
		return WRAMCNT;
	}
	Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented IO read! addr={:#x}", address));
	return 0;
}

void Bus::NDS7_writeIO8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000004: case 0x04000005: case 0x04000006: case 0x04000007:
		m_ppu->NDS7_writeIO(address, value);
		break;
	case 0x04000132: case 0x04000133:
		m_input->writeIORegister(address, value);
		break;
	case 0x04000180: case 0x04000181: case 0x04000184: case 0x04000185:
		m_ipc->NDS7_write8(address, value);
		break;
	case 0x04000208: case 0x04000209: case 0x0400020A: case 0x0400020B: case 0x04000210: case 0x04000211: case 0x04000212: case 0x04000213:
	case 0x04000214: case 0x04000215: case 0x04000216: case 0x04000217:
		m_interruptManager->NDS7_writeIO(address,value);
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented IO write! addr={:#x} val={:#x}", address, value));
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
	//special case: read ipcfifo
	if (address == 0x04100000)
		return m_ipc->NDS7_read32(address);

	uint16_t lower = NDS7_readIO16(address);
	uint16_t upper = NDS7_readIO16(address + 2);
	return (upper << 16) | lower;
}

void Bus::NDS7_writeIO32(uint32_t address, uint32_t value)
{
	//special case: write ipcfifo
	if (address == 0x04000188)
	{
		m_ipc->NDS7_write32(address, value);
		return;
	}

	NDS7_writeIO16(address, value & 0xFFFF);
	NDS7_writeIO16(address + 2, ((value >> 16) & 0xFFFF));
}

//Handle NDS9 IO
uint8_t Bus::NDS9_readIO8(uint32_t address)
{
	if (address >= 0x04000000 && address <= 0x04000058)
		return m_ppu->readIO(address);
	switch (address)
	{
	case 0x04000130: case 0x04000131: case 0x04000132: case 0x04000133:
		return m_input->readIORegister(address);
	case 0x04000180: case 0x04000181: case 0x04000184: case 0x04000185:
		return m_ipc->NDS9_read8(address);
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
		return m_math->readIO(address);
	}
	Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented IO read! addr={:#x}", address));
	return 0;
}

void Bus::NDS9_writeIO8(uint32_t address, uint8_t value)
{
	if ((address >= 0x04000000 && address <= 0x04000058) || (address >= 0x04000240 && address <= 0x04000249 && address!=0x04000247))
	{
		m_ppu->writeIO(address, value);
		return;
	}
	switch (address)
	{
	case 0x04000132: case 0x04000133:
		m_input->writeIORegister(address, value);
		break;
	case 0x04000180: case 0x04000181: case 0x04000184: case 0x04000185:
		m_ipc->NDS9_write8(address, value);
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
		m_math->writeIO(address, value);
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented IO write! addr={:#x} val={:#x}", address, value));
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
	//special case: read ipcfifo
	if (address == 0x04100000)
		return m_ipc->NDS9_read32(address);

	uint16_t lower = NDS9_readIO16(address);
	uint16_t upper = NDS9_readIO16(address + 2);
	return (upper << 16) | lower;
}

void Bus::NDS9_writeIO32(uint32_t address, uint32_t value)
{
	//special case: write ipcfifo
	if (address == 0x04000188)
	{
		m_ipc->NDS9_write32(address, value);
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