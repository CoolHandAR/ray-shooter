#include "g_common.h"

#include <string.h>

#include "game_info.h"
#include "u_math.h"

GameAssets assets;

bool Game_LoadAssets()
{
	memset(&assets, 0, sizeof(assets));

	if (!Image_CreateFromPath(&assets.wall_textures, "assets/walltextures3.png"))
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
	if (!Image_CreateFromPath(&assets.pickup_textures, "assets/pickup_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.imp_texture, "assets/imp.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.missile_textures, "assets/missile_sheet.png"))
	{
		return false;
	}
	return true;
}

void Game_DestructAssets()
{
	Image_Destruct(&assets.wall_textures);
	Image_Destruct(&assets.decoration_textures);
	Image_Destruct(&assets.weapon_textures);
	Image_Destruct(&assets.imp_texture);
	Image_Destruct(&assets.pickup_textures);
	Image_Destruct(&assets.missile_textures);
}

GameAssets* Game_GetAssets()
{
	return &assets;
}

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
				if (Object_CheckLine(obj, tile_object))
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
				if (tile_object->type == OT__LIGHT || tile_object->hp <= 0)
				{
					continue;
				}
				//handle pickups
				if (tile_object->type == OT__PICKUP)
				{
					//pickup pickups for player
					if (obj->type == OT__PLAYER)
					{
						Player_HandlePickup(tile_object);
					}
					
					//pickups dont block movement
					continue;
				}
				//we have collided with a missile
				if (tile_object->type == OT__MISSILE)
				{
					//dont collide if we are the owner
					if (tile_object->owner == obj)
					{
						continue;
					}
					//dont collide if we are a missile
					if (obj->type == OT__MISSILE)
					{
						continue;
					}

					//perform direct hit damage
					Missile_DirectHit(tile_object, obj);

					//explode the missile
					Missile_Explode(tile_object);

					//missiles dont block movement
					continue;
				}
				//we are a missile
				if (obj->type == OT__MISSILE)
				{
					//dont collide if hit the owner
					if (obj->owner == tile_object)
					{
						continue;
					}
					//dont collide if hit a missile
					if (obj->type == OT__MISSILE)
					{
						continue;
					}

					//if we have collided with player or a monster
					//perform direct hit damage
					Missile_DirectHit(obj, tile_object);
				}
				
				return false;
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

	//try to move with full direction
	if (Move_CheckStep(obj, p_moveX, p_moveY, 1))
	{
		obj->x = obj->x + p_moveX;
		obj->y = obj->y + p_moveY;
		Map_UpdateObjectTile(obj);
		return true;
	}

	//try to move only x
	if (Move_CheckStep(obj, p_moveX, 0, 1))
	{
		obj->x = obj->x + p_moveX;
		Map_UpdateObjectTile(obj);
		return true;
	}

	//try to move only y
	if (Move_CheckStep(obj, 0, p_moveY, 1))
	{
		obj->y = obj->y + p_moveY;
		Map_UpdateObjectTile(obj);
		return true;
	}

	//movement failed completely
	//so don't move

	return false;
}

void Missile_Update(Object* obj, float delta)
{
	bool exploding = obj->flags & OBJ_FLAG__EXPLODING;

	const MissileInfo* missile_info = &MISSILE_INFO[0];
	const AnimInfo* anim_info = &missile_info->anim_info[(int)exploding];
	
	obj->sprite.anim_speed_scale = 0.5;
	obj->sprite.frame_count = anim_info->frame_count;
	obj->sprite.frame_offset_x = anim_info->x_offset;
	obj->sprite.frame_offset_y = anim_info->y_offset;
	obj->sprite.looping = anim_info->looping;
	obj->sprite.anim_speed_scale = 0.5;
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

	float speed = missile_info->speed * delta;

	//we have moved fully
	if (Move_CheckStep(obj, obj->dir_x * speed, obj->dir_y * speed, 1))
	{
		obj->x = obj->x + (obj->dir_x * speed);
		obj->y = obj->y + (obj->dir_y * speed);
		Map_UpdateObjectTile(obj);
		return;
	}

	//missile exploded
	Missile_Explode(obj);
}

void Missile_DirectHit(Object* obj, Object* target)
{
	if (obj->owner != target)
	{
		const MissileInfo* missile_info = &MISSILE_INFO[0];
		Object_Hurt(target, obj, missile_info->direct_damage);

		obj->flags |= OBJ_FLAG__EXPLODING;
	}
}

