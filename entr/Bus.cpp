#include"Bus.h"

Bus::Bus()
{
	//todo
}

Bus::~Bus()
{
	//todo
}

uint8_t Bus::NDS7_read8(uint32_t address)
{
	return 0;
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
	return 0;
}

void Bus::NDS9_write8(uint32_t address, uint8_t value)
{

}

uint16_t Bus::NDS9_read16(uint32_t address)
{
	return 0;
}

void Bus::NDS9_write16(uint32_t address, uint16_t value)
{

}

uint32_t Bus::NDS9_read32(uint32_t address)
{
	return 0;
}

void Bus::NDS9_write32(uint32_t address, uint32_t value)
{

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