#include "g_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <cjson/cJSON.h>
#include <cglm/cglm.h>
#include "u_math.h"

#include "utility.h"

static Map s_map;
static Sprite* s_spritePointers[MAX_OBJECTS];

static int CompareSpriteDistances(const void* a, const void* b)
{
	Sprite* arg1 = *(Sprite**)a;
	Sprite* arg2 = *(Sprite**)b;

	if (arg1->dist > arg2->dist) return -1;
	if (arg1->dist < arg2->dist) return 1;

	return 0;
}

static void Map_UpdateSortedList()
{
	int index = 0;
	int max_index = 0;

	for (int i = 0; i < s_map.num_objects; i++)
	{
		Object* obj = &s_map.objects[i];

		if (obj->type != OT__NONE)
		{
			max_index = i;
		}

		if (obj->type == OT__NONE
			|| obj->type == OT__PLAYER)
		{
			continue;
		}

		s_map.sorted_list[index++] = i;
	}

	s_map.num_sorted_objects = index;
	//s_map.num_objects = max_index;
}

static void Map_UpdateObjectTilemap()
{
	memset(s_map.object_tiles, NULL_INDEX, sizeof(ObjectID) * s_map.width * s_map.height);

	for (int i = 0; i < s_map.num_objects; i++)
	{
		Object* obj = &s_map.objects[i];

		if (obj->type == OT__NONE || obj->type == OT__PARTICLE)
		{
			continue;
		}
		Map_UpdateObjectTile(obj);
	}
}
static void Map_ConnectTriggersToTargets()
{
	for (int i = 0; i < s_map.num_objects; i++)
	{
		Object* obj = &s_map.objects[i];

		if (obj->type != OT__TRIGGER)
		{
			continue;
		}

		int target_id = (int)obj->target;
		obj->target = NULL;

		for (int k = 0; k < s_map.num_objects; k++)
		{
			Object* target_obj = &s_map.objects[k];

			if (target_obj->type == OT__TARGET || target_obj->type == OT__DOOR)
			{
				if (target_obj->map_id == target_id)
				{
					obj->target = target_obj;
					break;
				}
			}
		}

		Object* t = obj->target;

		if (t && t->sub_type == SUB__TARGET_TELEPORT)
		{
			GameAssets* assets = Game_GetAssets();
			obj->sprite.img = &assets->decoration_textures;
			obj->sprite.frame_count = 4;
			obj->sprite.anim_speed_scale = 0.5;
			obj->sprite.playing = true;
			obj->sprite.looping = true;
			obj->sprite.offset_x = 0.5;
			obj->sprite.offset_y = 0.5;
			obj->sprite.frame_offset_y = 1;
			obj->sprite.light = 1;
		}
	}
}

static int Map_CalculateLight(int light_x, int light_y, int tile_x, int tile_y, float linear, float quadratic, float scale)
{
	if (light_x < 0) light_x = 0; else if (light_x >= s_map.width) light_x = s_map.width - 1;
	if (light_y < 0) light_y = 0; else if (light_y >= s_map.height) light_y = s_map.height - 1;

	int dir_x = light_x - tile_x;
	int dir_y = light_y - tile_y;

	float norm_dir_x = dir_x;
	float norm_dir_y = dir_y;

	Math_XY_Normalize(&norm_dir_x, &norm_dir_y);

	DirEnum dir = DirVectorToDirEnum(dir_x, dir_y);

	float light_linear = linear;
	float light_quadratic = quadratic;

	float distance = Math_XY_Length(tile_x - light_x, tile_y - light_y);
	float attenuation = 1.0 / (1.0 + light_linear * distance + light_quadratic * distance * distance);

	int index = light_x + light_y * s_map.width;
	LightTile* light_tile = &s_map.light_tiles[index];

	attenuation *= scale;

	int light = attenuation * 255;

	if (light > 255)
	{
		light = 255;
	}
	else if (light < 0)
	{
		light = 0;
	}

	if (dir == DIR_NORTH || dir == DIR_NORTH_EAST || dir == DIR_NORTH_WEST)
	{
		int l = light + (int)light_tile->light_north;

		if (l > 255) l = 255;

		light_tile->light_north = (uint8_t)l;
	}
	if (dir == DIR_SOUTH || dir == DIR_SOUTH_EAST || dir == DIR_SOUTH_WEST)
	{
		int l = light + (int)light_tile->light_south;

		if (l > 255) l = 255;

		light_tile->light_south = (uint8_t)l;
	}
	if (dir == DIR_WEST || dir == DIR_SOUTH_WEST || dir == DIR_NORTH_WEST)
	{
		int l = light + (int)light_tile->light_west;

		if (l > 255) l = 255;

		light_tile->light_west = (uint8_t)l;
	}
	if (dir == DIR_EAST || dir == DIR_SOUTH_EAST || dir == DIR_NORTH_EAST)
	{
		int l = light + (int)light_tile->light_east;

		if (l > 255) l = 255;

		light_tile->light_east = (uint8_t)l;
	}

	int l = light + (int)light_tile->light;

	if (l > 255) l = 255;

	light_tile->light = (uint8_t)l;
}

