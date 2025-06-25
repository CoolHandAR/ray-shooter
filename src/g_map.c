#include "g_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <cjson/cJSON.h>

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

		int tile_x = obj->x;
		int tile_y = obj->y;

		int index = tile_x + tile_y * s_map.width;

		s_map.object_tiles[index] = i;

		obj->tile_index = index;
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

	s_map.num_objects++;

	Map_UpdateSortedList();

	return obj;
}

bool Map_Load(const char* filename)
{
	memset(&s_map, 0, sizeof(s_map));

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

				char* type_value = cJSON_GetStringValue(type);

				if (!strcmp(type_value, "spawnpoint"))
				{
					s_map.player_spawn_point_x = cJSON_GetNumberValue(x_coord) / 64;
					s_map.player_spawn_point_y = cJSON_GetNumberValue(y_coord) / 64;
					continue;
				}

				Object* map_object = Map_NewObject(OT__NONE);

				map_object->x = cJSON_GetNumberValue(x_coord) / 64;
				map_object->y = cJSON_GetNumberValue(y_coord) / 64;

				map_object->sprite.x = map_object->x;
				map_object->sprite.y = map_object->y;

				if (!strcmp(type_value, "light"))
				{
					map_object->type = OT__LIGHT;

					map_object->hp = 1;
					map_object->sprite.frame = 0;
				}
				else if (!strcmp(type_value, "barrel"))
				{
					map_object->type = OT__THING;

					map_object->hp = 1;
					map_object->sprite.frame = 1;
				}
				else if (!strcmp(type_value, "torch"))
				{
					map_object->type = OT__THING;

					map_object->hp = 1;
					map_object->sprite.frame = 2;
				}
				else if (!strcmp(type_value, "tree"))
				{
					map_object->type = OT__THING;

					map_object->hp = 1;
					map_object->sprite.frame = 3;
				}
				else if (!strcmp(type_value, "monster"))
				{
					map_object->type = OT__MONSTER;
					map_object->sub_type = SUB__MOB_IMP;
					map_object->hp = 50;
					map_object->sprite.img = &assets->imp_texture;
					map_object->sprite.h_frames = 9;
					map_object->sprite.v_frames = 9;
					Sprite_GenerateAlphaSpans(&map_object->sprite);

					Monster_SetState(map_object, 0);
				}
				else if (!strcmp(type_value, "pickup_smallhp"))
				{
					map_object->type = SUB__PICKUP_SMALLHP;
					map_object->type = OT__PICKUP;
					map_object->hp = 5;
					map_object->sprite.h_frames = 2;
					map_object->sprite.v_frames = 1;
					map_object->sprite.img = &assets->pickup_textures;
					map_object->sprite.frame = 0;

				}
				if (map_object->type == OT__LIGHT || map_object->type == OT__THING)
				{
					map_object->sprite.img = &assets->decoration_textures;
					map_object->sprite.h_frames = 4;
					map_object->sprite.v_frames = 1;
				}
			}
		
		}
	}

	Map_UpdateObjectTilemap();
	Map_UpdateSortedList();

	cleanup:
	//cleanup
	cJSON_Delete(json);
	free(filestr);
	
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

Object* Map_Raycast(float p_x, float p_y, float dir_x, float dir_y)
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
			return NULL;
		}

		Object* obj = Map_GetObjectAtTile(map_x, map_y);

		//alive object found?
		if (obj && obj->hp > 0)
		{
			if (Trace_LineVsObject(p_x, p_y, dir_x * 1000, dir_y * 1000, obj))
			{
				return obj;
			}
		}
	}


	return NULL;
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

void Map_UpdateObjectTile(Object* obj)
{
	if (obj->x < 0 || obj->y < 0 || obj->x >= s_map.width || obj->y >= s_map.height)
	{
		return;
	}

	int tile_x = (int)obj->x;
	int tile_y = (int)obj->y;

	size_t index = tile_x + tile_y * s_map.width;

	ObjectID id = s_map.object_tiles[index];

	if (id != NULL_INDEX)
	{
		Object* tile_obj = &s_map.objects[id];

		//if we are a missile don't take take anyone's place
		if (obj->type == OT__MISSILE)
		{
			return;
		}

		//if there is overlap with light or a static object, stop
		if (tile_obj->type == OT__LIGHT || tile_obj->type == OT__THING)
		{
			return;
		}
	}

	//free up old tile index
	if (obj->tile_index != NULL_INDEX)
	{
		s_map.object_tiles[obj->tile_index] = NULL_INDEX;
	}
	
	obj->tile_index = index;
	s_map.object_tiles[index] = obj->id;
}

int Map_GetTotalTiles()
{
	return s_map.width * s_map.height;
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

void Map_DrawObjects(Image* image, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	int num_sprites = 0;

	for (int i = 0; i < s_map.num_sorted_objects; i++)
	{
		ObjectID id = s_map.sorted_list[i];
		Object* object = &s_map.objects[id];

		if (object->type == OT__NONE || !object->sprite.img)
		{
			continue;
		}

		Sprite* sprite = &object->sprite;

		//calc distance to point
		sprite->dist = (p_x - sprite->x) * (p_x - sprite->x) + (p_y - sprite->y) * (p_y - sprite->y);

		s_spritePointers[num_sprites++] = &object->sprite;
	}

	//sort by distance
	qsort(s_spritePointers, num_sprites, sizeof(Sprite*), CompareSpriteDistances);

	//draw all sprites
	for (int i = 0; i < num_sprites; i++)
	{
		Video_DrawSprite(image, s_spritePointers[i], depth_buffer, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY);
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

		if (obj->type == OT__NONE)
		{
			continue;
		}

		if (obj->type == OT__MONSTER)
		{
			Monster_Update(obj, delta);
		}
		else if (obj->type == OT__MISSILE)
		{
			Missile_Update(obj, delta);
		}

		bool sprite_position_changed = false;

		if (obj->sprite.x != obj->x || obj->sprite.y != obj->y)
		{
			sprite_position_changed = true;
		}

		obj->sprite.x = obj->x;
		obj->sprite.y = obj->y;

		//translate sprite position to relative to camera
		float local_sprite_x = obj->sprite.x - view_x;
		float local_sprite_y = obj->sprite.y - view_y;

		float transform_y = inv_det * (-plane_y * local_sprite_x + plane_x * local_sprite_y);

		//if the sprite is in view and its position changed, request sprite redraw
		if (transform_y > 0 && sprite_position_changed) 
		{
			Render_RedrawSprites();
		}
	}
}

void Map_DeleteObject(Object* obj)
{
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
}

void Map_Destruct()
{
	free(s_map.tiles);
	free(s_map.object_tiles);
	
	memset(&s_map, 0, sizeof(s_map));
}
