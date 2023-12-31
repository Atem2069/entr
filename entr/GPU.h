#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"
#include"NDSMem.h"
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

struct Vertex
{
	int64_t v[4];
	int16_t texcoord[2];
	uint16_t color;
};

struct TextureParameters
{
	//palette base not set by TEXIMAGE_PARAM, but cleaner to keep it here
	uint32_t paletteBase;
	uint32_t VRAMOffs;
	bool repeatX;
	bool repeatY;
	bool flipX;
	bool flipY;
	uint32_t sizeX;
	uint32_t sizeY;
	uint8_t format;
	bool col0Transparent;
	uint8_t transformationMode;
	
};

enum TextureType
{
	Tex_A3I5=1,
	Tex_Palette4Color=2,
	Tex_Palette16Color=3,
	Tex_Palette256Color=4,
	Tex_Compressed=5,
	Tex_A5I3=6,
	Tex_DirectColor=7
};

struct PolyAttributes
{
	bool drawFront;
	bool drawBack;
	uint8_t mode;
	bool depthEqual;
};

struct Poly
{
	uint8_t numVertices;
	Vertex m_vertices[10];	//polygon can be clipped with up to 10 vtxs
	bool cw;
	PolyAttributes attribs;
	TextureParameters texParams;

};

class GPU
{
public:
	GPU();
	~GPU();

	void init(InterruptManager* interruptManager, Scheduler* scheduler);

	void registerMemory(NDSMem* mem)
	{
		m_mem = mem;
	}

	void registerCallbacks(callbackFn DMACallback, void* ctx)
	{
		m_callback = DMACallback;
		m_callbackCtx = ctx;
	}

	uint8_t read(uint32_t address);
	void write(uint32_t address, uint8_t value);

	void writeGXFIFO(uint32_t value);
	void writeCmdPort(uint32_t address, uint32_t value);

	static void GXFIFOEventHandler(void* context);
	static void VBlankEventHandler(void* context);

	static uint16_t output[256 * 192];
private:
	NDSMem* m_mem;
	callbackFn m_callback;
	void* m_callbackCtx;

	uint16_t renderBuffer[256 * 192];
	uint64_t depthBuffer[256 * 192];
	bool wBuffer = false;
	bool swapBuffersPending = false;
	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;

	void onProcessCommandEvent();
	void onVBlank();

	void checkGXFIFOIRQs();
	void processCommand();

	std::queue<uint8_t> m_pendingCommands;
	std::queue<uint32_t> m_pendingParameters;
	std::queue<GXFIFOCommand> GXFIFO;

	uint32_t viewportX1=0, viewportX2=255, viewportY1=0, viewportY2=191;

	//todo: handle 2 sets of poly/vtx ram, swapped w/ SwapBuffers call
	Vertex m_vertexRAM[6144];
	Poly m_polygonRAM[2048];
	uint32_t m_vertexCount = 0, m_polygonCount = 0;
	uint32_t m_runningVtxCount = 0;	//reset at BEGIN_VTXS command

	uint8_t m_primitiveType = 0;

	uint16_t DISP3DCNT = {};
	uint32_t GXSTAT = {};
	uint16_t clearDepth = {};
	uint16_t clearColor = {};	//this is a 32 bit register, bit hacky rn. should fix

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
	Matrix tmp = {};

	//currently selected matrix
	Matrix m_projectionMatrix = {};
	Matrix m_coordinateMatrix = {};
	Matrix m_directionalMatrix = {};
	Matrix m_textureMatrix = {};

	//matrix stacks
	Matrix m_projectionStack = {};
	Matrix m_textureStack = {};
	//apparently these stacks are 32 long? but entry 31 causes overflow flag. need to handle
	Matrix m_coordinateStack[32] = {};
	Matrix m_directionalStack[32] = {};

	//stack pointer for coordinate/directional matrix stacks. they share same SP.
	int32_t m_coordinateStackPointer = 0;

	//identity matrix - used for MTX_IDENTITY
	Matrix m_identityMatrix = {};

	Vertex m_lastVertex = {};
	uint16_t m_lastColor = {};
	int64_t curTexcoords[2] = {};
	PolyAttributes pendingAttributes = {}, curAttributes = {};
	TextureParameters curTexParams = {};

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
	void cmd_texcoord(uint32_t* params);
	void cmd_vertex16Bit(uint32_t* params);
	void cmd_vertex10Bit(uint32_t* params);
	void cmd_vertexXY(uint32_t* params);
	void cmd_vertexXZ(uint32_t* params);
	void cmd_vertexYZ(uint32_t* params);
	void cmd_vertexDiff(uint32_t* params);
	void cmd_setPolygonAttributes(uint32_t* params);
	void cmd_setTexImageParameters(uint32_t* params);
	void cmd_setPaletteBase(uint32_t* params);
	void cmd_endVertexList();
	void cmd_materialColor0(uint32_t* params);
	void cmd_swapBuffers(uint32_t* params);
	void cmd_setViewport(uint32_t* params);

	void submitVertex(Vertex vtx);
	void submitPolygon();

	void render();
	
	void rasterizePolygon(Poly p);
	uint16_t decodeTexture(int32_t u, int32_t v, TextureParameters params);

	void debug_drawLine(int x0, int y0, int x1, int y1);
	void plotLow(int x0, int y0, int x1, int y1);
	void plotHigh(int x0, int y0, int x1, int y1);

	Poly clipPolygon(Poly p, bool& clip);
	Poly clipAgainstEdge(int edge, Poly p, bool& clip);
	Vertex getIntersectingPoint(Vertex v0, Vertex v1, int64_t pa, int64_t pb);
	bool getWindingOrder(Vertex v0, Vertex v1, Vertex v2);

