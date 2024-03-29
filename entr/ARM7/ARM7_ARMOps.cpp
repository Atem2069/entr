#include "ARM7TDMI.h"

void ARM7TDMI::ARM_Branch()
{
	bool link = ((m_currentOpcode >> 24) & 0b1);
	int32_t offset = m_currentOpcode & 0x00FFFFFF;
	offset <<= 2;	//now 26 bits
	if ((offset >> 25) & 0b1)	//if sign (bit 25) set, then must sign extend
		offset |= 0xFC000000;

	//set R15
	uint32_t oldR15 = getReg(15);
	setReg(15, getReg(15) + offset);
	if (link)
		setReg(14, (oldR15 - 4) & ~0b11);	
}

void ARM7TDMI::ARM_DataProcessing()
{
	bool immediate = ((m_currentOpcode >> 25) & 0b1);
	uint8_t operation = ((m_currentOpcode >> 21) & 0xF);
	bool setConditionCodes = ((m_currentOpcode >> 20) & 0b1);
	uint8_t op1Idx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t destRegIdx = ((m_currentOpcode >> 12) & 0xF);

	if (!setConditionCodes && (operation >> 2) == 0b10)
	{
		ARM_PSRTransfer();
		return;
	}

	bool setCPSR = false;
	if (destRegIdx == 15 && setConditionCodes)	//S=1,Rd=15 - copy SPSR over to CPSR
	{
		setCPSR = true;
		setConditionCodes = false;
	}


	uint32_t operand1 = getReg(op1Idx);
	uint32_t operand2 = 0;
	int shiftCarryOut = -1;

	//resolve operand 2
	if (immediate) //operand 2 is immediate
	{
		operand2 = m_currentOpcode & 0xFF;
		int shiftAmount = ((m_currentOpcode >> 8) & 0xF);
		if (shiftAmount > 0)
			operand2 = RORSpecial(operand2, shiftAmount, shiftCarryOut);
	}

	else		//operand 2 is a register
	{
		uint8_t op2Idx = m_currentOpcode & 0xF;
		operand2 = getReg(op2Idx);
		bool shiftIsRegister = ((m_currentOpcode >> 4) & 0b1);	//bit 4 specifies whether the amount to shift is a register or immediate
		int shiftAmount = 0;
		if (shiftIsRegister)
		{
			if (op2Idx == 15)	//account for R15 being 12 bytes ahead if register-specified shift amount
				operand2 += 4;
			if (op1Idx == 15)
				operand1 += 4;
			uint8_t shiftRegIdx = (m_currentOpcode >> 8) & 0xF;
			shiftAmount = getReg(shiftRegIdx) & 0xFF;	//only bottom byte used
			shiftCarryOut = m_getCarryFlag();			//just in case we don't shift (if register contains 0)
		}
		else
			shiftAmount = ((m_currentOpcode >> 7) & 0x1F);	//5 bit value (bits 7-11)

		uint8_t shiftType = ((m_currentOpcode >> 5) & 0b11);

		//(register specified shift) - If this byte is zero, the unchanged contents of Rm will be used - and the old value of the CPSR C flag will be passed on
		//(instruction specified shift) - Probs alright to just do a shift by 0, see what happens
		if ((shiftIsRegister && shiftAmount > 0) || (!shiftIsRegister))	//if imm shift, just go for it. if register shift, then only if shift amount > 0
		{
			switch (shiftType)
			{
			case 0: operand2 = LSL(operand2, shiftAmount, shiftCarryOut); break;
			case 1: operand2 = LSR(operand2, shiftAmount, shiftCarryOut); break;
			case 2: operand2 = ASR(operand2, shiftAmount, shiftCarryOut); break;
			case 3: operand2 = ROR(operand2, shiftAmount, shiftCarryOut); break;
			}
		}
	}

	uint32_t result = 0;
	uint32_t carryIn = m_getCarryFlag() & 0b1;
	bool realign = true;
	switch (operation)
	{
	case 0:	//AND
		result = operand1 & operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	case 1:	//EOR
		result = operand1 ^ operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	case 2:	//SUB
		result = operand1 - operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setArithmeticFlags(operand1, operand2, result, false); }
		break;
	case 3:	//RSB
		result = operand2 - operand1;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setArithmeticFlags(operand2, operand1, result, false); }
		break;
	case 4:	//ADD
		result = operand1 + operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setArithmeticFlags(operand1, operand2, result, true); }
		break;
	case 5:	//ADC
		result = operand1 + operand2 + carryIn;
		setReg(destRegIdx, result);
		if (setConditionCodes)
		{
			setArithmeticFlags(operand1, operand2, result, true);
			m_setCarryFlag((uint64_t)((uint64_t)operand1 + (uint64_t)operand2 + (uint64_t)carryIn) >> 32);
		}
		break;
	case 6:	//SBC
		result = operand1 - (uint64_t)(operand2 + !carryIn);
		setReg(destRegIdx, result);
		if (setConditionCodes)
		{
			setArithmeticFlags(operand1, operand2, result, false);
			m_setCarryFlag(operand1 >= ((uint64_t)operand2+(uint64_t)!carryIn));
		}
		break;
	case 7:	//RSC
		result = operand2 - ((uint64_t)operand1+(uint64_t)!carryIn);
		setReg(destRegIdx, result);
		if (setConditionCodes)
		{
			setArithmeticFlags(operand2, operand1, result, false);
			m_setCarryFlag(operand2 >= ((uint64_t)operand1+(uint64_t)!carryIn));
		}
		break;
	case 8:	//TST
		result = operand1 & operand2;
		realign = false;
		setLogicalFlags(result, shiftCarryOut);
		break;
	case 9:	//TEQ
		result = operand1 ^ operand2;
		realign = false;
		setLogicalFlags(result, shiftCarryOut);
		break;
	case 10: //CMP
		result = operand1 - operand2;
		realign = false;
		setArithmeticFlags(operand1, operand2, result, false);
		break;
	case 11: //CMN
		result = operand1 + operand2;
		realign = false;
		setArithmeticFlags(operand1, operand2, result, true);
		break;
	case 12: //ORR
		result = operand1 | operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	case 13: //MOV
		result = operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	case 14:	//BIC
		result = operand1 & (~operand2);
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	case 15:	//MVN
		result = ~operand2;
		setReg(destRegIdx, result);
		if (setConditionCodes) { setLogicalFlags(result, shiftCarryOut); }
		break;
	}

	if ((destRegIdx == 15))
	{
		if (setCPSR)
		{
			uint32_t newPSR = getSPSR();
			CPSR = newPSR;
			swapBankedRegisters();

			if (((CPSR >> 5) & 0b1)&&realign)
			{
				m_inThumbMode = true;
				setReg(15, getReg(15) & ~0b1);
			}
		}

	}
}

