#include "r_common.h"

#include <string.h>
#include <stdlib.h>
#include <glad/glad.h>
#include "g_common.h"
#include <main.h>
#include <windows.h>

#define NUM_RENDER_THREADS 8
#define MAX_DRAWSPRITES_PER_THREAD 10
#define MAX_DRAWSPRITES 1024
#define MAX_SCREENSPRITES 10
#define MAX_SCREENTEXTS 10

typedef enum
{
	TS__NONE,
	TS__SLEEPING,
	TS__WORKING,
} ThreadState;

typedef enum
{
	TWT__NONE,

	TWT__RAYCAST,
	TWT__DRAW_SPRITES
} ThreadWorkType;

typedef struct
{
	ThreadState state;
	ThreadWorkType work_type;

	HANDLE thread_handle;

	CRITICAL_SECTION mutex;
	HANDLE start_work_event;
	HANDLE active_event;
	HANDLE finished_work_event;

	int draw_sprite_indexes[MAX_DRAWSPRITES_PER_THREAD];
	int num_draw_sprites;
	int x_start, x_end;

	bool shutdown;
} RenderThread;

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

	RenderThread threads[NUM_RENDER_THREADS];

	Sprite* draw_sprites[MAX_DRAWSPRITES];
	int num_draw_sprites;

	Sprite* screen_sprites[MAX_SCREENSPRITES];
	int num_screen_sprites;

	int main_thread_draw_sprite_indices[MAX_DRAWSPRITES];
	int num_main_thread_draw_sprites;

	CRITICAL_SECTION object_mutex;
	HANDLE main_thread_active_event;
	HANDLE main_thread_standby_event;
	HANDLE main_thread_handle;
	HANDLE redraw_sprites_event;
	HANDLE redraw_walls_event;

	bool stall_main_thread;
	bool size_changed;
} RenderCore;

static RenderCore s_renderCore;

static void Render_ThreadMutexLock(RenderThread* thread)
{
	EnterCriticalSection(&thread->mutex);
}
static void Render_ThreadMutexUnlock(RenderThread* thread)
{
	LeaveCriticalSection(&thread->mutex);
}

static void Render_MainThreadLoop()
{
	GLFWwindow* window = Engine_GetWindow();

	glfwMakeContextCurrent(window);

	float view_x = 0;
	float view_y = 0;
	float view_dir_x = 0;
	float view_dir_y = 0;
	float view_plane_x = 0;
	float view_plane_y = 0;

	SetEvent(s_renderCore.main_thread_active_event);

	while (!glfwWindowShouldClose(window))
	{
		float aspect = Render_GetWindowAspect();

		Player_GetView(&view_x, &view_y, &view_dir_x, &view_dir_y, &view_plane_x, &view_plane_y);

		view_plane_x *= aspect;
		view_plane_y *= aspect;

		Render_View(view_x, view_y, view_dir_x, view_dir_y, view_plane_x, view_plane_y);

		glfwSwapBuffers(window);

		if (Game_GetState() == GS__EXIT)
		{
			break;
		}

		if (s_renderCore.size_changed)
		{
			glViewport(0, 0, s_renderCore.win_w, s_renderCore.win_h);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_renderCore.w, s_renderCore.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			s_renderCore.size_changed = false;
		}

		if (s_renderCore.stall_main_thread)
		{
			ResetEvent(s_renderCore.main_thread_active_event);
			SetEvent(s_renderCore.main_thread_standby_event);

			while (s_renderCore.stall_main_thread) {};

			//resume
			ResetEvent(s_renderCore.main_thread_standby_event);
			SetEvent(s_renderCore.main_thread_active_event);
		}
	}
}

