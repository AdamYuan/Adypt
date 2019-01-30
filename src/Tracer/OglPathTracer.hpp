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
#include <chrono>

class OglPathTracer
{
public:
	enum ViewerTypes { kDiffuse = 0, kSpecular, kEmissive, kPTRadiance, kPTVisibiliy = 4, kNormal, kPosition };
private:
	GLuint m_group_x, m_group_y;
	int m_uiwidth, m_uiheight, m_texture_count;
	Platform::PTConfig *m_config;
	//shader args
	struct CameraArgs
	{
		glm::vec4 m_origin_tmin;
		glm::mat4 m_inv_projection, m_inv_view;
	} *m_camera_args;
	struct PathTracerArgs
	{
		GLint m_spp, m_subpixel, m_tmplife, m_max_bounce;
		GLfloat m_clamp, m_sun[3];
	} *m_pt_args;
	GLint m_pt_local_spp;
	struct ViewerArgs
	{
		GLint m_type;
	} *m_viewer_args;
	ViewerTypes m_viewer_type = ViewerTypes::kDiffuse;

	mygl3::Shader m_pt_shader, //path tracer
			m_primaryray_shader, //trace primary ray
			m_screen_shader; //render a screen quad to show result

	mygl3::VertexArray m_screen_vao;
	mygl3::Buffer m_screen_vbo;

	mygl3::Texture2D m_result_tex, m_sobol_bias_tex, m_primary_tmp_tex;
	mygl3::Buffer m_camera_args_buffer, m_pt_args_buffer, m_viewer_args_buffer;

	//sobol
	Sobol m_sobol_gen;
	mygl3::Buffer m_sobol_buffer; GLfloat *m_sobol_seq;

	void create_shaders();
	void create_buffers();
	void bind_buffers(const OglScene &scene);

	void update_config_args(); //apply changes

	bool m_tracing_flag = false;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_tracing_start_time;

public:

	void Initialize(Platform::PTConfig *config, const OglScene &scene, int width, int height);
	void SetCamera(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &position);
	void Update();

	const mygl3::Texture2D &GetResult() { return m_result_tex; }
	void SaveResult(const char *name);

	bool &IsTracing() { return m_tracing_flag; }
	const bool &IsTracing() const { return m_tracing_flag; }

	ViewerTypes &ViewerType() { return m_viewer_type; }
	const ViewerTypes &ViewerType() const { return m_viewer_type; }

	int GetSPP() const { return m_pt_local_spp; }

	long GetTracingSec() const
	{
		return std::chrono::duration_cast<std::chrono::seconds>
				(std::chrono::high_resolution_clock::now() - m_tracing_start_time).count();
	}
};


#endif //ADYPT_OGLPATHTRACER_HPP
