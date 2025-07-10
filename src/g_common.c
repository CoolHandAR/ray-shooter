#include "g_common.h"

#include <string.h>

#include "game_info.h"
#include "u_math.h"

#include "sound.h"
#include "main.h"

static Game game;
static GameAssets assets;

bool Game_Init()
{
	memset(&game, 0, sizeof(game));

	game.state = GS__LEVEL;

	if (!Game_LoadAssets())
	{
		return false;
	}

	return true;
}

void Game_Exit()
{
	Game_DestructAssets();
}

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
	if (!Image_CreateFromPath(&assets.minigun_texture, "assets/minigun_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.pinky_texture, "assets/pinky_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.sky_texture, "assets/sky.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.particle_textures, "assets/particle_sheet.png"))
	{
		return false;
	}
	if (!Image_CreateFromPath(&assets.menu_texture, "assets/menu.png"))
	{
		return false;
	}
	assets.wall_textures.h_frames = 7;
	assets.wall_textures.v_frames = 1;

	assets.decoration_textures.h_frames = 4;
	assets.decoration_textures.v_frames = 2;

	assets.weapon_textures.h_frames = 10;
	assets.weapon_textures.v_frames = 1;

	assets.pickup_textures.h_frames = 4;
	assets.pickup_textures.v_frames = 1;

	assets.imp_texture.h_frames = 9;
	assets.imp_texture.v_frames = 9;

	assets.missile_textures.h_frames = 5;
	assets.missile_textures.v_frames = 1;

	assets.minigun_texture.h_frames = 3;
	assets.minigun_texture.v_frames = 1;

	assets.pinky_texture.h_frames = 8;
	assets.pinky_texture.v_frames = 6;

	assets.particle_textures.h_frames = 4;
	assets.particle_textures.v_frames = 2;

	Image_GenerateFrameInfo(&assets.wall_textures);
	Image_GenerateFrameInfo(&assets.decoration_textures);
	Image_GenerateFrameInfo(&assets.weapon_textures);
	Image_GenerateFrameInfo(&assets.pickup_textures);
	Image_GenerateFrameInfo(&assets.imp_texture);
	Image_GenerateFrameInfo(&assets.missile_textures);
	Image_GenerateFrameInfo(&assets.minigun_texture);
	Image_GenerateFrameInfo(&assets.pinky_texture);
	Image_GenerateFrameInfo(&assets.particle_textures);
	//Image_GenerateFrameInfo(&assets.menu_texture);

	Image_GenerateMipmaps(&assets.wall_textures);

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
	Image_Destruct(&assets.minigun_texture);
	Image_Destruct(&assets.pinky_texture);
	Image_Destruct(&assets.particle_textures);
	Image_Destruct(&assets.menu_texture);
}

void Game_Update(float delta)
{
	GLFWwindow* window = Engine_GetWindow();

	switch (game.state)
	{
	case GS__MENU:
	{
		Menu_Update(delta);
		break;
	}
	case GS__LEVEL:
	{
		Player_Update(window, delta);
		Map_UpdateObjects(delta);

		if (game.secret_timer > 0) game.secret_timer -= delta;
		break;
	}
	case GS__LEVEL_END:
	{
		Menu_LevelEnd_Update(delta, game.prev_secrets_found, game.prev_total_secrets, game.prev_monsters_killed, game.prev_total_monsters);
		break;
	}
	default:
		break;
	}

}

void Game_Draw(Image* image, FontData* fd, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	switch (game.state)
	{
	case GS__MENU:
	{
		Menu_Draw(image, fd);
		break;
	}
	case GS__LEVEL:
	{
		//sort and draw map objects
		Map_DrawObjects(image, depth_buffer, draw_spans, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY);

		//draw player stuff (gun and hud)
		Player_Draw(image, fd, draw_spans);

		if (game.secret_timer > 0)
		{
			Text_DrawColor(image, fd, 0.25, 0.2, 1, 1, 255, 255, 255, 255, "SECRET FOUND!");
		}

		break;
	}
	case GS__LEVEL_END:
	{
		Menu_LevelEnd_Draw(image, fd);
		break;
	}
	default:
		break;
	}
}

void Game_SetState(GameState state)
{
	if (state != game.state)
	{
		Render_RedrawWalls();
		Render_RedrawSprites();
	}

	game.state = state;
}

GameState Game_GetState()
{
	return game.state;
}

GameAssets* Game_GetAssets()
{
	return &assets;
}

void Game_ChangeLevel()
{
	game.prev_total_monsters = game.total_monsters;
	game.prev_total_secrets = game.total_secrets;
	game.prev_secrets_found = game.secrets_found;
	game.prev_monsters_killed = game.monsters_killed;

	game.total_secrets = 0;
	game.total_monsters = 0;
	game.secrets_found = 0;
	game.monsters_killed = 0;

	Map* map = Map_GetMap();

	map->level_index++;

	if (map->level_index > 1)
	{
		map->level_index = 1;
	}

	const char* level = LEVELS[map->level_index];

	Map_Load(level);
	Player_Init();

	Game_SetState(GS__LEVEL_END);
}

void Game_Reset(bool to_start)
{
	int index = 0;

	if (!to_start)
	{
		index = Map_GetLevelIndex();
	}

	const char* level = LEVELS[index];

	Map_Load(level);

	Player_Init();
}

void Game_SecretFound()
{
	Sound_Emit(SOUND__SECRET_FOUND);

	game.secret_timer = 1;
	game.secrets_found++;
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

		int x_sign = (rand() % 50 > 50) ? 1 : -1;
		int y_sign = (rand() % 50 > 50) ? 1 : -1;

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

	if (Move_CheckStep(obj, obj->dir_x * speed, obj->dir_y * speed, 0.25))
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

void Gun_Shoot(Object* obj, float p_x, float p_y, float p_dirX, float p_dirY)
{
	float map_hit_x = 0;
	float map_hit_y = 0;



	int render_w;
	Render_GetRenderSize(&render_w, NULL);

	int half_w = render_w / 2;
	int center_x = (half_w) - 1;
	int shoot_delta = render_w / 8;

	//Map_Raycast()

	Map* map = Map_GetMap();

	Object* closest = NULL;
	Object* old_closest = NULL;

	float max_dist = FLT_MAX;

	float hit_x = 0;
	float hit_y = 0;

	while (true)
	{
		old_closest = closest;

		for (int i = 0; i < map->num_sorted_objects; i++)
		{
			ObjectID id = map->sorted_list[i];
			Object* object = &map->objects[id];

			//behind the plane
			if (object->view_y <= 0)
			{
				continue;
			}

			int screen_x = (int)((half_w) * (1 + object->view_x / object->view_y));

			if (object->type == OT__MONSTER && object->hp > 0 && abs(screen_x - center_x) < shoot_delta)
			{
				float dist = (p_x - object->x) * (p_x - object->x) + (p_y - object->y) * (p_y - object->y);

				if (dist < max_dist)
				{
					max_dist = dist;
					closest = object;
				}

			}
		}

		if (closest == old_closest)
		{

			if (Map_Raycast(p_x, p_y, p_dirX, p_dirY, &map_hit_x, &map_hit_y) != EMPTY_TILE)
			{
				Object_Spawn(OT__PARTICLE, SUB__PARTICLE_WALL_HIT, (map_hit_x), (map_hit_y));
				return;
			}
			return;
		}

		if (Object_CheckLineToTarget(obj, closest))
		{			
			break;
		}
	}
	

	Object* ray_obj = closest;

	if (ray_obj && ray_obj->type == OT__MONSTER)
	{
		int dmg = 4;
		//modify damage based on distance
		if (max_dist <= 5)
		{
			dmg *= 4;
		}
		else if (max_dist <= 10)
		{
			dmg *= 2;
		}

		//way too far away
		if (max_dist >= 100)
		{
			return;
		}

		if (rand() % 100 > 50)
		{
			Object_Spawn(OT__PARTICLE, SUB__PARTICLE_BLOOD, (ray_obj->x) + (Math_randf() * 0.5), (ray_obj->y) + (Math_randf() * 0.5));
		}

		Object_Hurt(ray_obj, obj, dmg);
	}
}

void Object_Hurt(Object* obj, Object* src_obj, int damage)
{
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
			if (src_obj->sub_type != obj->sub_type)
			{
				obj->target = src_obj;
			}
		}
		//if hurt by a missile target the missiles owner
		else if (src_obj->type == OT__MISSILE)
		{
			Object* owner = src_obj->owner;

			if (owner && (owner->type == OT__PLAYER || owner->type == OT__MONSTER) && owner->sub_type != obj->sub_type)
			{
				obj->target = owner;
			}
		}
	}

	obj->hp -= damage;

	//set hurt state
	if (obj->hp > 0)
	{
		if (obj->type == OT__MONSTER)
		{
			Monster_SetState(obj, MS__HIT);
		}
		else if (obj->type == OT__PLAYER)
		{
			float dir_x = obj->x - src_obj->x;
			float dir_y = obj->y - src_obj->y;

			Player_Hurt(dir_x, dir_y);
		}
		return;
	}

	//set die state
	if (obj->type == OT__MONSTER)
	{
		game.monsters_killed++;
		Monster_SetState(obj, MS__DIE);
	}
	else if (obj->type == OT__PLAYER)
	{
		float dir_x = obj->x - src_obj->x;
		float dir_y = obj->y - src_obj->y;

		Player_Hurt(dir_x, dir_y);
	}
}

