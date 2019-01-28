#ifndef OGLTRACER_H
#define OGLTRACER_H

#include <mygl3/shader.hpp>
#include <mygl3/framebuffer.hpp>
#include <mygl3/utils/camera.hpp>
#include <mygl3/utils/framerate.hpp>
#include "../BVH/WideBVH.hpp"
#include "OglScene.hpp"
#include "OglPathTracer.hpp"
#include "../Util/Platform.hpp"

#include <GLFW/glfw3.h>

struct ScreenQuad
{
	mygl3::Shader m_shader;
	mygl3::VertexArray m_quad_vao;
	mygl3::Buffer m_quad_vbo;
	void Initialize();
	void Render(const mygl3::Texture2D &tex) const;
};

class OglInterface
{
private:
	std::string m_model_name;
	Platform::UIConfig m_config;
	GLFWwindow *m_window;

	//BVH m_bvh; Scene m_scene;
	OglScene m_oglscene;

	ScreenQuad m_screenquad;
	mygl3::Camera m_camera;
	mygl3::Framerate m_fps;
	glm::mat4 m_projection;

	OglPathTracer m_path_tracer;

	bool m_show_info_overlay = true;
	void ui_info_overlay();
	void ui_main_menubar();

	void init_window();
	void cam_control();
public:
	void Initialize(const Platform &platform, const Scene &scene, const WideBVH &bvh);
	void Run();
};

#endif // OGLTRACER_H