void ARM7TDMI::ARM_PSRTransfer()
{
	bool PSR = ((m_currentOpcode >> 22) & 0b1);		//1: set spsr
	bool immediate = ((m_currentOpcode >> 25) & 0b1);
	bool opcode = ((m_currentOpcode >> 21) & 0b1);

	if (opcode) // MSR
	{
		uint32_t input = 0;
		uint32_t fieldMask = 0;
		if (m_currentOpcode & 0x80000) { fieldMask |= 0xFF000000; }		//mask bits depend on specific psr transfer op. fwiw mostly affects lower/upper byte
		if (m_currentOpcode & 0x40000) { fieldMask |= 0x00FF0000; }
		if (m_currentOpcode & 0x20000) { fieldMask |= 0x0000FF00; }
		if (m_currentOpcode & 0x10000) { fieldMask |= 0x000000FF; }

		if (immediate)
		{
			input = m_currentOpcode & 0xFF;
			uint8_t shift = (m_currentOpcode >> 8) & 0xF;
			int meaningless = 0;
			input = RORSpecial(input, shift, meaningless);
		}
		else
		{
			uint8_t srcReg = m_currentOpcode & 0xF;
			if (srcReg == 15)
			{

			}
			input = getReg(srcReg);
			input &= fieldMask;
		}

		if (PSR)
		{
			uint32_t temp = getSPSR();
			temp &= ~fieldMask;
			temp |= input;
			setSPSR(temp);
		}
		else
		{
			CPSR &= ~fieldMask;
			CPSR |= input;
			swapBankedRegisters();
			if (CPSR & 0x20)
			{
				// Switch to THUMB
				m_inThumbMode = true;
				setReg(15, getReg(15) & ~0x1); // also flushes pipeline
			}
		}
	}
	else // MRS (transfer PSR to register)
	{
		uint8_t destReg = (m_currentOpcode >> 12) & 0xF;
		if (destReg == 15)
		{

		}
		if (PSR)
		{
			setReg(destReg, getSPSR());
		}
		else
		{
			setReg(destReg, CPSR);
		}
	}
}

