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