#pragma once

//Lazy hardcoded game data
#include <stddef.h>

extern void Monster_Bruiser_FireBall(struct Object* obj);
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


typedef void (*ActionFun)(struct Object* obj);

typedef struct
{
	ActionFun action_fun;
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
	float sprite_scale;
	float sprite_v_offset;
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
		60, //SPAWN HP
		2, //SPEED,
		14, //MELEE DAMAGE
		1, //SPRITE SCALE
		0, //SPRTE V OFFSET
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
		150, //SPAWN HP
		4, //SPEED,
		16, //MELEE DAMAGE
		1, //SPRITE SCALE
		0, //SPRTE V OFFSET
	},
	//BRUISER
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
			Monster_Bruiser_FireBall, 1, 4, 0, 3, 0,

			//ATTACK FORWARD SIDE
			Monster_Bruiser_FireBall, 1, 4, 1, 3, 0,

			//ATTACK SIDE
			Monster_Bruiser_FireBall, 1, 4, 2, 3, 0,

			//ATTACK BACK SIDE
			Monster_Bruiser_FireBall, 1, 4, 3, 3, 0,

			//ATTACK BACK
			Monster_Bruiser_FireBall, 2, 4, 4, 4, 0,

			//MELEE FORWARD
			Monster_Melee, 1, 8, 0, 3, 1,

			//MELEE FORWARD SIDE
			Monster_Melee, 1, 8, 1, 3, 1,

			//MELEE SIDE
			Monster_Melee, 1, 8, 2, 3, 1,

			//MELEE BACK SIDE
			Monster_Melee, 1, 8, 3, 3, 1,

			//MELEE BACK
			Monster_Melee, 2, 8, 4, 3, 1,

			//HIT FORWARD
			NULL, 0, 7, 0, 1, 0,

			//HIT SIDE
			NULL, 0, 7, 2, 1, 0,

			//HIT BACK
			NULL, 0, 7, 4, 1, 0,

			//DIE
			NULL, 0, 0, 5, 14, 0,

			//EXPLODE
			NULL, 0, 0, 5, 14, 0,
		},
		400, //SPAWN HP
		2, //SPEED,
		30, //MELEE DAMAGE
		2, //SPRITE SCALE
		-1, //SPRTE V OFFSET
	},
};

typedef struct
{
	AnimInfo anim_info[2];
	int speed;
	float size;
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
		0.25, //SIZE,
		0, //EXPLOSION DAMAGE
		10, //DIRECT HIT DAMAGE
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

	SOUND__BRUISER_ALERT,
	SOUND__BRUISER_HIT,
	SOUND__BRUISER_DIE,
	SOUND__BRUISER_ATTACK,

	SOUND__SHOTGUN_SHOOT,
	SOUND__PISTOL_SHOOT,
	SOUND__MACHINEGUN_SHOOT,
	SOUND__NO_AMMO,

	SOUND__DOOR_ACTION,

	SOUND__PLAYER_PAIN,
	SOUND__PLAYER_DIE,

	SOUND__SECRET_FOUND,

	SOUND__TELEPORT,
	
	SOUND__PICKUP_HP,
	SOUND__PICKUP_SPECIAL,

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

	//BRUISER ALERT
	"assets/bruiser_act.wav",

	//BRUISER HIT
	"assets/bruiser_pain.wav",

	//BRUISER  DIE
	"assets/bruiser_death.wav",

	//BRUISER  ATTACK
	"assets/bruiser_attack.wav",

	//SHOTGUN SHOOT
	"assets/shotgun_shoot.wav",

	//PISTOL SHOOT
	"assets/pistol_shoot.wav",

	//MACHINEGUN SHOOT
	"assets/machinegun_shoot.wav",

	//NO AMMO
	"assets/no_ammo.wav",

	//DOOR ACTION
	"assets/door_action.wav",

	//PLAYER PAIN
	"assets/player_pain.wav",

	//PLAYER DEATH
	"assets/player_death.wav",

	//SECRET FOUND
	"assets/secret.wav",

	//TELEPORT
	"assets/teleport.wav",

	//PICKUP HP
	"assets/pickup_hp.wav",

	//PICKUP SPECIAL
	"assets/pickup_special.wav"
};


typedef struct
{	
	int damage;
	int spread;
	float cooldown;
	float screen_x;
	float screen_y;
	float scale;
} GunInfo;

