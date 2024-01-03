#include "GPU.h"

void GPU::cmd_setMatrixMode(uint32_t* params)
{
	uint32_t mode = params[0] & 0b11;
	m_matrixMode = mode;
}

void GPU::cmd_pushMatrix()
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionStack = m_projectionMatrix;
		GXSTAT |= (1 << 13);
		break;
	case 1: case 2:
		m_coordinateStack[m_coordinateStackPointer&0x1F] = m_coordinateMatrix;
		m_directionalStack[m_coordinateStackPointer&0x1F] = m_directionalMatrix;
		m_coordinateStackPointer = (m_coordinateStackPointer + 1) & 63;
		if (m_coordinateStackPointer > 30)
		{
			Logger::msg(LoggerSeverity::Info, "gpu: push oob.");
			GXSTAT |= (1 << 15);
		}
		GXSTAT &= 0xFFFFE0FF;
		GXSTAT |= (m_coordinateStackPointer & 0x1F) << 8;
		break;
	}
}

void GPU::cmd_popMatrix(uint32_t* params)
{
	int32_t offs = params[0] & 0x3F;
	if ((offs >> 5) & 0b1)
		offs |= 0xFFFFFFC0;		//sign extend


	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_projectionStack;
		GXSTAT &= ~(1 << 13);
		break;
	case 1: case 2:
		uint32_t before = m_coordinateStackPointer;
		m_coordinateStackPointer = (m_coordinateStackPointer - offs) & 63;
		if (m_coordinateStackPointer < 0 || m_coordinateStackPointer > 30)	//games might be silly enough to trigger this.
		{
			GXSTAT |= (1 << 15);	
		}
		m_coordinateMatrix = m_coordinateStack[m_coordinateStackPointer&0x1F];
		m_directionalMatrix = m_directionalStack[m_coordinateStackPointer&0x1F];
		GXSTAT &= 0xFFFFE0FF;
		GXSTAT |= (m_coordinateStackPointer & 0x1F) << 8;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_storeMatrix(uint32_t* params)
{
	uint32_t addr = params[0] & 0x1F;

	switch (m_matrixMode)
	{
	case 0:
		m_projectionStack = m_projectionMatrix;
		break;
	case 1: case 2:
		m_coordinateStack[addr] = m_coordinateMatrix;
		m_directionalStack[addr] = m_directionalMatrix;
		break;
	}
}

void GPU::cmd_restoreMatrix(uint32_t* params)
{
	uint32_t addr = params[0] & 0x1F;

	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_projectionStack;
		break;
	case 1: case 2:
		m_coordinateMatrix = m_coordinateStack[addr];
		m_directionalMatrix = m_directionalStack[addr];
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_loadIdentity()
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = m_identityMatrix;
		break;
	case 1:
		m_coordinateMatrix = m_identityMatrix;
		break;
	case 2:
		m_coordinateMatrix = m_identityMatrix;
		m_directionalMatrix = m_identityMatrix;
		break;
	case 3:
		m_textureMatrix = m_identityMatrix;
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_load4x4Matrix(uint32_t* params)
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = gen4x4Matrix(params);
		break;
	case 1:
		m_coordinateMatrix = gen4x4Matrix(params);
		break;
	case 2:
		m_coordinateMatrix = gen4x4Matrix(params);
		m_directionalMatrix = m_coordinateMatrix;
		break;
	case 3:
		m_textureMatrix = gen4x4Matrix(params);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_load4x3Matrix(uint32_t* params)
{
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = gen4x3Matrix(params);
		break;
	case 1:
		m_coordinateMatrix = gen4x3Matrix(params);
		break;
	case 2:
		m_coordinateMatrix = gen4x3Matrix(params);
		m_directionalMatrix = m_coordinateMatrix;
		break;
	case 3:
		m_textureMatrix = gen4x3Matrix(params);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply4x4Matrix(uint32_t* params)
{
	Matrix m = gen4x4Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	case 3:
		m_textureMatrix = multiplyMatrices(m, m_textureMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply4x3Matrix(uint32_t* params)
{
	Matrix m = gen4x3Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	case 3:
		m_textureMatrix = multiplyMatrices(m, m_textureMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiply3x3Matrix(uint32_t* params)
{
	Matrix m = gen3x3Matrix(params);
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	case 3:
		m_textureMatrix = multiplyMatrices(m, m_textureMatrix);
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiplyByScale(uint32_t* params)
{
	Matrix m = {};
	m.m[0] = params[0];
	m.m[5] = params[1];
	m.m[10] = params[2];
	m.m[15] = (1 << 12);

	//directional/vector matrix (why does gbatek use two names) doesn't get updated by MTX_SCALE
	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1: case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 3:
		m_textureMatrix = multiplyMatrices(m, m_textureMatrix);
		break;
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_multiplyByTrans(uint32_t* params)
{
	Matrix m = m_identityMatrix;
	m.m[12] = params[0];
	m.m[13] = params[1];
	m.m[14] = params[2];

	switch (m_matrixMode)
	{
	case 0:
		m_projectionMatrix = multiplyMatrices(m, m_projectionMatrix);
		break;
	case 1:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		break;
	case 2:
		m_coordinateMatrix = multiplyMatrices(m, m_coordinateMatrix);
		m_directionalMatrix = multiplyMatrices(m, m_directionalMatrix);
		break;
	case 3:
		m_textureMatrix = multiplyMatrices(m, m_textureMatrix);
	}

	m_clipMatrix = multiplyMatrices(m_coordinateMatrix, m_projectionMatrix);
}

void GPU::cmd_beginVertexList(uint32_t* params)
{
	uint32_t primitiveType = params[0] & 0b11;		
	m_primitiveType = primitiveType;
	m_runningVtxCount = 0;							//reset running vertex count - primitive type may have changed so we construct new polys with the new vtx list being sent
	curAttributes = pendingAttributes;
}

void GPU::cmd_vertex16Bit(uint32_t* params)
{
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;
	int16_t z = params[1] & 0xFFFF;


	Vertex point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_vertex10Bit(uint32_t* params)
{
	int16_t x = (params[0] & 0x3FF) << 6;
	int16_t y = ((params[0] >> 10) & 0x3FF) << 6;
	int16_t z = ((params[0] >> 20) & 0x3FF) << 6;

	Vertex point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_vertexXY(uint32_t* params)
{
	int16_t x = params[0] & 0xFFFF;
	int16_t y = (params[0] >> 16) & 0xFFFF;

	Vertex point = {};
	point.v[0] = x; point.v[1] = y; point.v[2] = m_lastVertex.v[2]; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_vertexXZ(uint32_t* params)
{
	int16_t x = params[0] & 0xFFFF;
	int16_t z = (params[0] >> 16) & 0xFFFF;

	Vertex point = {};
	point.v[0] = x; point.v[1] = m_lastVertex.v[1]; point.v[2] = z; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_vertexYZ(uint32_t* params)
{
	int16_t y = params[0] & 0xFFFF;
	int16_t z = (params[0] >> 16) & 0xFFFF;

	Vertex point = {};
	point.v[0] = m_lastVertex.v[0]; point.v[1] = y; point.v[2] = z; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_vertexDiff(uint32_t* params)
{
	int16_t x = params[0] & 0x3FF;
	int16_t y = (params[0] >> 10) & 0x3FF;
	int16_t z = (params[0] >> 20) & 0x3FF;
	if ((x >> 9) & 0b1)
		x |= 0xFC00;
	if ((y >> 9) & 0b1)
		y |= 0xFC00;
	if ((z >> 9) & 0b1)
		z |= 0xFC00;
	
	Vertex point = m_lastVertex;
	point.v[0] += x; point.v[1] += y; point.v[2] += z; point.v[3] = (1 << 12);
	submitVertex(point);
}

void GPU::cmd_setPolygonAttributes(uint32_t* params)
{
	pendingAttributes = {};
	pendingAttributes.mode = (params[0] >> 4) & 0b11;
	pendingAttributes.drawBack = (params[0] >> 6) & 0b1;
	pendingAttributes.drawFront = (params[0] >> 7) & 0b1;
	pendingAttributes.depthEqual = (params[0] >> 14) & 0b1;
	pendingAttributes.alpha = (params[0] >> 16) & 0x1F;
	//todo: rest of attribs
}

void GPU::cmd_setTexImageParameters(uint32_t* params)
{
	curTexParams.VRAMOffs = (params[0] & 0xFFFF) << 3;
	curTexParams.repeatX = (params[0] >> 16) & 0b1;
	curTexParams.repeatY = (params[0] >> 17) & 0b1;
	curTexParams.flipX = (params[0] >> 18) & 0b1;
	curTexParams.flipY = (params[0] >> 19) & 0b1;
	curTexParams.sizeX = 8 << ((params[0] >> 20) & 0b111);
	curTexParams.sizeY = 8 << ((params[0] >> 23) & 0b111);
	curTexParams.format = (params[0] >> 26) & 0b111;
	curTexParams.col0Transparent = (params[0] >> 29) & 0b1;
	curTexParams.transformationMode = (params[0] >> 30) & 0b11;
}

void GPU::cmd_setPaletteBase(uint32_t* params)
{
	uint32_t offs = params[0] & 0x1FFF;
	curTexParams.paletteBase = offs;
}

void GPU::cmd_endVertexList()
{
	//this seems to be useless on real hardware.
}

void GPU::cmd_texcoord(uint32_t* params)
{
	int32_t u = (int16_t)(params[0] & 0xFFFF);
	int32_t v = (int16_t)((params[0] >> 16) & 0xFFFF);

	switch (curTexParams.transformationMode)
	{
	case 0:
		curTexcoords[0] = u;
		curTexcoords[1] = v;
		break;
	case 1:
	{
		Vector vec = {};
		vec[0] = (u << 8);
		vec[1] = (v << 8);
		vec[2] = (1 << 8);
		vec[3] = (1 << 8);
		vec = multiplyVectorMatrix(vec, m_textureMatrix);
		curTexcoords[0] = vec[0] >> 8;
		curTexcoords[1] = vec[1] >> 8;
		break;
	}
	default:
		curTexcoords[0] = u;
		curTexcoords[1] = v;
	}
}

void GPU::cmd_vtxColor(uint32_t* params)
{
	//m_lastColor = params[0] & 0x7FFF;
	uint16_t col = params[0] & 0x7FFF;
	m_lastColor.r = col & 0x1F;
	m_lastColor.g = (col >> 5) & 0x1F;
	m_lastColor.b = (col >> 10) & 0x1F;
}

void GPU::cmd_materialColor0(uint32_t* params)
{
	if ((params[0] >> 15) & 0b1)
	{
		uint16_t col = params[0] & 0x7FFF;
		m_lastColor.r = col & 0x1F;
		m_lastColor.g = (col >> 5) & 0x1F;
		m_lastColor.b = (col >> 10) & 0x1F;
	}
}

void GPU::cmd_swapBuffers(uint32_t* params)
{
	//stop cmd processing until vblank
	//then render and restart
	swapBuffersPending = true;
	wBuffer = (params[0] >> 1) & 0b1;
	m_scheduler->removeEvent(Event::GXFIFO);
}

void GPU::cmd_setViewport(uint32_t* params)
{
	//are y1/y2 right?
	viewportX1 = params[0] & 0xFF;
	viewportY1 = 191 - ((params[0] >> 24) & 0xFF);
	viewportX2 = (params[0] >> 16) & 0xFF;
	viewportY2 = 191 - (params[0] >> 8) & 0xFF;
}

void GPU::submitVertex(Vertex vtx)
{
	if(m_vertexCount >= 6144)
		return;
	Vertex clipPoint = {};
	clipPoint.v = multiplyVectorMatrix(vtx.v, m_clipMatrix);
	clipPoint.color = m_lastColor;
	clipPoint.color.a = curAttributes.alpha;

	clipPoint.texcoord[0] = (int64_t)(curTexcoords[0]);
	clipPoint.texcoord[1] = (int64_t)(curTexcoords[1]);
	m_vertexRAM[m_vertexCount++] = clipPoint;
	m_runningVtxCount++;
	submitPolygon();

	m_lastVertex = vtx;
}

void GPU::submitPolygon()
{
	if (m_polygonCount == 2048)
		return;

	Poly p = {};
	p.attribs = curAttributes;
	p.texParams = curTexParams;
	bool submitted = false;

	switch (m_primitiveType)
	{
	case 0:
		if (m_runningVtxCount && ((m_runningVtxCount % 3) == 0))
		{
			p.numVertices = 3;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			m_polygonRAM[m_polygonCount++] = p;
			submitted = true;
		}
		break;
	case 1:
		if (m_runningVtxCount && ((m_runningVtxCount % 4) == 0))
		{
			p.numVertices = 4;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 4];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[3] = m_vertexRAM[m_vertexCount - 1];
			m_polygonRAM[m_polygonCount++] = p;
			submitted = true;
		}
		break;
	case 2:
		if (m_runningVtxCount >= 3)
		{
			//supposedly: ds generates alternating cw/ccw, but we ignore that :)
			//num vtxs odd: n-3,n-2,n-1
			//num vtxs even: n-2,n-3,n-1
			p.numVertices = 3;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 2];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			if (!(m_runningVtxCount & 0b1))
			{
				p.m_vertices[0] = m_vertexRAM[m_vertexCount - 2];
				p.m_vertices[1] = m_vertexRAM[m_vertexCount - 3];
			}
			m_polygonRAM[m_polygonCount++] = p;
			submitted = true;
		}
		break;
	case 3:
		if (m_runningVtxCount >= 4 && (!(m_runningVtxCount & 0b1)))
		{
			p.numVertices = 4;
			p.m_vertices[0] = m_vertexRAM[m_vertexCount - 4];
			p.m_vertices[1] = m_vertexRAM[m_vertexCount - 3];
			p.m_vertices[2] = m_vertexRAM[m_vertexCount - 1];
			p.m_vertices[3] = m_vertexRAM[m_vertexCount - 2];
			m_polygonRAM[m_polygonCount++] = p;
			submitted = true;
		}
		break;
	}

	//holy fuck. this is the singlemost worst piece of shit i've written in my entire life, I hate this
	if (submitted)
	{
		//determine winding order of poly
		m_polygonRAM[m_polygonCount-1].cw = getWindingOrder(p.m_vertices[0], p.m_vertices[1], p.m_vertices[2]);
		bool clip = false;
		Poly clippedPoly = clipPolygon(p,clip);
		clippedPoly.cw = m_polygonRAM[m_polygonCount-1].cw;	//clipping won't affect winding order
		bool cull = (!clippedPoly.attribs.drawBack && clippedPoly.cw) || (!clippedPoly.attribs.drawFront && !clippedPoly.cw);
		if ((clippedPoly.numVertices == 0) || cull)
		{
			switch (m_primitiveType)
			{
			case 0: case 1:
				m_polygonCount--;
				m_vertexCount -= p.numVertices;
				break;
			case 2:
			{
				m_polygonCount--;
				Vertex v2 = m_vertexRAM[m_vertexCount - 2];
				Vertex v3 = m_vertexRAM[m_vertexCount - 1];
				m_vertexCount -= 3;
				m_vertexRAM[m_vertexCount++] = v2;
				m_vertexRAM[m_vertexCount++] = v3;
				break;
			}
			case 3:
			{
				m_polygonCount--;
				Vertex v2 = m_vertexRAM[m_vertexCount - 2];
				Vertex v3 = m_vertexRAM[m_vertexCount - 1];
				m_vertexCount -= 4;
				m_vertexRAM[m_vertexCount++] = v2;
				m_vertexRAM[m_vertexCount++] = v3;
				m_runningVtxCount = 2;
				break;
			}
			}
			return;
		}

		else if (clip)
		{
			m_polygonRAM[m_polygonCount - 1] = clippedPoly;
			//hack for winding order:
			Vertex v1 = m_vertexRAM[m_vertexCount - 3];
			Vertex v2 = m_vertexRAM[m_vertexCount - 2];
			Vertex v3 = m_vertexRAM[m_vertexCount - 1];

			switch (m_primitiveType)
			{
			case 0: case 1:
				m_vertexCount -= p.numVertices;
				for (int i = 0; i < clippedPoly.numVertices; i++)
				{
					if(m_vertexCount<6144)
						m_vertexRAM[m_vertexCount++] = clippedPoly.m_vertices[i];
				}
				break;
			case 2:
			{
				m_vertexCount -= p.numVertices;
				for (int i = 0; i < clippedPoly.numVertices; i++)
				{	
					if(m_vertexCount<6144)
						m_vertexRAM[m_vertexCount++] = clippedPoly.m_vertices[i];
				}
				if (m_vertexCount < 6142)
				{
					m_vertexRAM[m_vertexCount++] = v2;
					m_vertexRAM[m_vertexCount++] = v3;
				}

				break;
			}
			case 3:
			{
				m_vertexCount -= p.numVertices;
				for (int i = 0; i < clippedPoly.numVertices; i++)
				{
					if(m_vertexCount<6144)
						m_vertexRAM[m_vertexCount++] = clippedPoly.m_vertices[i];
				}
				if (m_vertexCount < 6142)
				{
					m_vertexRAM[m_vertexCount++] = v2;
					m_vertexRAM[m_vertexCount++] = v3;
				}
				m_runningVtxCount = 2;
				break;
			}
			}
		}

		//transform poly coordinates from clip space to screenspace
		for (int i = 0; i < m_polygonRAM[m_polygonCount - 1].numVertices; i++)
		{
			Vertex cur = m_polygonRAM[m_polygonCount - 1].m_vertices[i];
			if (cur.v[3] <= 0)			//bad w coord. skip this poly
				return;

			int64_t screenX = (((cur.v[0] + cur.v[3]) * ((viewportX2 - viewportX1) + 1)) / (cur.v[3] << 1)) + viewportX1;
			int64_t screenY = (((-cur.v[1] + cur.v[3]) * ((viewportY2 - viewportY1) + 1)) / (cur.v[3] << 1)) + viewportY1;

			uint64_t z = ((((uint64_t)cur.v[2] << 14) / (int64_t)cur.v[3]) + 0x3FFF) << 9;

			cur.v[0] = screenX;
			cur.v[1] = screenY;
			cur.v[2] = z;
			
			m_polygonRAM[m_polygonCount - 1].m_vertices[i] = cur;
		}
		m_polygonRAM[m_polygonCount - 1].drawable = true;	//all vtxs were transformed - mark as drawable
	}
}

Poly GPU::clipPolygon(Poly p, bool& clip)
{
	Poly out = {};

	clip = false;
	//clip against all 6 planes
	out = clipAgainstEdge(6, p, clip);
	for (int i = 5; i >= 0; i--)
	{
		bool tmp = false;
		out = clipAgainstEdge(i, out, tmp);
		clip |= tmp;
	}
	//don't like setting these again.
	out.attribs = curAttributes;
	out.texParams = curTexParams;
	return out;
}

Poly GPU::clipAgainstEdge(int edge, Poly p, bool& clip)
{
	clip = false;
	Poly out = {};
	//could cleanup with lut?
	int sign = (edge & 0b1);
	int axis = (edge >> 1);	//xxyyzz

	for (int i = 0; i < p.numVertices; i++)
	{
		Vertex cur = p.m_vertices[i];
		Vertex last = p.m_vertices[(i - 1 + p.numVertices) % p.numVertices];

		int64_t curPt = cur.v[axis];
		int64_t lastPt = last.v[axis];
		if (sign)
		{
			curPt = -curPt;
			lastPt = -lastPt;
		}

		//current vtx is in clip plane
		if (curPt >= -cur.v[3])
		{
			//last vtx outside clip plane
			//find intersection between line cur->last and clip plane, then insert
			if (lastPt < -last.v[3])
			{
				Vertex clippedPt = getIntersectingPoint(cur, last, curPt, lastPt);
				out.m_vertices[out.numVertices++] = clippedPt;
				clip = true;
			}
			out.m_vertices[out.numVertices++] = cur;
		}
		//cur vtx not in clip plane, last pt is
		//same as previous case, intersection between line (cur->last) and clip plane
		else if (lastPt >= -last.v[3])
		{
			clip = true;
			Vertex clippedPt = getIntersectingPoint(cur, last, curPt, lastPt);
				out.m_vertices[out.numVertices++] = clippedPt;
		}
	}

	return out;
}

Vertex GPU::getIntersectingPoint(Vertex v0, Vertex v1, int64_t pa, int64_t pb)
{
	Vertex v = {};
	int64_t d1 = pa + v0.v[3];
	int64_t d2 = pb + v1.v[3];
	if (d2 == d1)
		return v0;
	int64_t delta = (d2 - d1);

	v.v[0] = ((d2 * v0.v[0]) - (d1 * v1.v[0])) / delta;
	v.v[1] = ((d2 * v0.v[1]) - (d1 * v1.v[1])) / delta;
	v.v[2] = ((d2 * v0.v[2]) - (d1 * v1.v[2])) / delta;
	v.v[3] = ((d2 * v0.v[3]) - (d1 * v1.v[3])) / delta;
	v.texcoord[0] = ((d2 * v0.texcoord[0]) - (d1 * v1.texcoord[0])) / delta;
	v.texcoord[1] = ((d2 * v0.texcoord[1]) - (d1 * v1.texcoord[1])) / delta;

	int64_t r = ((d2 * v0.color.r) - (d1 * v1.color.r)) / delta;
	int64_t g = ((d2 * v0.color.g) - (d1 * v1.color.g)) / delta;
	int64_t b = ((d2 * v0.color.b) - (d1 * v1.color.b)) / delta;

	v.color.r = r & 0x1F;
	v.color.g = g & 0x1F;
	v.color.b = b & 0x1F;
	v.color.a = v0.color.a;	//<--should be fine? alpha is same for all vtxs across polygon
	return v;
}

bool GPU::getWindingOrder(Vertex v0, Vertex v1, Vertex v2)
{
	//replace z component with vtx's w component. we're still in homogenous clip space so need w to define depth
	v0.v[2] = v0.v[3]; v1.v[2] = v1.v[3]; v2.v[2] = v2.v[3];

	Vector a = {};
	a[0] = v2.v[0] - v1.v[0]; a[1] = v2.v[1] - v1.v[1]; a[2] = v2.v[2] - v1.v[2];
	Vector b = {};
	b[0] = v0.v[0] - v1.v[0]; b[1] = v0.v[1] - v1.v[1]; b[2] = v0.v[2] - v1.v[2];

	Vector cross = crossProduct(a, b);

	//normalize cross product until it fits into a 32 bit int.
	//otherwise we can get a 64 bit int overflow when calculating dot product, which screws up winding order
	while (cross[0] != (int32_t)cross[0] || cross[1] != (int32_t)cross[1] || cross[2] != (int32_t)cross[2])
	{
		cross[0] >>= 4;
		cross[1] >>= 4;
		cross[2] >>= 4;
	}

	int64_t dot = dotProduct(cross, v0.v);
	return (dot < 0);
}