static void Map_SetupLightTiles()
{
	s_map.light_tiles = malloc(sizeof(LightTile) * s_map.width * s_map.height);

	if (!s_map.light_tiles)
	{
		return;
	}

	memset(s_map.light_tiles, 0, sizeof(LightTile) * s_map.width * s_map.height);

	//set min light
	for (int x = 0; x < s_map.width; x++)
	{
		for (int y = 0; y < s_map.height; y++)
		{
			int index = x + y * s_map.width;
			LightTile* light_tile = &s_map.light_tiles[index];

			light_tile->light = MIN_LIGHT;
			light_tile->light_east = MIN_LIGHT;
			light_tile->light_west = MIN_LIGHT;
			light_tile->light_north = MIN_LIGHT;
			light_tile->light_south = MIN_LIGHT;
		}
	}

	for (int i = 0; i < s_map.num_objects; i++)
	{
		Object* object = &s_map.objects[i];

		if (!object || object->type != OT__LIGHT)
		{
			continue;
		}

		int tile_x = (int)object->x;
		int tile_y = (int)object->y;

		int light_size = 6;

		for (int x = -light_size; x <= light_size; x++)
		{
			for (int y = -light_size; y <= light_size; y++)
			{
				int light_x = tile_x + x;
				int light_y = tile_y + y;
				if (light_x < 0) light_x = 0; else if (light_x >= s_map.width) light_x = s_map.width - 1;
				if (light_y < 0) light_y = 0; else if (light_y >= s_map.height) light_y = s_map.height - 1;

				int dir_x = light_x - tile_x;
				int dir_y = light_y - tile_y;

				//check for blocking tiles, add a small bias so we won't check the exact tile that is blocking
				int check_sign_x = 0;
				int check_sign_y = 0;

				if (dir_x != 0)
				{
					check_sign_x = (dir_x > 0) ? 1 : -1;
				}
				if (dir_y != 0)
				{
					check_sign_y = (dir_y > 0) ? 1 : -1;
				}
				
				if (!Object_CheckLineToTile2(object, light_x, light_y))
				{
					continue;
				}
				
				float scale = 1;

				if (!Object_CheckLineToTile(object, light_x, light_y))
				{
					scale *= 0.5;
				}

				Map_CalculateLight(light_x, light_y, tile_x, tile_y, 0.02, 0.08, scale);
			}
		}
	}
}

static void Map_FreeListStoreID(ObjectID id)
{
	if (s_map.num_free_list >= MAX_OBJECTS)
	{
		return;
	}

	s_map.free_list[s_map.num_free_list++] = id;
}

static ObjectID Map_GetNewObjectIndex()
{
	ObjectID id = 0;
	//get from free list
	if (s_map.num_free_list > 0)
	{
		id = s_map.free_list[s_map.num_free_list - 1];
		s_map.num_free_list--;
	}
	else
	{
		id = s_map.num_objects++;
	}

	return id;
}

int Map_GetLevelIndex()
{
	return s_map.level_index;
}

Map* Map_GetMap()
{
	return &s_map;
}

