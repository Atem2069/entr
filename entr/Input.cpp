#include"Input.h"

Input::Input()
{
	keyInput = 0xFFFF;
}

Input::~Input()
{

}

void Input::registerInterrupts(InterruptManager* interruptManager)
{
	m_interruptManager = interruptManager;
}

void Input::tick()
{
	uint16_t newInputState = (~(inputState.reg)) & 0x3FF;
	bool shouldCheckIRQ = (newInputState != keyInput);
	keyInput = newInputState;
	extKeyInput = (~(extInputState.reg)) & 0xFF;
	/*if (shouldCheckIRQ)					//i'm confused.. if the irq was already asserted when KEYCNT written, then we can just trigger the irq on key input change
	{									//....otherwise, recheck if the irq can happen?? wtf is my code doing??
		if (irqActive)
			m_interruptManager->requestInterrupt(InterruptType::Keypad);
		else
			checkIRQ();
	}*/
}

bool Input::getIRQConditionsMet()
{
	bool irqMode = ((KEYCNT >> 15) & 0b1);
	bool shouldDoIRQ = (((~keyInput) & 0x3FF) | (KEYCNT & 0x3FF));
	if (irqMode)
		shouldDoIRQ = (((~keyInput) & 0x3FF) == (KEYCNT & 0x3FF));
	return shouldDoIRQ;
}

uint8_t Input::readIORegister(uint32_t address)
{
	switch (address)
	{
	case 0x04000130:
		return keyInput & 0xFF;
	case 0x04000131:
		return ((keyInput >> 8) & 0xFF);
	case 0x04000132:
		return KEYCNT & 0xFF;
	case 0x04000133:
		return ((KEYCNT >> 8) & 0xFF);
	case 0x04000136:
		return extKeyInput & 0x7F;
	case 0x04000137:
		return 0;
	}
}

void Input::writeIORegister(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000132:
		KEYCNT &= 0xFF00; KEYCNT |= value;
		break;
	case 0x04000133:
		KEYCNT &= 0xFF; KEYCNT |= (value << 8);
		checkIRQ();
		break;
	}
}

void Input::checkIRQ()
{
	/*irqActive = false;
	bool irqEnabled = ((KEYCNT >> 14) & 0b1);
	if (!irqEnabled)	//todo: doublecheck. it seems like STOP doesn't care about the irq bit.
		return;
	bool irqMode = ((KEYCNT >> 15) & 0b1);

	bool shouldDoIRQ = getIRQConditionsMet();

	if (shouldDoIRQ)
		m_interruptManager->requestInterrupt(InterruptType::Keypad);
	irqActive = shouldDoIRQ;*/
}

InputState Input::inputState = {};
ExtInputState Input::extInputState = {};