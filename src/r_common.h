#pragma once

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_IMAGE_MIPMAPS 8
#define DEPTH_CLEAR 9999999

#define BASE_RENDER_WIDTH 640
#define BASE_RENDER_HEIGHT 360

typedef struct
{
	uint8_t light;
	uint8_t hit_tile;
	int pointer_index;
	int width;
	int tex_x;
	float wall_dist;
	float floor_x;
	float floor_y;
} DrawSpan;

typedef struct
{
	int min, max;
} AlphaSpan;

typedef struct
{
	int min_real_x;
	int max_real_x;
	int width;
	AlphaSpan* alpha_spans;
} FrameInfo;

typedef struct
{
	int half_width;
	int half_height;

	int numChannels;
	int width;
	int height;
	unsigned char* data;

	int h_frames;
	int v_frames;

	FrameInfo* frame_info;

	//mipmap stuff
	int num_mipmaps;
	struct Image* mipmaps[MAX_IMAGE_MIPMAPS];
	float x_scale;
	float y_scale;
} Image;

bool Image_Create(Image* img, int p_width, int p_height, int p_numChannels);
bool Image_CreateFromPath(Image* img, const char* path);
bool Image_SaveToPath(Image* img, const char* filename);
void Image_Resize(Image* img, int p_width, int p_height);
void Image_Destruct(Image* img);
void Image_Copy(Image* dest, Image* src);
void Image_Blit(Image* dest, Image* src, int dstX0, int dstY0, int dstX1, int dstY1, int srcX0, int srcY0, int srcX1, int srcY1);
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
inline void Image_Set3(Image* img, int x, int y, unsigned char* color, int numChannels)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return;
	}

	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	memcpy(d, color, numChannels);
}
inline void Image_SetFast(Image* img, int x, int y, unsigned char* color)
{
	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	memcpy(d, color, img->numChannels);
}
inline void Image_SetScaled(Image* img, int x, int y, float scale, unsigned char* color)
{
	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	//no scale, so set directly
	if (scale == 1)
	{
		memcpy(d, color, img->numChannels);
	}
	//completely black
	else if (scale <= 0)
	{
		unsigned char clr[4] = { 0, 0, 0, 255 };

		memcpy(d, clr, img->numChannels);
	}
	//scale the color
	else
	{
		unsigned char clr[4];

		for (int i = 0; i < img->numChannels; i++)
		{
			//skip alpha component
			if (i == 3)
			{
				clr[i] == 255;
				continue;
			}

			int c = (float)color[i] * scale;

			if (c > 255)
			{
				c = 255;
			}

			clr[i] = c;
		}

		memcpy(d, clr, img->numChannels);
	}
	
}



inline unsigned char* Image_Get(Image* img, int x, int y)
{
	if (x < 0)
	{
		x = 0;
	}
	else if (x >= img->width)
	{
		x = img->width - 1;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y >= img->height)
	{
		y = img->height - 1;
	}

	return img->data + (x + y * img->width) * img->numChannels;
}

inline void Image_GetBlured(Image* img, int x, int y, int size, float scale, unsigned char* r_color)
{
	if (x < 0)
	{
		x = 0;
	}
	else if (x >= img->width)
	{
		x = img->width - 1;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y >= img->height)
	{
		y = img->height - 1;
	}

	int dir_x = 1;
	int dir_y = 0;

	int half_size = size / 2;

	for (int i = -half_size; i < half_size; i++)
	{
		if (i == 0)
		{
			continue;
		}

		float radius = (float)i * scale;

		int tx = x + dir_x * radius;
		int ty = y + dir_y * radius;

		unsigned char* sample = Image_Get(img, tx, ty);

		for (int k = 0; k < img->numChannels; k++)
		{
			r_color[k] += sample[k];
		}
	}
	for (int k = 0; k < img->numChannels; k++)
	{
		r_color[k] /= size;
	}
}


inline unsigned char* Image_GetMipmapped(Image* img, int x, int y, float dist)
{
	dist = fabs(dist);

	int mipmap_index = (int)dist;

	if (dist >= img->num_mipmaps - 1)
	{
		mipmap_index = img->num_mipmaps - 1;
	}

	if (mipmap_index == 0)
	{
		return Image_Get(img, x, y);
	}

	Image* mip_map = img->mipmaps[mipmap_index];

	x *= mip_map->x_scale;
	y *= mip_map->y_scale;

	if (x < 0)
	{
		x = 0;
	}
	else if (x >= mip_map->width)
	{
		x = mip_map->width - 1;
	}
	if (y < 0)
	{
		y = 0;
	}
	else if (y >= mip_map->height)
	{
		y = mip_map->height - 1;
	}

	return mip_map->data + (x + y * mip_map->width) * mip_map->numChannels;
}

