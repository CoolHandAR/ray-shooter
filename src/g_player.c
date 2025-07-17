#include "g_common.h"

#include <string.h>
#include <math.h>
#include <cglm/cglm.h>

#include "sound.h"
#include "game_info.h"
#include "main.h"
#include "u_math.h"

#define PLAYER_SPEED 10
#define MOUSE_SENS_DIVISOR 1000
#define MIN_SENS 0.5
#define MAX_SENS 16

typedef struct
{
	int ammo;
} GunData;

typedef struct
{
	Sprite gun_sprite;
	Object* obj;
	float plane_x, plane_y;
	int move_x;
	int move_y;
	int slow_move;
	float gun_timer;
	float hurt_timer;
	int gun_sound_id;

	int hurt_dir_x;
	int hurt_dir_y;

	int ammo;

	float sensitivity;

	GunData guns[GUN__MAX];

	bool* visited_tiles;
} PlayerData;

static PlayerData player;

static void Player_ShootGun()
{
	if (player.gun_timer > 0)
	{
		return;
	}
	if (player.ammo <= 0)
	{
		return;
	}
	if (player.obj->hp <= 0)
	{
		return;
	}

	float spread = 3;
	for (int i = 0; i < 8; i++)
	{
		float randomf = Math_randf();

		randomf = Math_Clamp(randomf, 0.0, 0.25);

		if (rand() % 100 > 50)
		{
			randomf = -randomf;
		}

		
		
		Gun_Shoot(player.obj, player.obj->x, player.obj->y, player.obj->dir_x + randomf, player.obj->dir_y + randomf);
	}

	//Object_Spawn(OT__PARTICLE, SUB__PARTICLE_BLOOD, player.obj->x, player.obj->y);

	player.ammo--;

	player.gun_sprite.playing = true;
	player.gun_sprite.frame = 1;
	player.gun_timer = 1.35;

	//Sound_EmitWorldTemp(SOUND__SHOTGUN_SHOOT, player.obj->x - 0.5, player.obj->y - 0.5, player.dir_x, player.dir_y);

	//Object* m = Object_Missile(player.obj, NULL);
//
	//m->owner = player.obj;


	Sound_Play(player.gun_sound_id);
}

static void Shader_Hurt(Image* img, int x, int y, int tx, int ty)
{
	int center_x = abs(x - img->half_width);
	int center_y = abs(y - img->half_height);

	float strenght_x = (float)center_x / (float)img->half_width;
	float strenght_y = (float)center_y / (float)img->half_height;

	float time_mix = Math_Clamp(player.hurt_timer, 0, 1);

	float lerp = glm_lerp(0, strenght_x, strenght_y) * time_mix;

	unsigned char* sample = Image_Get(img, x, y);

	sample[0] = glm_lerp(sample[0], 255, lerp);

	//screen shake effect
	if (player.hurt_timer > 0.48)
	{
		int r0 = rand() % 2;
		float random_lerped = glm_lerp(r0, 2, lerp);

		//Image_Set2(img, x + (random_lerped), y + (random_lerped * player.hurt_dir_y), sample);

	}
}

static void Shader_Dead(Image* img, int x, int y, int tx, int ty)
{
	int center_x = abs(x - img->half_width);
	int center_y = abs(y - img->half_height);

	float strenght_x = (float)center_x / (float)img->half_width;
	float strenght_y = (float)center_y / (float)img->half_height;

	unsigned char* sample = Image_Get(img, x, y);

	Image_Set2(img, x + (rand() % 2), y - (rand() % 2), sample);
}


static void Player_Die()
{

}

static void PressSwitch()
{
	for (int i = 1; i < 3; i++)
	{
		Object* obj = Map_GetObjectAtTile(player.obj->x + (player.obj->dir_x * i), player.obj->y + (player.obj->dir_y * i));

		if (!obj)
		{
			continue;
		}

		if (obj->type == OT__TRIGGER && obj->sub_type == SUB__TRIGGER_SWITCH)
		{
			Object_HandleTriggers(player.obj, obj);

			return;
		}
	}
}

