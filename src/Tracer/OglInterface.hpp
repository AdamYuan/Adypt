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
	const Platform::UIConfig m_config;
	GLFWwindow *m_window;

	//BVH m_bvh; Scene m_scene;
	OglScene m_oglscene;
	bool m_control{true}; //whether the window is under key or mouse control
	bool m_rendering{false}; //doing progressive render

	ScreenQuad m_screenquad;
	mygl3::Camera m_camera;
	mygl3::Framerate m_fps;
	glm::mat4 m_projection;

	OglPathTracer m_path_tracer;

	void init_window();
	void cam_control();
	static void key_callback(GLFWwindow *window, int key, int, int action, int);
	static void focus_callback(GLFWwindow *window, int focused);
public:
	OglInterface(const Platform &platform, const Scene &scene, const WideBVH &bvh);
	void Run();
};

#endif // OGLTRACER_H
