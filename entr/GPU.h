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

//todo: change this to 'Vertex', 'Vector' is a bad name
struct Vector
{
	int64_t v[4];
	uint16_t color;
};

struct Poly
{
	uint8_t numVertices;
	Vector m_vertices[4];	//max 4 vertices per polygon.
	bool cw;
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
	uint32_t depthBuffer[256 * 192];
	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;

	void onProcessCommandEvent();

	void checkGXFIFOIRQs();
	void processCommand();

	std::queue<uint8_t> m_pendingCommands;
	std::queue<uint32_t> m_pendingParameters;
	std::queue<GXFIFOCommand> GXFIFO;

	//todo: handle 2 sets of poly/vtx ram, swapped w/ SwapBuffers call
	Vector m_vertexRAM[6144];
	Poly m_polygonRAM[2048];
	uint32_t m_vertexCount = 0, m_polygonCount = 0;
	uint32_t m_runningVtxCount = 0;	//reset at BEGIN_VTXS command

	uint8_t m_primitiveType = 0;

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

	//MVP - used for transforming vtxs to clip space
	Matrix m_clipMatrix = {};

	//currently selected matrix
	Matrix m_projectionMatrix = {};
	Matrix m_coordinateMatrix = {};
	Matrix m_directionalMatrix = {};

	//matrix stacks
	Matrix m_projectionStack = {};
	//apparently these stacks are 32 long? but entry 31 causes overflow flag. need to handle
	Matrix m_coordinateStack[32] = {};
	Matrix m_directionalStack[32] = {};

	//stack pointer for coordinate/directional matrix stacks. they share same SP.
	uint32_t m_coordinateStackPointer = 0;

	//identity matrix - used for MTX_IDENTITY
	Matrix m_identityMatrix = {};

	Vector m_lastVertex = {};
	uint16_t m_lastColor = {};

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
	void cmd_vtxColor(uint32_t* params);
	void cmd_beginVertexList(uint32_t* params);
	void cmd_vertex16Bit(uint32_t* params);
	void cmd_vertex10Bit(uint32_t* params);
	void cmd_vertexXY(uint32_t* params);
	void cmd_vertexXZ(uint32_t* params);
	void cmd_vertexYZ(uint32_t* params);
	void cmd_vertexDiff(uint32_t* params);
	void cmd_endVertexList();
	void cmd_materialColor0(uint32_t* params);
	void cmd_swapBuffers();

	void submitVertex(Vector vtx);
	void submitPolygon();

	void debug_render();
	
	void rasterizePolygon(Poly p);

	void debug_drawLine(int x0, int y0, int x1, int y1);
	void plotLow(int x0, int y0, int x1, int y1);
	void plotHigh(int x0, int y0, int x1, int y1);

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

	//should move this all to a new header.
	
	inline Vector crossProduct(Vector a, Vector b)
	{
		Vector c = {};
		c.v[0] = (a.v[1] * b.v[2]) - (a.v[2] * b.v[1]);
		c.v[1] = (a.v[2] * b.v[0]) - (a.v[0] * b.v[2]);
		c.v[2] = (a.v[0] * b.v[1]) - (a.v[1] * b.v[0]);
		return c;
	}
	
