#include "r_common.h"

#include "g_common.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "u_math.h"

static const float PI = 3.14159265359;

static void SWAP_INT(int* a, int* b)
{
	int temp = *a;

	*a = *b;
	*b = temp;
}


void Video_DrawLine(Image* image, int x0, int y0, int x1, int y1, unsigned char* color)
{
	int steep = 0;

	if (abs(x0 - x1) < abs(y0 - y1))
	{
		SWAP_INT(&x0, &y0);
		SWAP_INT(&x1, &y1);

		steep = 1;
	}
	if (x0 > x1)
	{
		SWAP_INT(&x0, &x1);
		SWAP_INT(&y0, &y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;

	int derror = abs(dy) * 2;
	int error = 0;

	int y = y0;
	int y_add_sign = (y1 > y0) ? 1 : -1;
	for (int x = x0; x <= x1; x++)
	{
		if (steep == 1)
		{
			Image_Set2(image, y, x, color);
		}
		else
		{
			Image_Set2(image, x, y, color);
		}

		error += derror;

		if (error > dx)
		{
			y += y_add_sign;
			error -= dx * 2;
		}
	}
}

void Video_DrawRectangle(Image* image, int p_x, int p_y, int p_w, int p_h, unsigned char* p_color)
{
	for (int x = 0; x < p_w; x++)
	{
		for (int y = 0; y < p_h; y++)
		{
			Image_Set2(image, x + p_x, y + p_y, p_color);
		}
	}
}

void Video_Clear(Image* image, unsigned char c)
{
	memset(image->data, (int)c, sizeof(unsigned char) * image->width * image->height * image->numChannels);
}
#include <main.h>
void Video_RaycastMap(Image* image, Image* texture, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	const int max_tiles = Map_GetTotalTiles();

	//adapted from https://lodev.org/cgtutor/raycasting.html

	for (int x = 0; x < image->width; x++)
	{
		float cam_x = 2 * x / (float)image->width - 1;

		//calc raydir
		float ray_dir_x = p_dirX + p_planeX * cam_x;
		float ray_dir_y = p_dirY + p_planeY * cam_x;

		//length of ray from one x or y-side to next x or y-side
		float delta_dist_x = (ray_dir_x == 0) ? 1e30 : fabs(1.0 / ray_dir_x);
		float delta_dist_y = (ray_dir_y == 0) ? 1e30 : fabs(1.0 / ray_dir_y);

		float wall_dist = 0;

		int step_x = 0;
		int step_y = 0;

		int side = 0; //was a NS or a EW wall hit?

		int map_x = (int)p_x;
		int map_y = (int)p_y;

		//length of ray from current position to next x or y-side
		float side_dist_x;
		float side_dist_y;

		if (ray_dir_x < 0)
		{
			step_x = -1;
			side_dist_x = (p_x - map_x) * delta_dist_x;
		}
		else
		{
			step_x = 1;
			side_dist_x = (map_x + 1.0 - p_x) * delta_dist_x;
		}
		if (ray_dir_y < 0)
		{
			step_y = -1;
			side_dist_y = (p_y - map_y) * delta_dist_y;
		}
		else
		{
			step_y = 1;
			side_dist_y = (map_y + 1.0 - p_y) * delta_dist_y;
		}

		TileID tile = EMPTY_TILE;
		
		//perform DDA
		for (int step = 0; step < max_tiles; step++)
		{
			if (side_dist_x < side_dist_y)
			{
				side_dist_x += delta_dist_x;
				map_x += step_x;
				side = 0;
			}
			else
			{
				side_dist_y += delta_dist_y;
				map_y += step_y;
				side = 1;
			}

			tile = Map_GetTile(map_x, map_y);

			//proper tile found
			if (tile != EMPTY_TILE)
			{
				break;
			}
		}


		if (tile == EMPTY_TILE)
		{
			continue;
		}
		

		if (side == 0) 
		{
			wall_dist = (side_dist_x - delta_dist_x);
		}
		else
		{
			wall_dist = (side_dist_y - delta_dist_y);
		}

		if (wall_dist <= 0)
		{
			continue;
		}

		int v_offset = 0;
		int size_offset = 0;

		if (tile == 2)
		{
			int off = 100;

			v_offset = (int)((off / 2) / wall_dist);
			size_offset = (int)(off / wall_dist);
		}

		int original_height = 0;

		//Calculate height of collumn
		int collumn_height = (int)(image->height / wall_dist);

		original_height = collumn_height;

		collumn_height -= size_offset;

		if (collumn_height <= 0)
		{
			continue;
		}
		
		//calculate lowest and highest pixel to fill in current stripe
		int draw_start = -collumn_height / 2 + image->height / 2 + v_offset;
		if (draw_start < 0) draw_start = 0;
		int draw_end = collumn_height / 2 + image->height / 2 + v_offset;
		if (draw_end >= image->height) draw_end = image->height - 1;

		if (draw_start > image->width || draw_end < 0)
		{
			continue;
		}

		//calculate value of wallX
		float wall_x; //where exactly the wall was hit
		if (side == 0) wall_x = p_y + wall_dist * ray_dir_y;
		else          wall_x = p_x + wall_dist * ray_dir_x;
		wall_x -= floor((wall_x));

		//x coordinate on the texture
		int tex_x = (int)(wall_x * (double)(64));
		if (side == 0 && ray_dir_x > 0) tex_x = 64 - tex_x - 1;
		if (side == 1 && ray_dir_y < 0) tex_x = 64 - tex_x - 1;

		float step = 1.0 * 64 / original_height;
		// Starting texture coordinate
		float tex_pos = (draw_start - image->height / 2 + collumn_height / 2) * step;

		//draw the vertical collumn
		for (int y = draw_start; y < draw_end; y++)
		{
			// Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
			int tex_y = (int)tex_pos & (64 - 1);
			tex_pos += step;

			//get tile color
			unsigned char* tile_color = Image_GetFast(texture, tex_x + (64 * (tile - 1)), tex_y);
			
			//make y side darker
			if (side == 1)
			{
				unsigned char color[4];
				for (int l = 0; l < 4; l++)
				{
					color[l] = tile_color[l] / 2;
				}
				Image_SetFast(image, x, y, color);
			}
			else
			{
				Image_SetFast(image, x, y, tile_color);
			}
		}

		//set depth buffer 
		depth_buffer[x] = wall_dist;
	}
}

void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	//adapted from https://lodev.org/cgtutor/raycasting3.html

	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return;
	}
	//completely transparent
	if (sprite->transparency >= 1)
	{
		return;
	}

	int sprite_offset_x = (sprite->h_frames > 0) ? sprite->frame % sprite->h_frames : 0;
	int sprite_offset_y = (sprite->v_frames > 0) ? sprite->frame / sprite->h_frames : 0;

	sprite_offset_x += sprite->frame_offset_x;
	sprite_offset_y += sprite->frame_offset_y;

	int sprite_rect_width = (sprite->h_frames > 0) ? sprite->img->width / sprite->h_frames : sprite->img->width;
	int sprite_rect_height = (sprite->v_frames > 0) ? sprite->img->height / sprite->v_frames : sprite->img->height;

	//translate sprite position to relative to camera
	float local_sprite_x = sprite->x - p_x;
	float local_sprite_y = sprite->y - p_y;
	
	float inv_det = 1.0 / (p_planeX * p_dirY - p_dirX * p_planeY); 

	float transform_x = inv_det * (p_dirY * local_sprite_x - p_dirX * local_sprite_y);
	float transform_y = inv_det * (-p_planeY * local_sprite_x + p_planeX * local_sprite_y); 

	if (transform_y <= 0)
	{
		return;
	}

	int sprite_screen_x = (int)((image->width / 2) * (1 + transform_x / transform_y));

#define vMove 0.0
	int v_move_screen = (int)(vMove / transform_y);

	int sprite_width = fabs((int)(image->height / (transform_y))) * (1.0 / sprite->scale_x); // same as height of sprite, given that it's square
	int sprite_height = fabs((int)(image->height / (transform_y))) * (1.0 /  sprite->scale_y); //using "transformY" instead of the real distance prevents fisheye

	if (sprite_height <= 0 || sprite_width <= 0)
	{
		return;
	}

	//calculate lowest and highest pixel to fill in current stripe
	int draw_start_y = -sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_start_y < 0) draw_start_y = 0;
	int draw_end_y = sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_end_y >= image->height) draw_end_y = image->height - 1;

	if (draw_start_y > image->height || draw_end_y < 0)
	{
		return;
	}

	int draw_start_x = -sprite_width / 2 + sprite_screen_x;
	if (draw_start_x < 0) draw_start_x = 0;
	int draw_end_x = sprite_width / 2 + sprite_screen_x;
	if (draw_end_x > image->width) draw_end_x = image->width;

	if (draw_start_x > image->width || draw_end_x < 0)
	{
		return;
	}

	if (sprite->flip_h)
	{
		sprite_offset_x += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	for (int stripe = draw_start_x; stripe < draw_end_x; stripe++)
	{
		if (transform_y >= depth_buffer[stripe])
		{
			continue;
		}

		//depth_buffer[stripe] = transform_y;

		int tex_x = (int)(256 * (stripe - (-sprite_width / 2 + sprite_screen_x)) * sprite_rect_width / sprite_width) / 256;

		if (sprite->flip_h)
		{
			tex_x -= 1;
			tex_x = -tex_x;
		}
	
		for (int y = draw_start_y; y < draw_end_y; y++) //for every pixel of the current stripe
		{
			int d = (y - v_move_screen) * 256 - image->height * 128 + sprite_height * 128; //256 and 128 factors to avoid floats
			int tex_y = ((d * sprite_rect_height) / sprite_height) / 256;

			unsigned char* color = Image_Get(sprite->img, tex_x + (sprite_offset_x * sprite_rect_width), tex_y + (sprite_offset_y * sprite_rect_height));

			if (!color)
			{
				continue;
			}

			//alpha discard if possible
			if (sprite->img->numChannels >= 4)
			{
				if (color[3] < 128)
				{
					continue;
				}
			}

			//apply transparency
			if (transparency > 1)
			{
				unsigned char* old_color = Image_Get(image, stripe, y);

				if (!old_color)
				{
					continue;
				}

				//just do directly on the buffer pointer
				for (int k = 0; k < image->numChannels; k++)
				{
					old_color[k] = (old_color[k] / transparency) + (color[k] / 2);
				}

			}
			else
			{
				Image_Set2(image, stripe, y, color);
			}
		}
		
	}
}