void ARM7TDMI::ARM_Multiply()
{
	bool accumulate = ((m_currentOpcode >> 21) & 0b1);
	bool setConditions = ((m_currentOpcode >> 20) & 0b1);
	uint8_t destRegIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t accumRegIdx = ((m_currentOpcode >> 12) & 0xF);
	uint8_t op2RegIdx = ((m_currentOpcode >> 8) & 0xF);
	uint8_t op1RegIdx = ((m_currentOpcode) & 0xF);

	uint32_t operand1 = getReg(op1RegIdx);
	uint32_t operand2 = getReg(op2RegIdx);
	uint32_t accum = getReg(accumRegIdx);

	uint32_t result = 0;

	if (accumulate)
		result = operand1 * operand2 + accum;
	else
		result = operand1 * operand2;

	setReg(destRegIdx, result);
	if (setConditions)
	{
		m_setNegativeFlag(((result >> 31) & 0b1));
		m_setZeroFlag(result == 0);
	}
}

void ARM7TDMI::ARM_MultiplyLong()
{
	bool isSigned = ((m_currentOpcode >> 22) & 0b1);
	bool accumulate = ((m_currentOpcode >> 21) & 0b1);
	bool setFlags = ((m_currentOpcode >> 20) & 0b1);
	uint8_t destRegHiIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t destRegLoIdx = ((m_currentOpcode >> 12) & 0xF);
	uint8_t src2RegIdx = ((m_currentOpcode >> 8) & 0xF);
	uint8_t src1RegIdx = ((m_currentOpcode) & 0xF);

	uint64_t src1 = getReg(src1RegIdx);
	uint64_t src2 = getReg(src2RegIdx);
	uint32_t oldSrc2 = src2;

	uint64_t accumLow = getReg(destRegLoIdx);
	uint64_t accumHi = getReg(destRegHiIdx);
	uint64_t accum = ((accumHi << 32) | accumLow);

	if (isSigned)
	{
		if (((src1 >> 31) & 0b1))
			src1 |= 0xFFFFFFFF00000000;
		if (((src2 >> 31) & 0b1))
			src2 |= 0xFFFFFFFF00000000;
		//prob dont have to sign extend accum bc its inherent if its sign extended
	}

	uint64_t result = 0;
	if (accumulate)
		result = src1 * src2 + accum;
	else
		result = src1 * src2;

	if (setFlags)
	{
		m_setZeroFlag(result == 0);
		m_setNegativeFlag(((result >> 63) & 0b1));
	}

	setReg(destRegLoIdx, result & 0xFFFFFFFF);
	setReg(destRegHiIdx, ((result >> 32) & 0xFFFFFFFF));
}

