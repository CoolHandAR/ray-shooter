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

	DrawSpan* draw_spans;

	float* depth_buffer;
	float* wall_depth_buffer;

	int w, h;
	int win_w, win_h;

	bool redraw_walls;
	bool redraw_sprites;

	float view_x, view_y;
	float dir_x, dir_y;
	float plane_x, plane_y;

	int scale;

	FontData font_data;
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

void Render_WindowCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	//Render_ResizeWindow(width, height);

	s_renderCore.win_w = width;
	s_renderCore.win_h = height;
}

void Render_Init(int width, int height)
{
	memset(&s_renderCore, 0, sizeof(RenderCore));

	Image_Create(&s_renderCore.framebuffer, width, height, 4);

	Image_Create(&s_renderCore.clear_framebuffer, width, height, 4);

	Image_Create(&s_renderCore.wall_buffer, width, height, 4);

	Render_ResizeWindow(width, height);

	Text_LoadFont("assets/myFont.json", "assets/myFont.png", &s_renderCore.font_data);
}

void Render_ShutDown()
{
	Image_Destruct(&s_renderCore.clear_framebuffer);
	Image_Destruct(&s_renderCore.framebuffer);
	Image_Destruct(&s_renderCore.wall_buffer);
	Image_Destruct(&s_renderCore.font_data.font_image);

	free(s_renderCore.depth_buffer);
	free(s_renderCore.wall_depth_buffer);
	free(s_renderCore.draw_spans);
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

	if (s_renderCore.draw_spans)
	{
		free(s_renderCore.draw_spans);
	}

	s_renderCore.draw_spans = malloc(sizeof(DrawSpan) * width);

	if (!s_renderCore.draw_spans)
	{
		return;
	}

	memset(s_renderCore.draw_spans, 0, sizeof(DrawSpan) * width);

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

	s_renderCore.win_w = width;
	s_renderCore.win_h = height;

	s_renderCore.redraw_walls = true;
	s_renderCore.redraw_sprites = true;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

}

void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
	GameAssets* assets = Game_GetAssets();
	GameState game_state = Game_GetState();

	//check if we need to redraw walls
	s_renderCore.redraw_walls = Render_CheckForRedraw(x, y, dir_x, dir_y, plane_x, plane_y);

//	s_renderCore.redraw_walls = true;
//	s_renderCore.redraw_sprites = true;

	//raycast and render all wall tiles
	if (s_renderCore.redraw_walls && game_state == GS__LEVEL)
	{
		Image_Copy(&s_renderCore.wall_buffer, &s_renderCore.clear_framebuffer);

		//clear wall depth buffer
		memset(s_renderCore.wall_depth_buffer, (int)DEPTH_CLEAR, sizeof(float) * s_renderCore.w * s_renderCore.h);

		Video_RaycastMap(&s_renderCore.wall_buffer, &assets->wall_textures, NULL, NULL, s_renderCore.wall_depth_buffer, s_renderCore.draw_spans, x, y, dir_x, dir_y, plane_x, plane_y);
	}
	
	//draw map objects and player sprites
	if (s_renderCore.redraw_sprites || s_renderCore.redraw_walls)
	{
		Image_Copy(&s_renderCore.framebuffer, &s_renderCore.wall_buffer);

		memcpy(s_renderCore.depth_buffer, s_renderCore.wall_depth_buffer, sizeof(float) * s_renderCore.w * s_renderCore.h);

		Game_Draw(&s_renderCore.framebuffer, &s_renderCore.font_data, s_renderCore.depth_buffer, s_renderCore.draw_spans, x, y, dir_x, dir_y, plane_x, plane_y);

		//Video_DrawScreenTexture(&s_renderCore.framebuffer, assets->wall_textures.mipmaps[0], 100, 100, 1, 1);
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

void Render_GetWindowSize(int* r_width, int* r_height)
{
	if (r_width) *r_width = s_renderCore.win_w;
	if (r_height) *r_height = s_renderCore.win_h;
}

void Render_GetRenderSize(int* r_width, int* r_height)
{
	if (r_width) *r_width = s_renderCore.w;
	if (r_height) *r_height = s_renderCore.h;
}

int Render_GetRenderScale()
{
	return s_renderCore.scale;
}

void Render_SetRenderScale(int scale)
{
	if (scale < 1)
	{
		scale = 1;
	}
	else if (scale > 3)
	{
		scale = 3;
	}
	
	s_renderCore.scale = scale;

	Render_ResizeWindow(BASE_RENDER_WIDTH * scale, BASE_RENDER_HEIGHT * scale);
}

float Render_GetWindowAspect()
{
	if (s_renderCore.win_h <= 0)
	{
		return 1;
	}

	return (float)s_renderCore.win_w / (float)s_renderCore.win_h;
}
