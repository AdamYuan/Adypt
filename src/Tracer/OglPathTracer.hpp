//
// Created by adamyuan on 12/7/18.
//

#ifndef ADYPT_OGLPATHTRACER_HPP
#define ADYPT_OGLPATHTRACER_HPP

#include <mygl3/buffer.hpp>
#include <mygl3/texture.hpp>
#include <mygl3/shader.hpp>
#include "../Util/Shape.hpp"
#include "../Util/Platform.hpp"
#include "../Util/Sobol.hpp"
#include "OglScene.hpp"

class OglPathTracer
{
private:
	struct GPUShaderArgs
	{
		GLint m_img_width, m_img_height;
		GLint m_iteration;
		GLint _a1;
		glm::vec4 m_position;
		glm::mat4 m_inv_projection, m_inv_view;
	} *m_args;

	Platform::PTConfig m_config;

	mygl3::Shader m_shader;
	GLuint m_group_x, m_group_y;
	int m_uiwidth, m_uiheight;

	mygl3::Texture2D m_result_tex, m_sobol_bias_tex, m_primary_tmp_tex;
	mygl3::Buffer m_sobol_buffer, m_args_buffer;

	GLfloat *m_sobol_seq;
	Sobol m_sobol_gen;

	GLint m_local_iteration;

	void create_shaders(int texture_count);
	void create_buffers();
	void bind_buffers(const OglScene &scene);

public:
	void Initialize(const Platform &platform, const OglScene &scene);
	void UpdateCamera(const OglScene &scene, const glm::mat4 &projection, const glm::mat4 &view,
					  const glm::vec3 &position);
	void Render();
	int GetSPP() const { return m_local_iteration; }
	const mygl3::Texture2D &GetResult() { return m_result_tex; }
};


#endif //ADYPT_OGLPATHTRACER_HPP
