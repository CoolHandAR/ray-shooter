#ifndef UTILITY_MATH
#define UTILITY_MATH
#pragma once

#include <cglm/cglm.h>
#include <math.h>


#define Math_PI 3.1415926535897932384626433833
#define CMP_EPSILON 0.00001


static inline uint32_t Math_rand()
{
	return rand();
}

static inline float Math_randf()
{
	return (float)Math_rand() / (float)RAND_MAX;
}

static inline double Math_randd()
{
	return (double)Math_rand() / (double)RAND_MAX;
}
static inline float Math_randfb()
{
	return (Math_rand() & 0x7fff) / (float)0x7fff;
}

inline float Math_Clamp(float v, float min_v, float max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}
inline double Math_Clampd(double v, double min_v, double max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}
inline long Math_Clampl(long v, long min_v, long max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}

inline bool Math_IsZeroApprox(float s)
{
	return fabs(s) < (float)CMP_EPSILON;
}
inline int Math_signf(float x)
{
	return (x < 0) ? -1 : 1;
}
inline float Math_sign_float(float x)
{
	return x > 0 ? +1.0f : (x < 0 ? -1.0f : 0.0f);
}
inline float Math_move_towardf(float from, float to, float delta)
{
	return fabsf(to - from) <= delta ? to : from + Math_sign_float(to - from) * delta;
}

inline long double Math_fract2(long double x)
{
	return x - floor(x);
}

inline int Math_step(float edge, float x)
{
	return x < edge ? 0 : 1;
}

inline float Math_XY_Dot(float x1, float y1, float x2, float y2)
{
	return x1 * x2 + y1 * y2;
}

inline void Math_XY_Normalize(float* x, float* y)
{
	vec2 v;
	v[0] = *x;
	v[1] = *y;

	glm_vec2_normalize(v);

	*x = v[0];
	*y = v[1];
}


uint32_t Math_NearestPowerOf2(uint32_t num);

inline bool Math_RayIntersectsPlane(float x, float y, float ray_x, float ray_y, float normal_x, float normal_y, float d)
{
	float den = Math_XY_Dot(normal_x, normal_y, ray_x, ray_y);

	if (den == 0)
	{
		return false;
	}

	float dist = (Math_XY_Dot(normal_x, normal_y, x, y) - d) / den;

	if (dist > CMP_EPSILON)
	{
		return false;
	}

	dist = -dist;

	return true;
}


#endif // !UTILITY_MATH