Object* Map_NewObject(ObjectType type)
{
	if (s_map.num_objects >= MAX_OBJECTS)
	{
		assert(false && "Too many objects");
	}

	ObjectID index = Map_GetNewObjectIndex();

	Object* obj = &s_map.objects[index];

	obj->sprite.scale_x = 1;
	obj->sprite.scale_y = 1;
	obj->id = index;
	obj->tile_index = NULL_INDEX;
	obj->type = type;
	obj->hp = 1;
	obj->size = 0.5;

	s_map.num_objects++;

	Map_UpdateSortedList();

	return obj;
}

bool Map_Load(const char* filename)
{
	Map_Destruct();

	GameAssets* assets = Game_GetAssets();

	bool result = true;

	const unsigned char* filestr = File_Parse(filename, NULL);

	if (!filestr)
	{
		printf("ERROR::Failed to load map!\n");
		return false;
	}

	cJSON* json = cJSON_Parse(filestr);

	if (!json)
	{
		printf("ERROR::Failed to load map!\n");
		return false;
	}

	//we can free the raw char data
	free(filestr);
	filestr = NULL;

	cJSON* layers = cJSON_GetObjectItem(json, "layers");

	if (!layers)
	{
		printf("ERROR::Failed to load map!\n");
		result = false;
		goto cleanup;
	}

	cJSON* layer = NULL;
	cJSON_ArrayForEach(layer, layers)
	{
		cJSON* type = cJSON_GetObjectItem(layer, "type");

		if (!type || !cJSON_IsString(type))
		{
			printf("ERROR::Failed to load map!\n");
			result = false;
			goto cleanup;
		}

		//tile layer
		if(!strcmp(cJSON_GetStringValue(type), "tilelayer"))
		{
			cJSON* width = cJSON_GetObjectItem(layer, "width");
			cJSON* height = cJSON_GetObjectItem(layer, "height");

			if (!width || !height || !cJSON_IsNumber(width) || !cJSON_IsNumber(height))
			{
				printf("ERROR::Failed to load map!\n");
				result = false;
				goto cleanup;
			}

			s_map.width = cJSON_GetNumberValue(width);
			s_map.height = cJSON_GetNumberValue(height);

			cJSON* tile_data = cJSON_GetObjectItem(layer, "data");

			if (!tile_data || !cJSON_IsArray(tile_data))
			{
				printf("ERROR::Failed to load map!\n");
				result = false;
				goto cleanup;
			}

			//allocate tile buffer
			s_map.tiles = calloc(s_map.height * s_map.width, sizeof(TileID));

			if (!s_map.tiles)
			{
				printf("ERROR::Failed to load map!\n");
				result = false;
				goto cleanup;
			}

			//allocate object tile buffer
			s_map.object_tiles = calloc(s_map.height * s_map.width, sizeof(ObjectID));

			if (!s_map.object_tiles)
			{
				printf("ERROR::Failed to load map!\n");
				result = false;
				goto cleanup;
			}

			int index = 0;
			//parse each tile
			for (int y = 0; y < s_map.height; y++)
			{
				for (int x = 0; x < s_map.width; x++)
				{
					cJSON* tile = cJSON_GetArrayItem(tile_data, index);

					if (!tile || !cJSON_IsNumber(tile))
					{
						printf("ERROR::Failed to load map!\n");
						result = false;
						goto cleanup;
					}

					int tile_number = cJSON_GetNumberValue(tile);

					if (tile_number > 0)
					{
						s_map.num_non_empty_tiles++;
					}

					s_map.tiles[x + y * s_map.width] = tile_number;

					index++;
				}
			}

		}
		//parse objects
		else if (!strcmp(cJSON_GetStringValue(type), "objectgroup"))
		{
			cJSON* objects = cJSON_GetObjectItem(layer, "objects");

			if (!objects || !cJSON_IsArray(objects))
			{
				printf("ERROR::Failed to load map!\n");
				result = false;
				goto cleanup;
			}

			cJSON* object = NULL;
			cJSON_ArrayForEach(object, objects)
			{
				cJSON* x_coord = cJSON_GetObjectItem(object, "x");
				cJSON* y_coord = cJSON_GetObjectItem(object, "y");

				if (!x_coord || !y_coord || !cJSON_IsNumber(x_coord) || !cJSON_IsNumber(y_coord))
				{
					printf("ERROR::Failed to load map!\n");
					result = false;
					goto cleanup;
				}

				cJSON* type = cJSON_GetObjectItem(object, "type");
				if (!type || !cJSON_IsString(type))
				{
					printf("ERROR::Failed to load map!\n");
					result = false;
					goto cleanup;
				}

				cJSON* id = cJSON_GetObjectItem(object, "id");

				int map_id = -1;

				if (id && cJSON_IsNumber(id))
				{
					map_id = cJSON_GetNumberValue(id);
				}

				float obj_x = cJSON_GetNumberValue(x_coord) / TILE_SIZE;
				float obj_y = cJSON_GetNumberValue(y_coord) / TILE_SIZE;

				char* type_value = cJSON_GetStringValue(type);


				if (!strcmp(type_value, "spawnpoint"))
				{
					s_map.player_spawn_point_x = obj_x;
					s_map.player_spawn_point_y = obj_y;
					continue;
				}

				Object* map_object = NULL;

				bool is_tile_centered = false;

				if (!strcmp(type_value, "light"))
				{
					map_object = Object_Spawn(OT__LIGHT, 0, obj_x, obj_y);

					//is_tile_centered = true;
				}
				else if (!strcmp(type_value, "barrel"))
				{
					map_object = Object_Spawn(OT__THING, SUB__THING_BARREL, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "torch"))
				{
					map_object = Object_Spawn(OT__THING, SUB__THING_TORCH, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "monster_imp"))
				{
					map_object = Object_Spawn(OT__MONSTER, SUB__MOB_IMP, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "monster_pinky"))
				{
					map_object = Object_Spawn(OT__MONSTER, SUB__MOB_PINKY, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "pickup_smallhp"))
				{
					map_object = Object_Spawn(OT__PICKUP, SUB__PICKUP_SMALLHP, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "pickup_ammo"))
				{
					map_object = Object_Spawn(OT__PICKUP, SUB__PICKUP_AMMO, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "trigger") || !strcmp(type_value, "trigger_once") || !strcmp(type_value, "trigger_switch"))
				{
					if (!strcmp(type_value, "trigger_once"))
					{
						map_object = Object_Spawn(OT__TRIGGER, SUB__TRIGGER_ONCE, obj_x, obj_y);
					}
					else if(!strcmp(type_value, "trigger_switch"))
					{
						map_object = Object_Spawn(OT__TRIGGER, SUB__TRIGGER_SWITCH, obj_x, obj_y);

						is_tile_centered = true;

						cJSON* gid = cJSON_GetObjectItem(object, "gid");

						if (gid && cJSON_IsNumber(gid))
						{
							map_object->state = cJSON_GetNumberValue(gid);
						}
					}
					else
					{
						map_object = Object_Spawn(OT__TRIGGER, SUB__NONE, obj_x, obj_y);
					}

					if (map_object)
					{
						is_tile_centered = true;

						cJSON* props = cJSON_GetObjectItem(object, "properties");

						if (props && cJSON_IsArray(props))
						{
							cJSON* prop = NULL;
							cJSON_ArrayForEach(prop, props)
							{
								cJSON* prop_name = cJSON_GetObjectItem(prop, "name");

								if (prop_name && cJSON_IsString(prop_name) && !strcmp(cJSON_GetStringValue(prop_name), "target"))
								{
									cJSON* prop_value = cJSON_GetObjectItem(prop, "value");

									if (prop_value && cJSON_IsNumber(prop_value))
									{
										//hacky way to store int, but works for now
										map_object->target = (unsigned)cJSON_GetNumberValue(prop_value);
										break;
									}
								}
							}
						}
					}
					
				}
				else if (!strcmp(type_value, "trigger_secret"))
				{
					map_object = Object_Spawn(OT__TRIGGER, SUB__TRIGGER_SECRET, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "trigger_changelevel"))
				{
					map_object = Object_Spawn(OT__TRIGGER, SUB__TRIGGER_CHANGELEVEL, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "target_teleport"))
				{
					map_object = Object_Spawn(OT__TARGET, SUB__TARGET_TELEPORT, obj_x, obj_y);
				}
				else if (!strcmp(type_value, "door_v"))
				{
					map_object = Object_Spawn(OT__DOOR, SUB__DOOR_VERTICAL, obj_x, obj_y);

					is_tile_centered = true;
				}
				else if (!strcmp(type_value, "tile_fake"))
				{
					map_object = Object_Spawn(OT__SPECIAL_TILE, SUB__SPECIAL_TILE_FAKE, obj_x, obj_y);

					if (map_object)
					{
						is_tile_centered = true;

						cJSON* gid = cJSON_GetObjectItem(object, "gid");

						if (gid && cJSON_IsNumber(gid))
						{
							map_object->state = cJSON_GetNumberValue(gid);
						}

					}
				}

				if (map_object)
				{
					map_object->map_id = map_id;

					if (is_tile_centered)
					{
						map_object->y -= 1;
					}
				}
			}
		
		}
	}

	Map_ConnectTriggersToTargets();
	Map_UpdateObjectTilemap();
	Map_UpdateSortedList();
	Map_SetupLightTiles();

	cleanup:
	//cleanup
	cJSON_Delete(json);
	
	return result;
}

