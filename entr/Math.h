#pragma once

#include"Logger.h"

class DSMath
{
public:
	DSMath();
	~DSMath();

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value);
private:
	uint16_t DIVCNT;
	uint64_t DIV_NUMER;
	uint64_t DIV_DENOM;
	uint64_t DIV_RESULT;
	uint64_t DIVREM_RESULT;

	uint8_t SQRTCNT;
	uint32_t SQRT_RESULT;
	uint64_t SQRT_PARAM;

	void m_doDivision();
	void m_doSqrt();
};