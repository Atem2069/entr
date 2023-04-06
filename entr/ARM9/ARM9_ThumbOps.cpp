#include"ARM946E.h"


//start of Thumb instruction set
void ARM946E::Thumb_MoveShiftedRegister()
{
	uint8_t operation = ((m_currentOpcode >> 11) & 0b11);
	uint8_t shiftAmount = ((m_currentOpcode >> 6) & 0b11111);
	uint8_t srcRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t destRegIdx = m_currentOpcode & 0b111;

	uint32_t srcVal = R[srcRegIdx];
	uint32_t result = 0;
	int carry = -1;
	switch (operation)
	{
	case 0: result = LSL(srcVal, shiftAmount, carry); break;
	case 1: result = LSR(srcVal, shiftAmount, carry); break;
	case 2: result = ASR(srcVal, shiftAmount, carry); break;
	}

	setLogicalFlags(result, carry);
	R[destRegIdx] = result;
}

void ARM946E::Thumb_AddSubtract()
{
	uint8_t destRegIndex = m_currentOpcode & 0b111;
	uint8_t srcRegIndex = ((m_currentOpcode >> 3) & 0b111);
	uint8_t op = ((m_currentOpcode >> 9) & 0b1);
	bool immediate = ((m_currentOpcode >> 10) & 0b1);

	uint32_t operand1 = R[srcRegIndex];
	uint32_t operand2 = 0;
	uint32_t result = 0;

	if (immediate)
		operand2 = ((m_currentOpcode >> 6) & 0b111);
	else
	{
		uint8_t tmp = ((m_currentOpcode >> 6) & 0b111);
		operand2 = R[tmp];
	}

	switch (op)
	{
	case 0:
		result = operand1 + operand2;
		R[destRegIndex] = result;
		setArithmeticFlags(operand1, operand2, result, true);
		break;
	case 1:
		result = operand1 - operand2;
		R[destRegIndex] = result;
		setArithmeticFlags(operand1, operand2, result, false);
		break;
	}
}

void ARM946E::Thumb_MoveCompareAddSubtractImm()
{
	uint32_t offset = m_currentOpcode & 0xFF;
	uint8_t srcDestRegIdx = ((m_currentOpcode >> 8) & 0b111);
	uint8_t operation = ((m_currentOpcode >> 11) & 0b11);

	uint32_t operand1 = R[srcDestRegIdx];
	uint32_t result = 0;

	switch (operation)
	{
	case 0:
		result = offset;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	case 1:
		result = operand1 - offset;
		setArithmeticFlags(operand1, offset, result, false);
		break;
	case 2:
		result = operand1 + offset;
		R[srcDestRegIdx] = result;
		setArithmeticFlags(operand1, offset, result, true);
		break;
	case 3:
		result = operand1 - offset;
		R[srcDestRegIdx] = result;
		setArithmeticFlags(operand1, offset, result, false);
		break;
	}
}

