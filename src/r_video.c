#include "r_common.h"

#include "g_common.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "u_math.h"

static const float PI = 3.14159265359;
static unsigned char LIGHT_LUT[256][256];

static void SWAP_INT(int* a, int* b)
{
	int temp = *a;

	*a = *b;
	*b = temp;
}

void Video_Setup()
{
	//setup light lut
	for (int i = 0; i < 256; i++)
	{
		for (int k = 0; k < 256; k++)
		{
			float light = (float)k / 255.0f;
			
			int l = (float)i * light;

			if (l < 0) l = 0;
			else if (l > 255) l = 255;

			LIGHT_LUT[i][k] = (unsigned char)l;		
		}
	}
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
	if (p_x < 0)
	{
		p_x = 0;
	}
	if (p_x >= image->width)
	{
		p_x = image->width - 1;
	}
	if (p_y < 0)
	{
		p_y = 0;
	}
	if (p_y >= image->height)
	{
		p_y = image->height - 1;
	}

	for (int x = 0; x < p_w; x++)
	{
		for (int y = 0; y < p_h; y++)
		{
			Image_Set2(image, x + p_x, y + p_y, p_color);
		}
	}
}

static void Video_DrawFloor2(Image* image, Image* texture, float* depth_buffer, DrawSpan* draw_spans, int x_start, int x_end, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	float ray_dirX0 = p_dirX - p_planeX;
	float ray_dirY0 = p_dirY - p_planeY;
	float ray_dirX1 = p_dirX + p_planeX;
	float ray_dirY1 = p_dirY + p_planeY;

	float x_dir = ray_dirX1 - ray_dirX0;
	float y_dir = ray_dirY1 - ray_dirY0;

	float cam_z = 0.5 * image->height;

	for (int y = 0; y < image->height; ++y)
	{
		bool is_floor = y > image->half_height;

		// Current y position compared to the center of the screen (the horizon)
		int p = is_floor ? (y - image->half_height) : (image->half_height - y);

		if (p == 0)
		{
			continue;
		}

		float row_distance = cam_z / p;

		float floor_step_x = row_distance * (x_dir) / image->width;
		float floor_step_y = row_distance * (y_dir) / image->width;

		if (floor_step_x == 0 || floor_step_y == 0)
		{
			continue;
		}

		float floor_x = p_x + row_distance * ray_dirX0;
		float floor_y = p_y + row_distance * ray_dirY0;

		floor_x += (x_start * floor_step_x);
		floor_y += (x_start * floor_step_y);

		for (int x = x_start; x < x_end; ++x)
		{
			int tile_x = (int)(floor_x);
			int tile_y = (int)(floor_y);

			TileID tile = 0;

			if (is_floor)
			{
				tile = Map_GetFloorTile(tile_x, tile_y);
			}
			else
			{
				tile = Map_GetCeilTile(tile_x, tile_y);
			}

			LightTile* light_tile = Map_GetLightTile(tile_x, tile_y);

			int light = light_tile->light;

			light += light_tile->temp_light;

			if (light > 255) light = 255;

			// get the texture coordinate from the fractional part
			int tx = (int)(TILE_SIZE * (floor_x - tile_x)) & (TILE_SIZE - 1);
			int ty = (int)(TILE_SIZE * (floor_y - tile_y)) & (TILE_SIZE - 1);

			unsigned char* sample = Image_Get(texture, tx + (TILE_SIZE * (tile - 1)), ty);
			unsigned char color[4] = { LIGHT_LUT[sample[0]][light], LIGHT_LUT[sample[1]][light], LIGHT_LUT[sample[2]][light], 255 };

			float local_x_pos = floor_x;
			float local_y_pos = floor_y;
			int local_tx = tx;
			int local_ty = ty;

			int x_steps = 0;

			while (local_tx == tx && local_ty == ty && (x + x_steps) < x_end)
			{
				float depth = depth_buffer[(x + x_steps) + y * image->width];

				//depth is set by other tile
				if (depth < (int)DEPTH_CLEAR)
				{
					break;
				}

				Image_Set2(image, x + x_steps, y, color);
				//depth_buffer[(x + x_steps) + y * image->width] = row_distance;

				local_x_pos += floor_step_x;
				local_y_pos += floor_step_y;
				local_tx = (int)(TILE_SIZE * (local_x_pos - (int)local_x_pos)) & (TILE_SIZE - 1);
				local_ty = (int)(TILE_SIZE * (local_y_pos - (int)local_y_pos)) & (TILE_SIZE - 1);
				x_steps++;
			}

			if (x_steps == 0)
			{
				x_steps = 1;
			}

			x += x_steps - 1;

			floor_x += (x_steps * floor_step_x);
			floor_y += (x_steps * floor_step_y);
		}
	}
}

