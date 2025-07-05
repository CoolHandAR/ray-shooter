#include "r_common.h"

#include "g_common.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "u_math.h"

static const float PI = 3.14159265359;
static int doors_drawn = 0;

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

void Video_RaycastFloorCeilling(Image* image, Image* texture, float* depth_buffer, int x, int spans, int draw_start, int draw_end, float floor_x, float floor_y, float wall_dist, float p_x, float p_y, bool is_floor)
{
	//adapted from https://lodev.org/cgtutor/raycasting2.html

	float dist_view = 0;

	int y = (is_floor) ? draw_end + 1 : 0;
	int end = (is_floor) ? image->height : draw_start;

	for (y; y < end; y++)
	{
		float depth = depth_buffer[x + y * image->width];

		//depth is set by other tile
		if (depth < (int)DEPTH_CLEAR)
		{
			if (is_floor)
			{
				continue;
			}
			else
			{
				break;
			}
		}

		float current_dist = 0;

		if (is_floor)
		{
			current_dist = (image->height + (2.0)) / (2.0 * (y)-image->height);
		}
		else
		{
			current_dist = (image->height - (2.0)) / (image->height - 2.0 * (y));
		}

		double weight = (current_dist - dist_view) / (wall_dist - dist_view);

		// Some variables here are called floor but apply to the ceiling here
		double current_floor_x = weight * floor_x + (1.0 - weight) * p_x;
		double current_floor_y = weight * floor_y + (1.0 - weight) * p_y;

		int tx = (int)(current_floor_x * 64) & (64 - 1);
		int ty = (int)(current_floor_y * 64) & (64 - 1);
	
		unsigned char* color = Image_Get(texture, tx, ty);

		for (int span = 0; span < spans; span++)
		{
			Image_Set2(image, x + span, y, color);
		}
		
	}

}

