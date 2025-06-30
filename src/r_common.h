#pragma once

#include <stdbool.h>
#include <string.h>

#define MAX_IMAGE_MIPMAPS 8
#define DEPTH_CLEAR 9999999

typedef struct
{
	int numChannels;
	int width;
	int height;
	unsigned char* data;

	int h_frames;
	int v_frames;
} Image;

bool Image_Create(Image* img, int p_width, int p_height, int p_numChannels);
bool Image_CreateFromPath(Image* img, const char* path);
void Image_Resize(Image* img, int p_width, int p_height);
void Image_Destruct(Image* img);
void Image_Copy(Image* dest, Image* src);
void Image_Set(Image* img, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
inline void Image_Set2(Image* img, int x, int y, unsigned char* color)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return;
	}

	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	memcpy(d, color, img->numChannels);
}
inline void Image_SetFast(Image* img, int x, int y, unsigned char* color)
{
	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	memcpy(d, color, img->numChannels);
}

inline unsigned char* Image_Get(Image* img, int x, int y)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return 0;
	}

	return img->data + (x + y * img->width) * img->numChannels;
}

inline unsigned char* Image_GetFast(Image* img, int x, int y)
{
	return img->data + (x + y * img->width) * img->numChannels;
}

typedef struct
{
	int min, max;
} AlphaSpan;

typedef struct
{
	int min_real_x;
	int max_real_x;
	AlphaSpan* alpha_spans;
} FrameInfo;

typedef struct
{
	float x, y;
	float scale_x, scale_y;
	float dist;
	bool flip_h, flip_v;

	bool skip_draw;

	//modifiers
	float transparency;

	//animation stuff
	float _anim_frame_progress;
	float anim_speed_scale;
	int frame_count;
	int frame;
	bool looping;
	bool playing;

	int loops;
	int action_loop;

	int frame_offset_x;
	int frame_offset_y;

	FrameInfo* frame_info;

	//image data
	Image* img;
} Sprite;

void Sprite_UpdateAnimation(Sprite* sprite, float delta);
void Sprite_GenerateAlphaSpans(Sprite* sprite);
FrameInfo* Sprite_GetFrameInfo(Sprite* sprite, int frame);
AlphaSpan* Sprite_GetFrameAlphaSpan(Sprite* sprite, int x, int frame);

void Video_DrawLine(Image* image, int x0, int y0, int x1, int y1, unsigned char* color);
void Video_DrawRectangle(Image* image, int p_x, int p_y, int p_w, int p_h, unsigned char* p_color);
void Video_Clear(Image* image, unsigned char c);
void Video_RaycastMap(Image* image, Image* texture, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
bool Video_DrawCollumn(Image* image, Image* texture, int x, float size, float view_x, float view_y, float ray_dir_x, float ray_dir_y, float* depth_buffer, float wall_dist, int side, int tile);
void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Video_DrawScreenTexture(Image* image, Image* texture, float p_x, float p_y, float p_scaleX, float p_scaleY);
void Video_DrawScreenSprite(Image* image, Sprite* sprite, float p_x, float p_y);


typedef void (*ShaderFun)(Image* image, int x, int y, int tx, int ty);
void Video_Shade(Image* image, ShaderFun shader_fun, int x0, int y0, int x1, int y1);

void Render_Init(int width, int height);
void Render_ShutDown();

void Render_RedrawWalls();
void Render_RedrawSprites();
void Render_ResizeWindow(int width, int height);
void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y);