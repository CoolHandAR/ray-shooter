#include "g_common.h"

#include <string.h>
#include <math.h>
#include <cglm/cglm.h>

#define PLAYER_SPEED 10

typedef struct
{
	Sprite gun_sprite;
	Object* obj;
	float dir_x, dir_y;
	float plane_x, plane_y;
	int move_x;
	int move_y;
	int slow_move;
} PlayerData;

static PlayerData player;

static void Player_ShootGun()
{
	int hit = Map_Raycast(player.obj->x, player.obj->y, player.dir_x, player.dir_y);


	if (hit >= 0)
	{
		printf("hit monster %i \n", hit);
	}
	else
	{
		printf("hit wall %i \n", -(hit - 1));
	}

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

	player.obj = Map_NewObject();

	player.obj->type = OT__PLAYER;	

	player.obj->x = spawn_x;
	player.obj->y = spawn_y;
	player.plane_x = 0;
	player.plane_y = 0.66;
	player.dir_x = -1;
	player.dir_y = 0;

	GameAssets* assets = Game_GetAssets();

	player.gun_sprite.img = &assets->weapon_textures;
	player.gun_sprite.scale_x = 3;
	player.gun_sprite.scale_y = 3;
	player.gun_sprite.h_frames = 10;
	player.gun_sprite.v_frames = 1;
	player.gun_sprite.anim_speed_scale = 0.7;
	player.gun_sprite.frame = 0;
	player.gun_sprite.flip_h = false;
	player.gun_sprite.flip_v = false;
}

Object* Player_GetObj()
{
	return player.obj;
}

void Player_Update(GLFWwindow* window, float delta)
{
	Player_ProcessInput(window);

	float speed = PLAYER_SPEED * delta;

	if (player.slow_move == 1) speed *= 0.5;

	Move_Object(player.obj, (player.move_x * player.dir_x) * speed, (player.move_x * player.dir_y) * speed);
	Move_Object(player.obj, (player.move_y * player.plane_x) * speed, (player.move_y * player.plane_y) * speed);

	Sprite_UpdateAnimation(&player.gun_sprite);

	player.gun_sprite.looping = true;
}

void Player_GetView(float* r_x, float* r_y, float* r_dirX, float* r_dirY, float* r_planeX, float* r_planeY)
{
	if(r_x) *r_x = player.obj->x - 0.5;
	if(r_y) *r_y = player.obj->y - 0.5;
	if(r_dirX) *r_dirX = player.dir_x;
	if(r_dirY) *r_dirY = player.dir_y;
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

	double oldDirX = player.dir_x;
	player.dir_x = player.dir_x * cos(-rotSpeed) - player.dir_y * sin(-rotSpeed);
	player.dir_y = oldDirX * sin(-rotSpeed) + player.dir_y * cos(-rotSpeed);
	double oldPlaneX = player.plane_x;
	player.plane_x = player.plane_x * cos(-rotSpeed) - player.plane_y * sin(-rotSpeed);
	player.plane_y = oldPlaneX * sin(-rotSpeed) + player.plane_y * cos(-rotSpeed);
}

void Player_Draw(Image* image)
{
	//draw gun
	Video_DrawScreenSprite(image, &player.gun_sprite, 280, 500);
}
