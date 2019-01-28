#include <GL/gl3w.h>
#include "OglInterface.hpp"
#include "../BVH/WideBVH.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init("#version 450 core");
}

void OglInterface::Run()
{
	while(!glfwWindowShouldClose(m_window))
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		m_fps.Update();

		ui_main_menubar();
		if(m_show_info_overlay) ui_info_overlay();

		//update camera if not in rendering
		if(!m_path_tracer.PathTracingFlag())
		{
			cam_control();
			m_path_tracer.UpdateCamera(m_projection, m_camera.GetMatrix(), m_camera.GetPosition());
		}

		m_path_tracer.Render();

		glClear(GL_COLOR_BUFFER_BIT);
		m_screenquad.Render(m_path_tracer.GetResult());

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
	m_window = glfwCreateWindow(m_config.m_width, m_config.m_height, "Adypt", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, (void*)this);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	gl3wInit();
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

	static ImVec2 last_pos;
	if(glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT))
	{
		ImVec2 cur_pos = ImGui::GetMousePos();
		m_camera.MouseControl(cur_pos.x - last_pos.x, cur_pos.y - last_pos.y, m_config.m_mouse_sensitive);
		last_pos = cur_pos;
	} else
		last_pos = ImGui::GetMousePos();
}

void OglInterface::ui_info_overlay()
{
	ImGui::SetNextWindowPos(ImVec2(10.0f, ImGui::GetIO().DisplaySize.y - 10.0f),
							ImGuiCond_Always, ImVec2(0, 1));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f)); // Transparent background
	if (ImGui::Begin("INFO", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize
					 |ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
		ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION));
		ImGui::Text("FPS: %f", m_fps.GetFps());

		if(m_path_tracer.PathTracingFlag())
		{
			ImGui::Text("Render SPP: %d", m_path_tracer.GetSPP());
			ImGui::Text("Render Time: %ld sec", m_path_tracer.GetPathTracingTime());
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();

}

void OglInterface::ui_main_menubar()
{
	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("Window"))
	{
		ImGui::Checkbox("Info", &m_show_info_overlay);

		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("View"))
	{
		ImGui::Checkbox("Tracer", &m_path_tracer.PathTracingFlag());

		ImGui::EndMenu();
	}

	if(ImGui::MenuItem("SaveEXR"))
		m_path_tracer.SaveResult(m_model_name.c_str());

	ImGui::EndMainMenuBar();
}

void ScreenQuad::Initialize()
{
	m_shader.Initialize();
	m_shader.Load(
			"#version 450 core\n"
			"layout (location = 0) in vec2 aPosition;\n"
			"layout (location = 1) in vec2 aTexcoords;\n"
			"out vec2 vTexcoords;\n"
			"void main() {\n"
			"gl_Position = vec4(aPosition, 1.0, 1.0);\n"
			"vTexcoords = aTexcoords; }\n",
			GL_VERTEX_SHADER);
	m_shader.Load(
			"#version 450 core\n"
			"out vec4 FragColor;\n"
			"in vec2 vTexcoords;\n"
			"layout (binding = 0) uniform sampler2D uTexture;\n"
			"void main() { FragColor = vec4(pow(texture(uTexture, vTexcoords).rgb, vec3(1.0 / 2.2)), 1.0f); }\n",
			GL_FRAGMENT_SHADER);

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