static void Render_ThreadLoop(RenderThread* thread)
{
	GameAssets* assets = Game_GetAssets();

	SetEvent(thread->finished_work_event);

	while (!thread->shutdown)
	{
		//wait for work
		WaitForSingleObject(thread->start_work_event, INFINITE);

		//set active state
		ResetEvent(thread->finished_work_event);
		ResetEvent(thread->start_work_event);

		SetEvent(thread->active_event);
		thread->state = TS__WORKING;

		//do the work
		Render_ThreadMutexLock(thread);

		switch (thread->work_type)
		{
		case TWT__RAYCAST:
		{
			Video_RaycastMap(&s_renderCore.wall_buffer, &assets->wall_textures, s_renderCore.wall_depth_buffer, s_renderCore.draw_spans, thread->x_start, thread->x_end, s_renderCore.view_x, s_renderCore.view_y, s_renderCore.dir_x, s_renderCore.dir_y, s_renderCore.plane_x, s_renderCore.plane_y);
			break;
		}
		case TWT__DRAW_SPRITES:
		{
			for (int i = 0; i < thread->num_draw_sprites; i++)
			{
				int draw_sprite_index = thread->draw_sprite_indexes[i];
				Sprite* sprite = s_renderCore.draw_sprites[draw_sprite_index];

				Video_DrawSprite(&s_renderCore.framebuffer, sprite, s_renderCore.depth_buffer, s_renderCore.view_x, s_renderCore.view_y, s_renderCore.dir_x, s_renderCore.dir_y, s_renderCore.plane_x, s_renderCore.plane_y);
			}

			thread->num_draw_sprites = 0;
			break;
		}
		default:
			break;
		}

		Render_ThreadMutexUnlock(thread);

		//set inactive state
		ResetEvent(thread->active_event);
		SetEvent(thread->finished_work_event);

		thread->state = TS__SLEEPING;
	}
}

static void Render_WaitForThread(RenderThread* thread)
{
	WaitForSingleObject(thread->finished_work_event, INFINITE);
}

static void Render_WaitForAllThreads()
{
	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		Render_WaitForThread(thr);
	}
}

static void Render_SetWorkStateForAllThreads(ThreadWorkType work_type)
{
	//set start work event for all threads
	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];
		
		Render_ThreadMutexLock(thr);

		thr->work_type = work_type;

		SetEvent(thr->start_work_event);
		
		WaitForSingleObject(thr->active_event, INFINITE);

		Render_ThreadMutexUnlock(thr);
	}
}

static void Render_StallMainThread()
{
	s_renderCore.stall_main_thread = true;

	WaitForSingleObject(s_renderCore.main_thread_standby_event, INFINITE);
}

static void Render_ResumeMainThread()
{
	s_renderCore.stall_main_thread = false;

	WaitForSingleObject(s_renderCore.main_thread_active_event, INFINITE);
}

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

static void Render_SetThreadsStartAndEnd(int width)
{
	float num_threads = NUM_RENDER_THREADS;
	int slice = ceil((float)width / num_threads);

	int x_start = 0;
	int x_end = slice;

	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		thr->x_start = x_start;
		thr->x_end = x_end;

		x_start = x_end;
		x_end += slice;
	}
}
static void Render_ThreadAssignDrawSprites()
{
	s_renderCore.num_main_thread_draw_sprites = 0;

	if (s_renderCore.num_draw_sprites <= 0)
	{
		return;
	}

	int sprites_per_thread = ceil((float)s_renderCore.num_draw_sprites / (float)NUM_RENDER_THREADS);

	if (sprites_per_thread <= 0)
	{
		sprites_per_thread = 1;
	}
	else if (sprites_per_thread >= MAX_DRAWSPRITES_PER_THREAD)
	{
		sprites_per_thread = MAX_DRAWSPRITES_PER_THREAD;
	}

	int sprite_index = 0;

	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* render_thread = &s_renderCore.threads[i];

		if (sprite_index >= s_renderCore.num_draw_sprites)
		{
			break;
		}

		for (int k = 0; k < sprites_per_thread; k++)
		{
			if (sprite_index >= s_renderCore.num_draw_sprites)
			{
				break;
			}

			render_thread->draw_sprite_indexes[k] = sprite_index++;
			render_thread->num_draw_sprites++;
		}
	}

	//we have some sprites remaining
	if (sprite_index < s_renderCore.num_draw_sprites)
	{
		int remainder = s_renderCore.num_draw_sprites - sprite_index;

		//assign leftovers for the main thread
		for (int i = 0; i < remainder; i++)
		{
			s_renderCore.main_thread_draw_sprite_indices[i] = sprite_index++;
			s_renderCore.num_main_thread_draw_sprites++;

			if (sprite_index >= s_renderCore.num_draw_sprites)
			{
				break;
			}
		}
	}

}

void Render_WindowCallback(GLFWwindow* window, int width, int height)
{
	s_renderCore.win_w = width;
	s_renderCore.win_h = height;
	s_renderCore.size_changed = true;
}