TileID Map_GetTile(int x, int y)
{
	if (x < 0 || y < 0 || x >= s_map.width || y >= s_map.height)
	{
		return EMPTY_TILE;
	}

	size_t index = x + y * s_map.width;

	assert(index < s_map.width * s_map.height);

	return s_map.tiles[index];
}

Object* Map_GetObjectAtTile(int x, int y)
{
	if (x < 0 || y < 0 || x >= s_map.width || y >= s_map.height)
	{
		return NULL;
	}

	size_t index = x + y * s_map.width;

	ObjectID id = s_map.object_tiles[index];

	if (id == NULL_INDEX)
	{
		return NULL;
	}

	return &s_map.objects[id];
}

LightTile* Map_GetLightTile(int x, int y)
{
	if (x < 0)
	{
		x = 0;
	}
	else if (x >= s_map.width)
	{
		x = s_map.width - 1;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y >= s_map.height)
	{
		y = s_map.height - 1;
	}

	int index = x + y * s_map.width;

	return &s_map.light_tiles[index];
}

TileID Map_Raycast(float p_x, float p_y, float dir_x, float dir_y, float* r_hitX, float* r_hitY)
{
	const int max_tiles = Map_GetTotalTiles();

	float ray_dir_x = dir_x;
	float ray_dir_y = dir_y;

	//length of ray from one x or y-side to next x or y-side
	float delta_dist_x = (ray_dir_x == 0) ? 1e30 : fabs(1.0 / ray_dir_x);
	float delta_dist_y = (ray_dir_y == 0) ? 1e30 : fabs(1.0 / ray_dir_y);

	int step_x = 0;
	int step_y = 0;

	int side = 0; //was a NS or a EW wall hit?

	int map_x = (int)p_x;
	int map_y = (int)p_y;

	//length of ray from current position to next x or y-side
	float side_dist_x;
	float side_dist_y;

	if (ray_dir_x < 0)
	{
		step_x = -1;
		side_dist_x = (p_x - map_x) * delta_dist_x;
	}
	else
	{
		step_x = 1;
		side_dist_x = (map_x + 1.0 - p_x) * delta_dist_x;
	}
	if (ray_dir_y < 0)
	{
		step_y = -1;
		side_dist_y = (p_y - map_y) * delta_dist_y;
	}
	else
	{
		step_y = 1;
		side_dist_y = (map_y + 1.0 - p_y) * delta_dist_y;
	}

	TileID tile = EMPTY_TILE;

	//perform DDA
	for (int step = 0; step < max_tiles; step++)
	{
		if (side_dist_x < side_dist_y)
		{
			side_dist_x += delta_dist_x;
			map_x += step_x;
			side = 0;
		}
		else
		{
			side_dist_y += delta_dist_y;
			map_y += step_y;
			side = 1;
		}

		tile = Map_GetTile(map_x, map_y);

		if (tile != EMPTY_TILE)
		{			
			break;
		}

		Object* obj_tile = Map_GetObjectAtTile(map_x, map_y);

		if (obj_tile)
		{
			if ((obj_tile->type == OT__DOOR || obj_tile->type == OT__SPECIAL_TILE))
			{
				break;
			}
			else
			{
				return EMPTY_TILE;
			}
		}
	}

	float wall_dist = 0;

	if (side == 0)
	{
		wall_dist = (side_dist_x - delta_dist_x);
	}
	else
	{
		wall_dist = (side_dist_y - delta_dist_y);
	}

	//calculate value of wallX
	float wall_x; //where exactly the wall was hit
	if (side == 0) wall_x = p_y + wall_dist * ray_dir_y;
	else          wall_x = p_x + wall_dist * ray_dir_x;
	wall_x -= floor((wall_x));

	*r_hitX = p_x + wall_dist * ray_dir_x;
	*r_hitY = p_y + wall_dist * ray_dir_y;

	return tile;
}


