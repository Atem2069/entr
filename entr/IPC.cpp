#include"IPC.h"

IPC::IPC(std::shared_ptr<InterruptManager> interruptManager)
{
	m_interruptManager = interruptManager;

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
		return NDS7_IPCData & 0x4F;	//<--todo: IPCSYNC.13,14 (IRQ bits)
	case 0x04000184:
		return (NDS7_IPCFIFO.IRQOnEmpty << 2) | ((NDS7_IPCFIFO.size == 16) << 1) | (NDS7_IPCFIFO.size == 0);
	case 0x04000185:
		return (NDS7_IPCFIFO.enabled << 7) | (NDS7_IPCFIFO.error << 6) | (NDS9_IPCFIFO.IRQOnReceive << 2) | ((NDS9_IPCFIFO.size == 16) << 1) | (NDS9_IPCFIFO.size == 0);
	}
}

void IPC::NDS7_write8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000181:
		NDS7_IPCData = value & 0xFF;
		if (((value >> 5) & 0b1) && ((NDS9_IPCData>>6)&0b1))
			m_interruptManager->NDS9_requestInterrupt(InterruptType::IPCSync);
		break;
	case 0x04000184:
		NDS7_IPCFIFO.IRQOnEmpty = ((value >> 2) & 0b1);
		if ((value >> 3) & 0b1)
			NDS7_IPCFIFO.clear();
		break;
	case 0x04000185:
		NDS9_IPCFIFO.IRQOnReceive = ((value >> 2) & 0b1);
		if ((value >> 6) & 0b1)
			NDS7_IPCFIFO.error = false;
		NDS7_IPCFIFO.enabled = ((value >> 7) & 0b1);
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
		return NDS9_IPCData & 0x4F;	//same as NDS7, need to impl IPCSYNC.13,14
	case 0x04000184:
		return (NDS9_IPCFIFO.IRQOnEmpty << 2) | ((NDS9_IPCFIFO.size == 16) << 1) | (NDS9_IPCFIFO.size == 0);
	case 0x04000185:
		return (NDS9_IPCFIFO.enabled << 7) | (NDS9_IPCFIFO.error << 6) | (NDS7_IPCFIFO.IRQOnReceive << 2) | ((NDS7_IPCFIFO.size == 16) << 1) | (NDS7_IPCFIFO.size == 0);
	}
}

void IPC::NDS9_write8(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000181:
		NDS9_IPCData = value & 0xFF;
		if (((value >> 5) & 0b1) && ((NDS7_IPCData >> 6) & 0b1))
			m_interruptManager->NDS7_requestInterrupt(InterruptType::IPCSync);
		break;
	case 0x04000184:
		NDS9_IPCFIFO.IRQOnEmpty = ((value >> 2) & 0b1);
		if ((value >> 3) & 0b1)
			NDS9_IPCFIFO.clear();
		break;
	case 0x04000185:
		NDS7_IPCFIFO.IRQOnReceive = ((value >> 2) & 0b1);
		if ((value >> 6) & 0b1)
			NDS9_IPCFIFO.error = false;
		NDS9_IPCFIFO.enabled = ((value >> 7) & 0b1);
		break;
	}
}

uint32_t IPC::NDS7_read32(uint32_t address)
{
	switch (address)
	{
	case 0x04100000:
		if (NDS9_IPCFIFO.size == 0)
			NDS7_IPCFIFO.error = true;
		return NDS9_IPCFIFO.pop();
	}
}

void IPC::NDS7_write32(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x04000188:
		if (NDS7_IPCFIFO.size == 16)
			NDS7_IPCFIFO.error = true;
		NDS7_IPCFIFO.push(value);
		break;
	}
}

uint32_t IPC::NDS9_read32(uint32_t address)
{
	switch (address)
	{
	case 0x04100000:
		if (NDS7_IPCFIFO.size == 0)
			NDS9_IPCFIFO.error = true;
		return NDS7_IPCFIFO.pop();
	}
}

void IPC::NDS9_write32(uint32_t address, uint32_t value)
{
	switch (address)
	{
	case 0x04000188:
		if (NDS9_IPCFIFO.size == 16)
			NDS9_IPCFIFO.error = true;
		NDS9_IPCFIFO.push(value);
		break;
	}
}