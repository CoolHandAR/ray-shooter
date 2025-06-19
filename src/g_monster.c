#include "g_common.h"

#include "game_info.h"

#include <math.h>
#include <stdlib.h>

static DirEnum opposite_dirs[DIR_MAX] =
{
	//NONE			//NORT
	DIR_NONE,		DIR_SOUTH,
	//NORTH-EAST    //NORTH-WEST
	DIR_SOUTH_WEST,	DIR_SOUTH_EAST,
	//EAST			//SOUTH-EAST
	DIR_WEST,		DIR_NORTH_WEST,
	//SOUTH			//SOUTH-WEST
	DIR_NONE,		DIR_NORTH_EAST,
	//WEST			//MAX
	DIR_EAST
};

static DirEnum angle_dirs[] =
{
	DIR_NORTH_WEST, DIR_NORTH_EAST, DIR_SOUTH_WEST, DIR_SOUTH_EAST
};

static float x_diags[DIR_MAX] =
{
	//NONE			//NORTH
	0,				0,
	//NORTH-EAST    //NORTH-WEST
	-1,				1,
	//EAST			//SOUTH-EAST
	-1,				-1,
	//SOUTH			//SOUTH-WEST
	0,				1,
	//WEST			//MAX
	1
};
static float y_diags[DIR_MAX] =
{
	//NONE			//NORTH
	0,				-1,
	//NORTH-EAST    //NORTH-WEST
	-1,				-1,
	//EAST			//SOUTH-EAST
	0,				1,
	//SOUTH			//SOUTH-WEST
	1,				1,
	//WEST			//MAX
	0
};
#include "u_math.h"

static MonsterAnimState Monster_GetAnimState(Object* obj)
{
	//obj->dir_x = 1;
	//obj->dir_y = 0;

	float view_x, view_y, view_dir_x, view_dir_y;
	Player_GetView(&view_x, &view_y, &view_dir_x, &view_dir_y, NULL, NULL);

	float angle = atan2(obj->y - view_y, obj->x - view_x);
	float obj_angle = atan2(-obj->dir_y, -obj->dir_x);
	
	float rotation = angle - obj_angle;

	float abs_dir = fabs(rotation);
	int round_dir = roundl(abs_dir);

	if (rotation < 0)
	{
		obj->sprite.flip_h = true;
	}
	else
	{
		obj->sprite.flip_h = false;
	}

	if (round_dir >= 4)
	{
		obj->sprite.flip_h = (obj->dir_y == -1) ? true : false;
	}

	if (obj->state == MS__WALK)
	{
		if (round_dir == 0 || round_dir == 6)
		{
			return MAS__WALK_FORWARD;
		}
		else if(round_dir == 1 || round_dir == 5)
		{
			return MAS__WALK_FORWARD_SIDE;
		}
		else if(round_dir == 2 || round_dir == 4)
		{
			return MAS__WALK_SIDE;
		}
		else if(round_dir == 3)
		{
			return MAS__WALK_BACK;
		}
	}
	else if (obj->state == MS__MISSILE || obj->state == MS__MELEE)
	{
		if (round_dir == 0 || round_dir == 6)
		{
			return MAS__ATTACK_FORWARD;
		}
		else if (round_dir == 1 || round_dir == 5)
		{
			return MAS__ATTACK_FORWARD_SIDE;
		}
		else if (round_dir == 2 || round_dir == 4)
		{
			return MAS__ATTACK_SIDE;
		}
		else if (round_dir == 3)
		{
			return MAS__ATTACK_BACK;
		}
	}
	else if (obj->state == MS__HIT)
	{
		if (round_dir == 0 || round_dir == 6)
		{
			return MAS__HIT_FORWARD;
		}
		else if (round_dir == 1 || round_dir == 5 || round_dir == 2 || round_dir == 4)
		{
			return MAS__HIT_SIDE;
		}
		else if (round_dir == 3)
		{
			return MAS__HIT_BACK;
		}

	}
	else if (obj->state == MS__DIE)
	{
		return MAS__DIE;
	}

	return MAS__IDLE;
}

static void Monster_SetState(Object* obj, MonsterState state)
{
	if (obj->state == state)
	{
		//return;
	}

	obj->state = state;

	MonsterAnimState anim_state = Monster_GetAnimState(obj);

	const MonsterInfo* info = &MONSTER_INFO[obj->monster_type];
	const AnimInfo* anim_info = &info->anim_states[anim_state];

	Sprite* s = &obj->sprite;

	s->frame_count = anim_info->frame_count;
	s->frame_offset_x = anim_info->x_offset;
	s->frame_offset_y = anim_info->y_offset;
	s->looping = anim_info->looping;
	

}

static bool Monster_TryStep(Object* obj)
{	
	float dir_x = x_diags[obj->dir_enum];
	float dir_y = y_diags[obj->dir_enum];

	for (float step = 0.1; step <= 1; step += 0.1)
	{
		if (!Move_CheckStep(obj, dir_x * step, dir_y * step, 0.5))
		{
			return false;
		}
	}
	
	obj->dir_x = dir_x;
	obj->dir_y = dir_y;

	return true;
}