void Map_GetSize(int* r_width, int* r_height)
{
	if (r_width)
	{
		*r_width = s_map.width;
	}
	if (r_height)
	{
		*r_height = s_map.height;
	}
}

void Map_GetSpawnPoint(int* r_x, int* r_y)
{
	if (r_x)
	{
		*r_x = s_map.player_spawn_point_x;
	}
	if (r_y)
	{
		*r_y = s_map.player_spawn_point_y;
	}
}

bool Map_UpdateObjectTile(Object* obj)
{
	if (obj->x < 0 || obj->y < 0)
	{
		return false;
	}

	if (obj->type == OT__PARTICLE)
	{
		return true;
	}

	int total_tiles = Map_GetTotalTiles();

	int tile_x = (int)obj->x;
	int tile_y = (int)obj->y;

	int index = tile_x + tile_y * s_map.width;

	if (index >= total_tiles || index < 0)
	{
		return false;
	}

	ObjectID id = s_map.object_tiles[index];

	if (id != NULL_INDEX)
	{
		Object* tile_obj = &s_map.objects[id];

		if (tile_obj != obj)
		{
			//if we are a missile don't take take anyone's place
			if (obj->type == OT__MISSILE)
			{
				return true;
			}

			if (tile_obj->type == OT__PLAYER || tile_obj == OT__MONSTER || tile_obj->type == OT__THING)
			{
				return false;
			}

			//return true, but don't take the spot
			if (tile_obj->type == OT__DOOR || tile_obj->type == OT__TRIGGER || tile_obj->type == OT__SPECIAL_TILE || tile_obj->type == OT__PICKUP)
			{
				return true;
			}
			
		}
	}

	//free up old tile index
	if (obj->tile_index != NULL_INDEX)
	{
		s_map.object_tiles[obj->tile_index] = NULL_INDEX;
	}
	
	obj->tile_index = index;
	s_map.object_tiles[index] = obj->id;

	return true;
}

