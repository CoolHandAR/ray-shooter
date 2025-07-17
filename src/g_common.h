#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <GLFW/glfw3.h>

#include "r_common.h"
#include "miniaudio/miniaudio.h"

#define EMPTY_TILE 0
#define DOOR_TILE 2
#define NULL_INDEX -1
#define MAX_OBJECTS 1024
#define TILE_SIZE 64

#define DOOR_SLEEP 0
#define DOOR_OPEN 1
#define DOOR_CLOSE 2
#define DOOR_AUTOCLOSE_TIME 5

#define MIN_LIGHT 20

typedef uint16_t TileID;
typedef int16_t ObjectID;

typedef enum
{
	GS__NONE,

	GS__MENU,
	GS__LEVEL,
	GS__LEVEL_END,
	GS__EXIT
} GameState;

typedef struct
{
	GameState state;

	float secret_timer;

	int prev_total_secrets;
	int prev_total_monsters;

	int prev_monsters_killed;
	int prev_secrets_found;

	int total_secrets;
	int total_monsters;

	int secrets_found;
	int monsters_killed;
} Game;

//A lazy way to store assets, but good enough for now
typedef struct
{
	Image wall_textures;
	Image weapon_textures;
	Image decoration_textures;
	Image pickup_textures;
	Image missile_textures;
	Image imp_texture;
	Image pinky_texture;
	Image minigun_texture;
	Image sky_texture;
	Image particle_textures;
	Image menu_texture;
} GameAssets;

bool Game_Init();
void Game_Exit();
bool Game_LoadAssets();
void Game_DestructAssets();
void Game_Update(float delta);
void Game_Draw(Image* image, FontData* fd, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Game_DrawHud(Image* image, FontData* fd);
void Game_SetState(GameState state);
GameState Game_GetState();
GameAssets* Game_GetAssets();
void Game_ChangeLevel();
void Game_Reset(bool to_start);
void Game_SecretFound();

//Menu stuff
void Menu_Update(float delta);
void Menu_Draw(Image* image, FontData* fd);
void Menu_LevelEnd_Update(float delta, int secret_goal, int secret_max, int monster_goal, int monster_max);
void Menu_LevelEnd_Draw(Image* image, FontData* fd);


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
	OT__TRIGGER,
	OT__TARGET,
	OT__DOOR,
	OT__SPECIAL_TILE,
	OT__PARTICLE,
	OT__MAX
} ObjectType;

typedef enum
{
	SUB__NONE,

	//monsters
	SUB__MOB_IMP,
	SUB__MOB_PINKY,

	//pickups
	SUB__PICKUP_SMALLHP,
	SUB__PICKUP_MEDIUMHP,
	SUB__PICKUP_AMMO,

	//missiles
	SUB__MISSILE_FIREBALL,

	//TRIGGER
	SUB__TRIGGER_ONCE,
	SUB__TRIGGER_CHANGELEVEL,
	SUB__TRIGGER_SECRET,
	SUB__TRIGGER_SWITCH,

	//TARGET
	SUB__TARGET_TELEPORT,

	//DOOR
	SUB__DOOR_VERTICAL,

	//SPECIAL TILE
	SUB__SPECIAL_TILE_FAKE,
	
	//THINGS
	SUB__THING_TORCH,
	SUB__THING_BARREL,

	//PARTICLES
	SUB__PARTICLE_BLOOD,
	SUB__PARTICLE_WALL_HIT,

} SubType;

typedef enum
{
	OBJ_FLAG__NONE = 0,

	OBJ_FLAG__JUST_ATTACKED = 1 << 0,
	OBJ_FLAG__EXPLODING = 1 << 1,
	OBJ_FLAG__JUST_HIT = 1 << 2,

	OBJ_FLAG__DOOR_NEVER_CLOSE = 1 << 3,
	OBJ_FLAG__TRIGGER_SWITCHED_ON = 1 << 4
} ObjectFlag;

typedef struct
{
	int map_id; 

	int tile_index;
	ObjectID id;
	Sprite sprite;
	ObjectType type;
	SubType sub_type;

	float x, y;
	float view_x, view_y;
	float dir_x, dir_y;
	float size;
	float speed;

	int hp;

	int flags;

	//for missiles and certain attacks
	struct Object* owner;

	//for monsters and triggers
	struct Object* target;

	//for doors and general debugging
	struct Object* col_object;

	//for monsters and doors
	float move_timer;
	float attack_timer;
	float stop_timer;
	float stuck_timer;
	DirEnum dir_enum;
	int state;
} Object;