void Video_RaycastFloorCeilling(Image* image, Image* texture, float* depth_buffer, int x, int spans, int draw_start, int draw_end, float floor_x, float floor_y, float wall_dist, float p_x, float p_y, int doors_drawn, bool is_floor)
{
	//adapted from https://lodev.org/cgtutor/raycasting2.html

	int height = image->half_height;
	int h0 = image->height + 2;
	int h1 = image->height - 2;

	int y = (is_floor) ? draw_end + 2 : 0;
	int end = (is_floor) ? image->height : draw_start;

	bool cheap = true;

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
			current_dist = (h0) / (2.0 * (y) - image->height);
		}
		else
		{
			current_dist = (h1) / (image->height - 2.0 * (y));
		}

		float weight = (current_dist) / (wall_dist);

		float current_floor_x = weight * floor_x + (1.0 - weight) * p_x;
		float current_floor_y = weight * floor_y + (1.0 - weight) * p_y;

		int floor_tile_x = (int)current_floor_x;
		int floor_tile_y = (int)current_floor_y;

		TileID tile = Map_GetFloorTile(floor_tile_x, floor_tile_y);

		LightTile* light_tile = Map_GetLightTile(floor_tile_x, floor_tile_y);

		int light = (int)light_tile->light + (int)light_tile->temp_light;

		if (light > 255)
		{
			light = 255;
		}

		int tx = (int)(current_floor_x * 64) & (64 - 1);
		int ty = (int)(current_floor_y * 64) & (64 - 1);

		unsigned char* tile_color = Image_Get(texture, tx + (TILE_SIZE * (tile - 1)), ty);

		unsigned char color[4] = { LIGHT_LUT[tile_color[0]][light], LIGHT_LUT[tile_color[1]][light], LIGHT_LUT[tile_color[2]][light], 255 };

		for (int span = 0; span < spans; span++)
		{
			Image_Set2(image, x + span, y, color);
		}
		
	}

}

