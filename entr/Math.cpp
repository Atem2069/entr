#include"Math.h"

DSMath::DSMath()
{

}

DSMath::~DSMath()
{

}

uint8_t DSMath::readIO(uint32_t address)
{
	switch (address)
	{
	case 0x04000280:
		return DIVCNT & 0xFF;
	case 0x04000281:
		return ((DIVCNT >> 8) & 0xFF);
	case 0x04000290:
		return DIV_NUMER & 0xFF;
	case 0x04000291:
		return ((DIV_NUMER >> 8) & 0xFF);
	case 0x04000292:
		return ((DIV_NUMER >> 16) & 0xFF);
	case 0x04000293:
		return ((DIV_NUMER >> 24) & 0xFF);
	case 0x04000294:
		return ((DIV_NUMER >> 32) & 0xFF);
	case 0x04000295:
		return ((DIV_NUMER >> 40) & 0xFF);
	case 0x04000296:
		return ((DIV_NUMER >> 48) & 0xFF);
	case 0x04000297:
		return ((DIV_NUMER >> 56) & 0xFF);
	case 0x04000298:
		return DIV_DENOM & 0xFF;
	case 0x04000299:
		return ((DIV_DENOM >> 8) & 0xFF);
	case 0x0400029A:
		return ((DIV_DENOM >> 16) & 0xFF);
	case 0x0400029B:
		return ((DIV_DENOM >> 24) & 0xFF);
	case 0x0400029C:
		return ((DIV_DENOM >> 32) & 0xFF);
	case 0x0400029D:
		return ((DIV_DENOM >> 40) & 0xFF);
	case 0x0400029E:
		return ((DIV_DENOM >> 48) & 0xFF);
	case 0x0400029F:
		return ((DIV_DENOM >> 56) & 0xFF);
	case 0x040002A0:
		m_doDivision();
		return DIV_RESULT & 0xFF;
	case 0x040002A1:
		return ((DIV_RESULT >> 8) & 0xFF);
	case 0x040002A2:
		return ((DIV_RESULT >> 16) & 0xFF);
	case 0x040002A3:
		return ((DIV_RESULT >> 24) & 0xFF);
	case 0x040002A4:
		return ((DIV_RESULT >> 32) & 0xFF);
	case 0x040002A5:
		return ((DIV_RESULT >> 40) & 0xFF);
	case 0x040002A6:
		return ((DIV_RESULT >> 48) & 0xFF);
	case 0x040002A7:
		return ((DIV_RESULT >> 56) & 0xFF);
	case 0x040002A8:
		m_doDivision();
		return DIVREM_RESULT & 0xFF;
	case 0x040002A9:
		return ((DIVREM_RESULT >> 8) & 0xFF);
	case 0x040002AA:
		return ((DIVREM_RESULT >> 16) & 0xFF);
	case 0x040002AB:
		return ((DIVREM_RESULT >> 24) & 0xFF);
	case 0x040002AC:
		return ((DIVREM_RESULT >> 32) & 0xFF);
	case 0x040002AD:
		return ((DIVREM_RESULT >> 40) & 0xFF);
	case 0x040002AE:
		return ((DIVREM_RESULT >> 48) & 0xFF);
	case 0x040002AF:
		return ((DIVREM_RESULT >> 56) & 0xFF);
	case 0x040002B0:
		return SQRTCNT & 0b1;
	case 0x040002B4:
		m_doSqrt();
		return SQRT_RESULT & 0xFF;
	case 0x040002B5:
		return ((SQRT_RESULT >> 8) & 0xFF);
	case 0x040002B6:
		return ((SQRT_RESULT >> 16) & 0xFF);
	case 0x040002B7:
		return ((SQRT_RESULT >> 24) & 0xFF);
	case 0x040002B8:
		return SQRT_PARAM & 0xFF;
	case 0x040002B9:
		return ((SQRT_PARAM >> 8) & 0xFF);
	case 0x040002BA:
		return ((SQRT_PARAM >> 16) & 0xFF);
	case 0x040002BB:
		return ((SQRT_PARAM >> 24) & 0xFF);
	case 0x040002BC:
		return ((SQRT_PARAM >> 32) & 0xFF);
	case 0x040002BD:
		return ((SQRT_PARAM >> 40) & 0xFF);
	case 0x040002BE:
		return ((SQRT_PARAM >> 48) & 0xFF);
	case 0x040002BF:
		return ((SQRT_PARAM >> 56) & 0xFF);
	}
}

