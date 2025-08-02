#include "g_common.h"

#include <string.h>
#include <math.h>

#include "sound.h"
#include "game_info.h"
#include "main.h"
#include "u_math.h"

#define HIT_TIME 0.1
#define PLAYER_SIZE 0.4
#define PLAYER_MAX_HP 100
#define PLAYER_MAX_AMMO 100
#define PLAYER_OVERHEAL_HP_TICK_TIME 0.25
#define PLAYER_SPEED 10
#define MOUSE_SENS_DIVISOR 1000
#define MIN_SENS 0.5
#define MAX_SENS 16

typedef struct
{
	Sprite gun_sprite;
	Object* obj;
	float plane_x, plane_y;
	int move_x;
	int move_y;
	int slow_move;

	int buck_ammo;
	int bullet_ammo;
	int rocket_ammo;

	float alive_timer;
	float gun_timer;
	float hurt_timer;
	float godmode_timer;
	float quad_timer;
	float hp_tick_timer;
	float hit_timer;

	float bob;
	float gun_offset_x;
	float gun_offset_y;

	int stored_hp;

	GunType gun;
	GunInfo* gun_info;

	bool gun_check[GUN__MAX];
	Sprite gun_sprites[GUN__MAX];

	float sensitivity;
} PlayerData;

static PlayerData player;

static void Player_GiveAll()
{
	memset(player.gun_check, true, sizeof(player.gun_check));

	player.buck_ammo = PLAYER_MAX_AMMO;
	player.bullet_ammo = PLAYER_MAX_AMMO;
	player.rocket_ammo = PLAYER_MAX_AMMO;
}

