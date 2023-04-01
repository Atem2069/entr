#include"ARM946E.h"

ARM946E::ARM946E(uint32_t entry, std::shared_ptr<Bus> bus, std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<Scheduler> scheduler)
{
	m_bus = bus;
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;
	CPSR = 0x1F;				//we're direct booting, so maybe starts in sys or whatever?
	m_lastCheckModeBits = 0x1F;
	for (int i = 0; i < 16; i++)
		R[i] = 0;

	R[15] = entry + 8;
	m_pipelineFlushed = false;
}

ARM946E::~ARM946E()
{

}

consteval std::array<ARM946E::instructionFn, 4096> ARM946E::genARMTable()
{
	std::array<instructionFn, 4096> armTable;
	armTable.fill((instructionFn)&ARM946E::ARM_Undefined);
	//bypass compiler recursion limit by splitting up into 256 long chunks of filling the table
	setARMTableEntries<0, 256>(armTable);
	setARMTableEntries<256, 512>(armTable);
	setARMTableEntries<512, 768>(armTable);
	setARMTableEntries<768, 1024>(armTable);
	setARMTableEntries<1024, 1280>(armTable);
	setARMTableEntries<1280, 1536>(armTable);
	setARMTableEntries<1536, 1792>(armTable);
	setARMTableEntries<1792, 2048>(armTable);
	setARMTableEntries<2048, 2304>(armTable);
	setARMTableEntries<2304, 2560>(armTable);
	setARMTableEntries<2560, 2816>(armTable);
	setARMTableEntries<2816, 3072>(armTable);
	setARMTableEntries<3072, 3328>(armTable);
	setARMTableEntries<3328, 3584>(armTable);
	setARMTableEntries<3584, 3840>(armTable);
	setARMTableEntries<3840, 4096>(armTable);

	return armTable;
}

consteval std::array<ARM946E::instructionFn, 1024> ARM946E::genThumbTable()
{
	std::array<instructionFn, 1024> thumbTable;
	thumbTable.fill((instructionFn)&ARM946E::ARM_Undefined);
	setThumbTableEntries<0, 256>(thumbTable);
	setThumbTableEntries<256, 512>(thumbTable);
	setThumbTableEntries<512, 768>(thumbTable);
	setThumbTableEntries<768, 1024>(thumbTable);
	return thumbTable;
}

void ARM946E::run(int cycles)
{
	for (int i = 0; i < cycles; i++)
	{
		if (dispatchInterrupt())
			return;
		fetch();
		switch (m_inThumbMode)
		{
		case 0:
			executeARM(); break;
		case 1:
			executeThumb(); break;
		}

		if (!m_pipelineFlushed)
		{
			R[15] += incrAmountLUT[m_inThumbMode];
		}
		m_pipelineFlushed = false;
	}
}

void ARM946E::fetch()
{
	if (m_inThumbMode)
		m_currentOpcode = m_fetch16(R[15] - 4);
	else
		m_currentOpcode = m_fetch32(R[15] - 8);
}

void ARM946E::executeARM()
{
	//check conditions before executing
	uint8_t conditionCode = ((m_currentOpcode >> 28) & 0xF);
	static constexpr auto conditionLUT = genConditionCodeTable();
	bool conditionMet = (conditionLUT[(CPSR >> 28) & 0xF] >> conditionCode) & 0b1;
	if (conditionMet) [[likely]]
	{
		static constexpr auto armTable = genARMTable();
		uint32_t lookup = ((m_currentOpcode & 0x0FF00000) >> 16) | ((m_currentOpcode & 0xF0) >> 4);	//bits 20-27 shifted down to bits 4-11. bits 4-7 shifted down to bits 0-4
		instructionFn instr = armTable[lookup];
		(this->*instr)();
	}
}

void ARM946E::executeThumb()
{
	static constexpr auto thumbTable = genThumbTable();
	uint16_t lookup = m_currentOpcode >> 6;
	instructionFn instr = thumbTable[lookup];
	(this->*instr)();
}

