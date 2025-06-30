#include "g_common.h"

#include <string.h>
#include <math.h>
#include <cglm/cglm.h>

#include "sound.h"
#include "game_info.h"
#include "main.h"
#include "u_math.h"

#define PLAYER_SPEED 10

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
	int gun_sound_id;

	int ammo;

	GunData guns[GUN__MAX];
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

	float spread = 3;
	for (int i = 0; i < 8; i++)
	{
		float randomf = Math_randf();

		randomf = Math_Clamp(randomf, 0.0, 0.25);

		if (rand() % 100 > 50)
		{
			randomf = -randomf;
		}

		
		Gun_Shoot(player.obj, player.obj->x - 0.5, player.obj->y - 0.5, player.obj->dir_x + randomf, player.obj->dir_y + randomf);
	}

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
		Player_ShootGun();
	}

	player.move_x = move_x;
	player.move_y = move_y;
	player.slow_move = slow_move;
}

void Player_Init()
{
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
	player.gun_sprite.scale_x = 3;
	player.gun_sprite.scale_y = 3;
	player.gun_sprite.anim_speed_scale = 0.7;
	player.gun_sprite.frame = 0;
	player.gun_sprite.flip_h = false;
	player.gun_sprite.flip_v = false;
	player.gun_sprite.looping = false;
	player.gun_sprite.transparency = 0.0;
	player.gun_sprite.frame_count = 10;
	player.obj->hp = 5;

	Sprite_GenerateAlphaSpans(&player.gun_sprite);

	player.gun_sound_id = Sound_Preload(SOUND__SHOTGUN_SHOOT);

	Sound_setMasterVolume(0.2);

	player.ammo = 14;
}

Object* Player_GetObj()
{
	return player.obj;
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

	Move_Object(player.obj, dir_x * speed, dir_y * speed);

	Sprite_UpdateAnimation(&player.gun_sprite, delta);

	if (!player.gun_sprite.playing)
	{
		player.gun_sprite.frame = 0;
	}


	player.gun_timer -= delta;

	ma_engine* sound_engine = Sound_GetEngine();

	ma_engine_listener_set_position(sound_engine, 0, player.obj->x, 0, player.obj->y);
	ma_engine_listener_set_direction(sound_engine, 0, -player.obj->dir_x, 0, -player.obj->dir_y);
	Sound_Set(player.gun_sound_id, player.obj->x, player.obj->y, player.obj->dir_x, player.obj->dir_y);
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

	float rotSpeed = xOffset * (1.0 / 1000);

	double oldDirX = player.obj->dir_x;
	player.obj->dir_x = player.obj->dir_x * cos(-rotSpeed) - player.obj->dir_y * sin(-rotSpeed);
	player.obj->dir_y = oldDirX * sin(-rotSpeed) + player.obj->dir_y * cos(-rotSpeed);
	double oldPlaneX = player.plane_x;
	player.plane_x = player.plane_x * cos(-rotSpeed) - player.plane_y * sin(-rotSpeed);
	player.plane_y = oldPlaneX * sin(-rotSpeed) + player.plane_y * cos(-rotSpeed);
}

void Player_Draw(Image* image)
{
	//draw gun
	Video_DrawScreenSprite(image, &player.gun_sprite, 280, 500);

	//draw crosshair
	int x0 = (image->width / 2) - 5;
	int y0 = (image->height / 2) -5 ;
	int x1 = x0 + 15;
	int y1 = y0 + 15;

	unsigned char color[4] = { 255, 255, 255, 255 };
	Video_DrawLine(image, x0, y0, x1, y1, color);
}
