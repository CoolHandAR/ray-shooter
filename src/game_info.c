#include "game_info.h"

#include "g_common.h"

GunInfo* Info_GetGunInfo(int type)
{
	int index = 0;

	switch (type)
	{
	case GUN__SHOTGUN:
	{
		index = 0;
		break;
	}
	case GUN__PISTOL:
	{
		index = 1;
		break;
	}
	case GUN__MACHINEGUN:
	{
		index = 2;
		break;
	}
	default:
	{
		return NULL;
	}
		break;
	}

	return &GUN_INFOs[index];
}

MonsterInfo* Info_GetMonsterInfo(int type)
{
	if (type < 0)
	{
		return 0;
	}

	int index = 0;

	switch (type)
	{
	case SUB__MOB_IMP:
	{
		index = 0;
		break;
	}
	case SUB__MOB_PINKY:
	{
		index = 1;
		break;
	}
	case SUB__MOB_BRUISER:
	{
		index = 2;
		break;
	}
	default:
		break;
	}

	return &MONSTER_INFO[index];
}

ObjectInfo* Info_GetObjectInfo(int type, int sub_type)
{
	if (type < 0 || sub_type < 0)
	{
		return 0;
	}

	int index = 0;

	switch (type)
	{
	case OT__THING:
	{
		switch (sub_type)
		{
		case SUB__THING_BLUE_COLLUMN:
		{
			index = 5;
			break;
		}
		case SUB__THING_RED_COLLUMN:
		{
			index = 4;
			break;
		}
		case SUB__THING_BLUE_FLAG:
		{
			index = 6;
			break;
		}
		case SUB__THING_RED_FLAG:
		{
			index = 7;
			break;
		}
		default:
			break;
		}
	}
	case OT__LIGHT:
	{
		switch (sub_type)
		{
		case SUB__LIGHT_TORCH:
		{
			index = 0;
			break;
		}
		case SUB__LIGHT_LAMP:
		{
			index = 1;
			break;
		}
		default:
			break;
		}

		break;
	}
	case OT__PICKUP:
	{
		switch (sub_type)
		{
		case SUB__PICKUP_SMALLHP:
		{
			index = 2;
			break;
		}
		case SUB__PICKUP_BIGHP:
		{
			index = 3;
			break;
		}
		case SUB__PICKUP_INVUNERABILITY:
		{
			index = 9;
			break;
		}
		case SUB__PICKUP_QUAD_DAMAGE:
		{
			index = 10;
			break;
		}
		case SUB__PICKUP_AMMO:
		{
			index = 13;
			break;
		}
		case SUB__PICKUP_SHOTGUN:
		{
			index = 11;
			break;
		}
		case SUB__PICKUP_MACHINEGUN:
		{
			index = 12;
			break;
		}
		default:
			break;
		}
		break;
	}
	case OT__TRIGGER:
	{
		break;
	}
	case OT__TARGET:
	{
		if (sub_type == SUB__TARGET_TELEPORT)
		{
			index = 8;
		}

		break;
	}
	default:
		break;
	}

	return &OBJECT_INFOS[index];
}

LightInfo* Info_GetLightInfo(int sub_type)
{
	if (sub_type < 0)
	{
		return 0;
	}

	int index = 0;

	switch (sub_type)
	{
	case SUB__LIGHT_TORCH:
	{
		index = 0;
		break;
	}
	case SUB__LIGHT_LAMP:
	{
		index = 1;
		break;
	}
	default:
		break;
	}

	return &LIGHT_INFOS[index];
}

ParticleInfo* Info_GetParticleInfo(int sub_type)
{
	int index = 0;
	switch (sub_type)
	{
	case SUB__PARTICLE_BLOOD:
	{
		index = 0;
		break;
	}
	case SUB__PARTICLE_WALL_HIT:
	{
		index = 1;
		break;
	}
	default:
		break;
	}

	return &PARTICLE_INFOS[index];
}
