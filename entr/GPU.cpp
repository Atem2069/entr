#include"GPU.h"

GPU::GPU()
{
	createWorkerThreads();
}

GPU::~GPU()
{
	destroyWorkerThreads();
}

void GPU::createWorkerThreads()
{
	emuRunning = true;
	m_workerThreads = new GPUWorkerThread[numThreads];
	for (int i = 0; i < numThreads; i++)
	{
		m_workerThreads[i].rendering = false;
		m_workerThreads[i].yMin = i * linesPerThread;
		m_workerThreads[i].yMax = m_workerThreads[i].yMin + (linesPerThread - 1);
		m_workerThreads[i].m_thread = std::thread(&GPU::renderWorker, this, i);
	}
}

void GPU::destroyWorkerThreads()
{
	emuRunning = false;
	for (int i = 0; i < numThreads; i++)
	{
		Logger::msg(LoggerSeverity::Info, std::format("Stopping thread {}", i));
		m_workerThreads[i].rendering = true;
		m_workerThreads[i].cv.notify_one();
		m_workerThreads[i].m_thread.join();
	}
	delete[] m_workerThreads;
}

void GPU::init(InterruptManager* interruptManager, Scheduler* scheduler)
{
	m_interruptManager = interruptManager;
	m_scheduler = scheduler;

	//m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, 1);	//schedule event to handle GXFIFO commands

	//init identity matrix
	m_identityMatrix.m[0] = (1<<12);
	m_identityMatrix.m[5] = (1<<12);
	m_identityMatrix.m[10] = (1<<12);
	m_identityMatrix.m[15] = (1<<12);

	m_projectionMatrix = m_identityMatrix;
	m_coordinateMatrix = m_identityMatrix;
	m_directionalMatrix = m_identityMatrix;
	m_textureMatrix = m_identityMatrix;
}

uint8_t GPU::read(uint32_t address)
{
	switch (address)
	{
	case 0x04000060:
		return DISP3DCNT & 0xFF;
	case 0x04000061:
		return (DISP3DCNT >> 8) & 0xFF;
	case 0x04000600:
		GXSTAT |= 0b10;
		return GXSTAT & 0xFF;
	case 0x04000601:
		return (GXSTAT >> 8) & 0xFF;
	case 0x04000602:
		return (GXFIFO.size()) & 0xFF;
	case 0x04000603:
		return ((GXSTAT >> 24) & 0xFE) | ((GXFIFO.size())>>8);	//ew
	case 0x04000604:
		return m_polygonCount & 0xFF;
	case 0x04000605:
		return (m_polygonCount >> 8) & 0xF;
	case 0x04000606:
		return m_vertexCount & 0xFF;
	case 0x04000407:
		return (m_vertexCount >> 8) & 0x1F;
	}
	//lmao
	if (address >= 0x04000640 && address <= 0x0400067f)
	{
		//hacky: keep GXFIFO up-to-date so correct clip mtx is read
		if (address == 0x04000640)
		{
			while (!GXFIFO.empty())
				processCommand();
		}
		int mtxIdx = (address & 0x3C) >> 2;
		uint32_t m = m_clipMatrix.m[mtxIdx];
		return (m >> ((address & 0b11)*8)) & 0xFF;
	}

	return 0;
}