void ARM7TDMI::ARM_SingleDataSwap()
{
	bool byteWord = ((m_currentOpcode >> 22) & 0b1);
	uint8_t baseRegIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t destRegIdx = ((m_currentOpcode >> 12) & 0xF);
	uint8_t srcRegIdx = ((m_currentOpcode) & 0xF);

	uint32_t swapAddress = getReg(baseRegIdx);
	uint32_t srcData = getReg(srcRegIdx);

	if (byteWord)		//swap byte
	{
		uint8_t swapVal = m_bus->NDS7_read8(swapAddress);
		m_bus->NDS7_write8(swapAddress, srcData & 0xFF);
		setReg(destRegIdx, swapVal);
		
	}

	else				//swap word
	{
		uint32_t swapVal = m_bus->NDS7_read32(swapAddress);
		if (swapAddress & 3)
			swapVal = std::rotr(swapVal, (swapAddress & 3) * 8);
		m_bus->NDS7_write32(swapAddress, srcData);
		setReg(destRegIdx, swapVal);
	}
}


void ARM7TDMI::ARM_BranchExchange()
{
	uint8_t regIdx = m_currentOpcode & 0xF;
	uint32_t newAddr = getReg(regIdx);
	if (newAddr & 0b1)	//start executing THUMB instrs
	{
		CPSR |= 0b100000;	//set T bit in CPSR
		m_inThumbMode = true;
		setReg(15, newAddr & ~0b1);
	}
	else				//keep going as ARM (but pipeline will be flushed)
	{
		setReg(15, newAddr & ~0b11);
	}
}

void ARM7TDMI::ARM_HalfwordTransferRegisterOffset()
{
	bool prePost = ((m_currentOpcode >> 24) & 0b1);
	bool upDown = ((m_currentOpcode >> 23) & 0b1);
	bool writeback = ((m_currentOpcode >> 21) & 0b1);
	bool loadStore = ((m_currentOpcode >> 20) & 0b1);
	uint8_t baseRegIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t srcDestRegIdx = ((m_currentOpcode >> 12) & 0xF);
	uint8_t operation = ((m_currentOpcode >> 5) & 0b11);
	uint8_t offsetRegIdx = m_currentOpcode & 0xF;

	uint32_t base = getReg(baseRegIdx);
	uint32_t offset = getReg(offsetRegIdx);

	if (prePost)
	{
		if (upDown)
			base += offset;
		else
			base -= offset;
	}

	if (loadStore)
	{
		uint32_t val = 0;
		switch (operation)
		{
		case 0:
			Logger::msg(LoggerSeverity::Error, "SWP called from halfword transfer - opcode decoding is invalid!!!");
			break;
		case 1:
			val = m_bus->NDS7_read16(base);
			if (base & 1)
				val = std::rotr(val, 8);
			setReg(srcDestRegIdx, val);
			break;
		case 2:
			val = m_bus->NDS7_read8(base);
			if (((val >> 7) & 0b1))
				val |= 0xFFFFFF00;
			setReg(srcDestRegIdx, val);
			break;
		case 3:
			if (!(base & 0b1))
			{
				val = m_bus->NDS7_read16(base);
				if (((val >> 15) & 0b1))
					val |= 0xFFFF0000;
			}
			else
			{
				val = m_bus->NDS7_read8(base);
				if (((val >> 7) & 0b1))
					val |= 0xFFFFFF00;
			}
			setReg(srcDestRegIdx, val);
			break;
		}
	}
	else						//store
	{
		uint32_t data = getReg(srcDestRegIdx);
		if ((operation == 1 || operation == 3) && srcDestRegIdx == 15)	//PC+12 when specified as src and halfword transfer taking place
			data += 4;
		switch (operation)
		{
		case 0:
			Logger::msg(LoggerSeverity::Error, "Invalid halfword operation encoding");
			break;
		case 1:
			m_bus->NDS7_write16(base, data & 0xFFFF);
			break;
		case 2:
			m_bus->NDS7_write8(base, data & 0xFF);
			break;
		case 3:
			m_bus->NDS7_write16(base, data & 0xFFFF);
			break;
		}
	}

	if (!prePost)
	{
		if (!upDown)
			base -= offset;
		else
			base += offset;
	}

	if (((!prePost) || (prePost && writeback)) && ((baseRegIdx != srcDestRegIdx) || (baseRegIdx == srcDestRegIdx && !loadStore)))
		setReg(baseRegIdx, base);
}

