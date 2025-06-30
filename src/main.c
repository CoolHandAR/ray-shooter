
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "dynamic_array.h"

#include "u_math.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "g_common.h"
#include "r_common.h"
#include "utility.h"
#include "main.h"
#include "sound.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

typedef struct
{
	float delta;
	uint64_t ticks;
} EngineData;

static EngineData s_engine;

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


static void WindowCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	//Render_ResizeWindow(width, height);
}
static void MouseCallback(GLFWwindow* window, double x, double y)
{
	Player_MouseCallback(x, y);
}

uint64_t Engine_GetTicks()
{
	return s_engine.ticks;
}
float Engine_GetDeltaTime()
{
	return s_engine.delta;
}

int main()
{
	memset(&s_engine, 0, sizeof(EngineData));

	if (!Sound_Init())
	{
		printf("ERROR::Failed to init sound!\n");
		return -1;
	}

	if (!Game_LoadAssets())
	{
		printf("ERROR::Failed to load game assets!\n");
		return -1;
	}

	if(!Map_Load("test.json"))
	{
		printf("ERROR:: Failed to load map !\n");
		return -1;
	}

	Player_Init();

	srand(time(NULL));

	//load glfw
	if (!glfwInit())
	{
		printf("Failed to load glfw!");
		return -1;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "FPS_GAME", NULL, NULL);

	if (!window)
	{
		printf("Failed to create window!");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, WindowCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//load glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to init GLAD\n");
		return -1;
	}

	printf("GLAD Loaded\n");
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	unsigned shader_id = 0;

	//setup shaders
	char* vert_src = NULL;
	char* frag_src = NULL;

	vert_src = File_Parse("shader.vert", NULL);

	if (!vert_src)
	{
		printf("Failed to load vertex shader");
		return -1;
	}

	frag_src = File_Parse("shader.frag", NULL);

	if (!frag_src)
	{
		free(vert_src);
		printf("Failed to load fragment shader");
		return -1;
	}

	{
		GLuint vertex_id, frag_id;
		vertex_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_id, 1, &vert_src, NULL);
		glCompileShader(vertex_id);

		if (!Shader_checkCompileErrors(vertex_id, "Vertex"))
		{
			return -1;
		}
		frag_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_id, 1, &frag_src, NULL);
		glCompileShader(frag_id);

		if (!Shader_checkCompileErrors(frag_id, "Fragment"))
		{
			return -1;
		}

		shader_id = glCreateProgram();
		glAttachShader(shader_id, vertex_id);
		glAttachShader(shader_id, frag_id);

		glLinkProgram(shader_id);
		if (!Shader_checkCompileErrors(shader_id, "Program"))
			return -1;

		glDeleteShader(vertex_id);
		glDeleteShader(frag_id);

		free(vert_src);
		free(frag_src);
	}

	//setup main screen texture
	GLuint texture = 0;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

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

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	glBindTexture(GL_TEXTURE_2D, texture);

	Render_Init(WINDOW_WIDTH, WINDOW_HEIGHT);

	float lastTime = 0;
	float currentTime = 0;

	GameAssets* assets = Game_GetAssets();

	float view_x = 0;
	float view_y = 0;
	float view_dir_x = 0;
	float view_dir_y = 0;
	float view_plane_x = 0;
	float view_plane_y = 0;

	mat4 proj;
	glm_ortho(0, 1280, 720, 0, -1, 1, proj);

	unsigned id = glGetUniformLocation(shader_id, "u_proj");
	glUniformMatrix4fv(id, 1, GL_FALSE, proj);

	

	//MAIN LOOP
	while (!glfwWindowShouldClose(window))
	{
		currentTime = glfwGetTime();
		s_engine.delta = currentTime - lastTime;
		lastTime = currentTime;

		Player_Update(window, s_engine.delta);
		Map_UpdateObjects(s_engine.delta);

		Player_GetView(&view_x, &view_y, &view_dir_x, &view_dir_y, &view_plane_x, &view_plane_y);

		Render_View(view_x, view_y, view_dir_x, view_dir_y, view_plane_x, view_plane_y);

		glfwSwapBuffers(window);
		glfwPollEvents();

		s_engine.ticks++;
	}

	Map_Destruct();
	Game_DestructAssets();

	Render_ShutDown();
	Sound_Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