void GPU::write(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000060:
		DISP3DCNT &= 0xFF00; DISP3DCNT |= value; 
		break;
	case 0x04000061:
		DISP3DCNT &= 0xFF; DISP3DCNT |= (value << 8);
		break;
	case 0x04000304:
	{
		if (!GXFIFOEnabled && ((value >> 3) & 0b1))
		{
			GXFIFOEnabled = true;
			m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, m_scheduler->getCurrentTimestamp() + 1);
		}
		if (!((value >> 3) & 0b1))
		{
			GXFIFOEnabled = false;
			m_scheduler->removeEvent(Event::GXFIFO);
		}
		break;
	}
	case 0x04000601:
		GXSTAT &= 0xFFFFDFFF; GXSTAT |= (value & 0x20) << 8;		
		if ((value >> 7) & 0b1)
		{
			GXSTAT &= ~(1 << 15);
			GXSTAT &= ~(1 << 13);
			//"additionally resets the P rojection Stack Pointer (Bit13), and probably (?) also the Texture stack Pointer" <- todo: implement.
		}
		break;
	case 0x04000603:
		GXSTAT &= 0x3FFFFFFF; GXSTAT |= (value & 0xC0) << 24;
		break;
	case 0x04000340:
		alphaTestRef = value & 0x1F;
		break;
	case 0x04000350:
		clearColor &= 0xFFFFFF00; clearColor |= value;
		break;
	case 0x04000351:
		clearColor &= 0xFFFF00FF; clearColor |= (value << 8);
		break;
	case 0x04000352:
		clearColor &= 0xFF00FFFF; clearColor |= (value << 16);
		break;
	case 0x04000353:
		clearColor &= 0x00FFFFFF; clearColor |= (value << 24);
		break;
	case 0x04000354:
		clearDepth &= 0xFF00; clearDepth |= value;
		break;
	case 0x04000355:
		clearDepth &= 0xFF; clearDepth |= (value << 8);
		break;
	}

	if (address >= 0x04000380 && address <= 0x040003bf)
	{
		int tableIdx = (address & 63) >> 1;
		int offs = address & 0b1;
		if (!(offs & 0b1))
		{
			m_toonTable[tableIdx] &= 0xFF00;
			m_toonTable[tableIdx] |= value;
		}
		else
		{
			m_toonTable[tableIdx] &= 0xFF;
			m_toonTable[tableIdx] |= (value << 8);
		}
	}
}

void GPU::writeGXFIFO(uint32_t value)
{
	if (m_pendingCommands.empty())
	{
		for (int i = 0; i < 4; i++)
		{
			uint8_t cmd = (value >> (i << 3)) & 0xFF;
			//this solution is not ideal.
			//push cmd with no params straight to FIFO ONLY if no commands with params precede it to preserve order. see below.
			if ((m_cmdParameterLUT[cmd] == 0) && (m_pendingCommands.empty()))
			{
				GXFIFOCommand fifoCmd = {};
				fifoCmd.command = cmd;
				GXFIFO.push(fifoCmd);
			}
			else
				m_pendingCommands.push((value >> (i * 8)) & 0xFF);
		}
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

		//god I hate this. peek next command(s) - if no params push to fifo straight away
		//if not, wait until more params written to process them
		bool stopChecking = false;
		while (!m_pendingCommands.empty() && !stopChecking)
		{
			uint8_t nextCmd = m_pendingCommands.front();
			if (m_cmdParameterLUT[nextCmd] == 0)
			{
				m_pendingCommands.pop();
				GXFIFOCommand fifoCmd = {};
				fifoCmd.command = nextCmd;
				GXFIFO.push(fifoCmd);
			}
			else
				stopChecking = true;
		}

	}
}

void GPU::writeCmdPort(uint32_t address, uint32_t value)
{
	uint32_t cmd = (address & 0x1FF) >> 2;
	int noParams = m_cmdParameterLUT[cmd];
	if (noParams < 2)
	{
		GXFIFOCommand fifoCmd = {};
		fifoCmd.command = cmd;
		fifoCmd.parameter = value;
		GXFIFO.push(fifoCmd);
	}
	else
	{
		m_pendingParameters.push(value);
		if (m_pendingParameters.size() == noParams)
		{
			while (!m_pendingParameters.empty())
			{
				uint32_t param = m_pendingParameters.front();
				m_pendingParameters.pop();
				GXFIFOCommand fifoCmd = {};
				fifoCmd.command = cmd;
				fifoCmd.parameter = param;
				GXFIFO.push(fifoCmd);
			}
		}
	}
}