void Render_Init(int width, int height)
{
	memset(&s_renderCore, 0, sizeof(RenderCore));

	Video_Setup();

	Image_Create(&s_renderCore.framebuffer, width, height, 4);

	Image_Create(&s_renderCore.clear_framebuffer, width, height, 4);

	Image_Create(&s_renderCore.wall_buffer, width, height, 4);

	glfwMakeContextCurrent(NULL);

	s_renderCore.win_w = width;
	s_renderCore.win_h = height;

	DWORD render_thread_id = 0;
	s_renderCore.main_thread_handle = CreateThread(NULL, 0, Render_MainThreadLoop, NULL, 0, &render_thread_id);

	InitializeCriticalSection(&s_renderCore.object_mutex);
	s_renderCore.main_thread_active_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	s_renderCore.main_thread_standby_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	s_renderCore.redraw_sprites_event = CreateEvent(NULL, TRUE, TRUE, NULL);
	s_renderCore.redraw_walls_event = CreateEvent(NULL, TRUE, TRUE, NULL);

	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		InitializeCriticalSection(&thr->mutex);
		thr->start_work_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		thr->finished_work_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		thr->active_event = CreateEvent(NULL, TRUE, FALSE, NULL);

		thr->thread_handle = CreateThread(NULL, 0, Render_ThreadLoop, thr, 0, &render_thread_id);
	}
	Render_ResizeWindow(width, height);

	Text_LoadFont("assets/myFont.json", "assets/myFont.png", &s_renderCore.font_data);
}

void Render_ShutDown()
{
	//wait for main thread to exit
	WaitForSingleObject(s_renderCore.main_thread_handle, INFINITE);
	CloseHandle(s_renderCore.main_thread_handle);

	//shut down render threads
	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		//this will exit the thread loop
		thr->shutdown = true;
		SetEvent(thr->start_work_event);

		WaitForSingleObject(thr->thread_handle, INFINITE);

		DeleteCriticalSection(&thr->mutex);
		CloseHandle(thr->thread_handle);
		CloseHandle(thr->active_event);
		CloseHandle(thr->finished_work_event);
		CloseHandle(thr->start_work_event);
	}

	DeleteCriticalSection(&s_renderCore.object_mutex);
	CloseHandle(s_renderCore.main_thread_active_event);
	CloseHandle(s_renderCore.main_thread_standby_event);
	CloseHandle(s_renderCore.redraw_sprites_event);
	CloseHandle(s_renderCore.redraw_walls_event);

	Image_Destruct(&s_renderCore.clear_framebuffer);
	Image_Destruct(&s_renderCore.framebuffer);
	Image_Destruct(&s_renderCore.wall_buffer);
	Image_Destruct(&s_renderCore.font_data.font_image);

	free(s_renderCore.depth_buffer);
	free(s_renderCore.wall_depth_buffer);
	free(s_renderCore.draw_spans);
}


void Render_Loop()
{
	float view_x = 0;
	float view_y = 0;
	float view_dir_x = 0;
	float view_dir_y = 0;
	float view_plane_x = 0;
	float view_plane_y = 0;

	Player_GetView(&view_x, &view_y, &view_dir_x, &view_dir_y, &view_plane_x, &view_plane_y);

	Render_View(view_x, view_y, view_dir_x, view_dir_y, view_plane_x, view_plane_y);
}

void Render_LockObjectMutex()
{
	EnterCriticalSection(&s_renderCore.object_mutex);
}

void Render_UnlockObjectMutex()
{
	LeaveCriticalSection(&s_renderCore.object_mutex);
}

void Render_FinishAndStall()
{
	Render_StallMainThread();

	Render_WaitForAllThreads();
}

void Render_Resume()
{
	//not stalled
	if (!s_renderCore.stall_main_thread)
	{
		return;
	}

	Render_ResumeMainThread();
}

void Render_AddSpriteToQueue(Sprite* sprite)
{
	if (s_renderCore.num_draw_sprites >= MAX_DRAWSPRITES)
	{
		return;
	}

	s_renderCore.draw_sprites[s_renderCore.num_draw_sprites++] = sprite;
}

void Render_AddScreenSpriteToQueue(Sprite* sprite)
{
	if (s_renderCore.num_screen_sprites >= MAX_SCREENSPRITES)
	{
		return;
	}

	s_renderCore.screen_sprites[s_renderCore.num_screen_sprites++] = sprite;
}


void Render_RedrawWalls()
{
	s_renderCore.redraw_walls = true;
	SetEvent(s_renderCore.redraw_walls_event);
}

void Render_RedrawSprites()
{
	s_renderCore.redraw_sprites = true;
	SetEvent(s_renderCore.redraw_sprites_event);
}

