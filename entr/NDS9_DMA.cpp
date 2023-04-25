#include"Bus.h"

uint8_t Bus::NDS9_readDMAReg(uint32_t address)
{
	switch (address)
	{
	case 0x040000B0:
		return m_NDS9Channels[0].src & 0xFF;
	case 0x040000B1:
		return ((m_NDS9Channels[0].src >> 8) & 0xFF);
	case 0x040000B2:
		return ((m_NDS9Channels[0].src >> 16) & 0xFF);
	case 0x040000B3:
		return ((m_NDS9Channels[0].src >> 24) & 0xFF);
	case 0x040000B4:
		return ((m_NDS9Channels[0].dest) & 0xFF);
	case 0x040000B5:
		return ((m_NDS9Channels[0].dest >> 8) & 0xFF);
	case 0x040000B6:
		return ((m_NDS9Channels[0].dest >> 16) & 0xFF);
	case 0x040000B7:
		return ((m_NDS9Channels[0].dest >> 24) & 0xFF);
	case 0x040000B8:
		return m_NDS9Channels[0].wordCount & 0xFF;
	case 0x040000B9:
		return ((m_NDS9Channels[0].wordCount >> 8) & 0xFF);
	case 0x040000BA:
		return m_NDS9Channels[0].control & 0xFF;
	case 0x040000BB:
		return ((m_NDS9Channels[0].control >> 8) & 0xFF);
	case 0x040000BC:
		return m_NDS9Channels[1].src & 0xFF;
	case 0x040000BD:
		return ((m_NDS9Channels[1].src >> 8) & 0xFF);
	case 0x040000BE:
		return ((m_NDS9Channels[1].src >> 16) & 0xFF);
	case 0x040000BF:
		return ((m_NDS9Channels[1].src >> 24) & 0xFF);
	case 0x040000C0:
		return ((m_NDS9Channels[1].dest) & 0xFF);
	case 0x040000C1:
		return ((m_NDS9Channels[1].dest >> 8) & 0xFF);
	case 0x040000C2:
		return ((m_NDS9Channels[1].dest >> 16) & 0xFF);
	case 0x040000C3:
		return ((m_NDS9Channels[1].dest >> 24) & 0xFF);
	case 0x040000C4:
		return m_NDS9Channels[1].wordCount & 0xFF;
	case 0x040000C5:
		return ((m_NDS9Channels[1].wordCount >> 8) & 0xFF);
	case 0x040000C6:
		return m_NDS9Channels[1].control & 0xFF;
	case 0x040000C7:
		return ((m_NDS9Channels[1].control >> 8) & 0xFF);
	case 0x040000C8:
		return m_NDS9Channels[2].src & 0xFF;
	case 0x040000C9:
		return ((m_NDS9Channels[2].src >> 8) & 0xFF);
	case 0x040000CA:
		return ((m_NDS9Channels[2].src >> 16) & 0xFF);
	case 0x040000CB:
		return ((m_NDS9Channels[2].src >> 24) & 0xFF);
	case 0x040000CC:
		return m_NDS9Channels[2].dest & 0xFF;
	case 0x040000CD:
		return ((m_NDS9Channels[2].dest >> 8) & 0xFF);
	case 0x040000CE:
		return ((m_NDS9Channels[2].dest >> 16) & 0xFF);
	case 0x040000CF:
		return ((m_NDS9Channels[2].dest >> 24) & 0xFF);
	case 0x040000D0:
		return m_NDS9Channels[2].wordCount & 0xFF;
	case 0x040000D1:
		return ((m_NDS9Channels[2].wordCount >> 8) & 0xFF);
	case 0x040000D2:
		return m_NDS9Channels[2].control & 0xFF;
	case 0x040000D3:
		return ((m_NDS9Channels[2].control >> 8) & 0xFF);
	case 0x040000D4:
		return m_NDS9Channels[3].src & 0xFF;
	case 0x040000D5:
		return ((m_NDS9Channels[3].src >> 8) & 0xFF);
	case 0x040000D6:
		return ((m_NDS9Channels[3].src >> 16) & 0xFF);
	case 0x040000D7:
		return ((m_NDS9Channels[3].src >> 24) & 0xFF);
	case 0x040000D8:
		return m_NDS9Channels[3].dest & 0xFF;
	case 0x040000D9:
		return ((m_NDS9Channels[3].dest >> 8) & 0xFF);
	case 0x040000DA:
		return ((m_NDS9Channels[3].dest >> 16) & 0xFF);
	case 0x040000DB:
		return ((m_NDS9Channels[3].dest >> 24) & 0xFF);
	case 0x040000DC:
		return m_NDS9Channels[3].wordCount & 0xFF;
	case 0x040000DD:
		return ((m_NDS9Channels[3].wordCount >> 8) & 0xFF);
	case 0x040000DE:
		return m_NDS9Channels[3].control & 0xFF;
	case 0x040000DF:
		return ((m_NDS9Channels[3].control >> 8) & 0xFF);
	case 0x040000E0:
		return m_NDS9Channels[0].scratchpad & 0xFF;
	case 0x040000E1:
		return ((m_NDS9Channels[0].scratchpad >> 8) & 0xFF);
	case 0x040000E2:
		return ((m_NDS9Channels[0].scratchpad >> 16) & 0xFF);
	case 0x040000E3:
		return ((m_NDS9Channels[0].scratchpad >> 24) & 0xFF);
	case 0x040000E4:
		return m_NDS9Channels[1].scratchpad & 0xFF;
	case 0x040000E5:
		return ((m_NDS9Channels[1].scratchpad >> 8) & 0xFF);
	case 0x040000E6:
		return ((m_NDS9Channels[1].scratchpad >> 16) & 0xFF);
	case 0x040000E7:
		return ((m_NDS9Channels[1].scratchpad >> 24) & 0xFF);
	case 0x040000E8:
		return m_NDS9Channels[2].scratchpad & 0xFF;
	case 0x040000E9:
		return ((m_NDS9Channels[2].scratchpad >> 8) & 0xFF);
	case 0x040000EA:
		return ((m_NDS9Channels[2].scratchpad >> 16) & 0xFF);
	case 0x040000EB:
		return ((m_NDS9Channels[2].scratchpad >> 24) & 0xFF);
	case 0x040000EC:
		return m_NDS9Channels[3].scratchpad & 0xFF;
	case 0x040000ED:
		return ((m_NDS9Channels[3].scratchpad >> 8) & 0xFF);
	case 0x040000EE:
		return ((m_NDS9Channels[3].scratchpad >> 16) & 0xFF);
	case 0x040000EF:
		return ((m_NDS9Channels[3].scratchpad >> 24) & 0xFF);
	}
}

