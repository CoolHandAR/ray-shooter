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

void Image_Set2(Image* img, int x, int y, unsigned char* color)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return;
	}

	unsigned char* d = img->data + (x + y * img->width) * img->numChannels;

	memcpy(d, color, img->numChannels);
}

void Image_Copy(Image* dest, Image* src)
{
	assert(dest->numChannels == src->numChannels);
	
	memcpy(dest->data, src->data, sizeof(unsigned char) * dest->width * dest->height * dest->numChannels);
}

unsigned char* Image_Get(Image* img, int x, int y)
{
	if (!img->data || x < 0 || y < 0 || x >= img->width || y >= img->height)
	{
		return NULL;
	}

	return img->data + (x + y * img->width) * img->numChannels;
}

