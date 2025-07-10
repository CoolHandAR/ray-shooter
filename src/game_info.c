#include "game_info.h"

#include "g_common.h"

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
		case SUB__THING_TORCH:
		{
			index = 0;
			break;
		}
		default:
			break;
		}
		
	}
	default:
		break;
	}

	return &OBJECT_INFOS[index];
}
