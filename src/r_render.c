#include "r_common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glad/glad.h>
#include "g_common.h"
#include <main.h>
#include <windows.h>

#define NUM_RENDER_THREADS 8
#define MAX_DRAWSPRITES 1024
#define MAX_SCREENSPRITES 10
#define MAX_SCREENTEXTS 10


static const char* VERTEX_SHADER_SOURCE[] =
{
	"#version 330 core \n"

	"layout(location = 0) in vec3 a_Pos;\n"
	"layout(location = 1) in vec2 a_texCoords;\n"

	"out vec2 TexCoords;\n"

	"void main()\n"
	"{\n"
		"TexCoords = a_texCoords;\n"
		"gl_Position = vec4(a_Pos.x, -a_Pos.y, 1.0, 1.0);\n"
	"}\n"
};
static const char* FRAGMENT_SHADER_SOURCE[] =
{
	"#version 330 core \n"

	"out vec4 FragColor;\n"

	"in vec2 TexCoords;\n"

	"uniform sampler2D scene_texture;\n"

	"void main()\n"
	"{\n"
		"vec4 TexColor = texture(scene_texture, TexCoords);\n"

		"FragColor = TexColor;\n"
	"}\n"
};

typedef enum
{
	TS__NONE,
	TS__SLEEPING,
	TS__WORKING,
} ThreadState;

typedef enum
{
	TWT__NONE,

	TWT__SHADER,
	TWT__DRAW_LEVEL
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

	int x_start, x_end;

	bool shutdown;
} RenderThread;

typedef struct
{
	GLuint gl_texture, gl_vao, gl_vbo, gl_shader;

	Image framebuffer;

	DrawSpan* draw_spans;

	float* depth_buffer;

	int w, h;
	int win_w, win_h;

	float view_x, view_y;
	float dir_x, dir_y;
	float plane_x, plane_y;

	int scale;
	bool is_fullscreen;

	FontData font_data;

	RenderThread threads[NUM_RENDER_THREADS];

	Sprite* draw_sprites[MAX_DRAWSPRITES];
	int num_draw_sprites;

	int sorted_draw_sprite_indices[MAX_DRAWSPRITES];
	int num_sorted_draw_sprites;

	Sprite* screen_sprites[MAX_SCREENSPRITES];
	int num_screen_sprites;

	int main_thread_draw_sprite_indices[MAX_DRAWSPRITES];
	int num_main_thread_draw_sprites;

	ShaderFun fullscreen_shader_fun;

	CRITICAL_SECTION object_mutex;
	CRITICAL_SECTION main_thread_mutex;
	CONDITION_VARIABLE main_thread_cv;
	HANDLE main_thread_active_event;
	HANDLE main_thread_standby_event;
	HANDLE main_thread_handle;
	HANDLE redraw_sprites_event;
	HANDLE redraw_walls_event;

	bool stall_main_thread;
	bool size_changed;
	bool main_thread_shutdown;
} RenderCore;

static RenderCore s_renderCore;

static bool Shader_checkCompileErrors(unsigned int p_object, const char* p_type)
{
	int success;
	char infoLog[1024];
	if (p_type != "PROGRAM")
	{
		glGetShaderiv(p_object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);

			return false;
		}
	}
	else
	{
		glGetProgramiv(p_object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);
			return false;
		}
	}
	return true;
}

static void Render_SizeChanged()
{
	EnterCriticalSection(&s_renderCore.main_thread_mutex);

	s_renderCore.size_changed = true;

	LeaveCriticalSection(&s_renderCore.main_thread_mutex);
}

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

	while (!glfwWindowShouldClose(window) || !s_renderCore.main_thread_shutdown)
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

		EnterCriticalSection(&s_renderCore.main_thread_mutex);
		if (s_renderCore.size_changed)
		{
			glViewport(0, 0, s_renderCore.win_w, s_renderCore.win_h);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_renderCore.w, s_renderCore.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			s_renderCore.size_changed = false;
		}
		//check for stall request
		{
			if (s_renderCore.stall_main_thread)
			{
				ResetEvent(s_renderCore.main_thread_active_event);
				SetEvent(s_renderCore.main_thread_standby_event);

				while (s_renderCore.stall_main_thread)
				{
					SleepConditionVariableCS(&s_renderCore.main_thread_cv, &s_renderCore.main_thread_mutex, INFINITE);
				}

				//resume
				ResetEvent(s_renderCore.main_thread_standby_event);
				SetEvent(s_renderCore.main_thread_active_event);
			}
		}
		LeaveCriticalSection(&s_renderCore.main_thread_mutex);
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
		case TWT__SHADER:
		{
			Video_Shade(&s_renderCore.framebuffer, s_renderCore.fullscreen_shader_fun, thread->x_start, 0, thread->x_end, s_renderCore.h);
			break;
		}
		case TWT__DRAW_LEVEL:
		{
			Video_RaycastMap(&s_renderCore.framebuffer, &assets->wall_textures, s_renderCore.depth_buffer, s_renderCore.draw_spans, thread->x_start, thread->x_end, s_renderCore.view_x, s_renderCore.view_y, s_renderCore.dir_x, s_renderCore.dir_y, s_renderCore.plane_x, s_renderCore.plane_y);

			for (int i = 0; i < s_renderCore.num_sorted_draw_sprites; i++)
			{
				int sprite_index = s_renderCore.sorted_draw_sprite_indices[i];
				Sprite* sprite = s_renderCore.draw_sprites[sprite_index];

				if (!sprite)
				{
					continue;
				}

				Video_SpriteClipAndDraw(&s_renderCore.framebuffer, sprite, s_renderCore.depth_buffer, thread->x_start, thread->x_end);
			}

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
	WakeConditionVariable(&s_renderCore.main_thread_cv);

	WaitForSingleObject(s_renderCore.main_thread_active_event, INFINITE);
}

