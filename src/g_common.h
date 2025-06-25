#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <GLFW/glfw3.h>

#include "r_common.h"

#define EMPTY_TILE 0
#define NULL_INDEX -1
#define MAX_OBJECTS 1024


typedef uint16_t TileID;
typedef int16_t ObjectID;

//A lazy way to store assets, but good enough for now
typedef struct
{
	Image wall_textures;
	Image weapon_textures;
	Image decoration_textures;
	Image pickup_textures;
	Image missile_textures;
	Image imp_texture;
} GameAssets;

bool Game_LoadAssets();
void Game_DestructAssets();
GameAssets* Game_GetAssets();

typedef enum
{
	DIR_NONE,
	DIR_NORTH,
	DIR_NORTH_EAST,
	DIR_NORTH_WEST,
	DIR_EAST,
	DIR_SOUTH_EAST,
	DIR_SOUTH,
	DIR_SOUTH_WEST,
	DIR_WEST,
	DIR_MAX
} DirEnum;

typedef enum
{
	OT__NONE,
	OT__PLAYER,
	OT__MONSTER,
	OT__LIGHT,
	OT__THING,
	OT__PICKUP,
	OT__MISSILE,
	OT__MAX
} ObjectType;

typedef enum
{
	SUB__NONE,

	//monsters
	SUB__MOB_IMP,

	//pickups
	SUB__PICKUP_SMALLHP,
	SUB__PICKUP_MEDIUMHP,

	//missiles
	SUB__MISSILE_FIREBALL
} SubType;

typedef enum
{
	OBJ_FLAG__NONE = 0,

	OBJ_FLAG__JUST_ATTACKED = 1 << 0,
	OBJ_FLAG__EXPLODING = 1 << 1,
} ObjectFlag;

typedef struct
{
	int tile_index;
	ObjectID id;
	Sprite sprite;
	ObjectType type;
	SubType sub_type;
	float x, y;
	float dir_x, dir_y;
	float hp;

	int flags;

	//for missiles and certain attacks
	struct Object* owner;

	//for monsters
	float move_timer;
	float attack_timer;
	float stop_timer;
	DirEnum dir_enum;
	struct Object* target;
	int state;
	int monster_type;
} Object;

typedef struct
{
	int width, height;
	TileID* tiles;

	int num_objects;
	Object objects[MAX_OBJECTS];

	ObjectID free_list[MAX_OBJECTS];
	int num_free_list;

	ObjectID sorted_list[MAX_OBJECTS];
	int num_sorted_objects;

	ObjectID* object_tiles;

	int player_spawn_point_x;
	int player_spawn_point_y;
} Map;

Map* Map_GetMap();
Object* Map_NewObject(ObjectType type);
bool Map_Load(const char* filename);
TileID Map_GetTile(int x, int y);
Object* Map_GetObjectAtTile(int x, int y);
Object* Map_Raycast(float p_x, float p_y, float dir_x, float dir_y);
void Map_GetSize(int* r_width, int* r_height);
void Map_GetSpawnPoint(int* r_x, int* r_y);
void Map_UpdateObjectTile(Object* obj);
int Map_GetTotalTiles();
void Map_Draw(Image* image, Image* texture);
void Map_DrawObjects(Image* image, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Map_UpdateObjects(float delta);
void Map_DeleteObject(Object* obj);
void Map_Destruct();


//Player stuff
void Player_Init();
Object* Player_GetObj();
void Player_HandlePickup(Object* obj);
void Player_Update(GLFWwindow* window, float delta);
void Player_GetView(float* r_x, float* r_y, float* r_dirX, float* r_dirY, float* r_planeX, float* r_planeY);
void Player_MouseCallback(float x, float y);
void Player_Draw(Image* image);

//Explosion stuff
void Explosion(Object* obj, float size, int damage);

//Movement stuff
bool Move_CheckStep(Object* obj, float p_stepX, float p_stepY, float p_size);
bool Move_Object(Object* obj, float p_moveX, float p_moveY);

//Missile stuff
void Missile_Update(Object* obj, float delta);
void Missile_DirectHit(Object* obj, Object* target);
void Missile_Explode(Object* obj);

//Trace stuff
bool Trace_LineVsObject(float p_x, float p_y, float p_endX, float p_endY, Object* obj);

//Shooting stuff
void Gun_Shoot(Object* obj, float p_x, float p_y, float p_dirX, float p_dirY);

//Object stuff
void Object_Hurt(Object* obj, Object* src_obj, int damage);
bool Object_CheckLine(Object* obj, Object* target);
bool Object_CheckSight(Object* obj, Object* target);
Object* Object_Missile(Object* obj, Object* target);

//Monster stuff
void Monster_SetState(Object* obj, int state);
void Monster_Update(Object* obj, float delta);
void Monster_Imp_FireBall(Object* obj);