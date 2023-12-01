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

struct Matrix
{
	//64 bit int representation might be better. might switch up in the future.
	int32_t m[16];
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

	uint8_t m_matrixMode = 0;

	Matrix m_projectionMatrix = {};
	Matrix m_coordinateMatrix = {};
	Matrix m_directionalMatrix = {};

	Matrix m_projectionStack = {};
	//apparently these stacks are 32 long? but entry 31 causes overflow flag. need to handle
	Matrix m_coordinateStack[32] = {};
	Matrix m_directionalStack[32] = {};

	Matrix m_identityMatrix = {};

	uint32_t m_coordinateStackPointer = 0;


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

	//some matrix helper stuff 
	inline Matrix gen4x4Matrix(uint32_t* params)
	{
		//generating 4x4 matrix is simple :)
		Matrix m = {};
		for (int i = 0; i < 16; i++)
			m.m[i] = params[i];
		return m;
	}

	Matrix gen4x3Matrix(uint32_t* params)
	{
		//4x3 i'll do manually.. fuck it
		Matrix m = {};
		m.m[0] = params[0]; m.m[1] = params[1]; m.m[2] = params[2]; m.m[3] = 0;
		m.m[4] = params[3]; m.m[5] = params[4]; m.m[6] = params[5]; m.m[7] = 0;
		m.m[8] = params[6]; m.m[9] = params[7]; m.m[10] = params[8]; m.m[11] = 0;
		m.m[12] = params[9]; m.m[13] = params[10]; m.m[14] = params[11]; m.m[15] = (1 << 12);
		return m;
	}

	Matrix gen3x3Matrix(uint32_t* params)
	{
		Matrix m = {};
		m.m[0] = params[0]; m.m[1] = params[1]; m.m[2] = params[2]; m.m[3] = 0;
		m.m[4] = params[3]; m.m[5] = params[4]; m.m[6] = params[5]; m.m[7] = 0;
		m.m[8] = params[6]; m.m[9] = params[7]; m.m[10] = params[8]; m.m[11] = 0;
		m.m[12] = 0; m.m[13] = 0; m.m[14] = 0; m.m[15] = (1 << 12);
		return m;
	}
};