bool ARM946E::dispatchInterrupt()
{
	
	if (((CPSR>>7)&0b1) || !m_interruptManager->NDS9_getInterrupt() || !m_interruptManager->NDS9_getInterruptsEnabled())
		return false;	//only dispatch if pipeline full (or not about to flush)
	//irq bits: 10010
	uint32_t oldCPSR = CPSR;
	CPSR &= ~0x3F;
	CPSR |= 0x92;
	m_inThumbMode = false;
	swapBankedRegisters();

	bool wasThumb = ((oldCPSR >> 5) & 0b1);
	constexpr int pcOffsetAmount[2] = { 4,0 };
	setSPSR(oldCPSR);
	setReg(14, getReg(15) - pcOffsetAmount[wasThumb]);
	setReg(15, 0xFFFF0018);	//ARM9 uses high interrupt vector?
	return true;
}


//internal r/w functions
uint16_t ARM946E::m_fetch16(uint32_t address)
{
	address &= 0xFFFFFFFE;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && (ITCM_enabled && !ITCM_load))
		return Bus::getValue16(ITCM, address & 0x7FFF, 0x7FFF);
	return m_bus->NDS9_read16(address);
}

uint32_t ARM946E::m_fetch32(uint32_t address)
{
	address &= 0xFFFFFFFC;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && (ITCM_enabled && !ITCM_load))
		return Bus::getValue32(ITCM, address & 0x7FFF, 0x7FFF);
	return m_bus->NDS9_read32(address);
}

uint8_t ARM946E::m_read8(uint32_t address)
{
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && (ITCM_enabled && !ITCM_load))
		return ITCM[address & 0x7FFF];
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && (DTCM_enabled && !DTCM_load))
		return DTCM[address & 0x3FFF];
	return m_bus->NDS9_read8(address);
}

uint16_t ARM946E::m_read16(uint32_t address)
{
	address &= 0xFFFFFFFE;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && (ITCM_enabled && !ITCM_load))
		return Bus::getValue16(ITCM, address & 0x7FFF, 0x7FFF);
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && (DTCM_enabled && !DTCM_load))
		return Bus::getValue16(DTCM, address & 0x3FFF, 0x3FFF);
	return m_bus->NDS9_read16(address);
}

uint32_t ARM946E::m_read32(uint32_t address)
{
	address &= 0xFFFFFFFC;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && (ITCM_enabled && !ITCM_load))
		return Bus::getValue32(ITCM, address & 0x7FFF, 0x7FFF);
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && (DTCM_enabled && !DTCM_load))
		return Bus::getValue32(DTCM, address & 0x3FFF, 0x3FFF);
	return m_bus->NDS9_read32(address);
}

void ARM946E::m_write8(uint32_t address, uint8_t value)
{
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && ITCM_enabled)
	{
		ITCM[address & 0x7FFF] = value;
		return;
	}
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && DTCM_enabled)
	{
		DTCM[address & 0x3FFF] = value;
		return;
	}
	m_bus->NDS9_write8(address, value);
}

void ARM946E::m_write16(uint32_t address, uint16_t value)
{
	address &= 0xFFFFFFFE;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && ITCM_enabled)
	{
		Bus::setValue16(ITCM, address & 0x7FFF, 0x7FFF, value);
		return;
	}
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && DTCM_enabled)
	{
		Bus::setValue16(DTCM, address & 0x3FFF, 0x3FFF, value);
		return;
	}
	m_bus->NDS9_write16(address, value);
}

void ARM946E::m_write32(uint32_t address, uint32_t value)
{
	address &= 0xFFFFFFFC;
	if (address >= ITCM_Base && address < (ITCM_Base + ITCM_Size) && ITCM_enabled)
	{
		Bus::setValue32(ITCM, address & 0x7FFF, 0x7FFF, value);
		return;
	}
	if (address >= DTCM_Base && address < (DTCM_Base + DTCM_Size) && DTCM_enabled)
	{
		Bus::setValue32(DTCM, address & 0x3FFF, 0x3FFF, value);
		return;
	}
	m_bus->NDS9_write32(address, value);
}