void Bus::NDS9_writeDMAReg(uint32_t address, uint8_t value)
{
	switch (address)
	{
		//Channel 0
	case 0x040000B0:
		m_NDS9Channels[0].src &= 0xFFFFFF00; m_NDS9Channels[0].src |= value;
		break;
	case 0x040000B1:
		m_NDS9Channels[0].src &= 0xFFFF00FF; m_NDS9Channels[0].src |= (value << 8);
		break;
	case 0x040000B2:
		m_NDS9Channels[0].src &= 0xFF00FFFF; m_NDS9Channels[0].src |= (value << 16);
		break;
	case 0x040000B3:
		m_NDS9Channels[0].src &= 0x00FFFFFF; m_NDS9Channels[0].src |= (value << 24);
		break;
	case 0x040000B4:
		m_NDS9Channels[0].dest &= 0xFFFFFF00; m_NDS9Channels[0].dest |= value;
		break;
	case 0x040000B5:
		m_NDS9Channels[0].dest &= 0xFFFF00FF; m_NDS9Channels[0].dest |= (value << 8);
		break;
	case 0x040000B6:
		m_NDS9Channels[0].dest &= 0xFF00FFFF; m_NDS9Channels[0].dest |= (value << 16);
		break;
	case 0x040000B7:
		m_NDS9Channels[0].dest &= 0x00FFFFFF; m_NDS9Channels[0].dest |= (value << 24);
		break;
	case 0x040000B8:
		m_NDS9Channels[0].wordCount &= 0xFF00; m_NDS9Channels[0].wordCount |= value;
		break;
	case 0x040000B9:
		m_NDS9Channels[0].wordCount &= 0x00FF; m_NDS9Channels[0].wordCount |= (value << 8);
		break;
	case 0x040000BA:
		m_NDS9Channels[0].control &= 0xFF00; m_NDS9Channels[0].control |= value;
		break;
	case 0x040000BB:
	{
		bool wasEnabled = (m_NDS9Channels[0].control >> 15);
		m_NDS9Channels[0].control &= 0x00FF; m_NDS9Channels[0].control |= (value << 8);
		if ((!wasEnabled) && (value >> 7))
		{
			m_NDS9Channels[0].internalSrc = m_NDS9Channels[0].src;
			m_NDS9Channels[0].internalDest = m_NDS9Channels[0].dest;
			m_NDS9Channels[0].internalWordCount = m_NDS9Channels[0].wordCount | ((m_NDS9Channels[0].control & 0x1F) << 16);
			NDS9_checkDMAChannel(0);
		}
		break;
	}
		//Channel 1
	case 0x040000BC:
		m_NDS9Channels[1].src &= 0xFFFFFF00; m_NDS9Channels[1].src |= value;
		break;
	case 0x040000BD:
		m_NDS9Channels[1].src &= 0xFFFF00FF; m_NDS9Channels[1].src |= (value << 8);
		break;
	case 0x040000BE:
		m_NDS9Channels[1].src &= 0xFF00FFFF; m_NDS9Channels[1].src |= (value << 16);
		break;
	case 0x040000BF:
		m_NDS9Channels[1].src &= 0x00FFFFFF; m_NDS9Channels[1].src |= (value << 24);
		break;
	case 0x040000C0:
		m_NDS9Channels[1].dest &= 0xFFFFFF00; m_NDS9Channels[1].dest |= value;
		break;
	case 0x040000C1:
		m_NDS9Channels[1].dest &= 0xFFFF00FF; m_NDS9Channels[1].dest |= (value << 8);
		break;
	case 0x040000C2:
		m_NDS9Channels[1].dest &= 0xFF00FFFF; m_NDS9Channels[1].dest |= (value << 16);
		break;
	case 0x040000C3:
		m_NDS9Channels[1].dest &= 0x00FFFFFF; m_NDS9Channels[1].dest |= (value << 24);
		break;
	case 0x040000C4:
		m_NDS9Channels[1].wordCount &= 0xFF00; m_NDS9Channels[1].wordCount |= value;
		break;
	case 0x040000C5:
		m_NDS9Channels[1].wordCount &= 0x00FF; m_NDS9Channels[1].wordCount |= (value << 8);
		break;
	case 0x040000C6:
		m_NDS9Channels[1].control &= 0xFF00; m_NDS9Channels[1].control |= value;
		break;
	case 0x040000C7:
	{
		bool wasEnabled = (m_NDS9Channels[1].control >> 15);
		m_NDS9Channels[1].control &= 0x00FF; m_NDS9Channels[1].control |= (value << 8);
		if ((!wasEnabled) && (value >> 7))
		{
			m_NDS9Channels[1].internalSrc = m_NDS9Channels[1].src;
			m_NDS9Channels[1].internalDest = m_NDS9Channels[1].dest;
			m_NDS9Channels[1].internalWordCount = m_NDS9Channels[1].wordCount | ((m_NDS9Channels[1].control & 0x1F) << 16);
			NDS9_checkDMAChannel(1);
		}
		break;
	}
		//Channel 2
	case 0x040000C8:
		m_NDS9Channels[2].src &= 0xFFFFFF00; m_NDS9Channels[2].src |= value;
		break;
	case 0x040000C9:
		m_NDS9Channels[2].src &= 0xFFFF00FF; m_NDS9Channels[2].src |= (value << 8);
		break;
	case 0x040000CA:
		m_NDS9Channels[2].src &= 0xFF00FFFF; m_NDS9Channels[2].src |= (value << 16);
		break;
	case 0x040000CB:
		m_NDS9Channels[2].src &= 0x00FFFFFF; m_NDS9Channels[2].src |= (value << 24);
		break;
	case 0x040000CC:
		m_NDS9Channels[2].dest &= 0xFFFFFF00; m_NDS9Channels[2].dest |= value;
		break;
	case 0x040000CD:
		m_NDS9Channels[2].dest &= 0xFFFF00FF; m_NDS9Channels[2].dest |= (value << 8);
		break;
	case 0x040000CE:
		m_NDS9Channels[2].dest &= 0xFF00FFFF; m_NDS9Channels[2].dest |= (value << 16);
		break;
	case 0x040000CF:
		m_NDS9Channels[2].dest &= 0x00FFFFFF; m_NDS9Channels[2].dest |= (value << 24);
		break;
	case 0x040000D0:
		m_NDS9Channels[2].wordCount &= 0xFF00; m_NDS9Channels[2].wordCount |= value;
		break;
	case 0x040000D1:
		m_NDS9Channels[2].wordCount &= 0x00FF; m_NDS9Channels[2].wordCount |= (value << 8);
		break;
	case 0x040000D2:
		m_NDS9Channels[2].control &= 0xFF00; m_NDS9Channels[2].control |= value;
		break;
	case 0x040000D3:
	{
		bool wasEnabled = (m_NDS9Channels[2].control >> 15);
		m_NDS9Channels[2].control &= 0x00FF; m_NDS9Channels[2].control |= (value << 8);
		if ((!wasEnabled) && (value >> 7))
		{
			m_NDS9Channels[2].internalSrc = m_NDS9Channels[2].src;
			m_NDS9Channels[2].internalDest = m_NDS9Channels[2].dest;
			m_NDS9Channels[2].internalWordCount = m_NDS9Channels[2].wordCount | ((m_NDS9Channels[2].control & 0x1F) << 16);
			NDS9_checkDMAChannel(2);
		}
		break;
	}
		//Channel 3
	case 0x040000D4:
		m_NDS9Channels[3].src &= 0xFFFFFF00; m_NDS9Channels[3].src |= value;
		break;
	case 0x040000D5:
		m_NDS9Channels[3].src &= 0xFFFF00FF; m_NDS9Channels[3].src |= (value << 8);
		break;
	case 0x040000D6:
		m_NDS9Channels[3].src &= 0xFF00FFFF; m_NDS9Channels[3].src |= (value << 16);
		break;
	case 0x040000D7:
		m_NDS9Channels[3].src &= 0x00FFFFFF; m_NDS9Channels[3].src |= (value << 24);
		break;
	case 0x040000D8:
		m_NDS9Channels[3].dest &= 0xFFFFFF00; m_NDS9Channels[3].dest |= value;
		break;
	case 0x040000D9:
		m_NDS9Channels[3].dest &= 0xFFFF00FF; m_NDS9Channels[3].dest |= (value << 8);
		break;
	case 0x040000DA:
		m_NDS9Channels[3].dest &= 0xFF00FFFF; m_NDS9Channels[3].dest |= (value << 16);
		break;
	case 0x040000DB:
		m_NDS9Channels[3].dest &= 0x00FFFFFF; m_NDS9Channels[3].dest |= (value << 24);
		break;
	case 0x040000DC:
		m_NDS9Channels[3].wordCount &= 0xFF00; m_NDS9Channels[3].wordCount |= value;
		break;
	case 0x040000DD:
		m_NDS9Channels[3].wordCount &= 0x00FF; m_NDS9Channels[3].wordCount |= (value << 8);
		break;
	case 0x040000DE:
		m_NDS9Channels[3].control &= 0xFF00; m_NDS9Channels[3].control |= value;
		break;
	case 0x040000DF:
	{
		bool wasEnabled = (m_NDS9Channels[3].control >> 15);
		m_NDS9Channels[3].control &= 0x00FF; m_NDS9Channels[3].control |= (value << 8);
		if ((!wasEnabled) && (value >> 7))
		{
			m_NDS9Channels[3].internalSrc = m_NDS9Channels[3].src;
			m_NDS9Channels[3].internalDest = m_NDS9Channels[3].dest;
			m_NDS9Channels[3].internalWordCount = m_NDS9Channels[3].wordCount | ((m_NDS9Channels[3].control & 0x1F) << 16);
			NDS9_checkDMAChannel(3);
		}
		break;
	}
		//scratchpad regs
	case 0x040000E0:
		m_NDS9Channels[0].scratchpad &= 0xFFFFFF00; m_NDS9Channels[0].scratchpad |= value;
		break;
	case 0x040000E1:
		m_NDS9Channels[0].scratchpad &= 0xFFFF00FF; m_NDS9Channels[0].scratchpad |= (value << 8);
		break;
	case 0x040000E2:
		m_NDS9Channels[0].scratchpad &= 0xFF00FFFF; m_NDS9Channels[0].scratchpad |= (value << 16);
		break;
	case 0x040000E3:
		m_NDS9Channels[0].scratchpad &= 0x00FFFFFF; m_NDS9Channels[0].scratchpad |= (value << 24);
		break;
	case 0x040000E4:
		m_NDS9Channels[1].scratchpad &= 0xFFFFFF00; m_NDS9Channels[1].scratchpad |= value;
		break;
	case 0x040000E5:
		m_NDS9Channels[1].scratchpad &= 0xFFFF00FF; m_NDS9Channels[1].scratchpad |= (value << 8);
		break;
	case 0x040000E6:
		m_NDS9Channels[1].scratchpad &= 0xFF00FFFF; m_NDS9Channels[1].scratchpad |= (value << 16);
		break;
	case 0x040000E7:
		m_NDS9Channels[1].scratchpad &= 0x00FFFFFF; m_NDS9Channels[1].scratchpad |= (value << 24);
		break;
	case 0x040000E8:
		m_NDS9Channels[2].scratchpad &= 0xFFFFFF00; m_NDS9Channels[2].scratchpad |= value;
		break;
	case 0x040000E9:
		m_NDS9Channels[2].scratchpad &= 0xFFFF00FF; m_NDS9Channels[2].scratchpad |= (value << 8);
		break;
	case 0x040000EA:
		m_NDS9Channels[2].scratchpad &= 0xFF00FFFF; m_NDS9Channels[2].scratchpad |= (value << 16);
		break;
	case 0x040000EB:
		m_NDS9Channels[2].scratchpad &= 0x00FFFFFF; m_NDS9Channels[2].scratchpad |= (value << 24);
		break;
	case 0x040000EC:
		m_NDS9Channels[3].scratchpad &= 0xFFFFFF00; m_NDS9Channels[3].scratchpad |= value;
		break;
	case 0x040000ED:
		m_NDS9Channels[3].scratchpad &= 0xFFFF00FF; m_NDS9Channels[3].scratchpad |= (value << 8);
		break;
	case 0x040000EE:
		m_NDS9Channels[3].scratchpad &= 0xFF00FFFF; m_NDS9Channels[3].scratchpad |= (value << 16);
		break;
	case 0x040000EF:
		m_NDS9Channels[3].scratchpad &= 0x00FFFFFF; m_NDS9Channels[3].scratchpad |= (value << 24);
		break;
	}

}