static void Shader_Hurt(Image* img, int x, int y, int tx, int ty)
{
	int center_x = abs(x - img->half_width);
	int center_y = abs(y - img->half_height);

	float strenght_x = (float)center_x / (float)img->half_width;
	float strenght_y = (float)center_y / (float)img->half_height;

	float lerp = Math_lerp(0, strenght_x, strenght_y) * player.hurt_timer;

	unsigned char* sample = Image_Get(img, x, y);

	sample[0] = Math_lerp(sample[0], 255, lerp);
}
static void Shader_HurtSimple(Image* img, int x, int y, int tx, int ty)
{
	unsigned char* sample = Image_Get(img, x, y);

	int r = sample[0];

	r *= 2;
	
	if (r > 255)
	{
		r = 255;
	}

	sample[0] = r;
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

static void Shader_Godmode(Image* img, int x, int y, int tx, int ty)
{
	unsigned char* sample = Image_Get(img, x, y);

	sample[0] ^= sample[1];
}

static void Player_TraceBullet(float p_x, float p_y, float p_dirX, float p_dirY)
{
	float map_hit_x = 0;
	float map_hit_y = 0;

	int render_w;
	Render_GetRenderSize(&render_w, NULL);

	int half_w = render_w / 2;
	int center_x = (half_w) - 1;
	int shoot_delta = render_w / 8;

	Map* map = Map_GetMap();

	Object* closest = NULL;
	Object* prev_closest = NULL;

	float max_dist = FLT_MAX;

	while (true)
	{
		prev_closest = closest;

		for (int i = 0; i < map->num_sorted_objects; i++)
		{
			ObjectID id = map->sorted_list[i];
			Object* object = &map->objects[id];

			//behind the plane
			if (object->view_y <= 0)
			{
				continue;
			}

			int screen_x = (int)((half_w) * (1 + object->view_x / object->view_y));

			if (object->type == OT__MONSTER && object->hp > 0 && abs(screen_x - center_x) < shoot_delta)
			{
				float dist = (p_x - object->x) * (p_x - object->x) + (p_y - object->y) * (p_y - object->y);

				if (dist < max_dist)
				{
					max_dist = dist;
					closest = object;
				}
			}
		}

		if (closest == prev_closest)
		{
			if (Map_Raycast(p_x, p_y, p_dirX, p_dirY, &map_hit_x, &map_hit_y) != EMPTY_TILE)
			{
				Object_Spawn(OT__PARTICLE, SUB__PARTICLE_WALL_HIT, (map_hit_x), (map_hit_y));
				return;
			}
			return;
		}

		if (Object_CheckLineToTarget(player.obj, closest))
		{
			break;
		}
	}

	Object* ray_obj = closest;

	if (ray_obj && ray_obj->type == OT__MONSTER)
	{
		int dmg = player.gun_info->damage;

		//modify damage based on distance
		if (max_dist <= 5)
		{
			dmg *= 4;
		}
		else if (max_dist <= 10)
		{
			dmg *= 2;
		}

		//way too far away
		if (max_dist >= 100)
		{
			return;
		}

		//spawn blood particles
		if (rand() % 100 > 25)
		{
			Object_Spawn(OT__PARTICLE, SUB__PARTICLE_BLOOD, (ray_obj->x) + (Math_randf() * 0.5), (ray_obj->y) + (Math_randf() * 0.5));
		}

		if (ray_obj->hp > 0)
		{
			player.hit_timer = HIT_TIME;
		}

		Object_Hurt(ray_obj, player.obj, dmg);
	}
}

static void Player_SetGun(GunType gun_type)
{
	if (player.gun == gun_type)
	{
		return;
	}
	if (!player.gun_check[gun_type])
	{
		return;
	}
	if (player.gun_timer > 0)
	{
		return;
	}

	Sprite_ResetAnimState(&player.gun_sprites[player.gun]);
	Sprite_ResetAnimState(&player.gun_sprites[gun_type]);

	GunInfo* gun_info = Info_GetGunInfo(gun_type);

	player.gun = gun_type;
	player.gun_info = gun_info;
}

static void Player_ShootGun()
{
	if (player.gun_timer > 0)
	{
		return;
	}
	if (player.obj->hp <= 0)
	{
		return;
	}
	if (!player.gun_info)
	{
		return;
	}

	switch (player.gun)
	{
	case GUN__PISTOL:
	{
		//has infinite ammo
		float randomf = Math_randf();

		randomf = Math_Clamp(randomf, 0.0, 0.05);

		if (rand() % 100 > 50)
		{
			randomf = -randomf;
		}

		Player_TraceBullet(player.obj->x, player.obj->y, player.obj->dir_x + randomf, player.obj->dir_y + randomf);

		break;
	}
	case GUN__MACHINEGUN:
	{
		//no ammo
		if (player.bullet_ammo <= 0)
		{
			//Sound_Emit(SOUND__NO_AMMO);
			return;
		}

		float randomf = Math_randf();

		randomf = Math_Clamp(randomf, 0.0, 0.2);

		if (rand() % 100 > 50)
		{
			randomf = -randomf;
		}

		Player_TraceBullet(player.obj->x, player.obj->y, player.obj->dir_x + randomf, player.obj->dir_y + randomf);

		player.bullet_ammo--;

		break;
	}
	case GUN__SHOTGUN:
	{
		//no ammo
		if (player.buck_ammo <= 0)
		{
			//Sound_Emit(SOUND__NO_AMMO);
			return;
		}

		for (int i = 0; i < 8; i++)
		{
			float randomf = Math_randf();

			randomf = Math_Clamp(randomf, 0.0, 0.25);

			if (rand() % 100 > 50)
			{
				randomf = -randomf;
			}

			Player_TraceBullet(player.obj->x, player.obj->y, player.obj->dir_x + randomf, player.obj->dir_y + randomf);
		}

		player.buck_ammo--;
		break;
	}
	case GUN__DEVASTATOR:
	{
		//no ammo
		if (player.rocket_ammo <= 0)
		{
			return;
		}

		static float DEV_ANGLE = 0.1;

		for (int i = 0; i < 2; i++)
		{
			Object* missile = Object_Missile(player.obj, NULL, SUB__MISSILE_MEGASHOT);
			
			float offset = (float)i * DEV_ANGLE;

			missile->x += (offset) - DEV_ANGLE;
			missile->y += (offset) - DEV_ANGLE;
			missile->dir_x += offset;
			missile->dir_y += offset;
		}
		
		player.rocket_ammo--;
		break;
	}
	default:
		break;
	}

	Sprite* sprite = &player.gun_sprites[player.gun];

	sprite->playing = true;
	sprite->frame = 1;
	player.gun_timer = player.gun_info->cooldown;

	//PLAY SOUND
	int sound_index = -1;
	float volume = 1.0;

	switch (player.gun)
	{
	case GUN__PISTOL:
	{
		volume = 0.08;
		sound_index = SOUND__PISTOL_SHOOT;
		break;
	}
	case GUN__SHOTGUN:
	{
		volume = 0.1;
		sound_index = SOUND__SHOTGUN_SHOOT;
		break;
	}
	case GUN__MACHINEGUN:
	{
		volume = 0.1;
		sound_index = SOUND__MACHINEGUN_SHOOT;
		break;
	}
	case GUN__DEVASTATOR:
	{
		volume = 0.1;
		sound_index = SOUND__DEVASTATOR_SHOOT;
		break;
	}
	default:
		break;
	}

	if (sound_index >= 0)
	{
		Sound_Emit(sound_index, volume);
	}
}

static void Player_Die()
{

}

static void Player_UpdateTimers(float delta)
{
	if (player.obj->hp > 0) player.alive_timer += delta;
	if (player.gun_timer > 0) player.gun_timer -= delta;
	if (player.hurt_timer > 0) player.hurt_timer -= delta;
	if (player.quad_timer > 0) player.quad_timer -= delta;
	if (player.hp_tick_timer > 0) player.hp_tick_timer -= delta;
	if (player.hit_timer > 0) player.hit_timer -= delta;

	if (player.godmode_timer > 0)
	{
		player.godmode_timer -= delta;
		player.obj->flags |= OBJ_FLAG__GODMODE;
	}
	else
	{
		player.obj->flags &= ~OBJ_FLAG__GODMODE;
	}
}
static void Player_UpdateListener()
{
	ma_engine* sound_engine = Sound_GetEngine();

	ma_engine_listener_set_position(sound_engine, 0, player.obj->x, 0, player.obj->y);
	ma_engine_listener_set_direction(sound_engine, 0, -player.obj->dir_x, 0, -player.obj->dir_y);
}

static void Player_PressSwitch()
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

static void Player_ProcessInput(GLFWwindow* window)
{
	int move_x = 0;
	int move_y = 0;
	int slow_move = 0;

	//movement
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

	//gun input
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (player.obj->hp > 0)
		{
			Player_ShootGun();
		}
		else if(player.hurt_timer <= 1.0)
		{
			Game_Reset(false);
		}	
	}
	//gun stuff
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		Player_SetGun(GUN__PISTOL);
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		Player_SetGun(GUN__SHOTGUN);
	}
	else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		Player_SetGun(GUN__MACHINEGUN);
	}
	else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
	{
		Player_SetGun(GUN__DEVASTATOR);
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		Game_SetState(GS__MENU);
	}

	player.move_x = move_x;
	player.move_y = move_y;
	player.slow_move = slow_move;
}

