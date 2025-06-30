#include "u_math.h"

#include <string.h>


uint32_t Math_NearestPowerOf2(uint32_t num)
{
	uint32_t n = 1;

	while (n < num)
		n <<= 1;

	return n;
}

