#include "u_math.h"

#include <string.h>


uint32_t Math_NearestPowerOf2(uint32_t num)
{
	uint32_t n = 1;

	while (n < num)
		n <<= 1;

	return n;
}

void Math_mat4_lerp(mat4 from, mat4 to, float t, mat4 dest)
{

}

void Math_Model(vec3 position, vec3 size, float rotation, mat4 dest)
{
	mat4 m;
	glm_mat4_identity(m);

	vec3 pos;
	pos[0] = position[0];
	pos[1] = position[1];
	pos[2] = position[2];
	glm_translate(m, pos);


	if (rotation != 0)
	{
		pos[0] = size[0] * 0.5f;
		pos[1] = size[1] * 0.5f;
		pos[2] = size[2] * 0.5f;

		glm_translate(m, pos);

		vec3 axis;
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;

		glm_rotate(m, glm_rad(rotation), axis);

		pos[0] = size[0] * -0.5f;
		pos[1] = size[1] * -0.5f;
		pos[2] = size[2] * -0.5f;

		glm_translate(m, pos);
	}

	glm_scale(m, size);

	glm_mat4_copy(m, dest);
}

void Math_Model2D(vec2 position, vec2 size, float rotation, mat4 dest)
{
	vec3 pos;
	pos[0] = position[0];
	pos[1] = position[1];
	pos[2] = 0.0f;

	vec3 scale;
	scale[0] = size[0];
	scale[1] = size[1];
	scale[2] = 0.0f;

	Math_Model(pos, scale, rotation, dest);

}