static void Player_SetupGunSprites()
{
	GameAssets* assets = Game_GetAssets();

	for (int i = 1; i < GUN__MAX; i++)
	{
		Sprite* sprite = &player.gun_sprites[i];
		GunInfo* gun_info = Info_GetGunInfo(i);

		if (!gun_info)
		{
			continue;
		}

		switch (i)
		{
		case GUN__SHOTGUN:
		{
			sprite->img = &assets->shotgun_texture;
			sprite->anim_speed_scale = 2;
			break;
		}
		case GUN__PISTOL:
		{
			sprite->img = &assets->pistol_texture;
			sprite->anim_speed_scale = 1;
			break;
		}
		case GUN__MACHINEGUN:
		{
			sprite->img = &assets->machinegun_texture;
			sprite->anim_speed_scale = 1;
			break;
		}
		case GUN__DEVASTATOR:
		{
			sprite->img = &assets->devastator_texture;
			sprite->anim_speed_scale = 1;
			break;
		}
		default:
			break;
		}

		sprite->frame_count = sprite->img->h_frames * sprite->img->v_frames;
		sprite->scale_x = gun_info->scale;
		sprite->scale_y = gun_info->scale;
		sprite->x = gun_info->screen_x;
		sprite->y = gun_info->screen_y;
	}
}

void Player_Init(int keep)
{
	int hp = 100;
	float sens = player.sensitivity;
	bool guns_check[GUN__MAX];
	int buck_ammo = player.buck_ammo;
	int bullet_ammo = player.bullet_ammo;
	int gun = player.gun;

	if (keep)
	{
		memcpy(guns_check, player.gun_check, sizeof(guns_check));
		hp = player.stored_hp;
	}

	memset(&player, 0, sizeof(player));
	
	int spawn_x, spawn_y;
	Map_GetSpawnPoint(&spawn_x, &spawn_y);

	if (keep)
	{
		memcpy(player.gun_check, guns_check, sizeof(guns_check));
		player.bullet_ammo = bullet_ammo;
		player.buck_ammo = buck_ammo;
		Player_SetGun(gun);
	}

	player.gun_check[GUN__PISTOL] = true;

	player.obj = Map_NewObject(OT__PLAYER);

	player.obj->x = spawn_x;
	player.obj->y = spawn_y;

	player.obj->dir_x = -1;
	player.obj->dir_y = 0;
	player.plane_x = 0.0000001; //hack to prevent black floors and ceillings
	player.plane_y = 0.66;

	player.obj->hp = hp;
	player.obj->size = PLAYER_SIZE;

	player.sensitivity = sens;

	Player_SetupGunSprites();

	if(!keep)
		Player_SetGun(GUN__PISTOL);
}