typedef struct
{
	uint8_t light;
	uint8_t light_north;
	uint8_t light_west;
	uint8_t light_south;
	uint8_t light_east;
} LightTile;

typedef struct
{
	int width, height;
	TileID* tiles;

	LightTile* light_tiles;

	int num_objects;
	Object objects[MAX_OBJECTS];

	ObjectID free_list[MAX_OBJECTS];
	int num_free_list;

	ObjectID sorted_list[MAX_OBJECTS];
	int num_sorted_objects;

	ObjectID* object_tiles;

	int player_spawn_point_x;
	int player_spawn_point_y;

	int level_index;

	int num_non_empty_tiles;
} Map;

int Map_GetLevelIndex();
Map* Map_GetMap();
Object* Map_NewObject(ObjectType type);
bool Map_Load(const char* filename);
TileID Map_GetTile(int x, int y);
Object* Map_GetObjectAtTile(int x, int y);
LightTile* Map_GetLightTile(int x, int y);
TileID Map_Raycast(float p_x, float p_y, float dir_x, float dir_y, float* r_hitX, float* r_hitY);
void Map_GetSize(int* r_width, int* r_height);
void Map_GetSpawnPoint(int* r_x, int* r_y);
bool Map_UpdateObjectTile(Object* obj);
int Map_GetTotalTiles();
int Map_GetTotalNonEmptyTiles();
void Map_Draw(Image* image, Image* texture);
void Map_DrawObjects(Image* image, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Map_UpdateObjects(float delta);
void Map_DeleteObject(Object* obj);
void Map_Destruct();


//Player stuff
void Player_Init();
Object* Player_GetObj();
void Player_Hurt(float dir_x, float dir_y);
void Player_HandlePickup(Object* obj);
void Player_Update(GLFWwindow* window, float delta);
void Player_GetView(float* r_x, float* r_y, float* r_dirX, float* r_dirY, float* r_planeX, float* r_planeY);
void Player_MouseCallback(float x, float y);
void Player_Draw(Image* image, FontData* font);
float Player_GetSensitivity();
void Player_SetSensitivity(float sens);

//Explosion stuff
void Explosion(Object* obj, float size, int damage);

//Movement stuff
bool Move_CheckStep(Object* obj, float p_stepX, float p_stepY, float p_size);
bool Move_Object(Object* obj, float p_moveX, float p_moveY);
bool Move_CheckArea(Object* obj, float x, float y, float size);
bool Move_Unstuck(Object* obj);
bool Move_Teleport(Object* obj, float x, float y);
bool Move_Door(Object* obj, float delta);

//Missile stuff
void Missile_Update(Object* obj, float delta);
void Missile_DirectHit(Object* obj, Object* target);
void Missile_Explode(Object* obj);

//Trace stuff
bool Trace_LineVsObject(float p_x, float p_y, float p_endX, float p_endY, Object* obj, float* r_interX, float* r_interY);

//Shooting stuff
void Gun_Shoot(Object* obj, float p_x, float p_y, float p_dirX, float p_dirY);

//Object stuff
void Object_Hurt(Object* obj, Object* src_obj, int damage);

bool Object_IsSpecialCollidableTile(Object* obj);
bool Object_CheckLineToTile(Object* obj, float target_x, float target_y);
bool Object_CheckLineToTile2(Object* obj, float target_x, float target_y);
bool Object_CheckLineToTarget(Object* obj, Object* target);
bool Object_CheckSight(Object* obj, Object* target);
Object* Object_Missile(Object* obj, Object* target);
bool Object_HandleObjectCollision(Object* obj, Object* collision_obj);
bool Object_HandleSwitch(Object* obj);
void Object_HandleTriggers(Object* obj, Object* trigger);
Object* Object_Spawn(ObjectType type, SubType sub_type, float x, float y);

//Dir stuff
DirEnum DirVectorToDirEnum(int x, int y);
DirEnum DirVectorToRoundedDirEnum(int x, int y);

//Particle stuff
void Particle_Update(Object* obj, float delta);

//Monster stuff
void Monster_Spawn(Object* obj);
void Monster_SetState(Object* obj, int state);
void Monster_Update(Object* obj, float delta);
void Monster_Imp_FireBall(Object* obj);
void Monster_Melee(Object* obj);
