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

class OglInterface
{
private:
	Platform *m_platform;
	Platform::UIConfig *m_config;

	std::string m_model_name;
	GLFWwindow *m_window;

	//BVH m_bvh; Scene m_scene;
	OglScene m_oglscene;

	mygl3::Camera m_camera;
	mygl3::Framerate m_fps;
	glm::mat4 m_projection;

	OglPathTracer m_path_tracer;

	bool m_lock_camera = false;

	bool m_show_info_overlay = true;
	bool m_show_camera_settings = false;
	bool m_show_path_tracer_settings = false;
	void ui_control();
	void ui_info_overlay();
	void ui_main_menubar();
	void ui_camera_settings();
	void ui_path_tracer_settings();

	void init_window();
	void cam_update();
	void cam_control();
public:
	void Initialize(Platform *platform, const Scene &scene, const WideBVH &bvh);
	void Run();
	~OglInterface();
};

#endif // OGLTRACER_H
