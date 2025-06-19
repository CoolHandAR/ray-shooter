#include "g_common.h"

#include <string.h>

GameAssets assets;

bool Game_LoadAssets()
{
	memset(&assets, 0, sizeof(assets));

	if (!Image_CreateFromPath(&assets.wall_textures, "assets/walltextures3.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.monster_textures, "assets/monsters.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.decoration_textures, "assets/object_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.weapon_textures, "assets/gun_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.imp_texture, "assets/imp.png"))
	{
		return false;
	}

	return true;
}

void Game_DestructAssets()
{
	Image_Destruct(&assets.wall_textures);
	Image_Destruct(&assets.monster_textures);
	Image_Destruct(&assets.decoration_textures);
	Image_Destruct(&assets.weapon_textures);
	Image_Destruct(&assets.imp_texture);
}

GameAssets* Game_GetAssets()
{
	return &assets;
}

bool Move_CheckStep(Object* obj, float p_stepX, float p_stepY, float p_size)
{
	int x1 = (obj->x - p_size) + p_stepX;
	int y1 = (obj->y - p_size) + p_stepY;

	int x2 = (obj->x + p_size) + p_stepX;
	int y2 = (obj->y + p_size) + p_stepY;

	for (int x = x1; x < x2; x++)
	{
		for (int y = y1; y < y2; y++)
		{
			if (Map_GetTile(x, y) != EMPTY_TILE)
			{
				return false;
			}

			Object* tile_object = Map_GetObjectAtTile(x, y);

			if (tile_object != NULL)
			{
				//ignore self
				if (tile_object == obj)
				{
					continue;
				}

				//some objects are ignored from collisions
				if (tile_object->type == OT__LIGHT)
				{
					continue;
				}

				return false;
			}
		}
	}

	return true;
}

bool Move_Object(Object* obj, float p_moveX, float p_moveY)
{
	//nothing to move
	if (p_moveX == 0 && p_moveY == 0)
	{
		return false;
	}

	//try to move with full direction
	if (Move_CheckStep(obj, p_moveX, p_moveY, 0.5))
	{
		obj->x = obj->x + p_moveX;
		obj->y = obj->y + p_moveY;
		return true;
	}

	//try to move only x
	if (Move_CheckStep(obj, p_moveX, 0, 0.5))
	{
		obj->x = obj->x + p_moveX;
		return true;
	}

	//try to move only y
	if (Move_CheckStep(obj, 0, p_moveY, 0.5))
	{
		obj->y = obj->y + p_moveY;
		return true;
	}

	//movement failed completely
	//so don't move

	return false;
}

bool Trace_LineVsObject(float p_x, float p_y, float p_endX, float p_endY, Object* obj)
{
	float minDistance = 0;
	float maxDistance = 1;
	int axis = 0;
	float sign = 0;

	float box[2][2];

	box[0][0] = obj->x - 1;
	box[0][1] = obj->y - 1;

	box[1][0] = obj->x + 1;
	box[1][1] = obj->y + 1;

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

	return true;
}