void Bus::NDS9_checkDMAChannel(int channel)
{
	bool channelEnabled = (m_NDS9Channels[channel].control >> 15) & 0b1;
	uint8_t startTiming = (m_NDS9Channels[channel].control >> 11) & 0b111;
	if (channelEnabled)
	{
		if (startTiming == 0)
		{
			Logger::msg(LoggerSeverity::Info, std::format("Channel {} immediate DMA. src={:#x} dest={:#x} words={:#x}", channel, m_NDS9Channels[channel].internalSrc, m_NDS9Channels[channel].internalDest, m_NDS9Channels[channel].internalWordCount));
			NDS9_doDMATransfer(channel);
		}
		else
		{
			Logger::msg(LoggerSeverity::Info, std::format("Channel {} DMA, start timing={} src={:#x} dest={:#x}", channel, startTiming, m_NDS9Channels[channel].internalSrc, m_NDS9Channels[channel].internalDest));
			//m_NDS9Channels[channel].control &= 0x7FFF;	//dumb, remove when properly impl. other dma types
		}
	}
}

void Bus::NDS9_doDMATransfer(int channel)
{
	uint32_t srcAddressMask = 0x0FFFFFFF;
	uint32_t destAddressMask = 0x0FFFFFFF;

	uint32_t src = m_NDS9Channels[channel].internalSrc & srcAddressMask;
	uint32_t dest = m_NDS9Channels[channel].internalDest & destAddressMask;

	int numWords = m_NDS9Channels[channel].internalWordCount;
	if (numWords == 0)
		numWords = 0x20000;

	uint8_t srcAddressControl = (m_NDS9Channels[channel].control >> 7) & 0b11;
	uint8_t destAddressControl = (m_NDS9Channels[channel].control >> 5) & 0b11;
	bool repeat = (m_NDS9Channels[channel].control >> 9) & 0b1;
	bool wordTransfer = (m_NDS9Channels[channel].control >> 10) & 0b1;
	uint8_t startTiming = (m_NDS9Channels[channel].control >> 11) & 0b111;
	bool IRQ = (m_NDS9Channels[channel].control >> 14) & 0b1;
	bool reloadDest = false;
	static constexpr int offsetLUT[2] = { 2,4 };

	//i hate this hack....
	if (startTiming == 5)
		numWords = m_cartridge->getWordsLeft();

	for (int i = 0; i < numWords; i++)
	{
		if (wordTransfer)
		{
			src &= 0xFFFFFFFC;
			dest &= 0xFFFFFFFE;
			uint32_t val = NDS9_read32(src);
			NDS9_write32(dest, val);
		}
		else
		{
			src &= 0xFFFFFFFE;
			dest &= 0xFFFFFFFE;
			uint16_t val = NDS9_read16(src);
			NDS9_write16(dest, val);
		}

		switch (srcAddressControl)
		{
		case 0:
			src += offsetLUT[wordTransfer];
			break;
		case 1:
			src -= offsetLUT[wordTransfer];
			break;
		}

		switch (destAddressControl)
		{
		case 0:
			dest += offsetLUT[wordTransfer];
			break;
		case 1:
			dest -= offsetLUT[wordTransfer];
			break;
		case 3:
			dest += offsetLUT[wordTransfer];
			reloadDest = true;
			break;
		}
	}

	m_NDS9Channels[channel].internalSrc = src;
	m_NDS9Channels[channel].internalDest = dest;
	m_NDS9Channels[channel].internalWordCount = 0;	//maybe -1?

	if (IRQ)
	{
		static constexpr InterruptType interruptLUT[4] = { InterruptType::DMA0, InterruptType::DMA1, InterruptType::DMA2, InterruptType::DMA3 };
		m_interruptManager->NDS9_requestInterrupt(interruptLUT[channel]);
	}

	if (repeat && startTiming != 0)
	{
		if (reloadDest)
			m_NDS9Channels[channel].internalDest = m_NDS9Channels[channel].dest;
		m_NDS9Channels[channel].internalWordCount = m_NDS9Channels[channel].wordCount | ((m_NDS9Channels[channel].control & 0x1F) << 16);
	}
	else
		m_NDS9Channels[channel].control &= 0x7FFF;
}

