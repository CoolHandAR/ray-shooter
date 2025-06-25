#include "r_common.h"

#include <string.h>
#include <assert.h>
#include <stb_image/stb_image.h>

bool Image_Create(Image* img, int p_width, int p_height, int p_numChannels)
{	
	memset(img, 0, sizeof(Image));

	img->width = p_width;
	img->height = p_height;
	img->numChannels = p_numChannels;

	size_t size = img->width * img->height * img->numChannels;

	//allocate a buffer
	unsigned char* data = malloc(size);

	if (!data)
	{
		printf("ERROR:: Failed to load texture");
		return false;
	}

	memset(data, 0, size);

	img->data = data;

	return true;
}
bool Image_CreateFromPath(Image* img, const char* path)
{
	memset(img, 0, sizeof(Image));

	//load from file
	stbi_uc* data = stbi_load(path, &img->width, &img->height, &img->numChannels, 4);

	//failed to load
	if (!data)
	{
		printf("ERROR:: %s", stbi_failure_reason());
		return false;
	}

	img->data = data;

	return true;
}

void Image_Resize(Image* img, int p_width, int p_height)
{
	if (p_width <= 0 || p_height <= 0)
	{
		return;
	}

	assert(img->data);

	free(img->data);
	img->data = NULL;

	size_t size = p_width * p_height * img->numChannels;

	img->data = malloc(size);

	if (!img->data)
	{
		return;
	}

	memset(img->data, 0, size);

	img->width = p_width;
	img->height = p_height;
}

void Image_Destruct(Image* img)
{
	assert(img->data);

	free(img->data);
}

void Image_Set(Image* img, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return;
	}

	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	if (img->numChannels >= 1) d[0] = r;
	if (img->numChannels >= 2) d[1] = g;
	if (img->numChannels >= 3) d[2] = b;
	if (img->numChannels >= 4) d[3] = a;
}


void Image_Copy(Image* dest, Image* src)
{
	assert(dest->numChannels == src->numChannels);
	
	memcpy(dest->data, src->data, sizeof(unsigned char) * dest->width * dest->height * dest->numChannels);
}

void Sprite_UpdateAnimation(Sprite* sprite, float delta)
{
	if (!sprite->playing)
	{
		return;
	}
	if (sprite->anim_speed_scale == 0)
	{
		return;
	}

	float remaining = delta;

	int i = 0;

	while (remaining >= 0)
	{
		float speed = 12 * sprite->anim_speed_scale;
		float abs_speed = abs(speed);

		if (speed == 0)
		{
			break;
		}

		int frame_count = sprite->frame_count;

		int last_frame = frame_count - 1;

		if (speed > 0)
		{
			if (sprite->_anim_frame_progress >= 1.0)
			{
				//anim restart
				if (sprite->frame >= last_frame)
				{
					if (sprite->looping)
					{
						sprite->loops++;
						sprite->frame = 0;
					}
					else
					{
						sprite->frame = last_frame;
						sprite->playing = false;
					}
				}
				else
				{
					sprite->frame++;
				}
				sprite->_anim_frame_progress = 0.0;
			}

			float to_process = min((1.0 - sprite->_anim_frame_progress) / abs_speed, remaining);
			sprite->_anim_frame_progress += to_process * abs_speed;
			remaining -= to_process;
		}

		i++;
		if (i > frame_count)
		{
			break;
		}
	}

	Render_RedrawSprites();
}

void Sprite_GenerateAlphaSpans(Sprite* sprite)
{
	int total_frames = sprite->h_frames * sprite->v_frames;

	//only single frame
	if (total_frames == 0)
	{
		total_frames = 1;
	}

	FrameInfo* frame_infos = malloc(sizeof(FrameInfo) * total_frames);

	if (!frame_infos)
	{
		return;
	}

	int w = (sprite->h_frames > 0) ? sprite->img->width / sprite->h_frames : sprite->img->width;

	for (int i = 0; i < total_frames; i++)
	{
		FrameInfo* f_info = &frame_infos[i];
		f_info->alpha_spans = malloc(sizeof(AlphaSpan) * w);

		if (!f_info->alpha_spans)
		{
			return;
		}

		memset(f_info->alpha_spans, 0, sizeof(AlphaSpan) * w);

		bool min_x_set = false;
		int min_x = 0;
		int max_x = 0;

		int sprite_offset_x = (sprite->h_frames > 0) ? i % sprite->h_frames : 0;
		int sprite_offset_y = (sprite->v_frames > 0) ? i / sprite->h_frames : 0;

		int sprite_rect_width = (sprite->h_frames > 0) ? sprite->img->width / sprite->h_frames : sprite->img->width;
		int sprite_rect_height = (sprite->v_frames > 0) ? sprite->img->height / sprite->v_frames : sprite->img->height;

		for (int x = 0; x < sprite_rect_width; x++)
		{
			int tx = x + (sprite_offset_x * sprite_rect_width);
			
			AlphaSpan* sp = &f_info->alpha_spans[x];

			bool min_y_set = false;
			int min_y = 0;
			int max_y = 0;

			for (int y = 0; y < sprite_rect_height; y++)
			{
				int ty = y + (sprite_offset_y * sprite_rect_height);

				unsigned char* color = Image_Get(sprite->img, tx, ty);

				if (!color)
				{
					continue;
				}

				//valid pixel
				if (color[3] > 128)
				{
					if (!min_y_set)
					{
						min_y = y;
						min_y_set = true;
					}
					if (!min_x_set)
					{
						min_x = x;
						min_x_set = true;
					}
					max_x = x;
					max_y = y;
				}
				else
				{
					if (!min_y_set)
					{
						min_y = y;
					}
					if (!min_x_set)
					{
						min_x = x;
					}
				}
			}

			sp->min = (min_y > 0) ? min_y - 1 : 0;
			sp->max = max_y;
		}

		f_info->min_real_x = (min_x > 0) ? min_x - 1 : 0;
		f_info->max_real_x = max_x;
	}

	sprite->frame_info = frame_infos;
}

FrameInfo* Sprite_GetFrameInfo(Sprite* sprite, int frame)
{
	int total_frames = sprite->h_frames * sprite->v_frames;

	if (total_frames == 0) total_frames = 1;

	
}

AlphaSpan* Sprite_GetFrameAlphaSpan(Sprite* sprite, int x, int frame)
{
	int total_frames = sprite->h_frames * sprite->v_frames;

	if (total_frames == 0) total_frames = 1;
	
	int w = (sprite->h_frames > 0) ? sprite->img->width / sprite->h_frames : sprite->img->width;

	if (x < 0 || frame < 0 || x >= w || frame >= total_frames)
	{
		return NULL;
	}
	
	return &sprite->frame_info[frame].alpha_spans[x];
}
