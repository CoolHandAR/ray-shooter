#pragma once

#include <stdbool.h>

#define MAX_IMAGE_MIPMAPS 8

typedef struct
{
	int numChannels;
	int width;
	int height;
	unsigned char* data;
} Image;

bool Image_Create(Image* img, int p_width, int p_height, int p_numChannels);
bool Image_CreateFromPath(Image* img, const char* path);
void Image_Resize(Image* img, int p_width, int p_height);
void Image_Destruct(Image* img);
void Image_Set(Image* img, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void Image_Set2(Image* img, int x, int y, unsigned char* color);
void Image_Copy(Image* dest, Image* src);
unsigned char* Image_Get(Image* img, int x, int y);

typedef struct
{
	float x, y;
	float scale_x, scale_y;
	float dist;
	bool flip_h, flip_v;

	//animation stuff
	float _anim_frame_progress;
	float anim_speed_scale;
	int frame_count;
	int frame;
	bool looping;

	int h_frames;
	int v_frames;

	int frame_offset_x;
	int frame_offset_y;

	Image* img;
} Sprite;

void Sprite_UpdateAnimation(Sprite* sprite);

void Video_DrawLine(Image* image, int x0, int y0, int x1, int y1, unsigned char* color);
void Video_DrawRectangle(Image* image, int p_x, int p_y, int p_w, int p_h, unsigned char* p_color);
void Video_Clear(Image* image, unsigned char c);
void Video_RaycastMap(Image* image, Image* texture, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Video_DrawScreenTexture(Image* image, Image* texture, float p_x, float p_y, float p_scaleX, float p_scaleY);
void Video_DrawScreenSprite(Image* image, Sprite* sprite, float p_x, float p_y);