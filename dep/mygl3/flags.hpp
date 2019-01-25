//
// Created by adamyuan on 12/8/18.
//

#ifndef ADYPT_FLAGS_HPP
#define ADYPT_FLAGS_HPP

#include <GL/gl3w.h>

namespace mygl3
{
	constexpr GLuint kInvalidOglId = (GLuint)(-1);
	inline bool IsValidOglId(GLuint id) { return id != kInvalidOglId; }
}

#endif //ADYPT_FLAGS_HPP
