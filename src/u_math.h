#ifndef UTILITY_MATH
#define UTILITY_MATH
#pragma once

#include <cglm/cglm.h>
#include <math.h>


#define Math_PI 3.1415926535897932384626433833
#define CMP_EPSILON 0.00001

static unsigned long long Math_rng_seed = 1;

static void Math_srand(unsigned long long p_seed)
{
	Math_rng_seed = p_seed;
}

static inline uint32_t Math_rand()
{
	Math_rng_seed = (214013 * Math_rng_seed + 2531011);
	return (Math_rng_seed >> 32) & RAND_MAX;
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


static inline bool Math_AABB_PlanesIntersect(vec3 box[2], vec4* planes, int numPlanes)
{
	float* p, dp;
	int    i;

	for (i = 0; i < numPlanes; i++) 
	{
		p = planes[i];
		dp = p[0] * box[p[0] > 0.0f][0]
			+ p[1] * box[p[1] > 0.0f][1]
			+ p[2] * box[p[2] > 0.0f][2];

		if (dp < -p[3])
			return false;
	}

	return true;
}
static inline bool Math_AABB_PlanesFullyContain(vec3 box[2], vec4* planes, int numPlanes)
{
	float* p, dp;
	int    i;

	for (i = 0; i < numPlanes; i++) 
	{
		p = planes[i];
		dp = p[0] * box[p[0] < 0.0f][0]
			+ p[1] * box[p[1] < 0.0f][1]
			+ p[2] * box[p[2] < 0.0f][2];

		if (dp < -p[3])
			return false;
	}

	return true;
}

inline float Math_Clamp(float v, float min_v, float max_v)
{
	return v < min_v ? min_v : (v > max_v ? max_v : v);
}
inline double Math_Clampd(double v, double min_v, double max_v)
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

inline void Math_vec3_dir_to(vec3 from, vec3 to, vec3 dest)
{
	vec3 v;
	v[0] = to[0] - from[0];
	v[1] = to[1] - from[1];
	v[2] = to[2] - from[2];

	glm_vec3_normalize(v);
	glm_vec3_copy(v, dest);
};

inline void Math_vec3_scaleadd(vec3 va, vec3 vb, float scale, vec3 dest)
{
	dest[0] = va[0] + scale * vb[0];
	dest[1] = va[1] + scale * vb[1];
	dest[2] = va[2] + scale * vb[2];
}

inline bool Math_vec3_is_zero(vec3 v)
{
	for (int i = 0; i < 3; i++)
	{
		if (v[i] != 0)
		{
			return false;
		}
	}
	return true;
}

inline bool Math_AABB_IntersectsTrace(vec3 box[2], vec3 start, vec3 end, vec3 r_normal, vec3 r_intersection, float* r_dist)
{
	float minDistance = 0;
	float maxDistance = 1;
	int axis = 0;
	float sign = 0;

	for (int i = 0; i < 3; i++)
	{
		float box_begin = box[0][i];
		float box_end = box[1][i];
		float trace_start = start[i];
		float trace_end = end[i];

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
	
	if (r_normal)
	{
		glm_vec3_zero(r_normal);
		r_normal[axis] = sign;
	}
	if (r_intersection)
	{
		r_intersection[0] = start[0] + (end[0] - start[0]) * minDistance;
		r_intersection[1] = start[1] + (end[1] - start[1]) * minDistance;
		r_intersection[2] = start[2] + (end[2] - start[2]) * minDistance;
	}
	if (r_dist)
	{
		*r_dist = minDistance;
	}

	return true;
}

inline void Math_vec3_reflect(vec3 v, vec3 normal, vec3 dest)
{
	float d = glm_vec3_dot(v, normal);

	dest[0] = 2.0 * normal[0] * d - v[0];
	dest[1] = 2.0 * normal[1] * d - v[1];
	dest[2] = 2.0 * normal[2] * d - v[2];
}
inline void Math_vec3_bounce(vec3 v, vec3 normal, vec3 dest)
{
	//vec3 reflected;
	//Math_vec3_reflect(v, normal, reflected);

	//glm_vec3_negate_to(reflected, dest);
	float d = glm_vec3_dot(v, normal);

	dest[0] = v[0] - 2.0 * d * normal[0];
	dest[1] = v[1] - 2.0 * d * normal[1];
	dest[2] = v[2] - 2.0 * d * normal[2];
}

inline float Math_vec3_get_pitch(vec3 v)
{
	return asinf(-v[1]);
}
inline float Math_vec3_get_yaw(vec3 v)
{
	return atan2f(v[0], v[2]);
}


inline int Math_GetNormalSignbits(vec3 normal)
{
	int bits = 0;

	for (int i = 0; i < 3; i++)
	{
		if (normal[i] < 0)
		{
			bits |= 1 << i;
		}
	}

	return bits;
}

uint32_t Math_NearestPowerOf2(uint32_t num);

void Math_mat4_lerp(mat4 from, mat4 to, float t, mat4 dest);


typedef struct M_Rect2Df
{
	float x, y;
	float width, height;
} M_Rect2Df;

typedef struct M_Rect2Di
{
	int x, y;
	int width, height;
} M_Rect2Di;


void Math_Model(vec3 position, vec3 size, float rotation, mat4 dest);
void Math_Model2D(vec2 position, vec2 size, float rotation, mat4 dest);

#endif // !UTILITY_MATH