static void DrawMap(Image* image)
{
	GameAssets* assets = Game_GetAssets();

	int map_width, map_height;
	Map_GetSize(&map_width, &map_height);

	const int rect_width = (image->width / map_width);
	const int rect_height = image->height / map_height;

	for (int x = 0; x < map_width; x++)
	{
		for (int y = 0; y < map_height; y++)
		{
			TileID tile = Map_GetTile(x, y);

			if (tile == EMPTY_TILE) continue;

			size_t index = x + y * map_width;

			if (!player.visited_tiles[index])
			{
				//continue;
			}

			int cx = x * rect_width;
			int cy = y * rect_height;

			
			for (int tx = 0; tx < rect_width; tx++)
			{
				for (int ty = 0; ty < rect_height; ty++)
				{
					unsigned char* color = Image_Get(&assets->wall_textures, 64 * (tile - 1) + tx, ty);

					Image_Set2(image, cx + tx, cy + ty, color);
				}
			}

			unsigned char color[4] = { 0, 0, 0, 0 };


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

	unsigned char player_color[4] = { 255, 0, 255, 255 };

	Video_DrawRectangle(image, ((player.obj->x) * rect_width), (player.obj->y * rect_height), 32, 32, player_color);
}

static void Player_ProcessInput(GLFWwindow* window)
{
	int move_x = 0;
	int move_y = 0;
	int slow_move = 0;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		move_x = 1;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		move_x = -1;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		move_y = -1;
	}
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		move_y = 1;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		slow_move = 1;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (player.obj->hp > 0)
		{
			Player_ShootGun();
		}
		else if(player.hurt_timer <= 0.4)
		{
			//Game_Reset(false);
		}	
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		Game_SetState(GS__MENU);
	}

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		PressSwitch();
	}

	player.move_x = move_x;
	player.move_y = move_y;
	player.slow_move = slow_move;
}

static void MarkNearbyTiles()
{
	int map_width, map_height;
	Map_GetSize(&map_width, &map_height);


	int min_x = player.obj->x - 2;
	int min_y = player.obj->y - 2;
								
	int max_x = player.obj->x + 2;
	int max_y = player.obj->y + 2;

	for (int x = min_x; x <= max_x; x++)
	{
		for (int y = min_y; y <= max_y; y++)
		{
			if (Map_GetTile(x, y) != EMPTY_TILE)
			{
				size_t index = x + y * map_width;

				player.visited_tiles[index] = true;
			}
		}
	}
}

void Player_Init()
{
	if (player.visited_tiles)
	{
		free(player.visited_tiles);
	}

	memset(&player, 0, sizeof(player));
	
	int spawn_x, spawn_y;
	Map_GetSpawnPoint(&spawn_x, &spawn_y);

	player.obj = Map_NewObject(OT__PLAYER);

	player.obj->type = OT__PLAYER;	

	player.obj->x = spawn_x;
	player.obj->y = spawn_y;
	player.plane_x = 0;
	player.plane_y = 0.66;
	player.obj->dir_x = -1;
	player.obj->dir_y = 0;

	GameAssets* assets = Game_GetAssets();

	player.gun_sprite.img = &assets->weapon_textures;
	player.gun_sprite.scale_x = 2;
	player.gun_sprite.scale_y = 2;
	player.gun_sprite.anim_speed_scale = 0.7;
	player.gun_sprite.frame = 0;
	player.gun_sprite.flip_h = false;
	player.gun_sprite.flip_v = false;
	player.gun_sprite.looping = false;
	player.gun_sprite.transparency = 0.0;
	player.gun_sprite.frame_count = 10;
	player.obj->hp = 100;
	player.obj->size = 0.4;

	Player_SetSensitivity(1);

	player.gun_sound_id = Sound_Preload(SOUND__SHOTGUN_SHOOT);

	Sound_setMasterVolume(0.2);

	player.ammo = 14;

	int map_width, map_height;
	Map_GetSize(&map_width, &map_height);

	player.visited_tiles = malloc(sizeof(bool) * map_width * map_height);

	if (player.visited_tiles)
	{
		memset(player.visited_tiles, 0, sizeof(bool) * map_width * map_height);
	}
}

Object* Player_GetObj()
{
	return player.obj;
}

void Player_Hurt(float dir_x, float dir_y)
{

	if (player.obj->hp <= 0)
	{
		Sound_EmitWorldTemp(SOUND__PLAYER_DIE, player.obj->x, player.obj->y, player.obj->dir_x, player.obj->dir_y);

		//Game_Reset(false);
	}
	else
	{
		Sound_EmitWorldTemp(SOUND__PLAYER_PAIN, player.obj->x, player.obj->y, player.obj->dir_x, player.obj->dir_y);
	}

	player.hurt_timer = 0.5;

	player.hurt_dir_x = Math_signf(dir_x);
	player.hurt_dir_y = Math_signf(dir_y);
}

void Player_HandlePickup(Object* obj)
{
	switch (obj->sub_type)
	{
	case SUB__PICKUP_SMALLHP:
	{
		player.obj->hp += 5;
		break;
	}
	case SUB__PICKUP_AMMO:
	{
		player.ammo += 10;
		break;
	}
	default:
	{
		break;
	}
	}

	//delete the object
	Map_DeleteObject(obj);
}

