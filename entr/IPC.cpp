#include"IPC.h"

IPC::IPC()
{
	NDS7_IPCData = 0;
	NDS9_IPCData = 0;
}

IPC::~IPC()
{

}

uint8_t IPC::NDS7_read8(uint32_t address)
{
	switch (address)
	{
	case 0x04000180:
		return NDS9_IPCData & 0xF;
	case 0x04000181:
		return NDS7_IPCData & 0xF;	//<--todo: IPCSYNC.13,14 (IRQ bits)
	}
}

void IPC::NDS7_write8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000181:
		NDS7_IPCData = value & 0xF;
		break;
	}
}

uint8_t IPC::NDS9_read8(uint32_t address)
{
	switch (address)
	{
	case 0x04000180:
		return NDS7_IPCData & 0xF;
	case 0x04000181:
		return NDS9_IPCData & 0xF;	//same as NDS7, need to impl IPCSYNC.13,14
	}
}

void IPC::NDS9_write8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000181:
		NDS9_IPCData = value & 0xF;
		break;
	}
}