static void Video_SetupSpans(Image* image, Image *texture, float* depth_buffer, DrawSpan* draw_spans, int x_start, int x_end, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY, bool draw_floor_ceilling, int* r_doors_drawn)
{
	const int max_tiles = Map_GetTotalNonEmptyTiles();

	const int map_width, map_height;
	Map_GetSize(&map_width, &map_height);

	int prev_side = -1;
	int prev_tile = -1;
	int prev_tile_x = -1;
	int prev_tile_y = -1;
	float prev_size = -1;

	bool expensive_lighting = true;

	for (int x = x_start; x < x_end; x++)
	{
		DrawSpan* span = &draw_spans[x];

		span->width = 0;
		span->pointer_index = -1;

		double cam_x = 2.0 * x / (double)image->width - 1.0;

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

			if ((map_x < 0 && map_y < 0) || (map_x >= map_width && map_y >= map_height))
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

				if (object->type == OT__DOOR || object->type == OT__SPECIAL_TILE || object->type == OT__TRIGGER)
				{
					if (object->type == OT__DOOR)
					{
						draw_size = object->move_timer;
						tile = object->gid;
					}
					else if (object->type == OT__TRIGGER)
					{
						if (object->sub_type == SUB__TRIGGER_SWITCH)
						{
							tile = object->gid;

							if (object->flags & OBJ_FLAG__TRIGGER_SWITCHED_ON)
							{
								tile++;
							}
						}
						else
						{
							continue;
						}
					}
					else
					{
						tile = object->gid;
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
			int tex_x = (int)(wall_x * (double)(TILE_SIZE));
			if (side == 0 && ray_dir_x > 0) tex_x = TILE_SIZE - tex_x - 1;
			if (side == 1 && ray_dir_y < 0) tex_x = TILE_SIZE - tex_x - 1;

			float floor_x_wall = 0;
			float floor_y_wall = 0;

			if (draw_floor_ceilling || expensive_lighting)
			{
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
		
			span->light = 0;
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
					bool same_x = (prev_side == 0);
					bool same_y = (prev_side == 1);

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

			//calculate light
			LightTile* light_tile = Map_GetLightTile(map_x, map_y);

			if (light_tile)
			{
				span->light = light_tile->light;

				if (side == 0)
				{
					if (ray_dir_x < 0)
					{
						span->light = light_tile->light_east;
					}
					else
					{
						span->light = light_tile->light_west;
					}
				}
				else if (side == 1)
				{
					if (ray_dir_y < 0)
					{
						span->light = light_tile->light_south;
					}
					else
					{
						span->light = light_tile->light_north;
					}
				}

				int l = (int)span->light + (int)light_tile->temp_light;
				
				if (l > 255) l = 255;

				span->light = l;
			}

			

			prev_side = side;
			prev_tile = tile;
			prev_tile_x = map_x;
			prev_tile_y = map_y;
			prev_size = draw_size;

			if (draw_size < 1)
			{
				span->width = 0;

				if (!Video_DrawCollumn(image, texture, x, draw_size, depth_buffer, span->tex_x, span->wall_dist, span->light, span->hit_tile, 1, 0, NULL, NULL))
				{
					break;
				}

				*r_doors_drawn = *r_doors_drawn + 1;
				continue;
			}
			else
			{
				break;
			}
						
		}
	}
}


void Video_RaycastMap(Image* image, Image* texture, float* depth_buffer, DrawSpan* draw_spans, int x_start, int x_end, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	int doors_drawn = 0;

	bool draw_floor_ceilling;

	draw_floor_ceilling = true;

	Video_SetupSpans(image, texture, depth_buffer, draw_spans,x_start, x_end, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY, draw_floor_ceilling, &doors_drawn);

	int floors_drawn = 0;

	for (int x = x_start; x < x_end; x++)
	{
		DrawSpan* span = &draw_spans[x];

		if (span->width <= 0)
		{
			continue;
		}

		int draw_start = 0;
		int draw_end = 0;

		if (!Video_DrawCollumn(image, texture, x, 1, depth_buffer, span->tex_x, span->wall_dist, span->light, span->hit_tile, span->width, doors_drawn, &draw_start, &draw_end))
		{
			continue;
		}
	
		x += span->width - 1;
	}
	Video_DrawFloor2(image, texture, depth_buffer, draw_spans, x_start, x_end, p_x, p_y, p_dirX, p_dirY, p_planeX, p_planeY);
}

bool Video_DrawCollumn(Image* image, Image* texture, int x, float size, float* depth_buffer, int tex_x, float wall_dist, unsigned char light, int tile, int spans, int doors_drawn, int* r_draw_start, int* r_draw_end)
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

	//calculate lowest and highest pixel to fill in current stripe
	int draw_start = -collumn_height / 2 + image->height / 2 + v_offset;
	if (draw_start < 0) draw_start = 0;
	int draw_end = collumn_height / 2 + image->height / 2 + v_offset;
	if (draw_end >= image->height) draw_end = image->height - 1;

	if (draw_start >= image->width || draw_end <= 0)
	{
		return false;
	}

	float step = 1.0 * TILE_SIZE / original_height;

	// Starting texture coordinate
	float tex_pos = (draw_start - image->height / 2 + collumn_height / 2) * (step * size);

	int i_tex_pos = (int)tex_pos;

	int tx = tex_x + (TILE_SIZE * (tile - 1));

	//draw the vertical collumn
	for (int y = draw_start; y < draw_end; y++)
	{
		float depth = depth_buffer[x + y * image->width];

		//depth is set by other tile
		if (depth < (int)DEPTH_CLEAR)
		{
			break;
		}

		if (light > 0)
		{
			// Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
			int tex_y = i_tex_pos & (TILE_SIZE - 1);
			tex_pos += step;
			i_tex_pos = (int)tex_pos;

			//get tile color
			unsigned char* tile_color = Image_Get(texture, tx, tex_y);

			unsigned char color[4] = { LIGHT_LUT[tile_color[0]][light], LIGHT_LUT[tile_color[1]][light], LIGHT_LUT[tile_color[2]][light], 255 };
			
			//try to draw as many pixels in a row as possible
			float local_tex_pos = tex_pos;
			
			while (y < draw_end)
			{
				for (int span = 0; span < spans; span++)
				{
					int xs = x + span;

					Image_SetFast(image, xs, y, color);
					depth_buffer[xs + y * image->width] = wall_dist;
				}

				local_tex_pos += step;
				
				if ((int)local_tex_pos != i_tex_pos)
				{
					break;
				}

				y++;
			}

			tex_pos = local_tex_pos - step;
			
		}
		else
		{
			unsigned char color[4] = { 0, 0, 0, 255 };

			while (y < draw_end)
			{
				for (int span = 0; span < spans; span++)
				{
					int xs = x + span;

					Image_SetFast(image, xs, y, color);
					depth_buffer[xs + y * image->width] = wall_dist;
				}
				
				y++;
			}
		}
		
	}

	if(r_draw_start) *r_draw_start = draw_start;
	if(r_draw_end) *r_draw_end = draw_end;

	return true;
}


void Video_DrawSprite(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	//adapted parts from https://lodev.org/cgtutor/raycasting3.html

	if (!depth_buffer)
	{
		return;
	}

	//invalid scale
	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return;
	}
	//completely transparent
	if (sprite->transparency >= 1)
	{
		return;
	}

	int frame = sprite->frame;

	const int h_frames = sprite->img->h_frames;
	const int v_frames = sprite->img->v_frames;

	int sprite_offset_x = (h_frames > 0) ? frame % h_frames : 0;
	int sprite_offset_y = (v_frames > 0) ? frame / h_frames : 0;

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

	int sprite_screen_x = (int)((image->half_width) * (1 + transform_x / transform_y));

	int height_transform_y = fabs((int)(image->height / (transform_y)));

	int sprite_width = height_transform_y * (sprite->scale_x);
	int sprite_height = height_transform_y * (sprite->scale_y);

	if (sprite_height <= 0 || sprite_width <= 0)
	{
		return;
	}

	int v_move_screen = (int)((sprite->v_offset * image->half_height) / transform_y);
	
	int sprite_half_width = sprite_width / 2;
	int sprite_half_height = sprite_height / 2;

	//calculate lowest and highest pixel to fill in current stripe
	int draw_start_y = -sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_start_y < 0) draw_start_y = 0;
	int draw_end_y = sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_end_y >= image->height) draw_end_y = image->height - 1;

	if (draw_start_y >= image->height || draw_end_y <= 0)
	{
		return;
	}

	int draw_start_x = -sprite_width / 2 + sprite_screen_x;
	if (draw_start_x < 0) draw_start_x = 0;
	int draw_end_x = sprite_width / 2 + sprite_screen_x;
	if (draw_end_x > image->width) draw_end_x = image->width;

	if (draw_start_x >= image->width || draw_end_x <= 0)
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

	float t0 = inv_det * (-p_planeY * (local_sprite_x - sprite_half_width) + p_planeX * (local_sprite_y - sprite_half_height));
	float t1 = inv_det * (-p_planeY * (local_sprite_x + sprite_half_width) + p_planeX * (local_sprite_y - sprite_half_height));
	float t2 = inv_det * (-p_planeY * (local_sprite_x - sprite_half_width) + p_planeX * (local_sprite_y + sprite_half_height));
	float t3 = inv_det * (-p_planeY * (local_sprite_x + sprite_half_width) + p_planeX * (local_sprite_y + sprite_half_height));

	//check if all edges are fully occluded
	if (t0 >= d0 && t1 >= d1 && t2 >= d2 && t3 >= d3)
	{
		return;
	}

	int sprite_light = sprite->light * 255;

	LightTile* light_tile = Map_GetLightTile((int)sprite->x, (int)sprite->y);
	int light = light_tile->light;

	light += sprite_light;
	light += light_tile->temp_light;

	if (light > 255)
	{
		light = 255;
	}
	else if (light < 0)
	{
		light = 0;
	}

	FrameInfo* frame_info = Image_GetFrameInfo(sprite->img, frame + (sprite->frame_offset_x) + (sprite->frame_offset_y * sprite->img->h_frames));

	if (sprite->flip_h)
	{
		sprite_offset_x += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	int min_x = (sprite->flip_h) ? ((sprite_rect_width)-(frame_info->max_real_x)) : frame_info->min_real_x;
	int max_x = (sprite->flip_h) ? ((sprite_rect_width)-(frame_info->min_real_x)) : frame_info->max_real_x;

	if (sprite->flip_h)
	{
		if (min_x > 0) min_x -= 1;
		max_x += 1;
	}

	float x_tex_step = (256 * (float)sprite_rect_width / (float)sprite_width) / 256.0;

	for (int stripe = draw_start_x; stripe < draw_end_x; stripe++)
	{
		float tex_pos_x = (256.0 * (stripe - (-sprite_half_width + sprite_screen_x)) * sprite_rect_width / sprite_width) / 256.0;
		int tex_x = tex_pos_x;

		if (tex_x < min_x)
		{
			continue;
		}
		else if (tex_x > max_x)
		{
			break;
		}

		int span_x = (sprite->flip_h) ? (sprite_rect_width - tex_x) : tex_x;

		AlphaSpan* span = FrameInfo_GetAlphaSpan(frame_info, span_x);

		if (span->min > span->max)
		{
			continue;
		}

		int x_steps = 0;
		float test_step_x = tex_pos_x;

		while ((int)test_step_x == tex_x && (stripe + x_steps) < draw_end_x)
		{
			test_step_x += x_tex_step;
			x_steps++;
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

		bool next_tex_valid = false;
		int next_tex_y = -1;
		int next_d = -1;

		for (int y = draw_start_y; y < draw_end_y; y++)
		{
			int d = next_d;
			int tex_y = next_tex_y;

			if (!next_tex_valid)
			{
				d = (y - v_move_screen) * 256 - image->height * 128 + sprite_height * 128;
				tex_y = ((d * sprite_rect_height) / sprite_height) / 256;
			}

			next_tex_valid = false;

			if (tex_y < y_min)
			{
				continue;
			}
			else if (tex_y > y_max)
			{
				break;
			}

			unsigned char* tex_color = Image_Get(sprite->img, tex_x + (sprite_offset_x * sprite_rect_width), tex_y + (sprite_offset_y * sprite_rect_height));

			//alpha discard if possible
			if (sprite->img->numChannels >= 4)
			{
				if (tex_color[3] < 128)
				{
					continue;
				}
			}

			//apply transparency
			if (transparency > 1)
			{
				for (int l = 0; l < x_steps; l++)
				{
					unsigned char* old_color = Image_Get(image, stripe + l, y);

					//just do directly on the buffer pointer
					for (int k = 0; k < image->numChannels; k++)
					{
						old_color[k] = (old_color[k] / transparency) + (tex_color[k] / 2);
					}

					depth_buffer[(stripe + l) + y * image->width] = transform_y;
				}
			}
			else
			{
				unsigned char color[4] = { LIGHT_LUT[tex_color[0]][light], LIGHT_LUT[tex_color[1]][light], LIGHT_LUT[tex_color[2]][light], 255 };

				next_d = d;
				next_tex_y = tex_y;

				while (y < draw_end_y)
				{
					for (int l = 0; l < x_steps; l++)
					{
						int sl = (stripe + l);

						if (transform_y >= depth_buffer[sl + y * image->width])
						{
							continue;
						}

						Image_Set2(image, sl, y, color);
						depth_buffer[sl + y * image->width] = transform_y;
					}

					next_d = ((y + 1) - v_move_screen) * 256 - image->height * 128 + sprite_height * 128;
					next_tex_y = ((next_d * sprite_rect_height) / sprite_height) / 256;

					if (next_tex_y != tex_y)
					{
						break;
					}

					y++;

				}

				next_tex_valid = true;
			}
		}

		stripe += x_steps - 1;
	}
}

bool Video_SpriteSetup(Image* image, Sprite* sprite, float* depth_buffer, float p_x, float p_y, float p_dirX, float p_dirY, float p_planeX, float p_planeY)
{
	if (!depth_buffer)
	{
		return false;
	}

	//invalid scale
	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return false;
	}
	//completely transparent
	if (sprite->transparency >= 1)
	{
		return false;
	}

	//translate sprite position to relative to camera
	float local_sprite_x = sprite->x - p_x;
	float local_sprite_y = sprite->y - p_y;

	float inv_det = 1.0 / (p_planeX * p_dirY - p_dirX * p_planeY);

	float transform_x = inv_det * (p_dirY * local_sprite_x - p_dirX * local_sprite_y);
	float transform_y = inv_det * (-p_planeY * local_sprite_x + p_planeX * local_sprite_y);

	if (transform_y <= 0)
	{
		return false;
	}

	int sprite_screen_x = (int)((image->half_width) * (1 + transform_x / transform_y));

	int height_transform_y = fabs((int)(image->height / (transform_y)));

	int sprite_width = height_transform_y * (sprite->scale_x);
	int sprite_height = height_transform_y * (sprite->scale_y);

	if (sprite_height <= 0 || sprite_width <= 0)
	{
		return false;
	}

	int v_move_screen = (int)((sprite->v_offset * image->half_height) / transform_y);

	int sprite_half_width = sprite_width / 2;
	int sprite_half_height = sprite_height / 2;

	//calculate lowest and highest pixel to fill in current stripe
	int draw_start_y = -sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_start_y < 0) draw_start_y = 0;
	int draw_end_y = sprite_height / 2 + image->height / 2 + v_move_screen;
	if (draw_end_y >= image->height) draw_end_y = image->height - 1;

	if (draw_start_y >= image->height || draw_end_y <= 0)
	{
		return false;
	}

	int draw_start_x = -sprite_width / 2 + sprite_screen_x;
	if (draw_start_x < 0) draw_start_x = 0;
	int draw_end_x = sprite_width / 2 + sprite_screen_x;
	if (draw_end_x > image->width) draw_end_x = image->width;

	if (draw_start_x >= image->width || draw_end_x <= 0)
	{
		return false;
	}

	int safe_end_x = (draw_end_x >= image->width) ? draw_end_x - 1 : draw_end_x;
	int safe_end_y = (draw_end_y >= image->height) ? draw_end_y - 1 : draw_end_y;

	//all depth edges of a sprite rectangle
	float d0 = depth_buffer[draw_start_x + draw_start_y * image->width]; //top left edge
	float d1 = depth_buffer[safe_end_x + draw_start_y * image->width]; //top right edge
	float d2 = depth_buffer[draw_start_x + safe_end_y * image->width]; //bottom left edge
	float d3 = depth_buffer[safe_end_x + safe_end_y * image->width]; //bottom right edge

	//big bias for occlusion testing
	//since otherwise we get incorrect culling
	float check_size_x = 8 * sprite->scale_x;
	float check_size_y = 8 * sprite->scale_y;

	float t0 = inv_det * (-p_planeY * (local_sprite_x - check_size_x) + p_planeX * (local_sprite_y - check_size_y));
	float t1 = inv_det * (-p_planeY * (local_sprite_x + check_size_x) + p_planeX * (local_sprite_y - check_size_y));
	float t2 = inv_det * (-p_planeY * (local_sprite_x - check_size_x) + p_planeX * (local_sprite_y + check_size_y));
	float t3 = inv_det * (-p_planeY * (local_sprite_x + check_size_x) + p_planeX * (local_sprite_y + check_size_y));

	//check if all edges are fully occluded
	if (t0 >= d0 && t1 >= d1 && t2 >= d2 && t3 >= d3)
	{
		return false;
	}

	int sprite_light = sprite->light * 255;

	LightTile* light_tile = Map_GetLightTile((int)sprite->x, (int)sprite->y);
	int light = light_tile->light;

	light += sprite_light;
	light += light_tile->temp_light;

	if (light > 255)
	{
		light = 255;
	}
	else if (light < 0)
	{
		light = 0;
	}

	sprite->r_draw_start_x = draw_start_x;
	sprite->r_draw_end_x = draw_end_x;
	sprite->r_draw_start_y = draw_start_y;
	sprite->r_draw_end_y = draw_end_y;
	sprite->r_width = sprite_width;
	sprite->r_height = sprite_height;
	sprite->r_light = light;
	sprite->r_screen_x = sprite_screen_x;
	sprite->r_transform_y = transform_y;

	return true;
}