void DSMath::writeIO(uint32_t address, uint8_t value)
{
	switch (address)
	{
	case 0x04000280:
		DIVCNT &= 0xFF00; DIVCNT |= value;
		break;
	case 0x04000281:
		DIVCNT &= 0xFF; DIVCNT |= (value << 8);
		break;
	case 0x04000290:
		DIV_NUMER &= 0xFFFFFFFFFFFFFF00; DIV_NUMER |= value;
		break;
	case 0x04000291:
		DIV_NUMER &= 0xFFFFFFFFFFFF00FF; DIV_NUMER |= ((uint64_t)(value) << 8);
		break;
	case 0x04000292:
		DIV_NUMER &= 0xFFFFFFFFFF00FFFF; DIV_NUMER |= ((uint64_t)(value) << 16);
		break;
	case 0x04000293:
		DIV_NUMER &= 0xFFFFFFFF00FFFFFF; DIV_NUMER |= ((uint64_t)(value) << 24);
		break;
	case 0x04000294:
		DIV_NUMER &= 0xFFFFFF00FFFFFFFF; DIV_NUMER |= ((uint64_t)(value) << 32);
		break;
	case 0x04000295:
		DIV_NUMER &= 0xFFFF00FFFFFFFFFF; DIV_NUMER |= ((uint64_t)(value) << 40);
		break;
	case 0x04000296:
		DIV_NUMER &= 0xFF00FFFFFFFFFFFF; DIV_NUMER |= ((uint64_t)(value) << 48);
		break;
	case 0x04000297:
		DIV_NUMER &= 0x00FFFFFFFFFFFFFF; DIV_NUMER |= ((uint64_t)(value) << 56);
		break;
	case 0x04000298:
		DIV_DENOM &= 0xFFFFFFFFFFFFFF00; DIV_DENOM |= value;
		break;
	case 0x04000299:
		DIV_DENOM &= 0xFFFFFFFFFFFF00FF; DIV_DENOM |= ((uint64_t)(value) << 8);
		break;
	case 0x0400029A:
		DIV_DENOM &= 0xFFFFFFFFFF00FFFF; DIV_DENOM |= ((uint64_t)(value) << 16);
		break;
	case 0x0400029B:
		DIV_DENOM &= 0xFFFFFFFF00FFFFFF; DIV_DENOM |= ((uint64_t)(value) << 24);
		break;
	case 0x0400029C:
		DIV_DENOM &= 0xFFFFFF00FFFFFFFF; DIV_DENOM |= ((uint64_t)(value) << 32);
		break;
	case 0x0400029D:
		DIV_DENOM &= 0xFFFF00FFFFFFFFFF; DIV_DENOM |= ((uint64_t)(value) << 40);
		break;
	case 0x0400029E:
		DIV_DENOM &= 0xFF00FFFFFFFFFFFF; DIV_DENOM |= ((uint64_t)(value) << 48);
		break;
	case 0x0400029F:
		DIV_DENOM &= 0x00FFFFFFFFFFFFFF; DIV_DENOM |= ((uint64_t)(value) << 56);
		break;
	case 0x040002B0:
		SQRTCNT = value & 0b1;
		break;
	case 0x040002B8:
		SQRT_PARAM &= 0xFFFFFFFFFFFFFF00; SQRT_PARAM |= value;
		break;
	case 0x040002B9:
		SQRT_PARAM &= 0xFFFFFFFFFFFF00FF; SQRT_PARAM |= ((uint64_t)(value) << 8);
		break;
	case 0x040002BA:
		SQRT_PARAM &= 0xFFFFFFFFFF00FFFF; SQRT_PARAM |= ((uint64_t)(value) << 16);
		break;
	case 0x040002BB:
		SQRT_PARAM &= 0xFFFFFFFF00FFFFFF; SQRT_PARAM |= ((uint64_t)(value) << 24);
		break;
	case 0x040002BC:
		SQRT_PARAM &= 0xFFFFFF00FFFFFFFF; SQRT_PARAM |= ((uint64_t)(value) << 32);
		break;
	case 0x040002BD:
		SQRT_PARAM &= 0xFFFF00FFFFFFFFFF; SQRT_PARAM |= ((uint64_t)(value) << 40);
		break;
	case 0x040002BE:
		SQRT_PARAM &= 0xFF00FFFFFFFFFFFF; SQRT_PARAM |= ((uint64_t)(value) << 48);
		break;
	case 0x040002BF:
		SQRT_PARAM &= 0x00FFFFFFFFFFFFFF; SQRT_PARAM |= ((uint64_t)(value) << 56);
		break;
	}
}