void Image_Blur(Image* img, int size, float scale);
void Image_GenerateMipmaps(Image* img);
void Image_GenerateFrameInfo(Image* img);


FrameInfo* Image_GetFrameInfo(Image* img, int frame);
AlphaSpan* FrameInfo_GetAlphaSpan(FrameInfo* frame_info, int x);


typedef struct
{
	float x, y;
	float offset_x, offset_y;
	float scale_x, scale_y;
	float dist;
	float v_offset;
	bool flip_h, flip_v;

	bool skip_draw;

	//modifiers
	float transparency;
	float light;

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

	//image data
	Image* img;
} Sprite;


void Sprite_UpdateAnimation(Sprite* sprite, float delta);

void Video_Setup();
void Video_DrawLine(Image* image, int x0, int y0, int x1, int y1, unsigned char* color);
void Video_DrawRectangle(Image* image, int p_x, int p_y, int p_w, int p_h, unsigned char* p_color);
void Video_Clear(Image* image, unsigned char c);
void Video_RaycastFloorCeilling(Image* image, Image* texture, float* depth_buffer, int x, int spans, int draw_start, int draw_end, float floor_x, float floor_y, float wall_dist, float p_x, float p_y, int doors_drawn, bool is_floor);
void Video_RaycastMap(Image* image, Image* texture, float* depth_buffer, DrawSpan* draw_spans, int x_start, int x_end, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
bool Video_DrawCollumn(Image* image, Image* texture, int x, float size, float* depth_buffer, int tex_x, float wall_dist, unsigned char light, int tile, int spans, int doors_drawn, int* r_draw_start, int* r_draw_end);
void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY);
void Video_DrawScreenTexture(Image* image, Image* texture, float p_x, float p_y, float p_scaleX, float p_scaleY);
void Video_DrawScreenSprite(Image* image, Sprite* sprite);

typedef void (*ShaderFun)(Image* image, int x, int y, int tx, int ty);
void Video_Shade(Image* image, ShaderFun shader_fun, int x0, int y0, int x1, int y1);

void Render_Init(int width, int height);
void Render_ShutDown();
void Render_Loop();
void Render_LockObjectMutex();
void Render_UnlockObjectMutex();
void Render_FinishAndStall();
void Render_Resume();
void Render_AddSpriteToQueue(Sprite* sprite);
void Render_AddScreenSpriteToQueue(Sprite* sprite);

void Render_RedrawWalls();
void Render_RedrawSprites();
void Render_ResizeWindow(int width, int height);
void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y);
void Render_GetWindowSize(int* r_width, int* r_height);
void Render_GetRenderSize(int* r_width, int* r_height);
int Render_GetRenderScale();
void Render_SetRenderScale(int scale);
float Render_GetWindowAspect();

#define MAX_FONT_GLYPHS 100

typedef struct
{
	int distance_range;
	float size;
	int width, height;
} FontAtlasData;
typedef struct
{
	int em_size;
	double line_height;
	double ascender;
	double descender;
	double underline_y;
	double underline_thickness;
}  FontMetricData;

typedef struct
{
	double left;
	double bottom;
	double right;
	double top;
} FontBounds;

typedef struct
{
	unsigned unicode;
	double advance;
	FontBounds plane_bounds;
	FontBounds atlas_bounds;
} FontGlyphData;

typedef struct
{
	Image font_image;
	FontAtlasData atlas_data;
	FontMetricData metrics_data;
	FontGlyphData glyphs_data[MAX_FONT_GLYPHS];
} FontData;


bool Text_LoadFont(const char* filename, const char* image_path, FontData* font_data);
FontGlyphData* FontData_GetGlyphData(const FontData* font_data, char ch);
void Text_DrawStr(Image* image, const FontData* font_data, float _x, float _y, float scale_x, float scale_y, int r, int g, int b, int a, const char* str);
void Text_Draw(Image* image, const FontData* font_data, float _x, float _y, float scale_x, float scale_y, const char* fmt, ...);
void Text_DrawColor(Image* image, const FontData* font_data, float _x, float _y, float scale_x, float scale_y, int r, int g, int b, int a, const char* fmt, ...);