void ARM7TDMI::ARM_HalfwordTransferImmediateOffset()
{
	bool prePost = ((m_currentOpcode >> 24) & 0b1);
	bool upDown = ((m_currentOpcode >> 23) & 0b1);
	bool writeback = ((m_currentOpcode >> 21) & 0b1);
	bool loadStore = ((m_currentOpcode >> 20) & 0b1);
	uint8_t baseRegIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t srcDestRegIdx = ((m_currentOpcode >> 12) & 0xF);
	uint8_t offsHigh = ((m_currentOpcode >> 8) & 0xF);
	uint8_t op = ((m_currentOpcode >> 5) & 0b11);
	uint8_t offsLow = m_currentOpcode & 0xF;

	uint8_t offset = ((offsHigh << 4) | offsLow);

	uint32_t base = getReg(baseRegIdx);

	if (prePost)				//sort out pre-indexing
	{
		if (!upDown)
			base -= offset;
		else
			base += offset;
	}

	if (loadStore)				//load
	{
		uint32_t data = 0;
		switch (op)
		{
		case 0:
			Logger::msg(LoggerSeverity::Error, "Invalid halfword operation encoding");
			break;
		case 1:
			data = m_bus->NDS7_read16(base);
			if (base & 1)
				data = std::rotr(data, 8);
			setReg(srcDestRegIdx, data);
			break;
		case 2:
			data = m_bus->NDS7_read8(base);
			if (((data >> 7) & 0b1))	//sign extend byte if bit 7 set
				data |= 0xFFFFFF00;
			setReg(srcDestRegIdx, data);
			break;
		case 3:
			if (!(base & 0b1))
			{
				data = m_bus->NDS7_read16(base);
				if (((data >> 15) & 0b1))
					data |= 0xFFFF0000;
			}
			else
			{
				data = m_bus->NDS7_read8(base);
				if (((data >> 7) & 0b1))
					data |= 0xFFFFFF00;
			}
			setReg(srcDestRegIdx, data);
			break;
		}
	}
	else						//store
	{
		uint32_t data = getReg(srcDestRegIdx);
		if ((op == 1 || op == 3) && srcDestRegIdx == 15)	//PC+12 when specified as src and halfword transfer taking place
			data += 4;
		switch (op)
		{
		case 0:
			Logger::msg(LoggerSeverity::Error, "Invalid halfword operation encoding");
			break;
		case 1:
			m_bus->NDS7_write16(base, data & 0xFFFF);
			break;
		case 2:
			m_bus->NDS7_write8(base, data & 0xFF);
			break;
		case 3:
			m_bus->NDS7_write16(base, data & 0xFFFF);
			break;
		}
	}

	if (!prePost)
	{
		if (!upDown)
			base -= offset;
		else
			base += offset;
	}

	if (((!prePost) || (prePost && writeback)) && ((baseRegIdx != srcDestRegIdx) || (baseRegIdx == srcDestRegIdx && !loadStore)))
		setReg(baseRegIdx, base);
}

