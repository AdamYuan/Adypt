//
// Created by adamyuan on 4/14/18.
//

#ifndef MYGL2_TEXTURE_HPP
#define MYGL2_TEXTURE_HPP

#include <GL/gl3w.h>
#include <cstdio>
#include "flags.hpp"

namespace mygl3
{
	//deleted mipmap
#define DEF_TEXTURE_CLASS(name, target) class name { \
private: \
GLuint id_{kInvalidOglId}; \
public: \
name() = default; \
name& operator= (const name&) = delete; \
name(name &&texture) noexcept : id_(texture.id_) { texture.id_ = kInvalidOglId; } \
~name() { if(IsValidOglId(id_)) glDeleteTextures(1, &id_); } \
name (const name&) = delete; \
void Initialize() { glCreateTextures(target, 1, &id_); } \
void Bind(GLuint unit) const { glBindTextureUnit(unit, id_); } \
void SetSizeFilter(GLenum min_filter, GLenum mag_filter) { \
    glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, min_filter); \
    glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, mag_filter); \
} \
void SetWrapFilter(GLenum filter) { \
	glTextureParameteri(id_, GL_TEXTURE_WRAP_S, filter); \
	glTextureParameteri(id_, GL_TEXTURE_WRAP_T, filter); \
	glTextureParameteri(id_, GL_TEXTURE_WRAP_R, filter); \
} \
GLuint Get() const { return id_; }

	DEF_TEXTURE_CLASS(Texture1D, GL_TEXTURE_1D)
		void Storage(GLsizei width, GLenum internal_format)
		{ glTextureStorage1D(id_, 1, internal_format, width); }
		void Data(const GLvoid *pixels, GLsizei width, GLenum format, GLenum type)
		{ glTextureSubImage1D(id_, 0, 0, width, format, type, pixels); }
	};

	DEF_TEXTURE_CLASS(Texture2D, GL_TEXTURE_2D)
		void Storage(GLsizei width, GLsizei height, GLenum internal_format)
		{ glTextureStorage2D(id_, 1, internal_format, width, height); }
		void Data(const GLvoid *pixels, GLsizei width, GLsizei height, GLenum format, GLenum type)
		{ glTextureSubImage2D(id_, 0, 0, 0, width, height, format, type, pixels); }
	};
}

#endif //MYGL2_TEXTURE_HPP