static bool Render_CheckForRedraw(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
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

static bool Render_SetupGL(int width, int height)
{
	unsigned shader_id = 0;

	//setup shaders
	{
		GLuint vertex_id, frag_id;
		vertex_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_id, 1, &VERTEX_SHADER_SOURCE, NULL);
		glCompileShader(vertex_id);

		if (!Shader_checkCompileErrors(vertex_id, "Vertex"))
		{
			return false;
		}
		frag_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_id, 1, &FRAGMENT_SHADER_SOURCE, NULL);
		glCompileShader(frag_id);

		if (!Shader_checkCompileErrors(frag_id, "Fragment"))
		{
			return false;
		}

		shader_id = glCreateProgram();
		glAttachShader(shader_id, vertex_id);
		glAttachShader(shader_id, frag_id);

		glLinkProgram(shader_id);
		if (!Shader_checkCompileErrors(shader_id, "Program"))
			return false;

		glDeleteShader(vertex_id);
		glDeleteShader(frag_id);
	}

	//setup main screen texture
	GLuint texture = 0;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//setup quad vao and vbo
	unsigned vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	unsigned vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	const float quad_vertices[16] =
	{ -1, -1, 0, 0,
			-1,  1, 0, 1,
			1,  1, 1, 1,
			1, -1, 1, 0,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(1);

	//setup gl state
	glUseProgram(shader_id);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	glViewport(0, 0, width, height);

	glBindTexture(GL_TEXTURE_2D, texture);

	s_renderCore.gl_shader = shader_id;
	s_renderCore.gl_texture = texture;
	s_renderCore.gl_vao = vao;
	s_renderCore.gl_vbo = vbo;

	return true;
}

void Render_WindowCallback(GLFWwindow* window, int width, int height)
{
	s_renderCore.win_w = width;
	s_renderCore.win_h = height;

	Render_SizeChanged();
}

bool Render_Init(int width, int height)
{
	memset(&s_renderCore, 0, sizeof(RenderCore));

	Video_Setup();

	if (!Render_SetupGL(width, height))
	{
		return;
	}

	if (!Text_LoadFont("assets/font/font.json", "assets/font/font.png", &s_renderCore.font_data))
	{
		return false;
	}

	if (!Image_Create(&s_renderCore.framebuffer, width, height, 4))
	{
		return false;
	}

	glfwMakeContextCurrent(NULL);

	s_renderCore.win_w = width;
	s_renderCore.win_h = height;

	DWORD render_thread_id = 0;
	s_renderCore.main_thread_handle = CreateThread(NULL, 0, Render_MainThreadLoop, NULL, 0, &render_thread_id);

	InitializeCriticalSection(&s_renderCore.object_mutex);
	InitializeCriticalSection(&s_renderCore.main_thread_mutex);
	InitializeConditionVariable(&s_renderCore.main_thread_cv);
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

	return true;
}

void Render_ShutDown()
{
	//wait for main thread to exit
	s_renderCore.main_thread_shutdown = true;
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

	DeleteCriticalSection(&s_renderCore.main_thread_mutex);
	DeleteCriticalSection(&s_renderCore.object_mutex);
	CloseHandle(s_renderCore.main_thread_active_event);
	CloseHandle(s_renderCore.main_thread_standby_event);
	CloseHandle(s_renderCore.redraw_sprites_event);
	CloseHandle(s_renderCore.redraw_walls_event);

	Image_Destruct(&s_renderCore.framebuffer);
	Image_Destruct(&s_renderCore.font_data.font_image);

	free(s_renderCore.depth_buffer);
	free(s_renderCore.draw_spans);
}