static void Video_SetupSpans(Image* image, Image *texture, float* depth_buffer, DrawSpan* draw_spans, int width, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY, bool draw_floor_ceilling)
{
	const int max_tiles = Map_GetTotalTiles();

	const int map_width, map_height;
	Map_GetSize(&map_width, &map_height);

	float aspect = Render_GetWindowAspect();

	p_planeX *= aspect;
	p_planeY *= aspect;

	int prev_side = -1;
	int prev_tile = -1;
	int prev_tile_x = -1;
	int prev_tile_y = -1;
	float prev_size = -1;

	for (int x = 0; x < width; x++)
	{
		DrawSpan* span = &draw_spans[x];

		span->width = 0;
		span->pointer_index = -1;

		double cam_x = 2.0 * x / (double)width - 1.0;

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

			if ((map_x < 0 && map_y < 0) || (map_x > map_width && map_y > map_height))
			{
				break;
			}

			float draw_size = 1;

			tile = Map_GetTile(map_x, map_y);

			//no tile found?
			if (tile == EMPTY_TILE)
			{
				//look for a door or special  object
				Object* object = Map_GetObjectAtTile(map_x, map_y);

				if (!object)
				{
					continue;
				}

				if (object->type == OT__DOOR || object->type == OT__SPECIAL_TILE)
				{
					if (object->type == OT__DOOR)
					{
						draw_size = object->move_timer;
						tile = DOOR_TILE;
					}
					else
					{
						tile = object->state;
					}

				}
				else
				{
					continue;
				}
			}

			if (tile == EMPTY_TILE || draw_size <= 0)
			{
				prev_side = -1;
				prev_tile = -1;
				prev_tile_x = -1;
				prev_tile_y = -1;
				prev_size = -1;
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

			//calculate value of wallX
			float wall_x; //where exactly the wall was hit
			if (side == 0) wall_x = p_y + wall_dist * ray_dir_y;
			else          wall_x = p_x + wall_dist * ray_dir_x;
			wall_x -= floor((wall_x));

			//x coordinate on the texture
			int tex_x = (int)(wall_x * (double)(64));
			if (side == 0 && ray_dir_x > 0) tex_x = 64 - tex_x - 1;
			if (side == 1 && ray_dir_y < 0) tex_x = 64 - tex_x - 1;

			if (draw_floor_ceilling)
			{
				float floor_x_wall = 0;
				float floor_y_wall = 0;

				if (side == 0 && ray_dir_x > 0)
				{
					floor_x_wall = map_x;
					floor_y_wall = map_y + wall_x;
				}
				else if (side == 0 && ray_dir_x < 0)
				{
					floor_x_wall = map_x + 1.0;
					floor_y_wall = map_y + wall_x;
				}
				else if (side == 1 && ray_dir_y > 0)
				{
					floor_x_wall = map_x + wall_x;
					floor_y_wall = map_y;
				}
				else
				{
					floor_x_wall = map_x + wall_x;
					floor_y_wall = map_y + 1.0;
				}
				span->floor_x = floor_x_wall;
				span->floor_y = floor_y_wall;
			}
		
			span->side = side;
			span->tex_x = tex_x;
			span->wall_dist = wall_dist;
			span->hit_tile = tile;
			span->width = 1;

			//check if previous span can draw this span
			if (x > 0 && prev_side == side && prev_tile == tile && prev_size == 1)
			{
				DrawSpan* prev_span = &draw_spans[x - 1];

				if (tex_x == prev_span->tex_x)
				{
					bool same_x = (prev_side == 0 && prev_tile_y == map_y);
					bool same_y = (prev_side == 1 && prev_tile_x == map_x);

					if (same_x || same_y)
					{
						if (prev_span->pointer_index == -1)
						{
							span->pointer_index = x - 1;
							prev_span->width++;

						}
						else
						{
							DrawSpan* pointer_span = &draw_spans[prev_span->pointer_index];
							pointer_span->width++;
							span->pointer_index = prev_span->pointer_index;
						}

						span->width = 0;
					}
				}
			}

			prev_side = side;
			prev_tile = tile;
			prev_tile_x = map_x;
			prev_tile_y = map_y;
			prev_size = draw_size;

			if (draw_size < 1)
			{
				span->width = 0;

				if (!Video_DrawCollumn(image, texture, x, draw_size, depth_buffer, span->tex_x, span->wall_dist, span->side, span->hit_tile, 1, NULL, NULL))
				{
					break;
				}

				doors_drawn++;
				continue;
			}
			else
			{
				break;
			}
						
		}
	}
}


void Video_RaycastMap(Image* image, Image* texture, Image* floor_texture, Image* ceil_texture, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	bool draw_floor_ceilling = (floor_texture || ceil_texture);

	Video_SetupSpans(image, texture, depth_buffer, draw_spans, image->width, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY, draw_floor_ceilling);

	for (int x = 0; x < image->width; x++)
	{
		DrawSpan* span = &draw_spans[x];

		if (span->width <= 0)
		{
			continue;
		}

		int draw_start = 0;
		int draw_end = 0;

		if (!Video_DrawCollumn(image, texture, x, 1, depth_buffer, span->tex_x, span->wall_dist, span->side, span->hit_tile, span->width, &draw_start, &draw_end))
		{
			continue;
		}

		//draw floor
		if (floor_texture)
		{
			Video_RaycastFloorCeilling(image, texture, depth_buffer, x, span->width, draw_start, draw_end, span->floor_x, span->floor_y, span->wall_dist, p_x, p_y, true);
		}

		//draw ceilling
		if (ceil_texture)
		{
			Video_RaycastFloorCeilling(image, texture, depth_buffer, x, span->width, draw_start, draw_end, span->floor_x, span->floor_y, span->wall_dist, p_x, p_y, false);
		}

		x += span->width - 1;
	}
}

