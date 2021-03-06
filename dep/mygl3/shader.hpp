//
// Created by adamyuan on 3/11/18.
//

#ifndef MYGL2_SHADER_HPP
#define MYGL2_SHADER_HPP

#include <glm/glm.hpp> //glm is great
#include <glm/gtc/type_ptr.hpp>

#include <GL/gl3w.h>
#include <fstream>

#include "flags.hpp"

namespace mygl3
{
	class Shader
	{
	private:
		GLuint id_{kInvalidOglId};
		std::string load_file(const char *filename) const
		{
			std::ifstream in(filename);
			if(!in.is_open())
			{
				printf("failed to open file %s\n", filename);
				return "";
			}
			return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>{}};
		}

	public:
		Shader() = default;;
		~Shader()
		{
			if(IsValidOglId(id_))
				glDeleteProgram(id_);
		}
		Shader(const Shader&) = delete;
		Shader& operator= (const Shader&) = delete;
		Shader(Shader &&shader) noexcept : id_(shader.id_) { shader.id_ = 0; }
		/*Shader &operator=(Shader &&shader) noexcept
		{
			program_ = shader.program_;
			shader.program_ = 0;
			return *this;
		}*/

		void Initialize()
		{
			id_ = glCreateProgram();
		}

		void Load(const char *src, GLenum type)
		{
			GLuint shader = glCreateShader(type);
			glShaderSource(shader, 1, &src, nullptr);
			glCompileShader(shader);

			int success;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if(!success)
			{
				char log[16384];
				glGetShaderInfoLog(shader, 16384, nullptr, log);
				printf("******SHADER COMPILE ERROR******\nsrc:\n%s\nerr:\n%s\n\n\n", src, log);
			}
			glAttachShader(id_, shader);
			glLinkProgram(id_);

			glDeleteShader(shader);
		}

		void LoadFromFile(const char *filename, GLenum type)
		{
			std::string str {load_file(filename)};
			Load(str.c_str(), type);
		}

		void Use() const
		{
			glUseProgram(id_);
		}
		GLint GetUniform(const char *name) const
		{
			return glGetUniformLocation(id_, name);
		}
		GLuint GetProgram() const { return id_; }
		void SetMat4(GLint loc, const glm::mat4 &matrix4)
		{
			glProgramUniformMatrix4fv(id_, loc, 1, GL_FALSE, glm::value_ptr(matrix4));
		}
		void SetMat3(GLint loc, const glm::mat3 &matrix3)
		{
			glProgramUniformMatrix3fv(id_, loc, 1, GL_FALSE, glm::value_ptr(matrix3));
		}
		void SetIVec4(GLint loc, const glm::ivec4 &vector4)
		{
			glProgramUniform4iv(id_, loc, 1, glm::value_ptr(vector4));
		}
		void SetIVec3(GLint loc, const glm::ivec3 &vector3)
		{
			glProgramUniform3iv(id_, loc, 1, glm::value_ptr(vector3));
		}
		void SetIVec2(GLint loc, const glm::ivec2 &vector2)
		{
			glProgramUniform2iv(id_, loc, 1, glm::value_ptr(vector2));
		}
		void SetVec4(GLint loc, const glm::vec4 &vector4)
		{
			glProgramUniform4fv(id_, loc, 1, glm::value_ptr(vector4));
		}
		void SetVec3(GLint loc, const glm::vec3 &vector3)
		{
			glProgramUniform3fv(id_, loc, 1, glm::value_ptr(vector3));
		}
		void SetVec2(GLint loc, const glm::vec2 &vector2)
		{
			glProgramUniform2fv(id_, loc, 1, glm::value_ptr(vector2));
		}
		void SetInt(GLint loc, int i)
		{
			glProgramUniform1i(id_, loc, i);
		}
		void SetUint(GLint loc, unsigned i)
		{
			glProgramUniform1ui(id_, loc, i);
		}
		void SetFloat(GLint loc, float f)
		{
			glProgramUniform1f(id_, loc, f);
		}
	};
}

#endif //MYGL2_SHADER_HPP