void Render_LockThreadsMutex()
{
	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		Render_ThreadMutexLock(thr);
	}
}

void Render_UnlockThreadsMutex()
{
	for (int i = 0; i < NUM_RENDER_THREADS; i++)
	{
		RenderThread* thr = &s_renderCore.threads[i];

		Render_ThreadMutexUnlock(thr);
	}
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

void Render_QueueFullscreenShader(ShaderFun shader_fun)
{
	s_renderCore.fullscreen_shader_fun = shader_fun;
}


void Render_RedrawWalls()
{
	SetEvent(s_renderCore.redraw_walls_event);
}

void Render_RedrawSprites()
{
	SetEvent(s_renderCore.redraw_sprites_event);
}

void Render_ResizeWindow(int width, int height)
{
	Render_FinishAndStall();
	Render_SetThreadsStartAndEnd(width);

	Image_Resize(&s_renderCore.framebuffer, width, height);

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

	s_renderCore.w = width;
	s_renderCore.h = height;

	Render_RedrawSprites();
	Render_RedrawWalls();

	Render_SizeChanged();

	Render_Resume();
}

void Render_View(float x, float y, float dir_x, float dir_y, float plane_x, float plane_y)
{
	GameAssets* assets = Game_GetAssets();
	GameState game_state = Game_GetState();

	//store view information
	s_renderCore.view_x = x;
	s_renderCore.view_y = y;
	s_renderCore.dir_x = dir_x;
	s_renderCore.dir_y = dir_y;
	s_renderCore.plane_x = plane_x;
	s_renderCore.plane_y = plane_y;

	//clear image to black
	Image_Clear(&s_renderCore.framebuffer, 0);

	if (game_state == GS__LEVEL)
	{
		Render_LockObjectMutex();

		Game_Draw(&s_renderCore.framebuffer, &s_renderCore.font_data, s_renderCore.depth_buffer, s_renderCore.draw_spans, x, y, dir_x, dir_y, plane_x, plane_y);

		//setup world draw sprites
		int index = 0;

		for (int i = 0; i < s_renderCore.num_draw_sprites; i++)
		{
			Sprite* sprite = s_renderCore.draw_sprites[i];

			if (Video_SpriteSetup(&s_renderCore.framebuffer, sprite, s_renderCore.depth_buffer, x, y, dir_x, dir_y, plane_x, plane_y))
			{
				s_renderCore.sorted_draw_sprite_indices[index++] = i;
			}
		}

		s_renderCore.num_sorted_draw_sprites = index;


		//clear wall depth buffer
		memset(s_renderCore.depth_buffer, (int)DEPTH_CLEAR, sizeof(float) * s_renderCore.w * s_renderCore.h);

		//set work state for all thread
		Render_SetWorkStateForAllThreads(TWT__DRAW_LEVEL);

		//wait for it to finish
		Render_WaitForAllThreads();

		Render_UnlockObjectMutex();

		Game_DrawHud(&s_renderCore.framebuffer, &s_renderCore.font_data);
	}
	else
	{
		Game_Draw(&s_renderCore.framebuffer, &s_renderCore.font_data, s_renderCore.depth_buffer, s_renderCore.draw_spans, x, y, dir_x, dir_y, plane_x, plane_y);
	}
	if (s_renderCore.fullscreen_shader_fun)
	{
		Render_WaitForAllThreads();

		Render_SetWorkStateForAllThreads(TWT__SHADER);

		Render_WaitForAllThreads();
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, s_renderCore.w, s_renderCore.h, GL_RGBA, GL_UNSIGNED_BYTE, s_renderCore.framebuffer.data);

	//render fullscreen quad
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	//reset stuff
	s_renderCore.num_draw_sprites = 0;
	s_renderCore.num_screen_sprites = 0;
	s_renderCore.num_main_thread_draw_sprites = 0;
	s_renderCore.num_sorted_draw_sprites = 0;
	s_renderCore.fullscreen_shader_fun = NULL;
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
	else if (scale > MAX_RENDER_SCALE)
	{
		scale = MAX_RENDER_SCALE;
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

int Render_IsFullscreen()
{
	return s_renderCore.is_fullscreen;
}

void Render_ToggleFullscreen()
{
	s_renderCore.is_fullscreen = !s_renderCore.is_fullscreen;

	GLFWwindow* window = Engine_GetWindow();

	if (s_renderCore.is_fullscreen)
	{
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, s_renderCore.w, s_renderCore.h, GLFW_DONT_CARE);
	}
	else
	{
		glfwSetWindowMonitor(window, NULL, 0, 0, s_renderCore.w, s_renderCore.h, GLFW_DONT_CARE);
	}
}