bool Video_DrawCollumn(Image* image, Image* texture, int x, float size, float* depth_buffer, int tex_x, float wall_dist, int side, int tile, int spans, int* r_draw_start, int* r_draw_end)
{
	//nothing to draw
	if (tile == EMPTY_TILE || wall_dist <= 0 || spans <= 0)
	{
		return false;
	}

	int v_offset = 0;
	int size_offset = 0;

	int off = image->height * (1.0 - size);

	v_offset = (int)((off / 2) / wall_dist);
	size_offset = (int)(off / wall_dist);

	int original_height = 0;

	//Calculate height of collumn
	int collumn_height = (int)(image->height / wall_dist);

	original_height = collumn_height;

	collumn_height -= size_offset;

	if (collumn_height <= 0)
	{
		return false;
	}

	float collumn_pixels = (float)(image->width) / (float)collumn_height;

	collumn_pixels /= 20;

	//calculate lowest and highest pixel to fill in current stripe
	int draw_start = -collumn_height / 2 + image->height / 2 + v_offset;
	if (draw_start < 0) draw_start = 0;
	int draw_end = collumn_height / 2 + image->height / 2 + v_offset;
	if (draw_end >= image->height) draw_end = image->height - 1;

	if (draw_start > image->width || draw_end < 0)
	{
		return false;
	}

	float step = 1.0 * 64 / original_height;

	// Starting texture coordinate
	float tex_pos = (draw_start - image->height / 2 + collumn_height / 2) * (step * size);

	//draw the vertical collumn
	for (int y = draw_start; y < draw_end; y++)
	{
		float depth = depth_buffer[x + y * image->width];

		//depth is set by other tile
		if (depth < (int)DEPTH_CLEAR)
		{
			break;
		}

		// Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
		int tex_y = (int)tex_pos & (64 - 1);
		tex_pos += step;


		//get tile color
		unsigned char* tile_color = Image_Get(texture, tex_x + (64 * (tile - 1)), tex_y);

		//make y side darker
		if (side == 1)
		{
			unsigned char color[4];
			for (int l = 0; l < 4; l++)
			{
				color[l] = tile_color[l] / 2;
			}
			for (int span = 0; span < spans; span++)
			{
				Image_SetFast(image, x + span, y, color);
				depth_buffer[(x + span) + y * image->width] = wall_dist;
			}
		}
		else
		{
			for (int span = 0; span < spans; span++)
			{
				Image_SetFast(image, x + span, y, tile_color);
				depth_buffer[(x + span) + y * image->width] = wall_dist;
			}
		}
	}

	if(r_draw_start) *r_draw_start = draw_start;
	if(r_draw_end) *r_draw_end = draw_end;

	return true;
}