bool Object_CheckLineToTile(Object* obj, float target_x, float target_y)
{
	const int max_tiles = Map_GetTotalTiles();

	float x_point = obj->x - target_x;
	float y_point = obj->y - target_y;

	float ray_dir_x = x_point;
	float ray_dir_y = y_point;

	//length of ray from one x or y-side to next x or y-side
	float delta_dist_x = (ray_dir_x == 0) ? 1e30 : fabs(1.0 / ray_dir_x);
	float delta_dist_y = (ray_dir_y == 0) ? 1e30 : fabs(1.0 / ray_dir_y);

	int map_x = (int)obj->x;
	int map_y = (int)obj->y;

	int target_tile_x = (int)target_x;
	int target_tile_y = (int)target_y;

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
		if (map_x == target_tile_x && map_y != target_tile_y)
		{
			side_dist_y += delta_dist_y;
			map_y += step_y;
		}
		else if (map_y == target_tile_y && map_x != target_tile_x)
		{
			side_dist_x += delta_dist_x;
			map_x += step_x;
		}
		else if(map_x != target_tile_x && map_y != target_tile_y)
		{
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				map_x += step_x;
			}
			else
			{
				side_dist_y += delta_dist_y;
				map_y += step_y;
			}
		}
		else
		{
			//we have reached the target
			if (map_x == target_tile_x && map_y == target_tile_y)
			{
				break;
			}
		}

		//tile blocks the line
		if (Map_GetTile(map_x, map_y) != EMPTY_TILE)
		{
			return false;
		}

		//check for blocking objects
		Object* tile_obj = Map_GetObjectAtTile(map_x, map_y);

		if (!tile_obj)
		{
			continue;
		}

		//check for closed doors
		if (tile_obj->type == OT__DOOR)
		{
			//door is more than halfway closed
			if (tile_obj->move_timer >= 0.5)
			{
				return false;
			}
		}
		//check for special tiles
		else if (tile_obj->type == OT__SPECIAL_TILE)
		{

		}
		//check for special triggers
		else if (tile_obj->type == OT__TRIGGER)
		{
			if (tile_obj->sub_type == SUB__TRIGGER_SWITCH)
			{
				return false;
			}
		}
	}

	return true;
}