int Map_GetTotalTiles()
{
	return s_map.width * s_map.height;
}

int Map_GetTotalNonEmptyTiles()
{
	return s_map.num_non_empty_tiles;
}

void Map_Draw(Image* image, Image* texture)
{
	const int rect_width = (image->width / s_map.width);
	const int rect_height = image->height / s_map.height;

	for (int x = 0; x < s_map.width; x++)
	{
		for (int y = 0; y < s_map.height; y++)
		{
			TileID tile = Map_GetTile(x, y);

			if (tile == EMPTY_TILE) continue;

			int cx = x * rect_width;
			int cy = y * rect_height;

			

			unsigned char color[4] = { 255, 255, 255, 255 };

		

			//unsigned char* color = Image_Get(texture, x, y);

			if (!color)
			{
				continue;
			}

			//Video_DrawRectangle(image, cx, cy, rect_width, rect_height, color);

			//top line
			Video_DrawLine(image, cx, cy, cx + rect_width, cy, color);

			//right line
			Video_DrawLine(image, cx + rect_width, cy, cx + rect_width, cy + rect_height, color);

			//bottom line
			Video_DrawLine(image, cx, cy + rect_height, cx + rect_width, cy + rect_height, color);

			//left line
			Video_DrawLine(image, cx, cy, cx, cy + rect_height, color);

		}
	}
}

