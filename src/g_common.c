#include "g_common.h"

#include <string.h>

#include "game_info.h"
#include "u_math.h"

#include "sound.h"
#include "main.h"

//For shadow casting
#define SHADOW_CHECK_OCTS 8
#define SHADOW_CHECK_HISTORY_MAX 1000

static uint64_t SHADOW_CHECK_HISTORY_BITSET[SHADOW_CHECK_HISTORY_MAX];
static const int SHADOW_MULTIPLES[4][8] = 
{
	{1,  0,  0, -1, -1,  0,  0,  1},
	{0,  1, -1,  0,  0, -1,  1,  0},
	{0,  1,  1,  0,  0, -1, -1,  0},
	{1,  0,  0,  1, -1,  0,  0, -1},
};


void Explosion(Object* obj, float size, int damage)
{
	int x1 = (obj->x - size);
	int y1 = (obj->y - size);

	int x2 = (obj->x + size);
	int y2 = (obj->y + size);

	for (int x = x1; x < x2; x++)
	{
		for (int y = y1; y < y2; y++)
		{
			Object* tile_object = Map_GetObjectAtTile(x, y);

			if (!tile_object)
			{
				continue;
			}

			//ignore self
			if (tile_object == obj)
			{
				continue;
			}
			//ignore owner
			if (tile_object == obj->owner)
			{
				continue;
			}

			//only affects monsters and player
			if (tile_object->type == OT__MONSTER) 
			{
				//check straight line between explosion center and object
				if (Object_CheckLineToTarget(obj, tile_object))
				{
					Object_Hurt(tile_object, obj, damage);
				}
			}
		}
	}
	
}

bool Move_CheckStep(Object* obj, float p_stepX, float p_stepY, float p_size)
{
	int x1 = (obj->x - p_size) + p_stepX;
	int y1 = (obj->y - p_size) + p_stepY;

	int x2 = (obj->x + p_size) + p_stepX;
	int y2 = (obj->y + p_size) + p_stepY;

	obj->col_object = NULL;

	for (int x = x1; x <= x2; x++)
	{
		for (int y = y1; y <= y2; y++)
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

				if (!Object_HandleObjectCollision(obj, tile_object))
				{
					return false;
				}
			}
		}
	}

	//we can move
	return true;
}

bool Move_Object(Object* obj, float p_moveX, float p_moveY)
{
	//nothing to move
	if (p_moveX == 0 && p_moveY == 0)
	{
		return false;
	}

	float old_x = obj->x;
	float old_y = obj->y;

	//try to move with full direction
	if (Move_CheckStep(obj, p_moveX, p_moveY, obj->size))
	{
		obj->x = obj->x + p_moveX;
		obj->y = obj->y + p_moveY;
			
		if (Map_UpdateObjectTile(obj))
		{
			return true;
		}
	}

	obj->x = old_x;
	obj->y = old_y;

	//try to move only x
	if (Move_CheckStep(obj, p_moveX, 0, obj->size))
	{
		obj->x = obj->x + p_moveX;

		if (Map_UpdateObjectTile(obj))
		{
			return true;
		}
	}

	obj->x = old_x;
	obj->y = old_y;

	//try to move only y
	if (Move_CheckStep(obj, 0, p_moveY, obj->size))
	{
		obj->y = obj->y + p_moveY;

		if (Map_UpdateObjectTile(obj))
		{
			return true;
		}
	}

	//movement failed completely
	//so don't move
	obj->x = old_x;
	obj->y = old_y;

	return false;
}