bool Object_CheckLineToTarget(Object* obj, Object* target)
{
	return Object_CheckLineToTile(obj, target->x, target->y);
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
	if (!Object_CheckLineToTarget(obj, target))
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
	
	float offset = (obj->type == OT__PLAYER) ? 0.5 : 0;

	//if we have a target calc dir to target
	if (target)
	{
		float point_x = (obj->x - offset) - target->x;
		float point_y = (obj->y - offset) - target->y;

		Math_XY_Normalize(&point_x, &point_y);

		dir_x = -point_x;
		dir_y = -point_y;
	}

	missile->sub_type = SUB__MISSILE_FIREBALL;
	missile->x = (obj->x - offset) + dir_x * 2;
	missile->y = (obj->y - offset) + dir_y * 2;
	missile->dir_x = dir_x;
	missile->dir_y = dir_y;
	missile->hp = 5;
	missile->sprite.light = 1;
	//missile->sprite.h_frames = 5;
	//missile->sprite.v_frames = 1;
	missile->sprite.img = &assets->missile_textures;


	return missile;
}

bool Object_HandleObjectCollision(Object* obj, Object* collision_obj)
{
	//return true if we can move

	if (!collision_obj)
	{
		return true;
	}

	//ignore self
	if (obj == collision_obj)
	{
		return true;
	}

	if (collision_obj->type == OT__PARTICLE)
	{
		return;
	}

	//some objects are ignored from collisions
	if (collision_obj->type == OT__LIGHT || collision_obj->type == OT__TARGET || collision_obj->hp <= 0)
	{
		return true;
	}
	//handle doors
	if (collision_obj->type == OT__DOOR)
	{
		obj->col_object = collision_obj;

		//if door is moving in any way, don't move into it
		if (collision_obj->state != DOOR_SLEEP)
		{
			return false;
		}
		//door is closed, no movement
		if (collision_obj->move_timer >= 1)
		{
			return false;
		}
		//door is completely opened, move
		else if (collision_obj->move_timer <= 0)
		{
			return true;
		}
	}
	//handle special tiles
	else if (collision_obj->type == OT__SPECIAL_TILE)
	{
		if (collision_obj->sub_type == SUB__SPECIAL_TILE_FAKE)
		{
			return true;
		}

		return false;
	}
	//handle pickups
	else if (collision_obj->type == OT__PICKUP)
	{
		//pickup pickups for player
		if (obj->type == OT__PLAYER)
		{
			Player_HandlePickup(collision_obj);
		}

		//pickups dont block movement
		return true;
	}
	//handle triggers
	else if (collision_obj->type == OT__TRIGGER)
	{	
		if (collision_obj->sub_type == SUB__TRIGGER_SWITCH)
		{
			return false;
		}
		else
		{
			Object_HandleTriggers(obj, collision_obj);

			//dont block movement
			return true;
		}
	}
	//we have collided with a missile
	else if (collision_obj->type == OT__MISSILE)
	{
		Object* collision_owner = collision_obj->owner;

		//dont collide if we are the owner
		if (collision_owner == obj)
		{
			return true;
		}
		//dont collide if we are a missile
		if (obj->type == OT__MISSILE)
		{
			return true;
		}

		//dont collide with monster of same type
		if (collision_owner)
		{
			if (collision_owner->type == OT__MONSTER && obj->type == OT__MONSTER && collision_owner->sub_type == obj->sub_type)
			{
				return true;
			}

		}

		//perform direct hit damage
		Missile_DirectHit(collision_obj, obj);

		//explode the missile
		Missile_Explode(collision_obj);

		//missiles dont block movement
		return true;
	}
	//we are a missile
	if (obj->type == OT__MISSILE)
	{
		Object* owner = obj->owner;

		//dont collide if we are the owner
		if (owner == collision_obj)
		{
			return true;
		}
		//dont collide if hit a missile
		if (collision_obj->type == OT__MISSILE)
		{
			return true;
		}

		if (owner)
		{
			//dont collide with monster of same type
			if (collision_obj->type == OT__MONSTER && owner->type == OT__MONSTER && collision_obj->sub_type == owner->sub_type)
			{
				return true;
			}
		}
		
		//if we have collided with player or a monster
		//perform direct hit damage
		Missile_DirectHit(obj, collision_obj);
	}

	//we can't move
	return false;
}

