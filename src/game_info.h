#pragma once

//Lazy hardcoded game data

typedef enum
{
	MS__IDLE,
	MS__WALK,
	MS__MELEE,
	MS__MISSILE,
	MS__HIT,
	MS__DIE,
	MS__MAX
} MonsterState;

typedef enum
{
	MT__IMP,
	MT__MAX
} MonsterType;

typedef enum
{
	MAS__IDLE,
	   
	MAS__WALK_FORWARD,
	MAS__WALK_FORWARD_SIDE,
	MAS__WALK_SIDE,
	MAS__WALK_BACK_SIDE,
	MAS__WALK_BACK,
	   
	MAS__ATTACK_FORWARD,
	MAS__ATTACK_FORWARD_SIDE,
	MAS__ATTACK_SIDE,
	MAS__ATTACK_BACK_SIDE,
	MAS__ATTACK_BACK,
	   
	MAS__HIT_FORWARD,
	MAS__HIT_SIDE,
	MAS__HIT_BACK,
	   
	MAS__DIE,
	MAS__EXPLODE,
	   
	MAS__MAX
} MonsterAnimState;


typedef void (*MonsterActionFun)(Object* obj);

typedef struct
{
	MonsterActionFun action_fun;
	int action_frame;
	int x_offset;
	int y_offset;
	int frame_count;
	int looping;
} AnimInfo;

typedef struct
{
	AnimInfo anim_states[MAS__MAX];
	int spawn_hp;
	int speed;
} MonsterInfo;

static const MonsterInfo MONSTER_INFO[] =
{
	//IMP
	{
		//ANIM
		{
			//IDLE
			NULL, 0, 0, 0, 0, 0,

			//WALK FORWARD
			NULL, 0, 0, 0, 4, 0,

			//WALK FORWARD SIDE
			NULL, 0, 0, 1, 4, 0,

			//WALK SIDE
			NULL, 0, 0, 2, 4, 0,

			//WALK BACK SIDE
			NULL, 0, 0, 3, 4, 0,

			//WALK BACK
			NULL, 0, 0, 4, 4, 0,

			//ATTACK FORWARD
			NULL, 1, 4, 0, 3, 0,

			//ATTACK FORWARD SIDE
			NULL, 1, 4, 1, 3, 0,

			//ATTACK SIDE
			NULL, 1, 4, 2, 3, 0,

			//ATTACK BACK SIDE
			NULL, 1, 4, 3, 3, 0,

			//ATTACK BACK
			NULL, 2, 4, 4, 4, 0,

			//HIT FORWARD
			NULL, 0, 0, 5, 1, 0,

			//HIT SIDE
			NULL, 0, 1, 5, 1, 0,

			//HIT BACK
			NULL, 0, 2, 5, 1, 0,

			//DIE
			NULL, 0, 0, 6, 4, 0,

			//EXPLODE
			NULL, 0, 0, 7, 8, 0,
		},
		1, //SPAWN HP
		1, //SPEED,

	},
	



};