static const GunInfo GUN_INFOs[] =
{
	//SHOTGUN
	{
		4, //DMG
		1, //SPREAD
		1.25, //CD,
		0.05, //SCREEN X
		0.2, //SCREEN Y
		1.2, //SCALE
	},
	//PISTOL
	{
		10, //DMG
		1, //SPREAD
		0.3, //CD,
		0.55, //SCREEN X
		0.6, //SCREEN Y
		1.5, //SCALE
	},
	//MACHINE GUN
	{
		15, //DMG
		1, //SPREAD
		0.2, //CD,
		0.2, //SCREEN X
		0.18, //SCREEN Y
		1.5, //SCALE
	},
};

typedef struct
{	
	AnimInfo anim_info;
	float anim_speed;
	float sprite_offset_x;
	float sprite_offset_y;
} ObjectInfo;

static const ObjectInfo OBJECT_INFOS[] =
{
	//TORCH
	{
		//ANIM
		{
			NULL, 0, 0, 1, 4, 1,
		},
		0.5, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//LAMP
	{
		//ANIM
		{
			NULL, 0, 0, 0, 4, 1,
		},
		0.5, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//SMALL HP
	{
		//ANIM
		{
			NULL, 0, 0, 2, 0, 0,
		},
		0.0, //ANIM SPEED
		0.0, //SPRITE OFFSET X
		0.0, //SPRITE OFFSET Y
	},
	//BIG HP
	{
		//ANIM
		{
			NULL, 0, 1, 2, 0, 0,
		},
		0.0, //ANIM SPEED
		0.0, //SPRITE OFFSET X
		0.0, //SPRITE OFFSET Y
	},
	//RED COLLUMN
	{
		//ANIM
		{
			NULL, 0, 0, 4, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//BLUE COLLUMN
	{
		//ANIM
		{
			NULL, 0, 1, 4, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//RED FLAG
	{
		//ANIM
		{
			NULL, 0, 2, 4, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//BLUE FLAG
	{
		//ANIM
		{
			NULL, 0, 3, 4, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//TELEPORTER
	{
		//ANIM
		{
			NULL, 0, 0, 3, 4, 1,
		},
		0.5, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//INVUNERABILITY PICKUP
	{
		//ANIM
		{
			NULL, 0, 0, 5, 4, 1,
		},
		0.5, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//QUAD PICKUP
	{
		//ANIM
		{
			NULL, 0, 0, 6, 4, 1,
		},
		0.5, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//SHOTGUN PICKUP
	{
		//ANIM
		{
			NULL, 0, 0, 7, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//MACHINE PICKUP
	{
		//ANIM
		{
			NULL, 0, 1, 7, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
	//AMMO PICKUP
	{
		//ANIM
		{
			NULL, 0, 2, 2, 0, 0,
		},
		0.0, //ANIM SPEED
		0.5, //SPRITE OFFSET X
		0.5, //SPRITE OFFSET Y
	},
};

typedef struct
{
	float radius;
	float attenuation;
	float scale;
} LightInfo;

static const LightInfo LIGHT_INFOS[] =
{
	//TORCH
	{
		32, //radius
		1.0, //attenuation
		1 // scale
	},
	//LAMP
	{
		12, //radius
		1.5, //attenuation
		1 // scale
	}
};

typedef struct
{
	AnimInfo anim_info;
	float time;
	float sprite_scale;
} ParticleInfo;

static const ParticleInfo PARTICLE_INFOS[] =
{
	//BLOOD SPLATTER
	{
		//ANIM INFO
		{
			NULL, 0, 0, 0, 3, 0,
		},
		0.5, //time
		0.5, //sprite scale
	},
	//WALL HIT
	{
		//ANIM INFO
		{
			NULL, 0, 0, 1, 4, 0,
		},
		0.5, //time
		0.25, //sprite scale
	}

};

#define PICKUP_SMALLHP_HEAL 20
#define PICKUP_BIGHP_HEAL 50
#define PICKUP_AMMO_GIVE 12
#define PICKUP_ROCKETS_GIVE 12
#define PICKUP_SHOTGUN_AMMO_GIVE 12
#define PICKUP_MACHINEGUN_AMMO_GIVE 50
#define PICKUP_INVUNERABILITY_TIME 15
#define PICKUP_QUAD_TIME 15

GunInfo* Info_GetGunInfo(int type);
MonsterInfo* Info_GetMonsterInfo(int type);
ObjectInfo* Info_GetObjectInfo(int type, int sub_type);
LightInfo* Info_GetLightInfo(int sub_type);
ParticleInfo* Info_GetParticleInfo(int sub_type);

static const char* LEVELS[] =
{
	"test.json",

	"test2.json",
};