void DSMath::m_doSqrt()
{
	uint8_t mode = SQRTCNT & 0b1;
	uint64_t inp = SQRT_PARAM;
	if (!mode)
		inp &= 0xFFFFFFFF;

	uint64_t result = (uint64_t)sqrt((long double)inp);
	SQRT_RESULT = result & 0xFFFFFFFF;

	//hacks...
	if ((((uint64_t)SQRT_RESULT * (uint64_t)SQRT_RESULT) > inp))
		SQRT_RESULT--;
	if ((int64_t)inp == -1)							
		SQRT_RESULT = -1;
}

void DSMath::m_doDivision()
{
	uint8_t mode = DIVCNT & 0b11;
	int64_t numerator = 0, denominator = 0;
	switch (mode)
	{
	case 0:
	{
		int32_t a = DIV_NUMER & 0xFFFFFFFF;
		int32_t b = DIV_DENOM & 0xFFFFFFFF;
		if (DIV_DENOM==0 || b==0)
		{
			if(DIV_DENOM==0)
				DIVCNT |= (1 << 14);
			DIVREM_RESULT = (int64_t)a;
			int64_t divRes = (a >= 0) ? 0xFFFFFFFFFFFFFFFF : 1;
			DIV_RESULT = divRes & 0xFFFFFFFF;
			if (divRes == 1)
			{
				DIV_RESULT |= 0xFFFFFFFF00000000;
			}
			return;
		}
		if (a == 0x80000000 && b == -1)
		{
			DIV_RESULT = 0x80000000;
			return;
		}
		int32_t res = a / b;
		int32_t remainder = a % b;
		int64_t res_signExtend = (int64_t)res;
		int64_t remainder_signExtend = (int64_t)remainder;
		DIV_RESULT = res_signExtend;
		DIVREM_RESULT = remainder_signExtend;
		break;
	}
	case 1:
	{
		int64_t a = DIV_NUMER;
		int32_t b = DIV_DENOM & 0xFFFFFFFF;
		if (DIV_DENOM==0 || b == 0)
		{
			if(DIV_DENOM==0)
				DIVCNT |= (1 << 14);
			DIVREM_RESULT = a;
			int64_t divRes = (a >= 0) ? -1 : 1;
			DIV_RESULT = divRes;
			return;
		}
		if (a == 0x8000000000000000 && b == -1)
		{
			DIV_RESULT = 0x8000000000000000;
			DIVREM_RESULT = 0;
			return;
		}
		int64_t res = a / b;
		int64_t remainder = a % b;
		DIV_RESULT = res;
		DIVREM_RESULT = remainder;
		break;
	}
	case 2:
	{
		int64_t a = DIV_NUMER;
		int64_t b = DIV_DENOM;
		if (b == 0)
		{
			DIVCNT |= (1 << 14);
			int64_t divRes = (a >= 0) ? -1 : 1;
			DIV_RESULT = divRes;
			DIVREM_RESULT = a;
			return;
		}
		if (a == 0x8000000000000000 && b == -1)
		{
			DIV_RESULT = 0x8000000000000000;
			DIVREM_RESULT = 0;
			return;
		}
		int64_t res = a / b;
		int64_t remainder = a % b;
		DIV_RESULT = res;
		DIVREM_RESULT = remainder;
		break;
	}
	}
}