	//should move this all to a new header.
	
	inline Vertex crossProduct(Vertex a, Vertex b)
	{
		Vertex c = {};
		c.v[0] = (a.v[1] * b.v[2]) - (a.v[2] * b.v[1]);
		c.v[1] = (a.v[2] * b.v[0]) - (a.v[0] * b.v[2]);
		c.v[2] = (a.v[0] * b.v[1]) - (a.v[1] * b.v[0]);
		return c;
	}
	
	inline int64_t dotProduct(Vertex a, Vertex b)
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
				int64_t a = (ay1 * b1x); int64_t b = (ay2 * b2x); int64_t c = (ay3 * b3x); int64_t d = (ay4 * b4x);
				res.m[yxToIdx(y, x)] = (a + b + c + d) >> 12;
			}
		}
		return res;
	}

	inline Vertex multiplyVectorMatrix(Vertex v, Matrix m)
	{
		//could manually unroll this.
		Vertex res = {};
		for (int x = 0; x < 4; x++)
		{
			int64_t ay1 = v.v[0]; int64_t ay2 = v.v[1]; int64_t ay3 = v.v[2]; int64_t ay4 = v.v[3];
			int64_t b1x = m.m[yxToIdx(0, x)]; int64_t b2x = m.m[yxToIdx(1, x)]; int64_t b3x = m.m[yxToIdx(2, x)]; int64_t b4x = m.m[yxToIdx(3, x)];
			int64_t a = (ay1 * b1x); int64_t b = (ay2 * b2x); int64_t c = (ay3 * b3x); int64_t d = (ay4 * b4x);
			res.v[x] = (a + b + c + d) >> 12;
		}
		return res;
	}

	//fixed point helpers
	int64_t fixedPointMul(int64_t a, int64_t b)
	{
		int64_t res = ((int64_t)a) * ((int64_t)b);
		return (res >> 12);
	}

	int64_t fixedPointDiv(int64_t a, int64_t b)
	{
		int64_t quotient = ((int64_t)a << 12) / (int64_t)b;
		return quotient;
	}

	//interpolation
	int64_t linearInterpolate(int64_t x, int64_t y1, int64_t y2, int64_t x1, int64_t x2)
	{
		if (x1 == x2)
		{
			return y1;
		}
		//r1.v[0] + ((y - r1.v[1]) * (r2.v[0] - r1.v[0])) / (r2.v[1] - r1.v[1]);
		return y1 + ((x - x1) * (y2 - y1)) / (x2-x1);
	}

	uint64_t calcFactor(uint64_t length, uint64_t x, uint64_t w1, uint64_t w2, uint64_t shiftAmt)
	{
		uint64_t w1Denom = w1;
		if (shiftAmt == 9)
		{
			w1 >>= 1;
			w2 >>= 1;
			w1Denom >>= 1;
			if ((w1 & 1) && !(w2 & 1))
				w1Denom++;
		}

		uint64_t numer = ((x * w1) << shiftAmt);
		uint64_t denom = (((length - x) * w2) + (x * w1Denom));
		if (!denom)
			return 0;
		return numer / denom;
	}

	int64_t interpolatePerspectiveCorrect(int64_t factor, int64_t shiftAmt, int64_t u1, int64_t u2)
	{
		//if (u1 <= u2)
		//	return u1 + (((u2 - u1) * factor) >> shiftAmt);
		//else
			return u2 + (((u1 - u2) * ((1<<shiftAmt) - factor)) >> shiftAmt);
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

	uint16_t interpolateColorPerspectiveCorrect(int64_t factor, int64_t shiftAmt, uint16_t col1, uint16_t col2)
	{
		uint16_t r1 = col1 & 0x1F;
		uint16_t g1 = (col1 >> 5) & 0x1F;
		uint16_t b1 = (col1 >> 10) & 0x1F;

		uint16_t r2 = col2 & 0x1F;
		uint16_t g2 = (col2 >> 5) & 0x1F;
		uint16_t b2 = (col2 >> 10) & 0x1F;

		uint16_t r = interpolatePerspectiveCorrect(factor, shiftAmt, r1, r2) & 0x1F;
		uint16_t g = interpolatePerspectiveCorrect(factor, shiftAmt, g1, g2) & 0x1F;
		uint16_t b = interpolatePerspectiveCorrect(factor, shiftAmt, b1, b2) & 0x1F;

		return r | (g << 5) | (b << 10);
	}

	//read fns for tex/pal mem
	//could potentially speed up by figuring out what slot a texture lies in only initially, then just reading from page?
	uint8_t gpuReadTex(uint32_t address)
	{
		uint8_t page = (address >> 17);
		uint32_t offset = address & 0x1FFFF;
		uint8_t* pagePtr = m_mem->TexturePageTable[page];
		if (!pagePtr)
			return 0;
		return pagePtr[offset];
	}

	uint8_t gpuReadPal(uint32_t address)
	{

		uint8_t page = (address >> 14);
		uint32_t offset = address & 0x3FFF;
		uint8_t* pagePtr = m_mem->TexPalettePageTable[page];
		if (!pagePtr)
			return 0;
		return pagePtr[offset];
	}

	uint16_t gpuReadPal16(uint32_t address)
	{
		uint8_t page = (address >> 14);
		uint32_t offset = address & 0x3FFF;
		uint8_t* pagePtr = m_mem->TexPalettePageTable[page];
		if (!pagePtr)
			return 0;
		return (pagePtr[offset + 1] << 8) | pagePtr[offset];
	}
};