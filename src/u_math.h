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

inline float Math_XY_Length(float x, float y)
{
	vec2 v;
	v[0] = x;
	v[1] = y;

	return glm_vec2_norm(v);
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
inline void Math_XY_Reflect(float x, float y, float nx, float ny, float* r_x, float* r_y)
{
	float normal_dot = Math_XY_Dot(nx, ny, x, y);

	*r_x = 2.0 * nx * normal_dot - x;
	*r_y = 2.0 * ny * normal_dot - y;
}

inline void Math_XY_Bounce(float x, float y, float nx, float ny, float* r_x, float* r_y)
{
	float t_x = *r_x;
	float t_y = *r_y;

	Math_XY_Reflect(x, y, nx, ny, &t_x, &t_y);

	*r_x = -t_x;
	*r_y = -t_y;
}
inline bool Math_TraceLineVsBox(float p_x, float p_y, float p_endX, float p_endY, float box_x, float box_y, float size, float* r_interX, float* r_interY, float* r_dist)
{
	float minDistance = 0;
	float maxDistance = 1;
	int axis = 0;
	float sign = 0;

	float box[2][2];

	box[0][0] = box_x - size;
	box[0][1] = box_y - size;

	box[1][0] = box_x + size;
	box[1][1] = box_y + size;

	for (int i = 0; i < 2; i++)
	{
		float box_begin = box[0][i];
		float box_end = box[1][i];
		float trace_start = (i == 0) ? p_x : p_y;
		float trace_end = (i == 0) ? p_endX : p_endY;

		float c_min = 0;
		float c_max = 0;
		float c_sign = 0;

		if (trace_start < trace_end)
		{
			if (trace_start > box_end || trace_end < box_begin)
			{
				return false;
			}

			float trace_length = trace_end - trace_start;
			c_min = (trace_start < box_begin) ? ((box_begin - trace_start) / trace_length) : 0;
			c_max = (trace_end > box_end) ? ((box_end - trace_start) / trace_length) : 1;
			c_sign = -1.0;
		}
		else
		{
			if (trace_end > box_end || trace_start < box_begin)
			{
				return false;
			}

			float trace_length = trace_end - trace_start;
			c_min = (trace_start > box_end) ? ((box_end - trace_start) / trace_length) : 0;
			c_max = (trace_end < box_begin) ? ((box_begin - trace_start) / trace_length) : 1;
			c_sign = 1.0;
		}

		if (c_min > minDistance)
		{
			minDistance = c_min;
			axis = i;
			sign = c_sign;
		}
		if (c_max < maxDistance)
		{
			maxDistance = c_max;
		}
		if (maxDistance < minDistance)
		{
			return false;
		}
	}

	if (r_interX) *r_interX = p_x + (p_endX - p_x) * minDistance;
	if (r_interY) *r_interY = p_y + (p_endY - p_y) * minDistance;

	if (r_dist)
	{
		*r_dist = minDistance;
	}

	return true;
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
