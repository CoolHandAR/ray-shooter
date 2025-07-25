#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "u_math.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <windows.h>

#include "g_common.h"
#include "r_common.h"
#include "utility.h"
#include "main.h"
#include "sound.h"

#define WINDOW_SCALE 3

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 360
#define WINDOW_NAME "FPS_GAME"

typedef struct
{
	float time_scale;
	float delta;
	uint64_t ticks;
	GLFWwindow* window;
} EngineData;

static EngineData s_engine;

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

extern void Render_WindowCallback(GLFWwindow* window, int width, int height);

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

void Engine_SetTimeScale(float scale)
{
	if (scale < 0)
	{
		scale = 0;
	}
	else if (scale > 1)
	{
		scale = 1;
	}

	s_engine.time_scale = scale;
}

GLFWwindow* Engine_GetWindow()
{
	return s_engine.window;
}

int main()
{
	memset(&s_engine, 0, sizeof(EngineData));

	if (!Sound_Init())
	{
		printf("ERROR::Failed to init sound!\n");
		return -1;
	}

	printf("Sound loaded \n");

	if (!Game_Init())
	{
		printf("ERROR::Failed to load game assets!\n");
		return -1;
	}

	printf("Game loaded \n");

	if(!Map_Load("test2.json"))
	{
		printf("ERROR:: Failed to load map !\n");
		return -1;
	}

	printf("Map loaded \n");

	Player_Init(false);

	srand(time(NULL));

	//load glfw
	if (!glfwInit())
	{
		printf("Failed to load glfw!");
		return -1;
	}

	printf("Glfw loaded \n");
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	s_engine.window = glfwCreateWindow(WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE, WINDOW_NAME, NULL, NULL);

	if (!s_engine.window)
	{
		printf("Failed to create window!");
		glfwTerminate();
		return -1;
	}
	printf("Window created \n");

	glfwMakeContextCurrent(s_engine.window);
	glfwSetWindowSizeCallback(s_engine.window, Render_WindowCallback);
	glfwSetCursorPosCallback(s_engine.window, MouseCallback);
	glfwSetInputMode(s_engine.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
	{
		GLuint vertex_id, frag_id;
		vertex_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_id, 1, &VERTEX_SHADER_SOURCE, NULL);
		glCompileShader(vertex_id);

		if (!Shader_checkCompileErrors(vertex_id, "Vertex"))
		{
			return -1;
		}
		frag_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_id, 1, &FRAGMENT_SHADER_SOURCE, NULL);
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

	glViewport(0, 0, WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE);

	glBindTexture(GL_TEXTURE_2D, texture);

	glfwSwapInterval(0);

	if (!Render_Init(WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE))
	{
		printf("Failed to load renderer!\n");
		return false;
	}

	Render_SetRenderScale(WINDOW_SCALE);

	printf("Renderer Loaded\n");

	float lastTime = 0;
	float currentTime = 0;

	GameAssets* assets = Game_GetAssets();

	s_engine.time_scale = 1.0;

	Player_SetSensitivity(1);
	Sound_setMasterVolume(0.2);

	//MAIN LOOP
	while (!glfwWindowShouldClose(s_engine.window))
	{
		if (Game_GetState() == GS__EXIT)
		{
			break;
		}

		currentTime = glfwGetTime();
		s_engine.delta = currentTime - lastTime;
		lastTime = currentTime;

		s_engine.delta *= s_engine.time_scale;

		Game_Update(s_engine.delta);
		
		glfwPollEvents();

		s_engine.ticks++;
	}

	printf("Shutting down...\n");

	Render_ShutDown();
	Sound_Shutdown();

	Map_Destruct();
	Game_Exit();

	glfwDestroyWindow(s_engine.window);
	glfwTerminate();

	return 0;
}