Object* Player_GetObj()
{
	return player.obj;
}

void Player_Hurt(float dir_x, float dir_y)
{
	if (player.obj->hp <= 0)
	{
		player.hurt_timer = 2;
		Sound_Emit(SOUND__PLAYER_DIE, 0.35);
	}
	else
	{
		player.hurt_timer = 0.5;
		Sound_Emit(SOUND__PLAYER_PAIN, 0.35);
	}

}

void Player_HandlePickup(Object* obj)
{
	switch (obj->sub_type)
	{
	case SUB__PICKUP_SMALLHP:
	{
		Sound_Emit(SOUND__PICKUP_HP, 0.25);

		player.obj->hp += PICKUP_SMALLHP_HEAL;
		break;
	}
	case SUB__PICKUP_BIGHP:
	{
		Sound_Emit(SOUND__PICKUP_HP, 0.25);

		player.obj->hp += PICKUP_BIGHP_HEAL;
		break;
	}
	case SUB__PICKUP_AMMO:
	{
		Sound_Emit(SOUND__PICKUP_AMMO, 0.35);

		player.buck_ammo += PICKUP_AMMO_GIVE / 2;
		player.bullet_ammo += PICKUP_AMMO_GIVE;
		break;
	}
	case SUB__PICKUP_ROCKETS:
	{
		Sound_Emit(SOUND__PICKUP_AMMO, 0.35);

		player.rocket_ammo += PICKUP_ROCKETS_GIVE;
		break;
	}
	case SUB__PICKUP_SHOTGUN:
	{
		Sound_Emit(SOUND__PICKUP_AMMO, 0.35);

		bool just_picked = false;

		if (!player.gun_check[GUN__SHOTGUN])
		{
			just_picked = true;
		}
		
		player.gun_check[GUN__SHOTGUN] = true;
		player.buck_ammo += PICKUP_SHOTGUN_AMMO_GIVE;

		if (just_picked)
		{
			player.gun_timer = 0;
			Player_SetGun(GUN__SHOTGUN);
		}

		break;
	}
	case SUB__PICKUP_MACHINEGUN:
	{
		Sound_Emit(SOUND__PICKUP_AMMO, 0.35);

		bool just_picked = false;

		if (!player.gun_check[GUN__MACHINEGUN])
		{
			just_picked = true;
		}

		player.gun_check[GUN__MACHINEGUN] = true;
		player.bullet_ammo += PICKUP_MACHINEGUN_AMMO_GIVE;

		if (just_picked)
		{
			player.gun_timer = 0;
			Player_SetGun(GUN__MACHINEGUN);
		}

		break;
	}
	case SUB__PICKUP_DEVASTATOR:
	{
		Sound_Emit(SOUND__PICKUP_AMMO, 0.35);

		bool just_picked = false;

		if (!player.gun_check[GUN__DEVASTATOR])
		{
			just_picked = true;
		}

		player.gun_check[GUN__DEVASTATOR] = true;
		player.rocket_ammo += PICKUP_DEVASTATOR_AMMO_GIVE;

		if (just_picked)
		{
			player.gun_timer = 0;
			Player_SetGun(GUN__DEVASTATOR);
		}

		break;
	}
	case SUB__PICKUP_INVUNERABILITY:
	{
		Sound_Emit(SOUND__PICKUP_SPECIAL, 0.5);

		player.godmode_timer = PICKUP_INVUNERABILITY_TIME;
		break;
	}
	case SUB__PICKUP_QUAD_DAMAGE:
	{
		Sound_Emit(SOUND__PICKUP_SPECIAL, 0.5);

		player.quad_timer = PICKUP_QUAD_TIME;
		break;
	}
	default:
	{
		break;
	}
	}

	//clamp
	if (player.bullet_ammo >= PLAYER_MAX_AMMO) player.bullet_ammo = PLAYER_MAX_AMMO;
	if (player.buck_ammo >= PLAYER_MAX_AMMO) player.buck_ammo = PLAYER_MAX_AMMO;
	if (player.rocket_ammo >= PLAYER_MAX_AMMO) player.rocket_ammo = PLAYER_MAX_AMMO;

	//delete the object
	Map_DeleteObject(obj);
}