//misc flag stuff
bool ARM946E::m_getNegativeFlag()
{
	return (CPSR >> 31) & 0b1;
}

bool ARM946E::m_getZeroFlag()
{
	return (CPSR >> 30) & 0b1;
}

bool ARM946E::m_getCarryFlag()
{
	return (CPSR >> 29) & 0b1;
}

bool ARM946E::m_getOverflowFlag()
{
	return (CPSR >> 28) & 0b1;
}

void ARM946E::m_setNegativeFlag(bool value)
{
	constexpr uint32_t mask = (1 << 31);
	CPSR &= ~(mask);
	CPSR |= (mask & (value << 31));
}

void ARM946E::m_setZeroFlag(bool value)
{
	constexpr uint32_t mask = (1 << 30);
	CPSR &= ~(mask);
	CPSR |= (mask & (value << 30));
}

void ARM946E::m_setCarryFlag(bool value)
{
	constexpr uint32_t mask = (1 << 29);
	CPSR &= ~(mask);
	CPSR |= (mask & (value << 29));
}

void ARM946E::m_setOverflowFlag(bool value)
{
	constexpr uint32_t mask = (1 << 28);
	CPSR &= ~(mask);
	CPSR |= (mask & (value << 28));
}

void ARM946E::m_setSaturationFlag()
{
	CPSR |= (1 << 27);
}

uint32_t ARM946E::getReg(uint8_t reg)
{
	return R[reg];
}

void ARM946E::setReg(uint8_t reg, uint32_t value)
{
	R[reg] = value;
	if (reg == 15)
	{
		m_pipelineFlushed = true;
		R[reg] += (incrAmountLUT[m_inThumbMode] << 1);
	}
}

void ARM946E::swapBankedRegisters()
{
	uint8_t oldMode = m_lastCheckModeBits;
	uint8_t newMode = CPSR & 0x1F;
	m_lastCheckModeBits = newMode;

	if ((oldMode == newMode) || (oldMode == 0b10000 && newMode == 0b11111) || (oldMode == 0b11111 && newMode == 0b10000))
		return;

	//first, save registers back to correct bank
	uint32_t* srcPtr = nullptr;
	switch (oldMode)
	{
	case 0b10000:
		srcPtr = usrBankedRegisters; break;
	case 0b10001:
		srcPtr = fiqBankedRegisters;
		memcpy(fiqExtraBankedRegisters, &R[8], 5 * sizeof(uint32_t));	//save FIQ banked R8-R12
		R[8] = usrExtraBankedRegisters[0];          //then load original R8-R12 before banking
		R[9] = usrExtraBankedRegisters[1];
		R[10] = usrExtraBankedRegisters[2];
		R[11] = usrExtraBankedRegisters[3];
		R[12] = usrExtraBankedRegisters[4];
		break;
	case 0b10010:
		srcPtr = irqBankedRegisters; break;
	case 0b10011:
		srcPtr = svcBankedRegisters; break;
	case 0b10111:
		srcPtr = abtBankedRegisters; break;
	case 0b11011:
		srcPtr = undBankedRegisters; break;
	case 0b11111:
		srcPtr = usrBankedRegisters; break;
	default:
		srcPtr = usrBankedRegisters; break;
	}

	memcpy(srcPtr, &R[13], 2 * sizeof(uint32_t));

	uint32_t* destPtr = nullptr;
	switch (newMode)
	{
	case 0b10000:
		destPtr = usrBankedRegisters; break;
	case 0b10001:
		destPtr = fiqBankedRegisters;
		//save R8-R12
		usrExtraBankedRegisters[0] = R[8];
		usrExtraBankedRegisters[1] = R[9];
		usrExtraBankedRegisters[2] = R[10];
		usrExtraBankedRegisters[3] = R[11];
		usrExtraBankedRegisters[4] = R[12];
		//then load in banked R8-R12
		memcpy(&R[8], fiqExtraBankedRegisters, 5 * sizeof(uint32_t));
		break;
	case 0b10010:
		destPtr = irqBankedRegisters; break;
	case 0b10011:
		destPtr = svcBankedRegisters; break;
	case 0b10111:
		destPtr = abtBankedRegisters; break;
	case 0b11011:
		destPtr = undBankedRegisters; break;
	case 0b11111:
		destPtr = usrBankedRegisters; break;
	default:
		Logger::getInstance()->msg(LoggerSeverity::Error, std::format("Invalid destination mode: {:#x}", (int)newMode));
		destPtr = usrBankedRegisters;
		break;
	}

	memcpy(&R[13], destPtr, 2 * sizeof(uint32_t));
}