	inline int64_t dotProduct(Vector a, Vector b)
	{
		return (a.v[0] * b.v[0]) + (a.v[1] * b.v[1]) + (a.v[2] * b.v[2]);
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

	inline Matrix gen4x3Matrix(uint32_t* params)
	{
		//4x3 i'll do manually.. fuck it
		Matrix m = {};
		m.m[0] = params[0]; m.m[1] = params[1]; m.m[2] = params[2]; m.m[3] = 0;
		m.m[4] = params[3]; m.m[5] = params[4]; m.m[6] = params[5]; m.m[7] = 0;
		m.m[8] = params[6]; m.m[9] = params[7]; m.m[10] = params[8]; m.m[11] = 0;
		m.m[12] = params[9]; m.m[13] = params[10]; m.m[14] = params[11]; m.m[15] = (1 << 12);
		return m;
	}

	inline Matrix gen3x3Matrix(uint32_t* params)
	{
		Matrix m = {};
		m.m[0] = params[0]; m.m[1] = params[1]; m.m[2] = params[2]; m.m[3] = 0;
		m.m[4] = params[3]; m.m[5] = params[4]; m.m[6] = params[5]; m.m[7] = 0;
		m.m[8] = params[6]; m.m[9] = params[7]; m.m[10] = params[8]; m.m[11] = 0;
		m.m[12] = 0; m.m[13] = 0; m.m[14] = 0; m.m[15] = (1 << 12);
		return m;
	}

	inline int yxToIdx(int y, int x)
	{
		return (y * 4) + x;
	}

	inline Matrix multiplyMatrices(Matrix a, Matrix b)
	{
		//this is so horrible, should look into fast matrix multiplication.
		Matrix res = {};
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				int64_t ay1 = a.m[yxToIdx(y, 0)]; int64_t ay2 = a.m[yxToIdx(y, 1)]; int64_t ay3 = a.m[yxToIdx(y, 2)]; int64_t ay4 = a.m[yxToIdx(y, 3)];
				int64_t b1x = b.m[yxToIdx(0, x)]; int64_t b2x = b.m[yxToIdx(1, x)]; int64_t b3x = b.m[yxToIdx(2, x)]; int64_t b4x = b.m[yxToIdx(3, x)];
				int64_t a = (ay1 * b1x) >> 12; int64_t b = (ay2 * b2x) >> 12; int64_t c = (ay3 * b3x) >> 12; int64_t d = (ay4 * b4x) >> 12;
				res.m[yxToIdx(y, x)] = (int32_t)(a + b + c + d);
			}
		}
		return res;
	}

	inline Vector multiplyVectorMatrix(Vector v, Matrix m)
	{
		//could manually unroll this.
		Vector res = {};
		for (int x = 0; x < 4; x++)
		{
			int64_t ay1 = v.v[0]; int64_t ay2 = v.v[1]; int64_t ay3 = v.v[2]; int64_t ay4 = v.v[3];
			int64_t b1x = m.m[yxToIdx(0, x)]; int64_t b2x = m.m[yxToIdx(1, x)]; int64_t b3x = m.m[yxToIdx(2, x)]; int64_t b4x = m.m[yxToIdx(3, x)];
			int64_t a = (ay1 * b1x) >> 12; int64_t b = (ay2 * b2x) >> 12; int64_t c = (ay3 * b3x) >> 12; int64_t d = (ay4 * b4x) >> 12;
			res.v[x] = (int32_t)(a + b + c + d);
		}
		return res;
	}

	//fixed point helpers
	int32_t fixedPointMul(int32_t a, int32_t b)
	{
		int64_t res = ((int64_t)a) * ((int64_t)b);
		return (res >> 12);
	}

	int32_t fixedPointDiv(int32_t a, int32_t b)
	{
		int64_t quotient = ((int64_t)a << 12) / (int64_t)b;
		return quotient;
	}

	//interpolation
	int linearInterpolate(int x, int y1, int y2, int x1, int x2)
	{
		if (x1 == x2)
		{
			return y1;
		}
		//r1.v[0] + ((y - r1.v[1]) * (r2.v[0] - r1.v[0])) / (r2.v[1] - r1.v[1]);
		return y1 + ((x - x1) * (y2 - y1)) / (x2-x1);
	}

	uint16_t interpolateColor(int x, int y1, int y2, int x1, int x2)
	{
		uint16_t r1 = y1 & 0x1F;
		uint16_t g1 = (y1 >> 5) & 0x1F;
		uint16_t b1 = (y1 >> 10) & 0x1F;
		uint16_t r2 = y2 & 0x1F;
		uint16_t g2 = (y2 >> 5) & 0x1F;
		uint16_t b2 = (y2 >> 10) & 0x1F;
		
		uint16_t r = linearInterpolate(x, r1, r2, x1, x2) & 0x1F;
		uint16_t g = linearInterpolate(x, g1, g2, x1, x2) & 0x1F;
		uint16_t b = linearInterpolate(x, b1, b2, x1, x2) & 0x1F;
		return r | (g << 5) | (b << 10);
	}
};