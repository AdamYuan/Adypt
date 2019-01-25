//
// Created by adamyuan on 4/15/18.
//

#include "texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mygl3
{
	ImageLoader::ImageLoader(const char *filename, GLenum internal_format, GLenum format, GLenum data_type,
								 int req_channels)
	{
		info_.internal_format = internal_format;
		info_.format = format;
		info_.type = data_type;
		int nr_channels;
		info_.data = stbi_load(filename, &info_.width, &info_.height, &nr_channels, req_channels);
	}

	ImageLoader::~ImageLoader() { stbi_image_free(info_.data); }

	HDRLoader::HDRLoader(const char *filename)
	{
		info_.internal_format = GL_RGB32F;
		info_.format = GL_RGB;
		info_.type = GL_FLOAT;
		int nr_channels;
		info_.data = stbi_loadf(filename, &info_.width, &info_.height, &nr_channels, 3);
	}

	HDRLoader::~HDRLoader()
	{
		stbi_image_free(info_.data);
	}
}