void ARM946E::Thumb_ALUOperations()
{
	uint8_t srcDestRegIdx = m_currentOpcode & 0b111;
	uint8_t op2RegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t operation = ((m_currentOpcode >> 6) & 0xF);

	uint32_t operand1 = R[srcDestRegIdx];
	uint32_t operand2 = R[op2RegIdx];
	uint32_t result = 0;

	int tempCarry = -1;
	uint32_t carryIn = m_getCarryFlag() & 0b1;

	switch (operation)
	{
	case 0:	//AND
		result = operand1 & operand2;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	case 1:	//EOR
		result = operand1 ^ operand2;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	case 2:	//LSL
		if (operand2)
			result = LSL(operand1, operand2, tempCarry);
		else
			result = operand1;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, tempCarry);
		break;
	case 3:	//LSR
		if (operand2)
			result = LSR(operand1, operand2, tempCarry);
		else
			result = operand1;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, tempCarry);
		break;
	case 4:	//ASR
		if (operand2)
			result = ASR(operand1, operand2, tempCarry);
		else
			result = operand1;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, tempCarry);
		break;
	case 5:	//ADC
		result = operand1 + operand2 + carryIn;
		R[srcDestRegIdx] = result;
		setArithmeticFlags(operand1, operand2, result, true);
		break;
	case 6:	//SBC
		result = operand1 - operand2 - (!carryIn);
		R[srcDestRegIdx] = result;
		setArithmeticFlags(operand1, (uint64_t)operand2 + (uint64_t)(!carryIn), result, false);	//<--this is sussy...
		break;
	case 7:	//ROR
		operand2 &= 0xFF;
		tempCarry = -1;
		if (operand2)
			result = ROR(operand1, operand2, tempCarry);
		else
			result = operand1;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, tempCarry);
		break;
	case 8:	//TST
		result = operand1 & operand2;
		setLogicalFlags(result, -1);
		break;
	case 9:	//NEG
		result = (~operand2) + 1;
		R[srcDestRegIdx] = result;
		setArithmeticFlags(0, operand2, result, false);	//not sure about this
		break;
	case 10: //CMP
		result = operand1 - operand2;
		setArithmeticFlags(operand1, operand2, result, false);
		break;
	case 11: //CMN
		result = operand1 + operand2;
		setArithmeticFlags(operand1, operand2, result, true);
		break;
	case 12: //ORR
		result = operand1 | operand2;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	case 13: //MUL
		result = operand1 * operand2;
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);	//hmm...
		break;
	case 14: //BIC
		result = operand1 & (~operand2);
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	case 15: //MVN
		result = (~operand2);
		R[srcDestRegIdx] = result;
		setLogicalFlags(result, -1);
		break;
	}
}

void ARM946E::Thumb_HiRegisterOperations()
{
	uint8_t srcRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t dstRegIdx = m_currentOpcode & 0b111;
	uint8_t operation = ((m_currentOpcode >> 8) & 0b11);

	if ((m_currentOpcode >> 7) & 0b1)	//set them as 'high' registers
		dstRegIdx += 8;
	if ((m_currentOpcode >> 6) & 0b1)
		srcRegIdx += 8;

	uint32_t operand1 = getReg(dstRegIdx);	//check this? arm docs kinda confusing to read
	uint32_t operand2 = getReg(srcRegIdx);
	uint32_t result = 0;

	switch (operation)
	{
	case 0:
		result = operand1 + operand2;
		setReg(dstRegIdx, result);
		if (dstRegIdx == 15)
		{
			setReg(dstRegIdx, result & ~0b1);
		}
		break;
	case 1:
		result = operand1 - operand2;
		setArithmeticFlags(operand1, operand2, result, false);
		break;
	case 2:
		result = operand2;
		setReg(dstRegIdx, result);
		if (dstRegIdx == 15)
		{
			setReg(dstRegIdx, result & ~0b1);
		}
		break;
	case 3:
	{
		uint32_t oldPC = getReg(15);
		if (!(operand2 & 0b1))
		{
			//enter arm
			CPSR &= 0xFFFFFFDF;	//unset T bit
			m_inThumbMode = false;
			operand2 &= ~0b11;
			setReg(15, operand2);
		}
		else
		{
			//stay in thumb
			operand2 &= ~0b1;
			setReg(15, operand2);
		}
		bool link = ((m_currentOpcode >> 7) & 0b1);
		if (link)
			setReg(14, (oldPC&~0b1)-1);
		break;
	}
	}
}

void ARM946E::Thumb_PCRelativeLoad()
{
	uint32_t offset = (m_currentOpcode & 0xFF) << 2;
	uint32_t PC = R[15] & ~0b11;	//PC is force aligned to word boundary

	uint8_t destRegIdx = ((m_currentOpcode >> 8) & 0b111);

	uint32_t val = m_read32(PC + offset);
	R[destRegIdx] = val;
}