//calling this every cycle is probably slow - could deschedule/schedule depending on whether
//processing is stopped (SwapBuffers), or we know GXFIFO is empty..
void GPU::onProcessCommandEvent()
{
	//evil hack: we sync quite loosely, but some games depend on cmds being executed fairly quick to read back CLIPMTX_RESULT
	//so.... just execute a bunch of commands at once :)
	GXSTAT &= ~(1 << 24);
	GXSTAT &= ~(1 << 25);
	GXSTAT &= ~(1 << 26);
	GXSTAT &= ~(1 << 27);
	checkGXFIFOIRQs();

	int i = 0;
	while (!GXFIFO.empty() && i < 32)
	{
		processCommand();
		i++;
	}
	if (GXFIFO.empty())
		GXSTAT &= ~(1 << 27);

	if (swapBuffersPending)
		return;
	m_scheduler->addEvent(Event::GXFIFO, (callbackFn)&GPU::GXFIFOEventHandler, (void*)this, m_scheduler->getEventTime() + 16);

	//trigger gxfifo dma if less than half full
	if (GXFIFO.size() <= 128)
		m_callback(m_callbackCtx);
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
	GXSTAT &= ~(1 << 27);
	if (GXFIFO.empty() || swapBuffersPending)
		return;

	uint32_t params[32];	//I think the max param count is 32 params?

	GXFIFOCommand cmd = GXFIFO.front();
	GXFIFO.pop();
	
	int numParams = m_cmdParameterLUT[cmd.command];
	if (numParams)
		numParams -= 1;

	if (GXFIFO.size() < numParams)
		return;
	else
	{
		params[0] = cmd.parameter;
		for (int i = 0; i < numParams; i++)
		{
			params[i + 1] = GXFIFO.front().parameter;
			GXFIFO.pop();
		}
		switch (cmd.command)
		{
		case 0x10: cmd_setMatrixMode(params); break;
		case 0x11: cmd_pushMatrix(); break;
		case 0x12: cmd_popMatrix(params); break;
		case 0x13: cmd_storeMatrix(params); break;
		case 0x14: cmd_restoreMatrix(params); break;
		case 0x15: cmd_loadIdentity(); break;
		case 0x16: cmd_load4x4Matrix(params); break;
		case 0x17: cmd_load4x3Matrix(params); break;
		case 0x18: cmd_multiply4x4Matrix(params); break;
		case 0x19: cmd_multiply4x3Matrix(params); break;
		case 0x1A: cmd_multiply3x3Matrix(params); break;
		case 0x1B: cmd_multiplyByScale(params); break;
		case 0x1C: cmd_multiplyByTrans(params); break;
		case 0x20: cmd_vtxColor(params); break;
		case 0x21: cmd_normal(params); break;
		case 0x22: cmd_texcoord(params); break;
		case 0x23: cmd_vertex16Bit(params); break;
		case 0x24: cmd_vertex10Bit(params); break;
		case 0x25: cmd_vertexXY(params); break;
		case 0x26: cmd_vertexXZ(params); break;
		case 0x27: cmd_vertexYZ(params); break;
		case 0x28: cmd_vertexDiff(params); break;
		case 0x29: cmd_setPolygonAttributes(params); break;
		case 0x2a: cmd_setTexImageParameters(params); break;
		case 0x2b: cmd_setPaletteBase(params); break;
		case 0x30: cmd_materialColor0(params); break;
		case 0x31: cmd_materialColor1(params); break;
		case 0x34: cmd_shininess(params); break;
		case 0x32: cmd_setLightVector(params); break;
		case 0x33: cmd_setLightColor(params); break;
		case 0x40: cmd_beginVertexList(params); break;
		case 0x41: cmd_endVertexList(); break;
		case 0x50: cmd_swapBuffers(params); break;
		case 0x60: cmd_setViewport(params); break;
		}
	}

	checkGXFIFOIRQs();		//another check after cmds processed (a lot of values might have been popped);
	GXSTAT |= (1 << 27);
}

void GPU::GXFIFOEventHandler(void* context)
{
	GPU* thisPtr = (GPU*)context;
	thisPtr->onProcessCommandEvent();
}

void GPU::renderWorker(int threadIdx)
{
	Logger::msg(LoggerSeverity::Info, std::format("Render thread {} started", threadIdx));
	while (emuRunning)
	{
		std::unique_lock<std::mutex> lock(m_workerThreads[threadIdx].threadMutex);
		m_workerThreads[threadIdx].cv.wait(lock, [this, threadIdx] { return m_workerThreads[threadIdx].rendering; });
		if (!emuRunning)
			return;

		render(m_workerThreads[threadIdx].yMin, m_workerThreads[threadIdx].yMax);

		m_workerThreads[threadIdx].rendering = false;
	}
}

RenderOutput GPU::output = {};
int GPU::numThreads = 4;
int GPU::linesPerThread = 48;