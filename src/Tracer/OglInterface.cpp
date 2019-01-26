#include <GL/gl3w.h>
#include "OglInterface.hpp"
#include "../BVH/WideBVH.hpp"

void OglInterface::Initialize(const Platform &platform, const Scene &scene, const WideBVH &bvh)
{
	m_config = platform.GetUIConfig();
	m_model_name = platform.GetName();
	init_window();

	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	m_oglscene.Initialize(scene, bvh);
	m_screenquad.Initialize();
	m_projection = glm::perspective(glm::radians(m_config.m_fov), m_config.m_width / (float)m_config.m_height, 0.1f,
									glm::length(m_oglscene.GetAABB().GetExtent()));
	m_path_tracer.Initialize(platform, m_oglscene);
}

void OglInterface::Run()
{
	char title[32];
	while(!glfwWindowShouldClose(m_window))
	{
		m_fps.Update();
		sprintf(title, "fps: %f [%d spp]",
				m_fps.GetFps(), m_path_tracer.GetSPP());
		glfwSetWindowTitle(m_window, title);
		if(m_control && !m_rendering_flag)
		{
			cam_control();
			m_path_tracer.UpdateCamera(m_oglscene, m_projection, m_camera.GetMatrix(), m_camera.GetPosition());
		}
		if(m_rendering_flag)
			m_path_tracer.Render();

		glClear(GL_COLOR_BUFFER_BIT);
		m_screenquad.Render(m_path_tracer.GetResult());

		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}
}

void OglInterface::init_window()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(m_config.m_width,
								m_config.m_height, "", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, (void*)this);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetKeyCallback(m_window, key_callback);
	glfwSetWindowFocusCallback(m_window, focus_callback);

	gl3wInit();
	if(!gl3wIsSupported(4, 5))
		printf("[GL] OpenGL 4.5 not supported\n");
	if(!gl3wGetProcAddress("glGetTextureHandleARB"))
		printf("[GL] Bindless texture not supported\n");

}

void OglInterface::key_callback(GLFWwindow *window, int key, int, int action, int)
{
	auto *app = (OglInterface *)glfwGetWindowUserPointer(window);
	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
		{
			app->m_control = !app->m_control;
			glfwSetCursorPos(window, (float)app->m_config.m_width / 2,
							 (float)app->m_config.m_height / 2);
			glfwSetInputMode(window, GLFW_CURSOR, app->m_control ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
		}
		else if(key == GLFW_KEY_R)
			app->m_rendering_flag = !app->m_rendering_flag;
		else if(key == GLFW_KEY_P)
			app->m_path_tracer.SaveResult(app->m_model_name.c_str());
	}
}

void OglInterface::focus_callback(GLFWwindow *window, int focused)
{
	auto *app = (OglInterface *)glfwGetWindowUserPointer(window);
	if(focused == GLFW_FALSE)
	{
		app->m_control = false;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void OglInterface::cam_control()
{
	float speed = m_fps.GetDelta() * glm::length(m_oglscene.GetAABB().GetExtent())
				  * m_config.m_camera_speed;
	if(glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
		m_camera.MoveForward(speed, 0.0f);
	if(glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
		m_camera.MoveForward(speed, 90.0f);
	if(glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
		m_camera.MoveForward(speed, -90.0f);
	if(glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
		m_camera.MoveForward(speed, 180.0f);
	if(glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
		m_camera.MoveUp(speed);
	if(glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		m_camera.MoveUp(-speed);

	double mouse_x, mouse_y;
	glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
	m_camera.MouseControl((float)(mouse_x - m_config.m_width / 2), (float)(mouse_y - m_config.m_height / 2),
						  m_config.m_mouse_sensitive);
	glfwSetCursorPos(m_window, m_config.m_width / 2, m_config.m_height / 2);
}

void ScreenQuad::Initialize()
{
	m_shader.Initialize();
	m_shader.LoadFromFile("shaders/quad.vert", GL_VERTEX_SHADER);
	m_shader.LoadFromFile("shaders/quad.frag", GL_FRAGMENT_SHADER);

	constexpr float quad_vertices[] {
			-1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f, 0.0f,
			-1.0f, -1.0f, 0.0f, 1.0f };
	m_quad_vbo.Initialize();
	m_quad_vbo.Storage(quad_vertices, quad_vertices + 24, 0);
	m_quad_vao.Initialize(0, 2, 1, 2);
	m_quad_vao.BindVertices(m_quad_vbo);
}

void ScreenQuad::Render(const mygl3::Texture2D &tex) const
{
	m_shader.Use();
	tex.Bind(0);
	m_quad_vao.DrawArrays(GL_TRIANGLES);
}
