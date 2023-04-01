#include"InterruptManager.h"

InterruptManager::InterruptManager()
{

}

InterruptManager::~InterruptManager()
{

}

void InterruptManager::NDS7_requestInterrupt(InterruptType type)
{
	NDS7_IF |= (uint32_t)type;
}


bool InterruptManager::NDS7_getInterrupt()
{
	return (NDS7_IF & NDS7_IE);
}

uint8_t InterruptManager::NDS7_readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000208:
		return NDS7_IME & 0b1;
	case 0x04000210:
		return NDS7_IE & 0xFF;
	case 0x04000211:
		return ((NDS7_IE >> 8) & 0xFF);
	case 0x04000212:
		return ((NDS7_IE >> 16) & 0xFF);
	case 0x04000213:
		return ((NDS7_IE >> 24) & 0xFF);
	case 0x04000214:
		return NDS7_IF & 0xFF;
	case 0x04000215:
		return ((NDS7_IF >> 8) & 0xFF);
	case 0x04000216:
		return ((NDS7_IF >> 16) & 0xFF);
	case 0x04000217:
		return ((NDS7_IF >> 24) & 0xFF);
	}
}

void InterruptManager::NDS7_writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000208:
		NDS7_IME = value & 0b1;
		break;
	case 0x04000210:
		NDS7_IE &= 0xFFFFFF00; NDS7_IE |= value;
		break;
	case 0x04000211:
		NDS7_IE &= 0xFFFF00FF; NDS7_IE |= (value << 8);
		break;
	case 0x04000212:
		NDS7_IE &= 0xFF00FFFFF; NDS7_IE |= (value << 16);
		break;
	case 0x04000213:
		NDS7_IE &= 0x00FFFFFF; NDS7_IE |= (value << 24);
		break;
	case 0x04000214:
		NDS7_IF &= ~(value);
		break;
	case 0x04000215:
		NDS7_IF &= ~(value << 8);
		break;
	case 0x04000216:
		NDS7_IF &= ~(value << 16);
		break;
	case 0x04000217:
		NDS7_IF &= ~(value << 24);
		break;
	}
}

void InterruptManager::NDS9_requestInterrupt(InterruptType type)
{
	NDS9_IF |= (uint32_t)type;
}

bool InterruptManager::NDS9_getInterrupt()
{
	return (NDS9_IF & NDS9_IE);
}

uint8_t InterruptManager::NDS9_readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000208:
		return NDS9_IME & 0b1;
	case 0x04000210:
		return NDS9_IE & 0xFF;
	case 0x04000211:
		return ((NDS9_IE >> 8) & 0xFF);
	case 0x04000212:
		return ((NDS9_IE >> 16) & 0xFF);
	case 0x04000213:
		return ((NDS9_IE >> 24) & 0xFF);
	case 0x04000214:
		return NDS9_IF & 0xFF;
	case 0x04000215:
		return ((NDS9_IF >> 8) & 0xFF);
	case 0x04000216:
		return ((NDS9_IF >> 16) & 0xFF);
	case 0x04000217:
		return ((NDS9_IF >> 24) & 0xFF);
	}
}

void InterruptManager::NDS9_writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000208:
		NDS9_IME = value & 0b1;
		break;
	case 0x04000210:
		NDS9_IE &= 0xFFFFFF00; NDS9_IE |= value;
		break;
	case 0x04000211:
		NDS9_IE &= 0xFFFF00FF; NDS9_IE |= (value << 8);
		break;
	case 0x04000212:
		NDS9_IE &= 0xFF00FFFFF; NDS9_IE |= (value << 16);
		break;
	case 0x04000213:
		NDS9_IE &= 0x00FFFFFF; NDS9_IE |= (value << 24);
		break;
	case 0x04000214:
		NDS9_IF &= ~(value);
		break;
	case 0x04000215:
		NDS9_IF &= ~(value << 8);
		break;
	case 0x04000216:
		NDS9_IF &= ~(value << 16);
		break;
	case 0x04000217:
		NDS9_IF &= ~(value << 24);
		break;
	}
}