void ARM946E::Thumb_LoadStoreRegisterOffset()
{
	bool loadStore = ((m_currentOpcode >> 11) & 0b1);
	bool byteWord = ((m_currentOpcode >> 10) & 0b1);
	uint8_t offsetRegIdx = ((m_currentOpcode >> 6) & 0b111);
	uint8_t baseRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t srcDestRegIdx = m_currentOpcode & 0b111;

	uint32_t base = R[baseRegIdx];
	base += R[offsetRegIdx];

	if (loadStore)	//load
	{
		if (byteWord)
		{
			uint32_t val = m_read8(base);
			R[srcDestRegIdx] = val;
		}
		else
		{
			uint32_t val = m_read32(base);
			if (base & 3)
				val = std::rotr(val, (base & 3) * 8);
			R[srcDestRegIdx] = val;
		}
	}
	else			//store
	{
		if (byteWord)
		{
			uint8_t val = R[srcDestRegIdx] & 0xFF;
			m_write8(base, val);
		}
		else
		{
			uint32_t val = R[srcDestRegIdx];
			m_write32(base, val);
		}
	}
}

void ARM946E::Thumb_LoadStoreSignExtended()
{
	uint8_t op = ((m_currentOpcode >> 10) & 0b11);
	uint8_t offsetRegIdx = ((m_currentOpcode >> 6) & 0b111);
	uint8_t baseRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t srcDestRegIdx = m_currentOpcode & 0b111;

	uint32_t addr = R[baseRegIdx] + R[offsetRegIdx];


	if (op == 0)
	{
		uint16_t val = R[srcDestRegIdx] & 0xFFFF;
		m_write16(addr, val);
	}
	else if (op == 2)	//load halfword
	{
		uint32_t val = m_read16(addr);
		R[srcDestRegIdx] = val;
	}
	else if (op == 1)	//load sign extended byte
	{
		uint32_t val = m_read8(addr);
		if (((val >> 7) & 0b1))
			val |= 0xFFFFFF00;
		R[srcDestRegIdx] = val;
	}
	else if (op == 3)   //load sign extended halfword
	{
		uint32_t val = 0;
		if (!(addr & 0b1))
		{
			val = m_read16(addr);
			if (((val >> 15) & 0b1))
				val |= 0xFFFF0000;
		}
		else
		{
			val = m_read8(addr);
			if (((val >> 7) & 0b1))
				val |= 0xFFFFFF00;
		}
		R[srcDestRegIdx] = val;
	}
}

void ARM946E::Thumb_LoadStoreImmediateOffset()
{
	bool byteWord = ((m_currentOpcode >> 12) & 0b1);
	bool loadStore = ((m_currentOpcode >> 11) & 0b1);
	uint32_t offset = ((m_currentOpcode >> 6) & 0b11111);
	uint8_t baseRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t srcDestRegIdx = m_currentOpcode & 0b111;

	uint32_t baseAddr = R[baseRegIdx];
	if (!byteWord)		//if word, then it's a 7 bit address and word aligned so shl by 2
		offset <<= 2;
	baseAddr += offset;

	if (loadStore)	//Load value from memory
	{
		uint32_t val = 0;
		if (byteWord)
			val = m_read8(baseAddr);
		else
		{
			val = m_read32(baseAddr);
			if (baseAddr & 3)
				val = std::rotr(val, (baseAddr & 3) * 8);
		}
		R[srcDestRegIdx] = val;
	}
	else			//Store value to memory
	{
		uint32_t val = R[srcDestRegIdx];
		if (byteWord)
			m_write8(baseAddr, val & 0xFF);
		else
			m_write32(baseAddr, val);
	}
}

void ARM946E::Thumb_LoadStoreHalfword()
{
	bool loadStore = ((m_currentOpcode >> 11) & 0b1);
	uint32_t offset = ((m_currentOpcode >> 6) & 0x1F);
	uint8_t baseRegIdx = ((m_currentOpcode >> 3) & 0b111);
	uint8_t srcDestRegIdx = ((m_currentOpcode) & 0b111);

	offset <<= 1;	//6 bit address, might be sign extended? probs not

	uint32_t base = R[baseRegIdx];
	base += offset;

	if (loadStore)
	{
		uint32_t val = m_read16(base);
		R[srcDestRegIdx] = val;
	}
	else
	{
		uint16_t val = R[srcDestRegIdx] & 0xFFFF;
		m_write16(base, val);
	}
}

