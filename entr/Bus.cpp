#include"Bus.h"

Bus::Bus()
{
	m_mem = std::make_shared<NDSMem>();
}

Bus::~Bus()
{
	//todo
}

uint8_t Bus::NDS7_read8(uint32_t address)
{

}

void Bus::NDS7_write8(uint32_t address, uint8_t value)
{

}

uint16_t Bus::NDS7_read16(uint32_t address)
{
	return 0;
}

void Bus::NDS7_write16(uint32_t address, uint16_t value)
{

}

uint32_t Bus::NDS7_read32(uint32_t address)
{
	return 0;
}

void Bus::NDS7_write32(uint32_t address, uint32_t value)
{

}

uint8_t Bus::NDS9_read8(uint32_t address)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		return m_mem->ITCM[address & 0x7FFF];	//not sure about mirroring yet. would have to add CP15 first
	case 2:
		return m_mem->RAM[address & 0x3FFFFF];
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented read from shared WRAM!!");
		return 0;
	case 4:
		return NDS9_readIO8(address);
	case 5:
		return m_mem->PAL[address & 0x7FF];
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			return m_mem->VRAM[std::min(address, (uint32_t)0xA3FFF)];	//maybe not safe/right, shrug
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM read! addr={:#x}", address));
			return 0;
		}
	}
	case 7:
		return m_mem->OAM[address & 0x7FF];
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS read!!");
		return 0;
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
		m_mem->ITCM[address & 0x7FFF]=value;	//not sure about mirroring yet. would have to add CP15 first
		break;
	case 2:
		m_mem->RAM[address & 0x3FFFFF] = value;
		break;
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented write to shared WRAM!!");
		break;
	case 4:
		NDS9_writeIO8(address,value);
		break;
	case 5:
		m_mem->PAL[address & 0x7FF] = value;
		break;
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			m_mem->VRAM[std::min(address, (uint32_t)0xA3FFF)] = value;	//maybe not safe/right, shrug
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM write! addr={:#x}", address));
		}
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
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		return getValue16(m_mem->ITCM,address & 0x7FFF,0x7FFF);	//not sure about mirroring yet. would have to add CP15 first
	case 2:
		return getValue16(m_mem->RAM,address & 0x3FFFFF,0x3FFFFF);
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented read from shared WRAM!!");
		return 0;
	case 4:
		return NDS9_readIO16(address);
	case 5:
		return getValue16(m_mem->PAL,address & 0x7FF,0x7FF);
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			return getValue16(m_mem->VRAM,std::min(address, (uint32_t)0xA3FFF),0xFFFFFFFF);	//maybe not safe/right, shrug
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM read! addr={:#x}", address));
			return 0;
		}
	}
	case 7:
		return getValue16(m_mem->OAM,address & 0x7FF,0x7FF);
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS read!!");
		return 0;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write16(uint32_t address, uint16_t value)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		setValue16(m_mem->ITCM, address & 0x7FFF, 0x7FFF, value);
		break;
	case 2:
		setValue16(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF, value);
		break;
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented write to shared WRAM!!");
		break;
	case 4:
		NDS9_writeIO16(address, value);
		break;
	case 5:
		setValue16(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			setValue16(m_mem->VRAM, std::min(address, (uint32_t)0xA3FFF), 0xFFFFFFFF, value);
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM write! addr={:#x}", address));
		}
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
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		return getValue32(m_mem->ITCM, address & 0x7FFF, 0x7FFF);	//not sure about mirroring yet. would have to add CP15 first
	case 2:
		return getValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF);
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented read from shared WRAM!!");
		return 0;
	case 4:
		return NDS9_readIO32(address);
	case 5:
		return getValue32(m_mem->PAL, address & 0x7FF, 0x7FF);
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			return getValue32(m_mem->VRAM, std::min(address, (uint32_t)0xA3FFF), 0xFFFFFFFF);	//maybe not safe/right, shrug
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM read! addr={:#x}", address));
			return 0;
		}
	}
	case 7:
		return getValue32(m_mem->OAM, address & 0x7FF, 0x7FF);
	case 8: case 9: case 0xA:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented cartridge space read! addr={:#x}", address));
		return 0;
	case 0xFF:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented ARM9 BIOS read!!");
		return 0;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid memory read! addr={:#x}", address));
		return 0;
	}

	return 0;
}

void Bus::NDS9_write32(uint32_t address, uint32_t value)
{
	uint8_t page = (address >> 24) & 0xFF;
	switch (page)
	{
	case 0: case 1:
		setValue32(m_mem->ITCM, address & 0x7FFF, 0x7FFF, value);
		break;
	case 2:
		setValue32(m_mem->RAM, address & 0x3FFFFF, 0x3FFFFF, value);
		break;
	case 3:
		Logger::getInstance()->msg(LoggerSeverity::Error, "Unimplemented write to shared WRAM!!");
		break;
	case 4:
		NDS9_writeIO32(address, value);
		break;
	case 5:
		setValue32(m_mem->PAL, address & 0x7FF, 0x7FF, value);
		break;
	case 6:
	{
		uint8_t subPage = (address >> 20) & 0xF;
		if (subPage == 8)
		{
			address &= 0xFFFFF;
			setValue32(m_mem->VRAM, std::min(address, (uint32_t)0xA3FFF), 0xFFFFFFFF, value);
		}
		else
		{
			Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Unimplemented VRAM write! addr={:#x}", address));
		}
		break;
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
	return 0;
}

void Bus::NDS7_writeIO8(uint32_t address, uint8_t value)
{

}

uint16_t Bus::NDS7_readIO16(uint32_t address)
{
	return 0;
}

void Bus::NDS7_writeIO16(uint32_t address, uint16_t value)
{

}

uint32_t Bus::NDS7_readIO32(uint32_t address)
{
	return 0;
}

void Bus::NDS7_writeIO32(uint32_t address, uint32_t value)
{

}

//Handle NDS9 IO
uint8_t Bus::NDS9_readIO8(uint32_t address)
{
	return 0;
}

void Bus::NDS9_writeIO8(uint32_t address, uint8_t value)
{

}

uint16_t Bus::NDS9_readIO16(uint32_t address)
{
	return 0;
}

void Bus::NDS9_writeIO16(uint32_t address, uint16_t value)
{

}

uint32_t Bus::NDS9_readIO32(uint32_t address)
{
	return 0;
}

void Bus::NDS9_writeIO32(uint32_t address, uint32_t value)
{

}

//Handles reading/writing larger than byte sized values (the addresses should already be aligned so no issues there)
//This is SOLELY for memory - IO is handled differently bc it's not treated as a flat mem space
uint16_t Bus::getValue16(uint8_t* arr, int base, int mask)
{
	return (uint16_t)arr[base] | ((arr[(base + 1) & mask]) << 8);
}

void Bus::setValue16(uint8_t* arr, int base, int mask, uint16_t val)
{
	arr[base] = val & 0xFF;
	arr[(base + 1) & mask] = ((val >> 8) & 0xFF);
}

uint32_t Bus::getValue32(uint8_t* arr, int base, int mask)
{
	return (uint32_t)arr[base] | ((arr[(base + 1) & mask]) << 8) | ((arr[(base + 2) & mask]) << 16) | ((arr[(base + 3) & mask]) << 24);
}

void Bus::setValue32(uint8_t* arr, int base, int mask, uint32_t val)
{
	arr[base] = val & 0xFF;
	arr[(base + 1) & mask] = ((val >> 8) & 0xFF);
	arr[(base + 2) & mask] = ((val >> 16) & 0xFF);
	arr[(base + 3) & mask] = ((val >> 24) & 0xFF);
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