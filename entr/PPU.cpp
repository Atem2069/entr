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
	//todo

	//line=2130 cycles. hdraw=1606 cycles? hblank=524 cycles
	//set HBlank flag  here
	m_state = PPUState::HBlank;
	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);
}

void PPU::HBlank()
{
	//unset HBlank flag here
	VCOUNT++;
	//check vcount interrupt

	if (VCOUNT == 192)
	{
		//set vblank flag here, request vblank interrupt
		m_state = PPUState::VBlank;
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
		//set hblank flag here, maybe trigger hblank irq
		vblank_setHblankBit = true;
		m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 523);
		return;
	}
	vblank_setHblankBit = false;
	//unset hblank bit here
	VCOUNT++;
	if (VCOUNT == 262)
	{
		//unset vblank bit
		VCOUNT = 0;
		m_state = PPUState::HDraw;
	}
	//todo: weird behaviour of last line in vblank (same as in gba)

	m_scheduler->addEvent(Event::PPU, &PPU::onSchedulerEvent, (void*)this, m_scheduler->getEventTime() + 1607);
}

uint8_t PPU::readIO(uint32_t address)
{
	//todo
}

void PPU::writeIO(uint32_t address, uint8_t value)
{
	//todo
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