static bool Monster_Walk(Object* obj, float delta)
{
	if (Move_Object(obj, obj->dir_x * delta, obj->dir_y * delta))
	{
		Map_UpdateObjectTile(obj);
		Monster_SetState(obj, MS__WALK);
		return true;
	}

	Monster_SetState(obj, MS__IDLE);

	return false;
}

static bool Monster_CheckIfMeleePossible(Object* obj)
{
	Object* chase_obj = (Object*)obj->chase_obj;

	if (!chase_obj)
	{
		return true;
	}

	float dist = (obj->x - chase_obj->x) * (obj->x - chase_obj->x) + (obj->y - chase_obj->y) * (obj->y - chase_obj->y);

	if (dist < 5)
	{
		return true;
	}

	return false;
}

static void Monster_NewChaseDir(Object* obj)
{
	Object* player = Player_GetObj();

	obj->chase_obj = player;

	float dir_x = player->x - obj->x;
	float dir_y = player->y - obj->y;

	DirEnum old_dir = player->dir_enum;
	DirEnum opposite_dir = opposite_dirs[old_dir];

	//try direct route
	obj->dir_enum = DIR_NONE;

	DirEnum dirs[2];

	if (dir_x > 0)
	{
		dirs[0] = DIR_WEST;
	}
	else if (dir_x < 0)
	{
		dirs[0] = DIR_EAST;
	}
	else
	{
		dirs[0] = DIR_NONE;
	}
	if (dir_y > 0)
	{
		dirs[1] = DIR_SOUTH;
	}
	else if (dir_y < 0)
	{
		dirs[1] = DIR_NORTH;
	}
	else
	{
		dirs[1] = DIR_NONE;
	}


	if (dirs[0] != DIR_NONE && dirs[1] != DIR_NONE)
	{
		int i_x = (dirs[0] == DIR_EAST);
		int i_y = (dirs[1] == DIR_SOUTH) ? 2 : 0;
		DirEnum d = angle_dirs[i_x + i_y];

		if (d != opposite_dir)
		{
			obj->dir_enum = d;

			if (Monster_TryStep(obj))
			{
				return;
			}
		}
	}

	int r = rand() % 100;

	if (rand > 200 || abs(dir_y) > abs(dir_x))
	{
		int t = dirs[0];
		dirs[0] = dirs[1];
		dirs[1] = t;
	}
	if (dirs[0] == opposite_dir)
	{
		dirs[0] = DIR_NONE;
	}
	if (dirs[1] == opposite_dir)
	{
		dirs[1] = DIR_NONE;
	}
	if (dirs[0] != DIR_NONE)
	{
		obj->dir_enum = dirs[0];
		if (Monster_TryStep(obj))
		{
			return;
		}
	}
	if (dirs[1] != DIR_NONE)
	{
		obj->dir_enum = dirs[1];
		if (Monster_TryStep(obj))
		{
			return;
		}
	}
	if (old_dir != DIR_NONE)
	{
		obj->dir_enum = old_dir;
		if (Monster_TryStep(obj))
		{
			return;
		}
	}

	if (rand() & 1)
	{
		for (int i = DIR_NONE + 1; i < DIR_MAX; i++)
		{
			if (i != opposite_dir)
			{
				obj->dir_enum = i;
				if (Monster_TryStep(obj))
				{
					return;
				}
			}
		}
	}
	else
	{
		for (int i = DIR_MAX - 1; i > DIR_NONE; i--)
		{
			if (i != opposite_dir)
			{
				obj->dir_enum = i;
				if (Monster_TryStep(obj))
				{
					return;
				}
			}
		}
	}
	if (opposite_dir != DIR_NONE)
	{
		obj->dir_enum = opposite_dir;
		if (Monster_TryStep(obj))
		{
			return;
		}
	}

	obj->dir_enum = DIR_NONE;
}

void Monster_Update(Object* obj, float delta)
{

	static float t = 0;

	t += delta;

	obj->attack_timer -= delta;

	if (t > 0.3 && obj->sprite.frame_count > 0)
	{
		obj->sprite.frame = (obj->sprite.frame + 1) % obj->sprite.frame_count;
		
		t = 0;

	}

	

	if (obj->attack_timer > 0)
	{
		return;
	}

	
	//get a new chase dir or move forward in the direction
	if (obj->move_timer <= 0 || !Monster_TryStep(obj) || !Monster_Walk(obj, delta * 2))
	{
		Monster_NewChaseDir(obj);
		obj->move_timer = rand() % 3;
		return;
	}
	obj->move_timer -= delta;

	return;

	//check if possible to melee
	if (Monster_CheckIfMeleePossible(obj))
	{
		Monster_SetState(obj, MS__MELEE);
		obj->move_timer = -1;
		obj->attack_timer = 1;
		return;
	}
}