uint32_t ARM946E::getSPSR()
{
	uint8_t mode = CPSR & 0x1F;
	switch (mode)
	{
	case 0b10001: return SPSR_fiq;
	case 0b10011: return SPSR_svc;
	case 0b10111: return SPSR_abt;
	case 0b10010: return SPSR_irq;
	case 0b11011: return SPSR_und;
	};
	return CPSR;	//afaik if you try this in the wrong mode it gives you the CPSR
}

void ARM946E::setSPSR(uint32_t value)
{
	uint8_t mode = CPSR & 0x1F;
	switch (mode)
	{
	case 0b10001: SPSR_fiq = value; break;
	case 0b10011: SPSR_svc = value; break;
	case 0b10111: SPSR_abt = value; break;
	case 0b10010: SPSR_irq = value; break;
	case 0b11011: SPSR_und = value; break;
	default:break;
	}
}

int ARM946E::calculateMultiplyCycles(uint32_t operand, bool isSigned)
{
	int totalCycles = 4;
	if (isSigned)
	{
		if ((operand >> 8) == 0xFFFFFF)
			totalCycles = 1;
		else if ((operand >> 16) == 0xFFFF)
			totalCycles = 2;
		else if ((operand >> 24) == 0xFF)
			totalCycles = 3;
	}

	if ((operand >> 8) == 0)
		totalCycles = 1;
	else if ((operand >> 16) == 0)
		totalCycles = 2;
	else if ((operand >> 24) == 0)
		totalCycles = 3;
	return totalCycles;
}

uint32_t ARM946E::doSaturatingAdd(int64_t a, int64_t b)
{
	static constexpr int64_t I32_MAX = 0x7FFFFFFF;
	static constexpr int64_t I32_MIN = 0xFFFFFFFF80000000;
	if (b > I32_MAX)
	{
		b = I32_MAX;
		m_setSaturationFlag();
	}
	if (b < I32_MIN)
	{
		b = I32_MIN;
		m_setSaturationFlag();
	}
	
	int64_t result = a + b;

	if (result >= I32_MIN && result <= I32_MAX)
		return (uint32_t)result;
	else if (result > I32_MAX)
	{
		m_setSaturationFlag();
		return 0x7FFFFFFF;
	}
	else if (result < I32_MIN)
	{
		m_setSaturationFlag();
		return 0x80000000;
	}
}

uint32_t ARM946E::doSaturatingSub(int64_t a, int64_t b)
{
	static constexpr int64_t I32_MAX = 0x7FFFFFFF;
	static constexpr int64_t I32_MIN = 0xFFFFFFFF80000000;
	if (b > I32_MAX)
	{
		b = I32_MAX;
		m_setSaturationFlag();
	}
	if (b < I32_MIN)
	{
		b = I32_MIN;
		m_setSaturationFlag();
	}

	int64_t result = a - b;

	if (result >= I32_MIN && result <= I32_MAX)
		return (uint32_t)result;
	else if (result > I32_MAX)
	{
		m_setSaturationFlag();
		return 0x7FFFFFFF;
	}
	else if (result < I32_MIN)
	{
		m_setSaturationFlag();
		return 0x80000000;
	}
}