void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, DrawSpan* draw_spans, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	//adapted from https://lodev.org/cgtutor/raycasting3.html

	float aspect = Render_GetWindowAspect();

	p_planeX *= aspect;
	p_planeY *= aspect;

	int win_w, win_h;
	Render_GetWindowSize(&win_w, &win_h);

	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return;
	}
	//completely transparent
	if (sprite->transparency >= 1)
	{
		return;
	}

	const int h_frames = sprite->img->h_frames;
	const int v_frames = sprite->img->v_frames;

	int sprite_offset_x = (h_frames > 0) ? sprite->frame % h_frames : 0;
	int sprite_offset_y = (v_frames > 0) ? sprite->frame / h_frames : 0;

	sprite_offset_x += sprite->frame_offset_x;
	sprite_offset_y += sprite->frame_offset_y;

	int sprite_rect_width = (h_frames > 0) ? sprite->img->width / h_frames : sprite->img->width;
	int sprite_rect_height = (v_frames > 0) ? sprite->img->height / v_frames : sprite->img->height;

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

	int sprite_width = fabs((int)(image->height / (transform_y))) * (sprite->scale_x); // same as height of sprite, given that it's square
	int sprite_height = fabs((int)(image->height / (transform_y))) * (sprite->scale_y); //using "transformY" instead of the real distance prevents fisheye

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

	int safe_end_x = (draw_end_x >= image->width) ? draw_end_x - 1 : draw_end_x;
	int safe_end_y = (draw_end_y >= image->height) ? draw_end_y - 1 : draw_end_y;

	//all depth edges of a sprite rectangle
	float d0 = depth_buffer[draw_start_x + draw_start_y * image->width]; //top left edge
	float d1 = depth_buffer[safe_end_x + draw_start_y * image->width]; //top right edge
	float d2 = depth_buffer[draw_start_x + safe_end_y * image->width]; //bottom left edge
	float d3 = depth_buffer[safe_end_x + safe_end_y * image->width]; //bottom right edge

	float t0 = inv_det * (-p_planeY * (local_sprite_x - sprite_width / 2) + p_planeX * (local_sprite_y - sprite_height / 2));
	float t1 = inv_det * (-p_planeY * (local_sprite_x + sprite_width / 2) + p_planeX * (local_sprite_y - sprite_height / 2));
	float t2 = inv_det * (-p_planeY * (local_sprite_x - sprite_width / 2) + p_planeX * (local_sprite_y + sprite_height / 2));
	float t3 = inv_det * (-p_planeY * (local_sprite_x + sprite_width / 2) + p_planeX * (local_sprite_y + sprite_height / 2));
	
	//check if all edges are fully occluded
	if (t0 >= d0 && t1 >= d1 && t2 >= d2 && t3 >= d3)
	{
		return;
	}

	FrameInfo* frame_info = Image_GetFrameInfo(sprite->img, sprite->frame + (sprite->frame_offset_x) + (sprite->frame_offset_y * sprite->img->h_frames));

	if (sprite->flip_h)
	{
		sprite_offset_x += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	int min_x = (sprite->flip_h) ? ((sprite_rect_width) - (frame_info->max_real_x)) : frame_info->min_real_x;
	int max_x = (sprite->flip_h) ? ((sprite_rect_width) - (frame_info->min_real_x)) : frame_info->max_real_x;

	if (sprite->flip_h)
	{
		if (min_x > 0) min_x -= 1;
		max_x += 1;
	}

	int prev_tex_x = -1;

	for (int stripe = draw_start_x; stripe < draw_end_x; stripe++)
	{
		DrawSpan* span = &draw_spans[stripe];

		span->pointer_index = -1;
		span->width = 0;

		if (transform_y >= depth_buffer[stripe + draw_start_y * image->width] && doors_drawn == 0)
		{
			continue;
		}

		int tex_x = (int)(256 * (stripe - (-sprite_width / 2 + sprite_screen_x)) * sprite_rect_width / sprite_width) / 256;

		if (tex_x < min_x)
		{
			prev_tex_x = -1;
			continue;
		}
		else if (tex_x > max_x)
		{
			prev_tex_x = -1;
			draw_end_x = stripe + 1;
			break;
		}

		if (tex_x == min_x)
		{
			draw_start_x = stripe;
		}

		span->pointer_index = -1;
		span->width = 1;
		span->tex_x = tex_x;

		if (stripe > 0 && prev_tex_x == tex_x)
		{
			DrawSpan* prev_span = &draw_spans[stripe - 1];

			if (prev_span->pointer_index == -1)
			{
				span->pointer_index = stripe - 1;
				prev_span->width++;

			}
			else
			{
				DrawSpan* pointer_span = &draw_spans[prev_span->pointer_index];
				pointer_span->width++;
				span->pointer_index = prev_span->pointer_index;
			}

			span->width = 0;
		}


		prev_tex_x = tex_x;
	}

	for (int stripe = draw_start_x; stripe < draw_end_x; stripe++)
	{
		DrawSpan* draw_span = &draw_spans[stripe];

		if (draw_span->width == 0)
		{
			continue;
		}

		int tex_x = draw_span->tex_x;

		int span_x = (sprite->flip_h) ? (sprite_rect_width - tex_x) : tex_x;

		AlphaSpan* span = FrameInfo_GetAlphaSpan(frame_info, span_x);

		if (span->min > span->max)
		{
			continue;
		}

		int y_min = (sprite->flip_v) ? (sprite_rect_height - (span->max)) : span->min;
		int y_max = (sprite->flip_v) ? (sprite_rect_height - (span->min)) : span->max;

		if (sprite->flip_v)
		{
			if (y_min > 0) y_min -= 1;
			y_max += 1;
		}
		if (sprite->flip_h)
		{
			tex_x -= 1;
			tex_x = -tex_x;
		}


		for (int y = draw_start_y; y < draw_end_y; y++) //for every pixel of the current stripe
		{
			if (transform_y >= depth_buffer[stripe + y * image->width])
			{
				continue;
			}

			int d = (y - v_move_screen) * 256 - image->height * 128 + sprite_height * 128; //256 and 128 factors to avoid floats
			int tex_y = ((d * sprite_rect_height) / sprite_height) / 256;

			if (tex_y < y_min)
			{
				continue;
			}
			else if (tex_y > y_max)
			{
				break;
			}

			unsigned char* color = Image_Get(sprite->img, tex_x + (sprite_offset_x * sprite_rect_width), tex_y + (sprite_offset_y * sprite_rect_height));

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
				for (int l = 0; l < draw_span->width; l++)
				{
					unsigned char* old_color = Image_Get(image, stripe + l, y);

					//just do directly on the buffer pointer
					for (int k = 0; k < image->numChannels; k++)
					{
						old_color[k] = (old_color[k] / transparency) + (color[k] / 2);
					}

					depth_buffer[(stripe + l) + y * image->width] = transform_y;
				}
				

			}
			else
			{
				for (int l = 0; l < draw_span->width; l++)
				{
					Image_SetFast(image, stripe + l, y, color);
					depth_buffer[(stripe + l) + y * image->width] = transform_y;
				}
				
			}
		}
		
		stripe += draw_span->width - 1;
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

	int render_scale = Render_GetRenderScale();

	p_scaleX *= render_scale;
	p_scaleY *= render_scale;

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

	int render_scale = Render_GetRenderScale();

	int pix_x = p_x * image->width;
	int pix_y = p_y * image->height;

	const int h_frames = sprite->img->h_frames;
	const int v_frames = sprite->img->v_frames;

	int offset_x = (h_frames > 0) ? sprite->frame % h_frames : 0;
	int offset_y = (v_frames > 0) ? sprite->frame / h_frames : 0;

	int rect_width = (h_frames > 0) ? sprite->img->width / h_frames : sprite->img->width;
	int rect_height = (v_frames > 0) ? sprite->img->height / v_frames : sprite->img->height;

	float scale_x = sprite->scale_x * render_scale;
	float scale_y = sprite->scale_y * render_scale;

	float d_scale_x = 1.0 / scale_x;
	float d_scale_y = 1.0 / scale_y;

	rect_width *= scale_x;
	rect_height *= scale_y;


	if (sprite->flip_h)
	{
		offset_x += 1;
	}
	if (sprite->flip_v)
	{
		offset_y += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	FrameInfo* frame_info = Image_GetFrameInfo(sprite->img, sprite->frame + (sprite->frame_offset_x) + (sprite->frame_offset_y * sprite->img->h_frames));

	int min_x = (sprite->flip_h) ? (rect_width - (frame_info->max_real_x * scale_x)) : frame_info->min_real_x * scale_x;
	int max_x = (sprite->flip_h) ? (rect_width - (frame_info->min_real_x * scale_x)) : frame_info->max_real_x * scale_x;

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

		AlphaSpan* span = FrameInfo_GetAlphaSpan(frame_info, span_x * d_scale_x);

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
				Image_Set2(image, x + pix_x, y + pix_y, color);
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