void ARM946E::Thumb_SPRelativeLoadStore()
{
	bool loadStore = ((m_currentOpcode >> 11) & 0b1);
	uint8_t destRegIdx = ((m_currentOpcode >> 8) & 0b111);
	uint32_t offs = m_currentOpcode & 0xFF;
	offs <<= 2;

	uint32_t addr = getReg(13) + offs;

	if (loadStore)
	{
		uint32_t val = m_read32(addr);
		if (addr & 3)
			val = std::rotr(val, (addr & 3) * 8);
		R[destRegIdx] = val;
	}
	else
	{
		uint32_t val = R[destRegIdx];
		m_write32(addr, val);
	}
}

void ARM946E::Thumb_LoadAddress()
{
	uint32_t offset = m_currentOpcode & 0xFF;
	offset <<= 2;	//probs dont have to sign extend?

	uint8_t destRegIdx = ((m_currentOpcode >> 8) & 0b111);
	bool useSP = ((m_currentOpcode >> 11) & 0b1);

	if (useSP)	//SP used as base
	{
		uint32_t SP = getReg(13);
		SP += offset;
		R[destRegIdx] = SP;
	}
	else		//PC used as base
	{
		uint32_t PC = R[15] & ~0b11;
		PC += offset;
		R[destRegIdx] = PC;
	}
}

void ARM946E::Thumb_AddOffsetToStackPointer()
{
	uint32_t offset = m_currentOpcode & 0b1111111;
	offset <<= 2;
	uint32_t SP = getReg(13);
	if (((m_currentOpcode >> 7) & 0b1))
		SP -= offset;
	else
		SP += offset;

	setReg(13, SP);
}

void ARM946E::Thumb_PushPopRegisters()
{
	bool loadStore = ((m_currentOpcode >> 11) & 0b1);
	bool PCLR = ((m_currentOpcode >> 8) & 0b1);	//couldnt think of good abbreviation :/
	uint32_t regs = m_currentOpcode & 0xFF;

	uint32_t SP = getReg(13);

	bool firstTransfer = true;
	int transferCount = 0;

	if (loadStore) //Load - i.e. pop from stack
	{
		for (int i = 0; i < 8; i++)
		{
			if (((regs >> i) & 0b1))
			{
				transferCount++;
				uint32_t popVal = m_read32(SP);
				R[i] = popVal;
				SP += 4;
				firstTransfer = false;
			}
		}

		if (PCLR)
		{
			uint32_t newPC = m_read32(SP);
			//setReg(15, newPC & ~0b1);
			if (newPC & 0b1)	//stay in thumb mode, no change
				setReg(15, newPC & ~0b1);
			else				//switch out to ARM mode
			{
				CPSR &= ~0b100000;
				m_inThumbMode = false;
				setReg(15, newPC & ~0b11);
			}
			SP += 4;
		}
	}
	else          //Store - i.e. push to stack
	{

		if (PCLR)
		{
			SP -= 4;
			m_write32(SP, getReg(14));
			firstTransfer = false;
		}

		for (int i = 7; i >= 0; i--)
		{
			if (((regs >> i) & 0b1))
			{
				transferCount++;
				SP -= 4;
				m_write32(SP, R[i]);
				firstTransfer = false;
			}
		}
	}

	setReg(13, SP);
}