void Render_ResizeWindow(int width, int height)
{	
	Render_FinishAndStall();
	Render_SetThreadsStartAndEnd(width);

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
	unsigned char ceilling_color[4] = { 68, 125, 128, 255 };
	unsigned char black_color[4] = { 0, 0, 0, 255 };
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
				Image_Set2(&s_renderCore.clear_framebuffer, x, y, black_color);
			}
		}
	}

	s_renderCore.w = width;
	s_renderCore.h = height;

	Render_RedrawSprites();
	Render_RedrawWalls();

	s_renderCore.size_changed = true;

	Render_Resume();
}

void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
	GameAssets* assets = Game_GetAssets();
	GameState game_state = Game_GetState();

	bool redraw_walls = s_renderCore.redraw_walls;
	bool redraw_sprites = s_renderCore.redraw_sprites;

	//check if we need to redraw walls
	redraw_walls = Render_CheckForRedraw(x, y, dir_x, dir_y, plane_x, plane_y);
	
	//store view information
	s_renderCore.view_x = x;
	s_renderCore.view_y = y;
	s_renderCore.dir_x = dir_x;
	s_renderCore.dir_y = dir_y;
	s_renderCore.plane_x = plane_x;
	s_renderCore.plane_y = plane_y;

	if (WaitForSingleObject(s_renderCore.redraw_walls_event, (DWORD)0) != WAIT_TIMEOUT)
	{
		redraw_walls = true;
	}
	if (WaitForSingleObject(s_renderCore.redraw_sprites_event, (DWORD)0) != WAIT_TIMEOUT)
	{
		redraw_sprites = true;
	}

	//raycast and render all wall tiles
	if (redraw_walls && game_state == GS__LEVEL)
	{
		Render_WaitForAllThreads();

		Image_Copy(&s_renderCore.wall_buffer, &s_renderCore.clear_framebuffer);

		//clear wall depth buffer
		memset(s_renderCore.wall_depth_buffer, (int)DEPTH_CLEAR, sizeof(float) * s_renderCore.w * s_renderCore.h);

		Render_SetWorkStateForAllThreads(TWT__RAYCAST);
	}
	
	//draw map objects and player sprites
	if (redraw_sprites || redraw_walls)
	{
		Render_WaitForAllThreads();
		
		Image_Copy(&s_renderCore.framebuffer, &s_renderCore.wall_buffer);
		memcpy(s_renderCore.depth_buffer, s_renderCore.wall_depth_buffer, sizeof(float) * s_renderCore.w * s_renderCore.h);

		Render_LockObjectMutex();

		Game_Draw(&s_renderCore.framebuffer, &s_renderCore.font_data, s_renderCore.depth_buffer, s_renderCore.draw_spans, x, y, dir_x, dir_y, plane_x, plane_y);
		
		//Draw world sprites
		if (s_renderCore.num_draw_sprites > 0)
		{
			Render_ThreadAssignDrawSprites();
			Render_SetWorkStateForAllThreads(TWT__DRAW_SPRITES);

			for (int i = 0; i < s_renderCore.num_main_thread_draw_sprites; i++)
			{
				int sprite_index = s_renderCore.main_thread_draw_sprite_indices[i];
				Sprite* sprite = s_renderCore.draw_sprites[sprite_index];

				Video_DrawSprite(&s_renderCore.framebuffer, sprite, s_renderCore.depth_buffer, s_renderCore.view_x, s_renderCore.view_y, s_renderCore.dir_x, s_renderCore.dir_y, s_renderCore.plane_x, s_renderCore.plane_y);
			}

			Render_WaitForAllThreads();
		}
		//draw screen sprites
		Game_DrawHud(&s_renderCore.framebuffer, &s_renderCore.font_data);
		if (s_renderCore.num_screen_sprites > 0)
		{
			for (int i = 0; i < s_renderCore.num_screen_sprites; i++)
			{
				Sprite* sprite = s_renderCore.screen_sprites[i];
				//Video_DrawScreenSprite(&s_renderCore.framebuffer, sprite);
			}
		}
		
		Render_UnlockObjectMutex();
	}
	if (redraw_sprites || redraw_walls)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, s_renderCore.w, s_renderCore.h, GL_RGBA, GL_UNSIGNED_BYTE, s_renderCore.framebuffer.data);
	}

	//render fullscreen quad
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	//reset stuff
	ResetEvent(s_renderCore.redraw_walls_event);
	ResetEvent(s_renderCore.redraw_sprites_event);

	s_renderCore.redraw_walls = false;
	s_renderCore.redraw_sprites = false;

	s_renderCore.num_draw_sprites = 0;
	s_renderCore.num_screen_sprites = 0;
	s_renderCore.num_main_thread_draw_sprites = 0;
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