void Video_SpriteClipAndDraw(Image* image, Sprite* sprite, float* depth_buffer, int x_start, int x_end)
{
	int draw_start_x = sprite->r_draw_start_x;
	int draw_end_x = sprite->r_draw_end_x;

	if (draw_start_x < x_start) draw_start_x = x_start;
	if (draw_end_x > x_end) draw_end_x = x_end;

	//outside the clip range
	if (draw_start_x >= x_end || draw_end_x <= x_start)
	{
		return;
	}

	bool sprite_flip_h = sprite->flip_h;
	bool sprite_flip_v = sprite->flip_v;

	int light = sprite->r_light;

	float transform_y = sprite->r_transform_y;

	int v_move_screen = (int)((sprite->v_offset * image->half_height) / transform_y);

	int draw_start_y = sprite->r_draw_start_y;
	int draw_end_y = sprite->r_draw_end_y;

	int sprite_screen_x = sprite->r_screen_x;

	int sprite_width = sprite->r_width;
	int sprite_height = sprite->r_height;

	int sprite_half_width = sprite_width / 2;

	int frame = sprite->frame;

	const int h_frames = sprite->img->h_frames;
	const int v_frames = sprite->img->v_frames;

	int sprite_offset_x = (h_frames > 0) ? frame % h_frames : 0;
	int sprite_offset_y = (v_frames > 0) ? frame / h_frames : 0;

	sprite_offset_x += sprite->frame_offset_x;
	sprite_offset_y += sprite->frame_offset_y;

	int sprite_rect_width = (h_frames > 0) ? sprite->img->width / h_frames : sprite->img->width;
	int sprite_rect_height = (v_frames > 0) ? sprite->img->height / v_frames : sprite->img->height;

	FrameInfo* frame_info = Image_GetFrameInfo(sprite->img, frame + (sprite->frame_offset_x) + (sprite->frame_offset_y * sprite->img->h_frames));

	if (sprite_flip_h)
	{
		sprite_offset_x += 1;
	}

	float transparency = (sprite->transparency > 0) ? (1.0 / min(sprite->transparency, 1)) : 0;

	int min_x = (sprite_flip_h) ? ((sprite_rect_width)-(frame_info->max_real_x)) : frame_info->min_real_x;
	int max_x = (sprite_flip_h) ? ((sprite_rect_width)-(frame_info->min_real_x)) : frame_info->max_real_x;

	if (sprite_flip_h)
	{
		if (min_x > 0) min_x -= 1;
		max_x += 1;
	}

	float x_tex_step = (256 * (float)sprite_rect_width / (float)sprite_width) / 256.0;

	for (int stripe = draw_start_x; stripe < draw_end_x; stripe++)
	{
		float tex_pos_x = (256.0 * (stripe - (-sprite_half_width + sprite_screen_x)) * sprite_rect_width / sprite_width) / 256.0;
		int tex_x = tex_pos_x;

		if (tex_x < min_x)
		{
			continue;
		}
		else if (tex_x > max_x)
		{
			break;
		}

		int span_x = (sprite->flip_h) ? (sprite_rect_width - tex_x) : tex_x;

		AlphaSpan* span = FrameInfo_GetAlphaSpan(frame_info, span_x);

		if (span->min > span->max)
		{
			continue;
		}

		int x_steps = 0;
		float test_step_x = tex_pos_x;

		while ((int)test_step_x == tex_x && (stripe + x_steps) < draw_end_x)
		{
			test_step_x += x_tex_step;
			x_steps++;
		}

		int y_min = (sprite_flip_v) ? (sprite_rect_height - (span->max)) : span->min;
		int y_max = (sprite_flip_v) ? (sprite_rect_height - (span->min)) : span->max;

		if (sprite_flip_v)
		{
			if (y_min > 0) y_min -= 1;
			y_max += 1;
		}
		if (sprite_flip_h)
		{
			tex_x -= 1;
			tex_x = -tex_x;
		}

		bool next_tex_valid = false;
		int next_tex_y = -1;
		int next_d = -1;

		for (int y = draw_start_y; y < draw_end_y; y++)
		{
			int d = next_d;
			int tex_y = next_tex_y;

			if (!next_tex_valid)
			{
				d = (y - v_move_screen) * 256 - image->height * 128 + sprite_height * 128;
				tex_y = ((d * sprite_rect_height) / sprite_height) / 256;
			}

			next_tex_valid = false;

			if (tex_y < y_min)
			{
				continue;
			}
			else if (tex_y > y_max)
			{
				break;
			}

			unsigned char* tex_color = Image_Get(sprite->img, tex_x + (sprite_offset_x * sprite_rect_width), tex_y + (sprite_offset_y * sprite_rect_height));

			//alpha discard if possible
			if (sprite->img->numChannels >= 4)
			{
				if (tex_color[3] < 128)
				{
					continue;
				}
			}

			unsigned char color[4] = { LIGHT_LUT[tex_color[0]][light], LIGHT_LUT[tex_color[1]][light], LIGHT_LUT[tex_color[2]][light], 255 };

			next_d = d;
			next_tex_y = tex_y;

			while (y < draw_end_y)
			{
				for (int l = 0; l < x_steps; l++)
				{
					int sl = (stripe + l);

					if (transform_y >= depth_buffer[sl + y * image->width])
					{
						continue;
					}

					Image_Set2(image, sl, y, color);
					depth_buffer[sl + y * image->width] = transform_y;
				}

				next_d = ((y + 1) - v_move_screen) * 256 - image->height * 128 + sprite_height * 128;
				next_tex_y = ((next_d * sprite_rect_height) / sprite_height) / 256;

				if (next_tex_y != tex_y)
				{
					break;
				}

				y++;

			}

			next_tex_valid = true;
		}

		stripe += x_steps - 1;
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

	float d_scale_x = 1.0 / p_scaleX;
	float d_scale_y = 1.0 / p_scaleY;

	const int rect_width = texture->width * p_scaleX;
	const int rect_height = texture->height * p_scaleY;

	for (int x = 0; x < rect_width; x++)
	{
		for (int y = 0; y < rect_height; y++)
		{
			unsigned char* color = Image_Get(texture, x * d_scale_x, y * d_scale_y);

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

void Video_DrawScreenSprite(Image* image, Sprite* sprite)
{
	if (sprite->scale_x <= 0 || sprite->scale_y <= 0)
	{
		return;
	}

	//completely transparent
	if (sprite->transparency >= 1)
	{
		return;
	}

	int render_scale = Render_GetRenderScale();

	int pix_x = sprite->x * image->width;
	int pix_y = sprite->y * image->height;

	//out of bounds
	if (pix_x < 0 || pix_x >= image->width || pix_y < 0 || pix_y >= image->height)
	{
		return;
	}

	int light = sprite->light * 255.0;

	if (light < 0) light = 0;
	else if (light > 255) light = 255;

	int frame = sprite->frame;

	const int h_frames = sprite->img->h_frames;
	const int v_frames = sprite->img->v_frames;

	int offset_x = (h_frames > 0) ? frame % h_frames : 0;
	int offset_y = (v_frames > 0) ? frame / h_frames : 0;

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

	FrameInfo* frame_info = Image_GetFrameInfo(sprite->img, frame + (sprite->frame_offset_x) + (sprite->frame_offset_y * sprite->img->h_frames));

	int min_x = (sprite->flip_h) ? (rect_width - (frame_info->max_real_x * scale_x)) : frame_info->min_real_x * scale_x;
	int max_x = (sprite->flip_h) ? (rect_width - (frame_info->min_real_x * scale_x)) : frame_info->max_real_x * scale_x;

	if (sprite->flip_h)
	{
		if (min_x > 0) min_x -= 1;
		max_x += 1;
	}

	float x_tex_step = 0;
	float x_tex_pos = 0;
	
	for (int x = min_x; x <= max_x; x++)
	{
		if (sprite->flip_h)
		{
			x_tex_pos = (offset_x * rect_width - x - 1) * d_scale_x;
			x_tex_step = -d_scale_x;
		}
		else
		{
			x_tex_pos = (x + offset_x * rect_width) * d_scale_x;
			x_tex_step = d_scale_x;
		}

		int tx = (int)x_tex_pos;
		int span_x = (sprite->flip_h) ? (rect_width - x - 1) : x;

		AlphaSpan* span = FrameInfo_GetAlphaSpan(frame_info, span_x * d_scale_x);

		if (span->min > span->max)
		{
			continue;
		}

		int x_steps = 0;

		float test_pos_x = x_tex_pos;
		//calculate how many steps we can take
		while ((int)test_pos_x == tx && (x + x_steps) <= max_x)
		{
			test_pos_x += x_tex_step;
			x_steps++;
		}

		int y_min = (sprite->flip_v) ? (rect_height - (span->max * scale_y)) : span->min * scale_y;
		int y_max = (sprite->flip_v) ? (rect_height - (span->min * scale_y)) : span->max * scale_y;

		if (sprite->flip_v)
		{
			if (y_min > 0) y_min -= 1;
			y_max += 1;
		}

		for (int y = y_min; y < y_max; y++)
		{
			float y_tex_pos = 0;
			float y_tex_step = 0;

			if (sprite->flip_v)
			{
				y_tex_pos = (offset_y * rect_height - y - 1) * d_scale_y;
				y_tex_step = -d_scale_y;
			}
			else
			{
				y_tex_pos = (y + offset_y * rect_height) * d_scale_y;
				y_tex_step = d_scale_y;
			}

			int ty = (int)y_tex_pos;

			unsigned char* color = Image_Get(sprite->img, tx, ty);

			//perform alpha discarding if there is 4 color channels
			//most pixels should be discarded with alpha spans
			if (sprite->img->numChannels >= 4)
			{
				if (color[3] < 128)
				{
					continue;
				}
			}

			unsigned char clr[4] = { LIGHT_LUT[color[0]][light], LIGHT_LUT[color[1]][light], LIGHT_LUT[color[2]][light], 255 };

			//apply transparency
			if (transparency > 1)
			{
				
			}
			else
			{
				//try to set as many pixels in a row as possible
				while (y < y_max)
				{
					for (int l = 0; l < x_steps; l++)
					{
						Image_Set2(image, (x + pix_x + l), y + pix_y, clr);
					}

					y_tex_pos += y_tex_step;

					if ((int)y_tex_pos != ty)
					{
						break;
					}

					y++;
				}
			}		
		}
		x += x_steps - 1;
	}
}

void Video_Shade(Image* image, ShaderFun shader_fun, int x0, int y0, int x1, int y1)
{
	if (!shader_fun)
	{
		return;
	}

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