void Player_Update(GLFWwindow* window, float delta)
{
	Player_ProcessInput(window);

	float speed = PLAYER_SPEED * delta;

	if (player.slow_move == 1) speed *= 0.5;

	float dir_x = (player.move_x * player.obj->dir_x) + (player.move_y * player.plane_x);
	float dir_y = (player.move_x * player.obj->dir_y) + (player.move_y * player.plane_y);

	if(player.obj->hp > 0)
		Move_Object(player.obj, dir_x * speed, dir_y * speed);

	Sprite_UpdateAnimation(&player.gun_sprite, delta);

	if (!player.gun_sprite.playing)
	{
		player.gun_sprite.frame = 0;
	}

	if (dir_x != 0 || dir_y != 0)
	{
		MarkNearbyTiles();
	}
	player.obj->hp = 100;

	player.gun_timer -= delta;

	ma_engine* sound_engine = Sound_GetEngine();

	ma_engine_listener_set_position(sound_engine, 0, player.obj->x, 0, player.obj->y);
	ma_engine_listener_set_direction(sound_engine, 0, -player.obj->dir_x, 0, -player.obj->dir_y);
	Sound_Set(player.gun_sound_id, player.obj->x, player.obj->y, player.obj->dir_x, player.obj->dir_y);

	player.hurt_timer -= delta;

	if (player.hurt_timer >= 0.4)
	{
		//Engine_SetTimeScale(0.95);
	}
	else
	{
	//	Engine_SetTimeScale(1);
	}

	if (player.obj->hp <= 0)
	{
		//Engine_SetTimeScale(0.1);
	}
}

void Player_GetView(float* r_x, float* r_y, float* r_dirX, float* r_dirY, float* r_planeX, float* r_planeY)
{
	if(r_x) *r_x = player.obj->x;
	if(r_y) *r_y = player.obj->y;
	if(r_dirX) *r_dirX = player.obj->dir_x;
	if(r_dirY) *r_dirY = player.obj->dir_y;
	if(r_planeX) *r_planeX = player.plane_x;
	if(r_planeY) *r_planeY = player.plane_y;
}

void Player_MouseCallback(float x, float y)
{
	if (player.obj->hp <= 0)
	{
		return;
	}

	static bool s_FirstMouse = true;

	static float last_x = 0.0f;
	static float last_y = 0.0f;

	if (s_FirstMouse)
	{
		x = 0;
		y = 0;
		s_FirstMouse = false;
	}

	float xOffset = x - last_x;
	float yOffset = last_y - y;

	last_x = x;
	last_y = y;

	float rotSpeed = xOffset * (player.sensitivity / MOUSE_SENS_DIVISOR);

	//rotSpeed *= Engine_GetDeltaTime();

	double oldDirX = player.obj->dir_x;
	player.obj->dir_x = player.obj->dir_x * cos(-rotSpeed) - player.obj->dir_y * sin(-rotSpeed);
	player.obj->dir_y = oldDirX * sin(-rotSpeed) + player.obj->dir_y * cos(-rotSpeed);
	double oldPlaneX = player.plane_x;
	player.plane_x = player.plane_x * cos(-rotSpeed) - player.plane_y * sin(-rotSpeed);
	player.plane_y = oldPlaneX * sin(-rotSpeed) + player.plane_y * cos(-rotSpeed);
}

void Player_Draw(Image* image, FontData* font)
{
	LightTile* light_tile = Map_GetLightTile(player.obj->x, player.obj->y);

	//draw gun
	if (player.obj->hp > 0)
	{
		player.gun_sprite.light = (float)light_tile->light / 255.0f;
		player.gun_sprite.x = 0.35;
		player.gun_sprite.y = 0.5;

		Video_DrawScreenSprite(image, &player.gun_sprite);
	}
		

	if (player.hurt_timer > 0)
	{
		//Video_Shade(image, Shader_Hurt, 0, 0, image->width, image->height);
	}
	if (player.obj->hp > 0)
	{
		//draw hud
		//hp
		Text_Draw(image, font, 0.02, 0.88, 1, 1, "HP %i", player.obj->hp);

		//ammo
		Text_Draw(image, font, 0.75, 0.88, 1, 1, "AMMO %i", player.ammo);
	}
	else
	{
		//Video_Shade(image, Shader_Hurt, 0, 0, image->width, image->height);
		//Video_Shade(image, Shader_Dead, 0, 0, image->width, image->height);
		//Text_Draw(image, font, 0.2, 0.2, 1, 1, "DEAD\n PRESS FIRE TO CONTINUE...");
	}
	
	//DrawMap(image);
}

float Player_GetSensitivity()
{
	return player.sensitivity;
}

void Player_SetSensitivity(float sens)
{
	if(sens < MIN_SENS)
	{
		sens = MIN_SENS;
	}
	else if (sens > MAX_SENS)
	{
		sens = MAX_SENS;
	}

	player.sensitivity = sens;
}