void ARM7TDMI::ARM_SingleDataTransfer()
{
	bool immediate = ((m_currentOpcode >> 25) & 0b1);
	bool preIndex = ((m_currentOpcode >> 24) & 0b1);
	bool upDown = ((m_currentOpcode >> 23) & 0b1);	//1=up,0=down
	bool byteWord = ((m_currentOpcode >> 22) & 0b1);//1=byte,0=word
	bool writeback = ((m_currentOpcode >> 21) & 0b1);
	bool loadStore = ((m_currentOpcode >> 20) & 0b1);

	uint8_t baseRegIdx = ((m_currentOpcode >> 16) & 0xF);
	uint8_t destRegIdx = ((m_currentOpcode >> 12) & 0xF);

	uint32_t base = getReg(baseRegIdx);

	int32_t offset = 0;
	//resolve offset
	if (!immediate)	//I=0 means immediate!!
	{
		offset = m_currentOpcode & 0xFFF;	//extract 12-bit imm offset
	}
	else
	{
		uint8_t offsetRegIdx = m_currentOpcode & 0xF;
		offset = getReg(offsetRegIdx);


		//register specified shifts not available
		uint8_t shiftAmount = ((m_currentOpcode >> 7) & 0x1F);	//5 bit shift amount
		uint8_t shiftType = ((m_currentOpcode >> 5) & 0b11);
		if (((m_currentOpcode >> 4) & 0b1) == 1)
			Logger::msg(LoggerSeverity::Error, "Opcode encoding is not valid! bit 4 shouldn't be set!!");

		int garbageCarry = 0;
		switch (shiftType)
		{
		case 0: offset = LSL(offset, shiftAmount, garbageCarry); break;
		case 1: offset = LSR(offset, shiftAmount, garbageCarry); break;
		case 2: offset = ASR(offset, shiftAmount, garbageCarry); break;
		case 3: offset = ROR(offset, shiftAmount, garbageCarry); break;
		}

	}

	//offset now resolved
	if (!upDown)	//offset subtracted if up/down bit is 0
		offset = -offset;

	if (preIndex)
		base += offset;

	if (loadStore) //load value
	{
		uint32_t val = 0;
		if (byteWord)
		{
			val = m_bus->NDS7_read8(base);
		}
		else
		{
			val = m_bus->NDS7_read32(base);
			if(base&3)
				val = std::rotr(val, (base & 3) * 8);
		}
		setReg(destRegIdx, val);
	}
	else //store value
	{
		uint32_t val = getReg(destRegIdx);
		if (destRegIdx == 15)	//R15 12 bytes ahead (instead of 8) if used for store
			val += 4;
		if (byteWord)
		{
			m_bus->NDS7_write8(base, val & 0xFF);
		}
		else
		{
			m_bus->NDS7_write32(base, val);
		}
	}

	if (!preIndex)
		base += offset;

	if (((!preIndex) || (preIndex && writeback)) && ((baseRegIdx != destRegIdx) || (baseRegIdx == destRegIdx && !loadStore)))
	{
		setReg(baseRegIdx, base);
	}
}

void ARM7TDMI::ARM_Undefined()
{
	//Logger::msg(LoggerSeverity::Error, std::format("Undefined instruction!! PC={}",getReg(15)-4));
	uint32_t oldCPSR = CPSR;
	uint32_t oldPC = R[15] - 4;
	if (m_inThumbMode)
	{
		oldPC = R[15] - 2;
		m_inThumbMode = false;
	}

	Logger::msg(LoggerSeverity::Error, std::format("Undefined instruction {:#x} R15={:#x}", m_currentOpcode, oldPC));

	CPSR &= 0xFFFFFFC0;	//clear thumb + mode bits (0-5)
	CPSR |= 0b0011011;	//set und bits (11011)
	swapBankedRegisters();

	setSPSR(oldCPSR);			//set SPSR_svc
	setReg(14, oldPC);			//Save old R15
	setReg(15, 0x00000004);		//UND entry point=0x00000004

}

