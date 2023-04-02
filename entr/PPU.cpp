#include"PPU.h"

PPU::PPU(std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<Scheduler> scheduler)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;

	memset(m_renderBuffer, 0, 256 * 384 * 8);
	memset(m_safeDisplayBuffer, 0, 256 * 384 * 4);

	VCOUNT = 0;
	m_state = PPUState::HDraw;

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, 1606);
}

PPU::~PPU()
{

}

void PPU::updateDisplayOutput()
{
	memcpy(m_safeDisplayBuffer, m_renderBuffer[!pageIdx], 256 * 384 * sizeof(uint32_t));
}

void PPU::registerMemory(std::shared_ptr<NDSMem> mem)
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Registered memory with PPU!");
	m_mem = mem;
}

void PPU::eventHandler()
{
	switch (m_state)
	{
	case PPUState::HDraw:
		HDraw();
		break;
	case PPUState::HBlank:
		HBlank();
		break;
	case PPUState::VBlank:
		VBlank();
		break;
	}
}

void PPU::HDraw()
{
	//this is a hack, should fix!
	renderLCDCMode();

	//line=2130 cycles. hdraw=1606 cycles? hblank=524 cycles
	setHBlankFlag(true);
	m_state = PPUState::HBlank;
	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);
}

void PPU::HBlank()
{
	setHBlankFlag(false);
	VCOUNT++;
	//check vcount interrupt

	if (VCOUNT == 192)
	{
		//set vblank flag here, request vblank interrupt
		setVBlankFlag(true);
		m_state = PPUState::VBlank;

		pageIdx = !pageIdx;

		m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
		return;
	}
	else
		m_state = PPUState::HDraw;

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

void PPU::VBlank()
{
	if (!vblank_setHblankBit)
	{
		setHBlankFlag(true);
		vblank_setHblankBit = true;
		m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);
		return;
	}
	vblank_setHblankBit = false;
	setHBlankFlag(false);
	VCOUNT++;
	if (VCOUNT == 262)
	{
		setVBlankFlag(false);
		VCOUNT = 0;
		m_state = PPUState::HDraw;
	}
	//todo: weird behaviour of last line in vblank (same as in gba)

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

void PPU::renderLCDCMode()
{
	for (int i = 0; i < 256; i++)
	{
		uint32_t address = (VCOUNT * (256 * 2)) + (i * 2);
		uint8_t colLow = m_mem->VRAM[address];
		uint8_t colHigh = m_mem->VRAM[address + 1];
		uint16_t col = ((colHigh << 8) | colLow);
		m_renderBuffer[pageIdx][((VCOUNT * 256) + i)] = col16to32(col);
	}
}

uint8_t PPU::readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000000:
		return DISPCNT & 0xFF;
	case 0x04000001:
		return ((DISPCNT >> 8) & 0xFF);
	case 0x04000002:
		return ((DISPCNT >> 16) & 0xFF);
	case 0x04000003:
		return ((DISPCNT >> 24) & 0xFF);
	case 0x04000004:
		return DISPSTAT & 0xFF;
	case 0x04000005:
		return ((DISPSTAT >> 8) & 0xFF);
	case 0x04000006:
		return VCOUNT & 0xFF;
	case 0x04000007:
		return ((VCOUNT >> 8) & 0xFF);
	}
	Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO read! addr={:#x}", address));
	return 0;
}

void PPU::writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000000:
		DISPCNT &= 0xFFFFFF00; DISPCNT |= value;
		break;
	case 0x04000001:
		DISPCNT &= 0xFFFF00FF; DISPCNT |= (value << 8);
		break;
	case 0x04000002:
		DISPCNT &= 0xFF00FFFF; DISPCNT |= (value << 16);
		break;
	case 0x04000003:
		DISPCNT &= 0x00FFFFFF; DISPCNT |= (value << 24);
		break;
	case 0x04000004:
		DISPSTAT &= 0xFF07; DISPSTAT |= (value & 0xF8);
		break;
	case 0x04000005:
		DISPSTAT &= 0x00FF; DISPSTAT |= (value << 8);
		break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Warn, std::format("Unimplemented PPU IO write! addr={:#x} val={:#x}", address, value));
	}
}

uint8_t PPU::NDS7_readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000004:
		return NDS7_DISPSTAT & 0xFF;
	case 0x04000005:
		return ((NDS7_DISPSTAT >> 8) & 0xFF);
	case 0x04000006:
		return VCOUNT & 0xFF;
	case 0x04000007:
		return ((VCOUNT >> 8) & 0xFF);
	}
}

void PPU::NDS7_writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000004:
		NDS7_DISPSTAT &= 0xFF07; NDS7_DISPSTAT |= (value & 0xF8);
		break;
	case 0x04000005:
		NDS7_DISPSTAT &= 0x00FF; NDS7_DISPSTAT |= (value << 8);
		break;
	}
}

void PPU::setVBlankFlag(bool value)
{
	DISPSTAT &= ~0b1;
	DISPSTAT |= value;
	NDS7_DISPSTAT &= ~0b1;
	NDS7_DISPSTAT |= value;
}

void PPU::setHBlankFlag(bool value)
{
	DISPSTAT &= ~0b10;
	DISPSTAT |= (value << 1);
	NDS7_DISPSTAT &= ~0b10;
	NDS7_DISPSTAT |= (value << 1);
}

void PPU::setVCounterFlag(bool value)
{
	DISPSTAT &= ~0b100;
	DISPSTAT |= (value << 2);
	NDS7_DISPSTAT &= ~0b100;
	NDS7_DISPSTAT |= (value << 2);
}

uint32_t PPU::col16to32(uint16_t col)
{
	int red = (col & 0b0000000000011111);
	red = (red << 3) | (red >> 2);
	int green = (col & 0b0000001111100000) >> 5;
	green = (green << 3) | (green >> 2);
	int blue = (col & 0b0111110000000000) >> 10;
	blue = (blue << 3) | (blue >> 2);

	return (red << 24) | (green << 16) | (blue << 8) | 0x000000FF;
}

void PPU::onSchedulerEvent(void* context)
{
	PPU* thisPtr = (PPU*)context;
	thisPtr->eventHandler();
}

uint32_t PPU::m_safeDisplayBuffer[256 * 384] = {};