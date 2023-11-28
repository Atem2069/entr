#include"GPU.h"

GPU::GPU()
{

}

GPU::~GPU()
{

}

void GPU::init(InterruptManager* interruptManager, Scheduler* scheduler)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;

	m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, 1);	//schedule event to handle GXFIFO commands
}

uint8_t GPU::read(uint32_t address)
{
	switch (address)
	{
	case 0x04000600:
		return GXSTAT & 0xFF;
	case 0x04000601:
		return (GXSTAT >> 8) & 0xFF;
	case 0x04000602:
		return (GXSTAT >> 16) & 0xFF;
	case 0x04000603:
		return (GXSTAT >> 24) & 0xFF;
	}
}

void GPU::write(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000601:
		GXSTAT &= 0xFFFFDFFF; GXSTAT |= (value & 0x20) << 8;		//todo: handle bit 15 being 'writeable'
		break;
	case 0x04000603:
		GXSTAT &= 0x3FFFFFFF; GXSTAT |= (value & 0xC0) << 24;
		break;
	}
}

void GPU::writeGXFIFO(uint32_t value)
{
	if (m_pendingCommands.empty())
	{
		for (int i = 0; i < 4; i++)
			m_pendingCommands.push((value >> (i * 8)) & 0xFF);
	}
	else
	{
		m_pendingParameters.push(value);
		uint8_t curCmd = m_pendingCommands.front();
		int numParams = std::max(m_cmdParameterLUT[curCmd], (uint8_t)1);
		if (m_pendingParameters.size() == numParams)
		{
			m_pendingCommands.pop();
			while (!m_pendingParameters.empty())
			{
				uint32_t param = m_pendingParameters.front();
				m_pendingParameters.pop();
				GXFIFOCommand fifoCmd = {};
				fifoCmd.command = curCmd;
				fifoCmd.parameter = param;
				GXFIFO.push(fifoCmd);
			}
			//Logger::msg(LoggerSeverity::Info, std::format("gpu: processed command {:#x}, param count={}", (uint32_t)curCmd, numParams));
		}
	}
}

void GPU::writeCmdPort(uint32_t address, uint32_t value)
{
	//todo: figure out port from address, some way to enqueue..
	//hopefully parameters are sent to the port too? that would probably make my life easier.
	uint32_t cmd = ((address - 0x4000440) >> 2)+0x10;
	//std::cout << "cmd port: " << std::hex << address << " " << cmd << '\n';
	//if (!m_pendingCommands.empty())
	//	Logger::msg(LoggerSeverity::Error, "gpu: what the fuck? unprocessed commands from gxfifo write, this shouldn't happen.");
}

//calling this every cycle is probably slow - could deschedule/schedule depending on whether
//processing is stopped (SwapBuffers), or we know GXFIFO is empty..
void GPU::onProcessCommandEvent()
{
	processCommand();
	m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, m_scheduler->getCurrentTimestamp() + 1);
}

void GPU::checkGXFIFOIRQs()
{
	uint8_t IRQMode = (GXSTAT >> 30) & 0b11;
	if (GXFIFO.size() < 128)
	{
		GXSTAT |= (1 << 25);
		if (IRQMode == 1)
			m_interruptManager->NDS9_requestInterrupt(InterruptType::GXFIFO);
	}
	if (GXFIFO.size() == 0)
	{
		GXSTAT |= (1 << 26);
		if (IRQMode == 2)
			m_interruptManager->NDS9_requestInterrupt(InterruptType::GXFIFO);
	}
}

void GPU::processCommand()
{
	//not a fan of this function as a whole, must admit. 
	GXSTAT &= ~(1 << 24);
	GXSTAT &= ~(1 << 25);
	GXSTAT &= ~(1 << 26);
	GXSTAT &= ~(1 << 27);

	checkGXFIFOIRQs();		//initial check, as fifo may be empty.

	if (GXFIFO.empty())
		return;

	GXFIFOCommand cmd = GXFIFO.front();
	GXFIFO.pop();
	
	//this code is entirely useless. it just keeps cmd processor running and popping off cmds.
	int numParams = m_cmdParameterLUT[cmd.command];
	if (numParams)
		numParams -= 1;
	if (GXFIFO.size() < numParams)
		Logger::msg(LoggerSeverity::Error, std::format("gpu: gxfifo state fucked, not enough params to process command!!!!! {} {}",GXFIFO.size(),numParams));
	else
	{
		for (int i = 0; i < numParams; i++)
			GXFIFO.pop();
	}

	checkGXFIFOIRQs();		//another check after cmds processed (a lot of values might have been popped);
	GXSTAT |= (1 << 27);
}

void GPU::GXFIFOEventHandler(void* context)
{
	GPU* thisPtr = (GPU*)context;
	thisPtr->onProcessCommandEvent();
}