bool Move_CheckArea(Object* obj, float x, float y, float size)
{
	int x1 = x - size;
	int y1 = y - size;

	int x2 = x + size;
	int y2 = y + size;

	for (int x = x1; x <= x2; x++)
	{
		for (int y = y1; y <= y2; y++)
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

				if (!Object_HandleObjectCollision(obj, tile_object))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool Move_Unstuck(Object* obj)
{
	for (int i = 0; i < 8; i++)
	{
		float x_random = Math_randf();
		float y_random = Math_randf();

		int x_sign = (rand() % 100 > 50) ? 1 : -1;
		int y_sign = (rand() % 100 > 50) ? 1 : -1;

		if (Move_Object(obj, x_random * x_sign, y_random * y_sign))
		{
			return true;
		}
	}

	return false;
}

bool Move_Teleport(Object* obj, float x, float y)
{
	int map_width, map_height;
	Map_GetSize(&map_width, &map_height);

	//out of bounds
	if (x < 0 || y < 0 || x + 1 > map_width || y + 1 > map_height)
	{
		return false;
	}
	
	float old_x = obj->x;
	float old_y = obj->y;

	//try teleporting at the direct path
	if (Move_CheckArea(obj, x, y, obj->size * 2))
	{
		obj->x = x;
		obj->y = y;

		//last check
		if (Map_UpdateObjectTile(obj))
		{
			return true;
		}

		//failed
		obj->x = old_x;
		obj->y = old_y;
	}

	int map_x = (int)obj->x;
	int map_y = (int)obj->y;

	int target_x = (int)x;
	int target_y = (int)y;
	
	//try other tiles in a grid area
	const int TEST_SIZE = 2;

	int x1 = target_x - TEST_SIZE;
	int y1 = target_y - TEST_SIZE;

	int x2 = target_x + TEST_SIZE;
	int y2 = target_y + TEST_SIZE;

	for (int x = x1; x < x2; x++)
	{
		for (int y = y1; y < y2; y++)
		{
			if (Move_CheckArea(obj, x, y, 1))
			{
				obj->x = x;
				obj->y = y;

				//last check
				if (Map_UpdateObjectTile(obj))
				{
					return true;
				}

				//failed
				obj->x = old_x;
				obj->y = old_y;
			}
		}
	}
	
	//failed
	obj->x = old_x;
	obj->y = old_y;

	return false;
}

bool Move_Door(Object* obj, float delta)
{
	// 0 == door open
	// 1 == door closed

	if (obj->type != OT__DOOR)
	{
		return false;
	}

	if (obj->state == DOOR_CLOSE && obj->flags & OBJ_FLAG__DOOR_NEVER_CLOSE)
	{
		obj->move_timer = 0;
		obj->state == DOOR_SLEEP;
		return false;
	}
	else if (obj->state == DOOR_SLEEP)
	{
		//door is open
		if (obj->move_timer == 0 && !(obj->flags & OBJ_FLAG__DOOR_NEVER_CLOSE))
		{
			obj->stop_timer += delta;

			//close the door
			if (obj->stop_timer >= DOOR_AUTOCLOSE_TIME)
			{
				obj->state = DOOR_CLOSE;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}		
	}

	bool just_moved = false;

	if (obj->move_timer == 1 || obj->move_timer == 0)
	{
		just_moved = true;

		Map* map = Map_GetMap();
		
		//lazy check all objects if we can move safely
		//since we dont allow objects to take doors tile
		//need to seperate static and moving tiles
		for (int i = 0; i < map->num_objects; i++)
		{
			Object* map_obj = &map->objects[i];

			if (map_obj == obj)
			{
				continue;
			}
	
			if ((int)map_obj->x == obj->x && (int)map_obj->y == (int)obj->y || obj->col_object == map_obj)
			{
				obj->state = DOOR_SLEEP;
				obj->stop_timer = 0;
				return false;
			}
		}
	}

	if (just_moved)
	{
		//play the sound
		Sound_EmitWorldTemp(SOUND__DOOR_ACTION, obj->x, obj->y, 0, 0);
	}

	//open the door
	if (obj->state == DOOR_OPEN)
	{
		obj->move_timer -= delta;

		if (obj->move_timer <= 0)
		{
			obj->move_timer = 0;
			obj->stop_timer = 0;
			obj->state = DOOR_SLEEP;
		}
	}
	else if (obj->state == DOOR_CLOSE)
	{
		obj->move_timer += delta;

		if (obj->move_timer >= 1)
		{
			obj->stop_timer = 0;
			obj->move_timer = 1;
			obj->state = DOOR_SLEEP;
		}
	}

	//redrawn the walls
	Render_RedrawWalls();


	return true;
}

bool Check_IsBlockingTile(int x, int y)
{
	if (Map_GetTile(x, y) != EMPTY_TILE)
	{
		return true;
	}

	Object* tile_obj = Map_GetObjectAtTile(x, y);

	if (tile_obj)
	{
		if (Object_IsSpecialCollidableTile(tile_obj))
		{
			return true;
		}
	}
	

	return false;
}

void Missile_Update(Object* obj, float delta)
{
	bool exploding = obj->flags & OBJ_FLAG__EXPLODING;

	const MissileInfo* missile_info = &MISSILE_INFO[0];
	const AnimInfo* anim_info = &missile_info->anim_info[(int)exploding];
	
	obj->sprite.anim_speed_scale = 1.0;
	obj->sprite.frame_count = anim_info->frame_count;
	obj->sprite.frame_offset_x = anim_info->x_offset;
	obj->sprite.frame_offset_y = anim_info->y_offset;
	obj->sprite.looping = anim_info->looping;
	obj->sprite.playing = true;

	Sprite_UpdateAnimation(&obj->sprite, delta);

	//the explosion animation is still playing
	if (exploding)
	{
		//the animation is finished, delete the object
		if (!obj->sprite.playing)
		{
			Map_DeleteObject(obj);
		}
		return;
	}

	//Map_SetDirtyTempLight();
	//Map_SetTempLight(obj->x, obj->y, 2, 255);

	float speed = missile_info->speed * delta;

	if (Move_CheckStep(obj, obj->dir_x * speed, obj->dir_y * speed, obj->size))
	{
		float old_x = obj->x;
		float old_y = obj->y;

		obj->x = obj->x + (obj->dir_x * speed);
		obj->y = obj->y + (obj->dir_y * speed);

		//we have moved fully
		if (Map_UpdateObjectTile(obj))
		{
			return;
		}

		obj->x = old_x;
		obj->y = old_y;
	
	}

	//missile exploded
	Missile_Explode(obj);
}

void Missile_DirectHit(Object* obj, Object* target)
{
	//already exploded
	if (obj->flags & OBJ_FLAG__EXPLODING)
	{
		return;
	}

	if (obj->owner != target)
	{
		const MissileInfo* missile_info = &MISSILE_INFO[0];
		Object_Hurt(target, obj, missile_info->direct_damage);

		obj->flags |= OBJ_FLAG__EXPLODING;
	}
}

void Missile_Explode(Object* obj)
{
	//already exploded
	if (obj->flags & OBJ_FLAG__EXPLODING)
	{
		return;
	}

	const MissileInfo* missile_info = &MISSILE_INFO[0];

	//cause damage if any 
	if (missile_info->explosion_damage > 0)
	{
		Explosion(obj, 5, missile_info->explosion_damage);
	}

	//play sound
	Sound_EmitWorldTemp(SOUND__FIREBALL_EXPLODE, obj->x, obj->y, 0, 0);

	obj->flags |= OBJ_FLAG__EXPLODING;
}

bool Trace_LineVsObject(float p_x, float p_y, float p_endX, float p_endY, Object* obj, float* r_interX, float* r_interY)
{
	float minDistance = 0;
	float maxDistance = 1;
	int axis = 0;
	float sign = 0;

	float box[2][2];

	float size = obj->size;

	box[0][0] = obj->x - size;
	box[0][1] = obj->y - size;

	box[1][0] = obj->x + size;
	box[1][1] = obj->y + size;

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

	return true;
}

static void ShadowCastRecursive(int x, int y, int x_max, int y_max, int radius, int octant, int row, float start_slope, float end_slope, int xx, int xy, int yx, int yy, IsVisibleFun is_visible_fun)
{
	if (start_slope < end_slope)
	{
		return;
	}
	int radius2 = radius * radius;
	int dimensions = radius * 2 - 1;
	float next_start_slope = start_slope;

	for (int i = row; i <= radius; i++)
	{
		bool blocked = false;

		int dy = -i;

		for (int dx = -i; dx <= 0; dx++)
		{
			float l_slope = (dx - 0.5) / (dy + 0.5);
			float r_slope = (dx + 0.5) / (dy - 0.5);

			if (start_slope < r_slope)
			{
				continue;
			}
			else if (end_slope > l_slope)
			{
				continue;
			}

			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;

			if ((sax < 0 && abs(sax) > x) || (say < 0 && abs(say) > y))
			{
				continue;
			}

			int ax = x + sax;
			int ay = y + say;

			if (ax >= x_max || ay >= y_max)
			{
				continue;
			}

			bool leakage_blocked = false;
			{
				int delta_x = x - ax;
				int delta_y = y - ax;
				//check for leaks
				if (delta_x != 0 && delta_y != 0)
				{
					int dir_x = (x > ax) ? 1 : -1;
					int dir_y = (y > ay) ? 1 : -1;

					if (Check_IsBlockingTile(ax + dir_x, ay) && Check_IsBlockingTile(ax, ay + dir_y))
					{
						leakage_blocked = true;
					}
				}
			}
			
			if ((int)(dx * dx + dy * dy) < radius2 && !leakage_blocked)
			{
				int hx = ax - x + (dimensions) / 2;
				int hy = ay - y + (dimensions) / 2;
				int hindex = hy * (dimensions) + hx;

				uint64_t mask_index = hindex / 64;
				uint64_t local_index = hindex % 64;

				uint64_t mask = ((uint64_t)1 << local_index);

				//bounds check just in case, but probably should be enough
				if (mask_index < SHADOW_CHECK_HISTORY_MAX)
				{
					if (!(SHADOW_CHECK_HISTORY_BITSET[mask_index] & mask))
					{
						SHADOW_CHECK_HISTORY_BITSET[mask_index] |= mask;
						is_visible_fun(x, y, ax, ay);
					}
				}
				else
				{
					is_visible_fun(x, y, ax, ay);
				}
				
			}

			if (blocked) 
			{
				if (Check_IsBlockingTile(ax, ay) || leakage_blocked)
				{
					next_start_slope = r_slope;
					continue;
				}
				else 
				{
					blocked = false;
					start_slope = next_start_slope;
				}
			}
			else if (Check_IsBlockingTile(ax, ay)|| leakage_blocked)
			{
				blocked = true;
				next_start_slope = r_slope;
				ShadowCastRecursive(x, y, x_max, y_max, radius, octant, i + 1, start_slope, l_slope, xx, xy, yx, yy, is_visible_fun);
			}
		}
		
		if (blocked)
		{
			break;
		}
		
	}
}

void Trace_ShadowCast(int x, int y, int radius, IsVisibleFun is_visible_fun)
{
	//adapted from https://jordansavant.com/book/algorithms/shadowcasting.md
	is_visible_fun(x, y, x, y);

	int dim = radius * 2 - 1;
	int size = dim * dim;

	memset(SHADOW_CHECK_HISTORY_BITSET, 0, sizeof(SHADOW_CHECK_HISTORY_BITSET));

	int map_w, map_h;
	Map_GetSize(&map_w, &map_h);

	int x_max = map_w;
	int y_max = map_h;

	for (int oct = 0; oct < SHADOW_CHECK_OCTS; oct++)
	{
		ShadowCastRecursive(x, y, x_max, y_max, radius, oct, 1, 1.0, 0.0, SHADOW_MULTIPLES[0][oct], SHADOW_MULTIPLES[1][oct], SHADOW_MULTIPLES[2][oct], SHADOW_MULTIPLES[3][oct], is_visible_fun);
	}
}


DirEnum DirVectorToDirEnum(int x, int y)
{
	if (x > 0)
	{
		if (y > 0)
		{
			return DIR_NORTH_WEST;
		}
		else if (y < 0)
		{
			return DIR_SOUTH_WEST;
		}
		else
		{
			return DIR_WEST;
		}
	}
	else if (x < 0)
	{
		if (y > 0)
		{
			return DIR_NORTH_EAST;
		}
		else if (y < 0)
		{
			return DIR_SOUTH_EAST;
		}
		else
		{
			return DIR_EAST;
		}
	}
	else if (x == 0)
	{
		if (y > 0)
		{
			return DIR_NORTH;
		}
		else if (y < 0)
		{
			return DIR_SOUTH;
		}
	}

	return DIR_NONE;
}

DirEnum DirVectorToRoundedDirEnum(int x, int y)
{
	if (x > 0)
	{
		return DIR_WEST;
	}
	else if (x < 0)
	{
		return DIR_EAST;
	}
	else if (x == 0)
	{
		if (y > 0)
		{
			return DIR_NORTH;
		}
		else if (y < 0)
		{
			return DIR_SOUTH;
		}
	}

	return DIR_NONE;
}

void Particle_Update(Object* obj, float delta)
{
	if (obj->sprite.img && obj->sprite.frame_count > 0 && obj->sprite.anim_speed_scale > 0)
	{
		Sprite_UpdateAnimation(&obj->sprite, delta);
	}

	if (obj->sub_type == SUB__PARTICLE_BLOOD)
	{
		obj->sprite.v_offset += delta * 4;
	}
	else
	{
		obj->sprite.v_offset -= delta * 4;

	}

	obj->move_timer -= delta;

	if (obj->move_timer < 0 || obj->sprite.v_offset > 1)
	{
		Map_DeleteObject(obj);
	}
}