void Video_DrawScreenTexture(Image* image, Image* texture, float p_x, float p_y, float p_scaleX, float p_scaleY)
{
	if (p_scaleX <= 0 || p_scaleY <= 0)
	{
		return;
	}

	//out of bounds
	if (p_x < 0 || p_x > image->width || p_y < 0 || p_y > image->height)
	{
		return;
	}

	const int rect_width = texture->width * p_scaleX;
	const int rect_height = texture->height * p_scaleY;

	for (int x = 0; x < rect_width; x++)
	{
		for (int y = 0; y < rect_height; y++)
		{
			unsigned char* color = Image_Get(texture, x * (1.0 / p_scaleX), y * (1.0 / p_scaleY));

			if (!color)
			{
				continue;
			}

			//perform alpha discarding if there is an alpha channel
			if (texture->numChannels >= 4)
			{
				if (color[3] < 128)
				{
					continue;
				}
			}
			
			Image_Set2(image, x + p_x, y + p_y, color);
		}
	}
	
}

void Video_DrawScreenSprite(Image* image, Sprite* sprite, float p_x, float p_y)
{
	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return;
	}

	//out of bounds
	if (p_x < 0 || p_x > image->width || p_y < 0 || p_y > image->height)
	{
		return;
	}
	//completely transparent
	if (sprite->transparency >= 1)
	{
		return;
	}


	int offset_x = (sprite->h_frames > 0) ? sprite->frame % sprite->h_frames : 0;
	int offset_y = (sprite->v_frames > 0) ? sprite->frame / sprite->h_frames : 0;

	int rect_width = (sprite->h_frames > 0) ? sprite->img->width / sprite->h_frames : sprite->img->width;
	int rect_height = (sprite->v_frames > 0) ? sprite->img->height / sprite->v_frames : sprite->img->height;

	float scale_x = 3;
	float scale_y = 3;

	float d_scale_x = 1.0 / scale_x;
	float d_scale_y = 1.0 / scale_y;

	rect_width *= scale_x;
	rect_height *= scale_y;

	//sprite->flip_h = true;
	
	if (sprite->flip_h)
	{
		offset_x += 1;
	}
	if (sprite->flip_v)
	{
		offset_y += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	FrameInfo* frame_info = &sprite->frame_info[sprite->frame];

	int min_x = (sprite->flip_h) ? (rect_width - (frame_info->max_real_x * scale_y)) : frame_info->min_real_x * scale_y;
	int max_x = (sprite->flip_h) ? (rect_width - (frame_info->min_real_x * scale_y)) : frame_info->max_real_x * scale_y;

	if (sprite->flip_h)
	{
		if (min_x > 0) min_x -= 1;
		max_x += 1;
	}

	for (int x = min_x; x <= max_x; x++)
	{
		int tx = 0;
		int ty = 0;

		int span_x = (sprite->flip_h) ? (rect_width - x - 1) : x;

		AlphaSpan* span = Sprite_GetFrameAlphaSpan(sprite, span_x * d_scale_x, sprite->frame);

		if (span->min > span->max)
		{
			continue;
		}
		
		//flip the sprite horizontally
		if (sprite->flip_h)
		{
			tx = (offset_x * rect_width - x - 1) * d_scale_x;
		}
		else
		{
			tx = (x + offset_x * rect_width) * d_scale_x;
		}


		int y_min = (sprite->flip_v) ? (rect_height - (span->max * scale_y)) : span->min * scale_y;
		int y_max = (sprite->flip_v) ? (rect_height - (span->min * scale_y)) : span->max * scale_y;

		if (sprite->flip_v)
		{
			if (y_min > 0) y_min -= 1;
			y_max += 1;
		}

		for (int y = y_min; y < y_max ; y++)
		{
			//flip the sprite vertically
			if (sprite->flip_v)
			{
				ty = (offset_y * rect_height - y - 1) * d_scale_y;
			}
			else
			{
				ty = (y + offset_y * rect_height) * d_scale_y;
			}


			unsigned char* color = Image_Get(sprite->img, tx, ty);

			if (!color)
			{
				continue;
			}

			//perform alpha discarding if there is 4 color channels
			//most pixels should be discarded with alpha spans
			if (sprite->img->numChannels >= 4)
			{
				if (color[3] < 128)
				{
					continue;
				}
			}

			//apply transparency
			if (transparency > 1)
			{
				unsigned char* old_color = Image_Get(image, x + p_x, y + p_y);

				if (!old_color)
				{
					continue;
				}

				//just do directly on the buffer pointer
				for (int k = 0; k < image->numChannels; k++)
				{
					old_color[k] = (old_color[k] / transparency) + (color[k] / 2);
				}

			}
			else
			{
				Image_Set2(image, x + p_x, y + p_y, color);
			}

			
		}
	}

}

void Video_Shade(Image* image, ShaderFun shader_fun, int x0, int y0, int x1, int y1)
{
	//clamp all the values
	x0 = Math_Clampl(x0, 0, image->width);
	y0 = Math_Clampl(y0, 0, image->height);

	x1 = Math_Clampl(x1, 0, image->width);
	y1 = Math_Clampl(y1, 0, image->height);

	for (int x = x0; x < x1; x++)
	{
		for (int y = y0; y < y1; y++)
		{
			shader_fun(image, x, y, 0, 0);
		}
	}

}