bool Object_HandleSwitch(Object* obj)
{
	if (obj->type != OT__TRIGGER)
	{
		return false;
	}
	if (obj->sub_type != SUB__TRIGGER_SWITCH)
	{
		return false;
	}

	Object* target = obj->target;

	if (target)
	{
		//only allow switching if door is fully closed or opened
		if (target->type == OT__DOOR)
		{
			if (target->move_timer > 0 && target->move_timer < 1)
			{
				return false;
			}
		}
	}

	obj->flags ^= OBJ_FLAG__TRIGGER_SWITCHED_ON;

	return true;
}

void Object_HandleTriggers(Object* obj, Object* trigger)
{
	//not a trigger
	if (trigger->type != OT__TRIGGER)
	{
		return;
	}

	if(trigger->sub_type == SUB__TRIGGER_SWITCH)
	{
		if (!Object_HandleSwitch(trigger))
		{
			return;
		}
	}

	Object* target = trigger->target;
	
	if (target)
	{
		if (target->type == OT__TARGET)
		{
			switch (target->sub_type)
			{
			case SUB__TARGET_TELEPORT:
			{
				Sound_EmitWorldTemp(SOUND__TELEPORT, trigger->x, trigger->y, 0, 0);

				Move_Teleport(obj, target->x, target->y);
				break;
			}
			default:
				break;
			}

		}
		else if (target->type == OT__DOOR)
		{
			// 0 == door open
			// 1 == door closed

			//door is open
			if (target->move_timer == 0)
			{
				//close the door
				target->state = DOOR_CLOSE;
			}
			//door is closed
			else if (target->move_timer == 1)
			{
				//open the door
				target->state = DOOR_OPEN;
			}

			if (trigger->sub_type == SUB__TRIGGER_ONCE)
			{
				target->flags |= OBJ_FLAG__DOOR_NEVER_CLOSE;
			}
		}
	}
	//some do not have targets
	else
	{
		if (obj->type == OT__PLAYER)
		{
			if (trigger->sub_type == SUB__TRIGGER_CHANGELEVEL)
			{
				Game_ChangeLevel();
			}
			else if (trigger->sub_type == SUB__TRIGGER_SECRET)
			{
				Game_SecretFound();
			}
		}
		
	}

	//delete the object if it triggers once
	if (trigger->sub_type == SUB__TRIGGER_ONCE || trigger->sub_type == SUB__TRIGGER_SECRET)
	{
		Map_DeleteObject(trigger);
	}
}

