#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"
#include<queue>

struct GXFIFOCommand
{
	uint8_t command;		//encodes command - only used for initial cmd value pushed to gxfifo
	uint32_t parameter;		//encodes parameters for command
};

class GPU
{
public:
	GPU();
	~GPU();

	void init(InterruptManager* interruptManager, Scheduler* scheduler);

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	void writeGXFIFO(uint32_t value);
	void writeCmdPort(uint32_t address, uint32_t value);

	static void GXFIFOEventHandler(void* context);

	static uint16_t output[256 * 192];
private:
	uint16_t renderBuffer[256 * 192];
	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;

	void onProcessCommandEvent();

	void checkGXFIFOIRQs();
	void processCommand();

	std::queue<uint8_t> m_pendingCommands;
	std::queue<uint32_t> m_pendingParameters;
	std::queue<GXFIFOCommand> GXFIFO;

	uint32_t GXSTAT = {};

	static constexpr uint8_t m_cmdParameterLUT[256] =
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,				//0x0
		1,0,1,1,1,0,16,12,16,12,9,3,3,0,0,0,			//0x10
		1,1,1,2,1,1,1,1,1,1,1,1,0,0,0,0,				//0x20
		1,1,1,1,32,0,0,0,0,0,0,0,0,0,0,0,				//0x30
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,				//0x40
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,				//0x50
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,				//0x60
		3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0					//0x70
	};


	//gpu commands
	void cmd_setMatrixMode(uint32_t* params);
	void cmd_pushMatrix();
	void cmd_popMatrix(uint32_t* params);
	void cmd_storeMatrix(uint32_t* params);
	void cmd_restoreMatrix(uint32_t* params);
	void cmd_loadIdentity();
	void cmd_load4x4Matrix(uint32_t* params);
	void cmd_load4x3Matrix(uint32_t* params);
	void cmd_multiply4x4Matrix(uint32_t* params);
	void cmd_multiply4x3Matrix(uint32_t* params);
	void cmd_multiply3x3Matrix(uint32_t* params);
	void cmd_multiplyByScale(uint32_t* params);
	void cmd_multiplyByTrans(uint32_t* params);
	void cmd_beginVertexList(uint32_t* params);
	void cmd_vertex16Bit(uint32_t* params);
	void cmd_vertex10Bit(uint32_t* params);
	void cmd_endVertexList();
	void cmd_swapBuffers();

	void debug_renderDots();

	//shitty debug command
	double debug_cvtVtx16(uint16_t vtx)
	{
		bool negative = (vtx >> 15);
		if (negative)
			vtx = (~vtx) + 1;
		double result = 0, counter = 4;
		for (int i = 14; i >= 0; i--)
		{
			if ((vtx >> i) & 0b1)
				result += counter;
			counter /= 2;
		}
		if (negative)
			result = -result;
		return result;
	}
};