void Map_DrawObjects(Image* image, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	int num_sprites = 0;

	for (int i = 0; i < s_map.num_sorted_objects; i++)
	{
		ObjectID id = s_map.sorted_list[i];
		Object* object = &s_map.objects[id];

		if (object->type == OT__NONE || !object->sprite.img || object->sprite.skip_draw == true)
		{
			continue;
		}

		Sprite* sprite = &object->sprite;

		//calc distance to point
		sprite->dist = (p_x - sprite->x) * (p_x - sprite->x) + (p_y - sprite->y) * (p_y - sprite->y);

		s_spritePointers[num_sprites++] = &object->sprite;

		Render_AddSpriteToQueue(sprite);
	}

	//sort by distance
	//qsort(s_spritePointers, num_sprites, sizeof(Sprite*), CompareSpriteDistances);

	//draw all sprites
	for (int i = 0; i < num_sprites; i++)
	{
		//Video_DrawSprite(image, s_spritePointers[i], depth_buffer, draw_spans, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY);
	}
}

void Map_UpdateObjects(float delta)
{
	float view_x, view_y, dir_x, dir_y, dir_z, plane_x, plane_y;
	Player_GetView(&view_x, &view_y, &dir_x, &dir_y, &plane_x, &plane_y);

	float inv_det = 1.0 / (plane_x * dir_y - dir_x * plane_y);

	for (int i = 0; i < s_map.num_sorted_objects; i++)
	{
		ObjectID id = s_map.sorted_list[i];
		Object* obj = &s_map.objects[id];

		obj->sprite.skip_draw = false;

		if (obj->type == OT__NONE)
		{
			continue;
		}

		switch (obj->type)
		{
		case OT__MONSTER:
		{
			Monster_Update(obj, delta);
			break;
		}
		case OT__MISSILE:
		{
			Missile_Update(obj, delta);
			break;
		}
		case OT__DOOR:
		{
			Move_Door(obj, delta);
			break;
		}
		case OT__PARTICLE:
		{
			Particle_Update(obj, delta);
			break;
		}
		//fallthrough
		case OT__TRIGGER:
		case OT__PICKUP:
		case OT__THING:
		{
			if (obj->sprite.img && obj->sprite.frame_count > 0 && obj->sprite.anim_speed_scale > 0)
			{
				Sprite_UpdateAnimation(&obj->sprite, delta);
			}
			break;
		}
		default:
			break;
		}

		if (!obj->sprite.img)
		{
			continue;
		}

		bool sprite_position_changed = false;

		float next_sprite_x = obj->x + obj->sprite.offset_x;
		float next_sprite_y = obj->y + obj->sprite.offset_y;

		if (obj->sprite.x != next_sprite_x || next_sprite_y)
		{
			sprite_position_changed = true;
		}

		obj->sprite.x = next_sprite_x;
		obj->sprite.y = next_sprite_y;


		//translate sprite position to relative to camera
		float local_sprite_x = obj->sprite.x - view_x;
		float local_sprite_y = obj->sprite.y - view_y;

		float transform_x = inv_det * (dir_y * local_sprite_x - dir_x * local_sprite_y);
		float transform_y = inv_det * (-plane_y * local_sprite_x + plane_x * local_sprite_y);

		obj->view_x = transform_x;
		obj->view_y = transform_y;

		//if the sprite is in view and its position changed, request sprite redraw
		if (transform_y > 0) 
		{
			if(sprite_position_changed)
				Render_RedrawSprites();
		}
		else
		{
			obj->sprite.skip_draw = true;
		}
		
	}
}

void Map_DeleteObject(Object* obj)
{
	//Render_FinishAndStall();

	Render_LockObjectMutex();

	ObjectID id = obj->id;

	assert(id < MAX_OBJECTS);

	//remove from object tilemap
	assert(obj->tile_index < s_map.width * s_map.height);

	if (obj->tile_index >= 0)
	{
		s_map.object_tiles[obj->tile_index] = NULL_INDEX;
	}

	//reset the object
	memset(obj, 0, sizeof(Object));

	//store the id in free list
	Map_FreeListStoreID(id);

	//update sorted list
	Map_UpdateSortedList();

	Render_UnlockObjectMutex();

	//Render_Resume();
}

void Map_Destruct()
{
	//keep old level index
	int old_level_index = s_map.level_index;

	if(s_map.tiles) free(s_map.tiles);
	if(s_map.object_tiles) free(s_map.object_tiles);
	if (s_map.light_tiles) free(s_map.light_tiles);
	
	memset(&s_map, 0, sizeof(s_map));

	s_map.level_index = old_level_index;
}
