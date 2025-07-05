#pragma once

//Lazy hardcoded game data
#include <stddef.h>

extern void Monster_Imp_FireBall(struct Object* obj);
extern void Monster_Melee(struct Object* obj);

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

	MAS__MELEE_FORWARD,
	MAS__MELEE_FORWARD_SIDE,
	MAS__MELEE_SIDE,
	MAS__MELEE_BACK_SIDE,
	MAS__MELEE_BACK,
	   
	MAS__HIT_FORWARD,
	MAS__HIT_SIDE,
	MAS__HIT_BACK,
	   
	MAS__DIE,
	MAS__EXPLODE,
	   
	MAS__MAX
} MonsterAnimState;


typedef void (*MonsterActionFun)(struct Object* obj);

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
	int melee_damage;
} MonsterInfo;

static const MonsterInfo MONSTER_INFO[] =
{
	//IMP
	{
		//ANIM
		{
			//IDLE
			NULL, 0, 0, 0, 1, 1,

			//WALK FORWARD
			NULL, 0, 0, 0, 4, 1,

			//WALK FORWARD SIDE
			NULL, 0, 0, 1, 4, 1,

			//WALK SIDE
			NULL, 0, 0, 2, 4, 1,

			//WALK BACK SIDE
			NULL, 0, 0, 3, 4, 1,

			//WALK BACK
			NULL, 0, 0, 4, 4, 1,

			//ATTACK FORWARD
			Monster_Imp_FireBall, 1, 4, 0, 3, 0,

			//ATTACK FORWARD SIDE
			Monster_Imp_FireBall, 1, 4, 1, 3, 0,

			//ATTACK SIDE
			Monster_Imp_FireBall, 1, 4, 2, 3, 0,

			//ATTACK BACK SIDE
			Monster_Imp_FireBall, 1, 4, 3, 3, 0,

			//ATTACK BACK
			Monster_Imp_FireBall, 2, 4, 4, 4, 0,

			//MELEE FORWARD
			Monster_Melee, 1, 4, 0, 3, 1,

			//MELEE FORWARD SIDE
			Monster_Melee, 1, 4, 1, 3, 1,

			//MELEE SIDE
			Monster_Melee, 1, 4, 2, 3, 1,

			//MELEE BACK SIDE
			Monster_Melee, 1, 4, 3, 3, 1,

			//MELEE BACK
			Monster_Melee, 2, 4, 4, 4, 1,

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
		50, //SPAWN HP
		2, //SPEED,
		3, //MELEE DAMAGE
	},
	//PINKY
	{
		//ANIM
		{
			//IDLE
			NULL, 0, 0, 0, 1, 1,

			//WALK FORWARD
			NULL, 0, 0, 0, 4, 1,

			//WALK FORWARD SIDE
			NULL, 0, 0, 1, 4, 1,

			//WALK SIDE
			NULL, 0, 0, 2, 4, 1,

			//WALK BACK SIDE
			NULL, 0, 0, 3, 4, 1,

			//WALK BACK
			NULL, 0, 0, 4, 4, 1,

			//ATTACK FORWARD
			NULL, 0, 0, 0, 0, 0,

			//ATTACK FORWARD SIDE
			NULL, 0, 0, 0, 0, 0,

			//ATTACK SIDE
			NULL, 0, 0, 0, 0, 0,

			//ATTACK BACK SIDE
			NULL, 0, 0, 0, 0, 0,

			//ATTACK BACK
			NULL, 0, 0, 0, 0, 0,

			//MELEE FORWARD
			Monster_Melee, 1, 4, 0, 3, 1,

			//MELEE FORWARD SIDE
			Monster_Melee, 1, 4, 1, 3, 1,

			//MELEE SIDE
			Monster_Melee, 1, 4, 2, 3, 1,

			//MELEE BACK SIDE
			Monster_Melee, 1, 4, 3, 3, 1,

			//MELEE BACK
			Monster_Melee, 2, 4, 4, 4, 1,

			//HIT FORWARD
			NULL, 0, 0, 0, 0, 0,

			//HIT SIDE
			NULL, 0, 0, 0, 0, 0,

			//HIT BACK
			NULL, 0, 0, 0, 0, 0,

			//DIE
			NULL, 0, 0, 5, 6, 0,

			//EXPLODE
			NULL, 0, 0, 5, 6, 0,
		},
		50, //SPAWN HP
		4, //SPEED,
		12, //MELEE DAMAGE
	},

};

typedef struct
{
	AnimInfo anim_info[2];
	int speed;
	int size;
	int explosion_damage;
	int direct_damage;
} MissileInfo;

static const MissileInfo MISSILE_INFO[] =
{
	//SIMPLE FIREBALL
	{
		//ANIM
		{
			//FLY
			NULL, 0, 0, 0, 2, 1,

			//EXPLODE
			NULL, 0, 2, 0, 3, 0,
		},
		8, //SPEED
		1, //SIZE,
		0, //EXPLOSION DAMAGE
		5, //DIRECT HIT DAMAGE
	}
};

typedef enum
{
	SOUND__NONE,

	SOUND__FIREBALL_THROW,
	SOUND__FIREBALL_EXPLODE,
	
	SOUND__IMP_ALERT,
	SOUND__IMP_HIT,
	SOUND__IMP_DIE,
	SOUND__IMP_ATTACK,

	SOUND__PINKY_ALERT,
	SOUND__PINKY_HIT,
	SOUND__PINKY_DIE,
	SOUND__PINKY_ATTACK,

	SOUND__SHOTGUN_SHOOT,

	SOUND__DOOR_ACTION,

	SOUND__PLAYER_PAIN,
	SOUND__PLAYER_DIE,

	SOUND__MAX
} SoundType;

static const char* SOUND_INFO[SOUND__MAX] =
{
	//NONE
	NULL,

	//FIREBALL THROW
	"assets/fireball_throw.wav",

	//FIREBALL EXPLODE
	"assets/fireball_explode.wav",

	//IMP ALERT
	"assets/imp_alert.wav",

	//IMP HIT
	"assets/imp_hit.wav",

	//IMP DIE
	"assets/imp_die.wav",

	//IMP ATTACK
	"assets/imp_attack.wav",

	//PINKY ALERT
	"assets/pinky_alert.wav",

	//PINKY HIT
	"assets/pinky_hit.wav",

	//PINKY  DIE
	"assets/pinky_die.wav",

	//PINKY  ATTACK
	"assets/pinky_attack.wav",

	//SHOTGUN SHOOT
	"assets/shotgun_shoot.wav",

	//DOOR ACTION
	"assets/door_action.wav",

	//PLAYER PAIN
	"assets/player_pain.wav",

	//PLAYER DEATH
	"assets/player_death.wav",
};

typedef enum
{
	GUN__NONE,

	GUN__SHOTGUN,

	GUN__MAX
} GunType;

typedef struct
{
	int damage;
	int spread;
	float cooldown;
} GunInfo;

MonsterInfo* Info_GetMonsterInfo(int type);

static const char* LEVELS[] =
{
	"test.json",

	"test2.json",
};