void ARM946E::Thumb_MultipleLoadStore()
{
	bool loadStore = (m_currentOpcode >> 11) & 0b1;
	uint8_t baseRegIdx = ((m_currentOpcode >> 8) & 0b111);
	uint8_t regList = m_currentOpcode & 0xFF;

	bool writeback = true;								//writeback implied, except for some odd LDM behaviour
	bool baseIsFirst = false;
	int transferCount = 0;

	for (int i = 0; i < 8; i++)
	{
		if ((regList >> i) & 0b1)
		{
			transferCount++;
			if (transferCount == 1 && (i == baseRegIdx))	//base reg first to be transferred? 
				baseIsFirst = true;
		}
	}

	uint32_t base = R[baseRegIdx];
	uint32_t finalBase = base + (transferCount * 4);	//figure out final val of base address

	bool firstAccess = true;
	for (int i = 0; i < 8; i++)
	{
		if ((regList >> i) & 0b1)
		{
			if (loadStore)
			{
				uint32_t val = m_read32(base);
				R[i] = val;
				if (i == baseRegIdx)		//load with base included -> no writeback
					writeback = false;
			}

			else
			{
				uint32_t val = R[i];
				if (i == baseRegIdx && !baseIsFirst)
					val = finalBase;
				m_write32(base, val);
			}
			firstAccess = false;
			base += 4;
		}
	}

	if (transferCount)
	{
		int totalCycles = transferCount + ((loadStore) ? 2 : 1);
	}
	else
	{
		if (loadStore)
		{
			//not sure about this timing.
			setReg(15, m_read32(base));
		}
		else
		{
			m_write32(base, R[15] + 2);	//+2 for pipeline effect
		}
		R[baseRegIdx] = base + 0x40;
		writeback = false;
	}

	if (writeback)
		R[baseRegIdx] = finalBase;
}

void ARM946E::Thumb_ConditionalBranch()
{
	uint32_t offset = m_currentOpcode & 0xFF;
	offset <<= 1;
	if (((offset >> 8) & 0b1))	//sign extend (FFFFFF00 shouldn't matter bc bit 8 should be a 1 anyway)
		offset |= 0xFFFFFF00;

	uint8_t condition = ((m_currentOpcode >> 8) & 0xF);
	if (condition == 14 || condition == 15)
		Logger::getInstance()->msg(LoggerSeverity::Error, "Invalid condition code - opcode decoding is likely wrong!!");
	static constexpr auto conditionTable = genConditionCodeTable();
	bool conditionMet = (conditionTable[(CPSR >> 28) & 0xF] >> condition) & 0b1;
	if (!conditionMet)
	{
		return;
	}

	setReg(15, R[15] + offset);
}

void ARM946E::Thumb_SoftwareInterrupt()
{
	//std::cout << "thumb swi" << (int)(m_currentOpcode&0xFF) << '\n';
	int swiId = m_currentOpcode & 0xFF;
	//svc mode bits are 10011
	uint32_t oldCPSR = CPSR;
	uint32_t oldPC = R[15] - 2;	//-2 because it points to next instruction

	CPSR &= 0xFFFFFFE0;	//clear mode bits (0-4)
	CPSR &= ~0b100000;	//clear T bit
	CPSR |= 0b10010011;	//set svc bits
	m_inThumbMode = false;
	swapBankedRegisters();

	setSPSR(oldCPSR);			//set SPSR_svc
	setReg(14, oldPC);			//Save old R15
	setReg(15, 0xFFFF0008);		//SWI entry point is 0x08
}

void ARM946E::Thumb_UnconditionalBranch()
{
	uint32_t offset = m_currentOpcode & 0x7FF;
	offset <<= 1;
	if (((offset >> 11) & 0b1))	//offset is two's complement so sign extend if necessary
		offset |= 0xFFFFF000;

	setReg(15, R[15] + offset);
}

void ARM946E::Thumb_LongBranchWithLink()
{
	bool isBLX = ((m_currentOpcode >> 11) & 0x1F) == 0b11101;
	bool highLow = ((m_currentOpcode >> 11) & 0b1);
	uint32_t offset = m_currentOpcode & 0b11111111111;
	if (!highLow)	//H=0: leftshift offset by 12 and add to PC, then store in LR
	{
		offset <<= 12;
		if (offset & 0x400000) { offset |= 0xFF800000; }
		uint32_t res = R[15] + offset;
		setReg(14, res & ~0b1);
	}
	else			//H=1: leftshift by 1 and add to LR - then copy LR to PC. copy old PC (-2) to LR and set bit 0
	{
		offset <<= 1;
		uint32_t LR = getReg(14);
		LR += offset;
		setReg(14, ((R[15] - 2) | 0b1));	//set LR to point to instruction after this one

		if (isBLX)
		{
			CPSR &= 0xFFFFFFDF;	//unset T bit
			m_inThumbMode = false;
			LR &= ~0b11;	//align address properly
		}

		setReg(15, LR);				//set PC to old LR contents (plus the offset)
	}
}