void Player_Update(GLFWwindow* window, float delta)
{
	Player_ProcessInput(window);
	Player_UpdateTimers(delta);
	Player_UpdateListener();
	Sprite_UpdateAnimation(&player.gun_sprites[player.gun], delta);

	//tick down if overhealed
	if (player.obj->hp > PLAYER_MAX_HP)
	{
		if (player.hp_tick_timer <= 0)
		{
			player.obj->hp--;
			player.hp_tick_timer = PLAYER_OVERHEAL_HP_TICK_TIME;
		}
	}
	
	//move player
	float speed = PLAYER_SPEED * delta;
	if (player.slow_move == 1) speed *= 0.5;

	float dir_x = (player.move_x * player.obj->dir_x) + (player.move_y * player.plane_x);
	float dir_y = (player.move_x * player.obj->dir_y) + (player.move_y * player.plane_y);

	if (player.obj->hp > 0)
	{
		Move_Object(player.obj, dir_x * speed, dir_y * speed);
	}

	if (player.move_x != 0 || player.move_y != 0)
	{
		float bob_amp = 0.001;
		float bob_freq = 16;

		if (player.slow_move == 1) bob_freq *= 0.5;
			
		player.bob += delta;

		player.gun_offset_x = cosf(player.bob * bob_freq / bob_freq) * bob_amp;
		player.gun_offset_y = sinf(player.bob * bob_freq) * bob_amp;
	}

	//make sure to reset the frame
	if (!player.gun_sprites[player.gun].playing)
	{
		player.gun_sprites[player.gun].frame = 0;
	}

	//hacky way to store hp, since we reset the map objects on map change 
	player.stored_hp = player.obj->hp;
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
	if (player.obj->hp > 0 && light_tile)
	{
		int l = (int)light_tile->light + (int)light_tile->temp_light;

		if (l > 255) l = 255;

		Sprite* sprite = &player.gun_sprites[player.gun];

		float old_x = sprite->x;
		float old_y = sprite->y;

		sprite->light = (float)l / 255.0f;

		sprite->x += player.gun_offset_x;
		sprite->y += player.gun_offset_y;

		Video_DrawScreenSprite(image, sprite);

		sprite->x = old_x;
		sprite->y = old_y;
	}
		
	//emit hurt shader
	if (player.hurt_timer > 0)
	{
		Render_QueueFullscreenShader(Shader_HurtSimple);
	}
	else if (player.godmode_timer > 0)
	{
		Render_QueueFullscreenShader(Shader_Godmode);
	}
	//draw hud
	if (player.obj->hp > 0)
	{
		//pseudo crosshair
		if (player.hit_timer > 0)
		{
			Text_DrawColor(image, font, 0.5, 0.49, 0.3, 0.3, 255, 0, 0, 255, "+");
		}
		else
		{
			Text_Draw(image, font, 0.5, 0.49, 0.3, 0.3, "+");
		}

		//hp
		Text_Draw(image, font, 0.02, 0.95, 1, 1, "HP %i", player.obj->hp);

		//ammo
		switch (player.gun)
		{
		case GUN__PISTOL:
		{
			Text_Draw(image, font, 0.75, 0.95, 1, 1, "AMMO INF");
			break;
		}
		case GUN__MACHINEGUN:
		{
			Text_Draw(image, font, 0.75, 0.95, 1, 1, "AMMO %i", player.bullet_ammo);
			break;
		}
		case GUN__SHOTGUN:
		{
			Text_Draw(image, font, 0.75, 0.95, 1, 1, "AMMO %i", player.buck_ammo);
			break;
		}
		case GUN__DEVASTATOR:
		{
			Text_Draw(image, font, 0.75, 0.95, 1, 1, "AMMO %i", player.rocket_ammo);
			break;
		}
		default:
			break;
		}

	}
	else
	{
		//draw crazy death stuff
		Video_Shade(image, Shader_Hurt, 0, 0, image->width, image->height);
		Video_Shade(image, Shader_Dead, 0, 0, image->width, image->height);
		Text_Draw(image, font, 0.45, 0.2, 1, 1, "DEAD");
		Text_Draw(image, font, 0.2, 0.5, 1, 1, "PRESS FIRE TO CONTINUE...");
	}
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