void ARM7TDMI::ARM_BlockDataTransfer()
{
	bool prePost = ((m_currentOpcode >> 24) & 0b1);
	bool upDown = ((m_currentOpcode >> 23) & 0b1);
	bool psr = ((m_currentOpcode >> 22) & 0b1);
	bool writeBack = ((m_currentOpcode >> 21) & 0b1);
	bool loadStore = ((m_currentOpcode >> 20) & 0b1);
	uint8_t baseReg = ((m_currentOpcode >> 16) & 0xF);
	uint16_t r_list = m_currentOpcode & 0xFFFF;

	uint8_t oldMode = 0;
	uint32_t base_addr = getReg(baseReg);
	uint32_t old_base = base_addr;

	uint32_t oldCPSR = CPSR;
	if (psr)
	{
		//force user mode
		CPSR = 0x1F;
		swapBankedRegisters();
	}

	int transferCount = __popcnt16(r_list);
	bool baseIsFirst = ((r_list >> baseReg) & 0b1) && !((r_list << (16 - baseReg)) & 0xFFFF);

	uint32_t finalBase = base_addr;
	if (!upDown)
	{
		finalBase = finalBase - (transferCount * 4);
		base_addr = finalBase;
		prePost = !prePost;	//flip pre/post-indexing order for descending r-list load/store
	}
	else
		finalBase = finalBase + (transferCount * 4);

	bool firstTransfer = true;

	for (int i = 0; i < 16; i++)
	{
		if ((r_list >> i) & 0b1)
		{
			if (prePost)
				base_addr += 4;

			uint32_t val = 0;
			if (loadStore)
				val = m_bus->NDS7_read32(base_addr);
			else
			{
				val = getReg(i);
				if (i == 15)
					val += 4;	//account for pipeline behaviour (pc+12)
			}

			//special case behaviour (base included in rlist)
			if (i == baseReg && writeBack)
			{
				if (loadStore)
					writeBack = false;
				else
				{
					if (baseIsFirst)
						val = old_base;
					else
						val = finalBase;
				}
			}

			if (loadStore)
				setReg(i, val);
			else
				m_bus->NDS7_write32(base_addr, val);

			firstTransfer = false;

			if (!prePost)
				base_addr += 4;
		}
	}

	if (writeBack)
		setReg(baseReg, finalBase);	//don't use base_addr, because decrementing load/stores will modify the internal transfer register differently

	int cycles = transferCount + ((loadStore) ? 2 : 1);

	if (transferCount == 0)		//no registers to transfer, weird behaviour
	{

		if (!upDown)                 //seems for descending load/store that the -40h offset is applied before loading/storing PC
		{
			base_addr -= 0x40;
			setReg(baseReg, base_addr);
		}
		 
		if (prePost)				//preincrement causes 'DA' or 'IB' (decrementing after, incrementing before) to do load/store either +4h or -3Ch bytes from base
			base_addr += 4;

		if (loadStore)
		{
			setReg(15, m_bus->NDS7_read32(base_addr));
		}
		else
		{
			m_bus->NDS7_write32(base_addr, getReg(15) + 4);
		}

		if (upDown)
			setReg(baseReg, old_base + 0x40);	//use old_base bc 'base_addr' might have been offset by 4h already
	}


	if (psr)
	{
		CPSR = oldCPSR;
		swapBankedRegisters();
	}
}

void ARM7TDMI::ARM_CoprocessorDataTransfer()
{
	Logger::msg(LoggerSeverity::Error, "Unimplemented");
	throw std::runtime_error("unimplemented");
}

void ARM7TDMI::ARM_CoprocessorDataOperation()
{
	Logger::msg(LoggerSeverity::Error, "Unimplemented");
	throw std::runtime_error("unimplemented");
}

void ARM7TDMI::ARM_CoprocessorRegisterTransfer()
{
	uint8_t coprocessorNumber = (m_currentOpcode >> 8) & 0xF;
	if (coprocessorNumber == 14)
	{
		Logger::msg(LoggerSeverity::Warn, "Ignoring ARM7 CP14 access..");
		return;
	}
	Logger::msg(LoggerSeverity::Error, std::format("Undefined attempt to access CP{}", coprocessorNumber));
	ARM_Undefined();
}

void ARM7TDMI::ARM_SoftwareInterrupt()
{
	uint32_t oldCPSR = CPSR;
	uint32_t oldPC = R[15] - 4;	//-4 because it points to next instruction

	CPSR &= 0xFFFFFFE0;	//clear mode bits (0-4)
	CPSR |= 0b10010011;	//set svc bits
	swapBankedRegisters();

	setSPSR(oldCPSR);			//set SPSR_svc
	setReg(14, oldPC);			//Save old R15
	setReg(15, 0x00000008);		//SWI entry point is 0x08
}