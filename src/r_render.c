#include "r_common.h"

#include <string.h>
#include <stdlib.h>
#include <glad/glad.h>
#include "g_common.h"

typedef struct
{
	Image framebuffer;
	Image clear_framebuffer;

	Image wall_buffer;

	float* depth_buffer;
	float* wall_depth_buffer;

	int w, h;

	bool redraw_walls;
	bool redraw_sprites;

	float view_x, view_y;
	float dir_x, dir_y;
	float plane_x, plane_y;
} RenderCore;

static RenderCore s_renderCore;

static bool Render_CheckForRedraw(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
	//already requested
	if (s_renderCore.redraw_walls)
	{
		return true;
	}

	if (x != s_renderCore.view_x)
	{
		return true;
	}
	if (y != s_renderCore.view_y)
	{
		return true;
	}
	if (dir_x != s_renderCore.dir_x)
	{
		return true;
	}
	if (dir_y != s_renderCore.dir_y)
	{
		return true;
	}
	if (plane_x != s_renderCore.plane_x)
	{
		return true;
	}
	if (plane_y != s_renderCore.plane_y)
	{
		return true;
	}
	return false;
}

void Render_Init(int width, int height)
{
	memset(&s_renderCore, 0, sizeof(RenderCore));

	Image_Create(&s_renderCore.framebuffer, width, height, 4);

	Image_Create(&s_renderCore.clear_framebuffer, width, height, 4);

	Image_Create(&s_renderCore.wall_buffer, width, height, 4);

	Render_ResizeWindow(width, height);
}

void Render_ShutDown()
{
	Image_Destruct(&s_renderCore.clear_framebuffer);
	Image_Destruct(&s_renderCore.framebuffer);
	Image_Destruct(&s_renderCore.wall_buffer);

	free(s_renderCore.depth_buffer);
	free(s_renderCore.wall_depth_buffer);
}

void Render_RedrawWalls()
{
	s_renderCore.redraw_walls = true;
}

void Render_RedrawSprites()
{
	s_renderCore.redraw_sprites = true;
}

void Render_ResizeWindow(int width, int height)
{	
	Image_Resize(&s_renderCore.framebuffer, width, height);
	Image_Resize(&s_renderCore.clear_framebuffer, width, height);
	Image_Resize(&s_renderCore.wall_buffer, width, height);

	if (s_renderCore.depth_buffer)
	{
		free(s_renderCore.depth_buffer);
	}

	s_renderCore.depth_buffer = malloc(sizeof(float) * width * height);

	if (!s_renderCore.depth_buffer)
	{
		return;
	}

	memset(s_renderCore.depth_buffer, 1e3, sizeof(float) * width * height);

	if (s_renderCore.wall_depth_buffer)
	{
		free(s_renderCore.wall_depth_buffer);
	}

	s_renderCore.wall_depth_buffer = malloc(sizeof(float) * width * height);

	if (!s_renderCore.wall_depth_buffer)
	{
		return;
	}

	memset(s_renderCore.wall_depth_buffer, 1e3, sizeof(float) * width * height);

	//split video
	unsigned char floor_color[4] = { 128, 128, 128, 255 };
	unsigned char ceilling_color[4] = { 128, 128, 159, 255 };
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			if (y < height / 2)
			{
				Image_Set2(&s_renderCore.clear_framebuffer, x, y, ceilling_color);
			}
			else
			{
				Image_Set2(&s_renderCore.clear_framebuffer, x, y, floor_color);
			}
		}
	}

	s_renderCore.w = width;
	s_renderCore.h = height;

	s_renderCore.redraw_walls = true;
	s_renderCore.redraw_sprites = true;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
	GameAssets* assets = Game_GetAssets();

	//check if we need to redraw walls
	s_renderCore.redraw_walls = Render_CheckForRedraw(x, y, dir_x, dir_y, plane_x, plane_y);

	//raycast and render all wall tiles
	if (s_renderCore.redraw_walls)
	{
		Image_Copy(&s_renderCore.wall_buffer, &s_renderCore.clear_framebuffer);

		//clear wall depth buffer
		memset(s_renderCore.wall_depth_buffer, (int)DEPTH_CLEAR, sizeof(float) * s_renderCore.w * s_renderCore.h);

		Video_RaycastMap(&s_renderCore.wall_buffer, &assets->wall_textures, s_renderCore.wall_depth_buffer, x, y, dir_x, dir_y, plane_x, plane_y);
	}
	
	//draw map objects and player sprites
	if (s_renderCore.redraw_sprites || s_renderCore.redraw_walls)
	{
		Image_Copy(&s_renderCore.framebuffer, &s_renderCore.wall_buffer);

		memcpy(s_renderCore.depth_buffer, s_renderCore.wall_depth_buffer, sizeof(float) * s_renderCore.w * s_renderCore.h);

		//sort and draw map objects
		Map_DrawObjects(&s_renderCore.framebuffer, s_renderCore.depth_buffer, x, y, dir_x, dir_y, plane_x, plane_y);

		//draw player stuff (gun and hud)
		Player_Draw(&s_renderCore.framebuffer);
	}

	//upload video bytes
	if (s_renderCore.redraw_sprites || s_renderCore.redraw_walls)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, s_renderCore.w, s_renderCore.h, GL_RGBA, GL_UNSIGNED_BYTE, s_renderCore.framebuffer.data);
	}

	//render fullscreen quad
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	

	//store view information
	s_renderCore.view_x = x;
	s_renderCore.view_y = y;
	s_renderCore.dir_x = dir_x;
	s_renderCore.dir_y = dir_y;
	s_renderCore.plane_x = plane_x;
	s_renderCore.plane_y = plane_y;

	s_renderCore.redraw_walls = false;
	s_renderCore.redraw_sprites = false;
}