//this is sort of slow, not super nice. could use bitmasks instead.
void Bus::NDS9DMA_onHBlank()
{
	for (int i = 0; i < 4; i++)
	{
		DMAChannel curChannel = m_NDS9Channels[i];
		bool channelEnabled = (curChannel.control >> 15) & 0b1;
		uint8_t startTiming = (curChannel.control >> 11) & 0b111;
		if (channelEnabled && (startTiming == 2))
			NDS9_doDMATransfer(i);
	}
}

void Bus::NDS9DMA_onVBlank()
{
	for (int i = 0; i < 4; i++)
	{
		DMAChannel curChannel = m_NDS9Channels[i];
		bool channelEnabled = (curChannel.control >> 15) & 0b1;
		uint8_t startTiming = (curChannel.control >> 11) & 0b111;
		if (channelEnabled && (startTiming == 1))
			NDS9_doDMATransfer(i);
	}
}

void Bus::NDS9DMA_onCartridge()
{
	for (int i = 0; i < 4; i++)
	{
		DMAChannel curChannel = m_NDS9Channels[i];
		bool channelEnabled = (curChannel.control >> 15) & 0b1;
		uint8_t startTiming = (curChannel.control >> 11) & 0b111;
		if (channelEnabled && (startTiming == 5))
			NDS9_doDMATransfer(i);
	}
}

void Bus::NDS9_HBlankDMACallback(void* context)
{
	Bus* thisPtr = (Bus*)context;
	thisPtr->NDS9DMA_onHBlank();
}

void Bus::NDS9_VBlankDMACallback(void* context)
{
	Bus* thisPtr = (Bus*)context;
	thisPtr->NDS9DMA_onVBlank();
}

void Bus::NDS9_CartridgeDMACallback(void* context)
{
	Bus* thisPtr = (Bus*)context;
	thisPtr->NDS9DMA_onCartridge();
}