Object* Object_Spawn(ObjectType type, SubType sub_type, float x, float y)
{
	if (type == OT__NONE)
	{
		return NULL;
	}

	Object* obj = Map_NewObject(type);
	
	if (!obj)
	{
		return NULL;
	}

	obj->x = x;
	obj->y = y;
	
	obj->sprite.x = x + obj->sprite.offset_x;
	obj->sprite.y = y + obj->sprite.offset_y;

	obj->sub_type = sub_type;

	switch (type)
	{
	case OT__PLAYER:
	{
		break;
	}
	case OT__MONSTER:
	{
		game.total_monsters++;

		Monster_Spawn(obj);
		break;
	}
	case OT__LIGHT:
	{
		obj->sprite.img = &assets.decoration_textures;
		break;
	}
	case OT__THING:
	{
		ObjectInfo* object_info = Info_GetObjectInfo(obj->type, obj->sub_type);
		AnimInfo* anim_info = &object_info->anim_info;

		obj->sprite.img = &assets.decoration_textures;

		obj->sprite.anim_speed_scale = object_info->anim_speed;
		obj->sprite.frame_count = anim_info->frame_count;
		obj->sprite.looping = anim_info->looping;
		obj->sprite.offset_x = anim_info->x_offset;
		obj->sprite.offset_y = anim_info->y_offset;

		obj->sprite.playing = true;
		break;
	}
	case OT__PICKUP:
	{
		obj->sprite.img = &assets.pickup_textures;

		if (obj->sub_type == SUB__PICKUP_SMALLHP)
		{
			obj->sprite.frame = 0;
		}
		else if (obj->sub_type == SUB__PICKUP_AMMO)
		{
			obj->sprite.frame = 2;
		}
		break;
	}
	case OT__TRIGGER:
	{
		Object* target = obj->target;

		if (obj->sub_type == SUB__TRIGGER_SECRET)
		{
			game.total_secrets++;
		}
		break;
	}
	case OT__TARGET:
	{
		break;
	}
	case OT__DOOR:
	{
		obj->move_timer = 1; // the door spawns closed
		obj->state = DOOR_SLEEP;
		break;
	}
	case OT__SPECIAL_TILE:
	{
		break;
	}
	case OT__PARTICLE:
	{
		obj->sprite.img = &assets.particle_textures;
		obj->move_timer = 0.5;
		obj->sprite.v_offset = Math_randf();
		obj->sprite.scale_x = 0.5;
		obj->sprite.scale_y = 0.5;
	
		if (obj->sub_type == SUB__PARTICLE_BLOOD)
		{
			obj->sprite.frame_count = 3;
			obj->sprite.frame_offset_y = 0;
			obj->sprite.frame = rand() % 3;
		}
		else
		{
			obj->sprite.frame_count = 4;
			obj->sprite.frame_offset_y = 1;
			obj->sprite.scale_x = 0.2;
			obj->sprite.scale_y = 0.2;

			obj->sprite.anim_speed_scale = 3;
			obj->sprite.playing = true;
			obj->sprite.looping = false;
		}		

		break;
	}
	default:
		break;
	}
	
	return obj;
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