void Missile_Explode(Object* obj)
{
	const MissileInfo* missile_info = &MISSILE_INFO[0];

	//cause damage if any 
	if (missile_info->explosion_damage > 0)
	{
		Explosion(obj, 5, missile_info->explosion_damage);
	}

	obj->flags |= OBJ_FLAG__EXPLODING;
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

void Gun_Shoot(Object* obj, float p_x, float p_y, float p_dirX, float p_dirY)
{

}

void Object_Hurt(Object* obj, Object* src_obj, int damage)
{
	if (obj->type != OT__MONSTER)
	{
		return;
	}

	//already dead or no damage
	if (obj->hp <= 0 || damage <= 0)
	{
		return;
	}
	if (src_obj)
	{
		//dont hurt ourselves
		if (src_obj->owner == obj)
		{
			return;
		}

		//if hurt by a player or monster, set as target
		if (src_obj->type == OT__PLAYER || src_obj->type == OT__MONSTER)
		{
			obj->target = src_obj;
		}
		//if hurt by a missile target the missiles owner
		else if (src_obj->type == OT__MISSILE)
		{
			Object* owner = src_obj->owner;

			if (owner && (owner->type == OT__PLAYER || owner->type == OT__MONSTER))
			{
				obj->target = owner;
			}
		}
	}

	obj->hp -= damage;

	//set hurt state
	if (obj->hp > 0)
	{
		Monster_SetState(obj, MS__HIT);
		return;
	}

	//set die state
	Monster_SetState(obj, MS__DIE);
}

bool Object_CheckLine(Object* obj, Object* target)
{
	const int max_tiles = Map_GetTotalTiles();

	float x_point = obj->x - target->x;
	float y_point = obj->y - target->y;

	float ray_dir_x = x_point;
	float ray_dir_y = y_point;
	
	//length of ray from one x or y-side to next x or y-side
	float delta_dist_x = (ray_dir_x == 0) ? 1e30 : fabs(1.0 / ray_dir_x);
	float delta_dist_y = (ray_dir_y == 0) ? 1e30 : fabs(1.0 / ray_dir_y);

	int map_x = (int)obj->x;
	int map_y = (int)obj->y;

	int target_tile_x = (int)target->x;
	int target_tile_y = (int)target->y;

	//we have already reached the target
	if (map_x == target_tile_x && map_y == target_tile_y)
	{
		return true;
	}

	//length of ray from current position to next x or y-side
	float side_dist_x = 0;
	float side_dist_y = 0;

	int step_x = 0;
	int step_y = 0;

	if (x_point > 0)
	{
		step_x = -1;
		side_dist_x = (obj->x - map_x) * delta_dist_x;
	}
	else
	{
		step_x = 1;
		side_dist_x = (map_x + 1.0 - obj->x) * delta_dist_x;
	}
	if (y_point > 0)
	{
		step_y = -1;
		side_dist_y = (obj->y - map_y) * delta_dist_y;
	}
	else
	{
		step_y = 1;
		side_dist_y = (map_y + 1.0 - obj->y) * delta_dist_y;
	}

	//perform DDA
	for (int step = 0; step < max_tiles; step++)
	{
		if (side_dist_x < side_dist_y && map_x != target_tile_x)
		{
			side_dist_x += delta_dist_x;
			map_x += step_x;
		}
		else if(map_y != target_tile_y)
		{
			side_dist_y += delta_dist_y;
			map_y += step_y;
		}
		
		//tile blocks the line
		if (Map_GetTile(map_x, map_y) != EMPTY_TILE)
		{
			return false;
		}

		//we have reached the target
		if (map_x == target_tile_x && map_y == target_tile_y)
		{
			break;
		}
	}

	return true;
}

bool Object_CheckSight(Object* obj, Object* target)
{
	float x_point = obj->x - target->x;
	float y_point = obj->y - target->y;

	//make sure the object is facing the target
	//west
	if (obj->dir_x > 0 && x_point > 0)
	{
		return false;
	}
	//east
	else if (obj->dir_x < 0 && x_point < 0)
	{
		return false;
	}
	//north
	if (obj->dir_y > 0 && y_point > 0)
	{
		return false;
	}
	//south
	else if (obj->dir_y < 0 && y_point < 0)
	{
		return false;
	}

	//calc distance to target
	float d = (obj->x - target->x) * (obj->x - target->x) + (obj->y - target->y) * (obj->y - target->y);

	//very close to target
	if (d <= 10)
	{
		return true;
	}

	//check direct line, making sure no tile is in the way
	if (!Object_CheckLine(obj, target))
	{
		return false;
	}

	return true;
}

Object* Object_Missile(Object* obj, Object* target)
{
	Object* missile = Map_NewObject(OT__MISSILE);

	GameAssets* assets = Game_GetAssets();

	float dir_x = obj->dir_x;
	float dir_y = obj->dir_y;

	//if we have a target calc dir to target
	if (target)
	{
		float point_x = obj->x - target->x;
		float point_y = obj->y - target->y;

		Math_XY_Normalize(&point_x, &point_y);

		dir_x = -point_x;
		dir_y = -point_y;
	}

	missile->sub_type = SUB__MISSILE_FIREBALL;
	missile->type = OT__MISSILE;
	missile->x = obj->x + dir_x * 2;
	missile->y = obj->y + dir_y * 2;
	missile->dir_x = dir_x;
	missile->dir_y = dir_y;
	missile->hp = 5;
	missile->sprite.h_frames = 5;
	missile->sprite.v_frames = 1;
	missile->sprite.img = &assets->missile_textures;

	return missile;
}
