#include <GL/gl3w.h>
#include "OglInterface.hpp"
#include "../BVH/WideBVH.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>

void OglInterface::Initialize(Platform *platform, const Scene &scene, const WideBVH &bvh)
{
	m_platform = platform;
	m_config = &(m_platform->GetUIConfig());
	m_model_name = m_platform->GetName();
	init_window();

	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	m_oglscene.Initialize(scene, bvh);
	m_path_tracer.Initialize(&(m_platform->GetPTConfig()), m_oglscene, m_config->m_width, m_config->m_height);

	cam_update();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiStyle &st = ImGui::GetStyle();
	st.WindowBorderSize = 0.0f;
	st.Alpha = 0.7f;
	st.WindowRounding = 0.0f;
	st.ChildRounding = 0.0f;
	st.FrameRounding = 0.0f;
	st.ScrollbarRounding = 0.0f;
	st.GrabRounding = 0.0f;
	st.TabRounding = 0.0f;


	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init("#version 450 core");
}

void OglInterface::Run()
{
	while(!glfwWindowShouldClose(m_window))
	{
		m_fps.Update();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ui_control();

		//update camera if not in rendering
		if(!m_path_tracer.IsTracing() && !m_lock_camera)
		{
			cam_control();
			m_path_tracer.SetCamera(m_projection, m_camera.GetMatrix(), m_camera.GetPosition());
		}

		glClear(GL_COLOR_BUFFER_BIT);
		m_path_tracer.Update();

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
	m_window = glfwCreateWindow(m_config->m_width, m_config->m_height, "Adypt", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, (void*)this);

	gl3wInit();
}

void OglInterface::cam_control()
{
	float speed = m_fps.GetDelta() * glm::length(m_oglscene.GetAABB().GetExtent())
				  * m_config->m_camera_speed;
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
	if(!ImGui::GetCurrentContext()->NavWindow && glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT))
	{
		ImVec2 cur_pos = ImGui::GetMousePos();
		m_camera.MouseControl(cur_pos.x - last_pos.x, cur_pos.y - last_pos.y, m_config->m_mouse_sensitive);
		last_pos = cur_pos;
	} else
		last_pos = ImGui::GetMousePos();
}

void OglInterface::ui_info_overlay()
{
	ImGui::SetNextWindowPos(ImVec2(10.0f, ImGui::GetIO().DisplaySize.y - 10.0f),
							ImGuiCond_Always, ImVec2(0, 1));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f)); // Transparent background
	if (ImGui::Begin("INFO", nullptr,
					 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize
					 |ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove
					 |ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBringToFrontOnFocus))
	{
		ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
		ImGui::Text("OpenGL version: %s", glGetString(GL_VERSION));
		ImGui::Text("FPS: %f", m_fps.GetFps());

		if(m_path_tracer.IsTracing())
		{
			ImGui::Text("Render SPP: %d", m_path_tracer.GetSPP());
			ImGui::Text("Render Time: %ld sec", m_path_tracer.GetTracingSec());
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void OglInterface::ui_camera_settings()
{
	ImGui::Begin("Camera Settings");

	ImGui::Checkbox("Lock", &m_lock_camera);

	if(m_lock_camera)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	ImGui::DragFloat("Fov", &m_config->m_fov, 1.0f, 0.0f, 360.0f, "%.1f deg");
	ImGui::InputFloat3("Position", (float *)(&m_camera.Position()), 6);
	ImGui::DragFloat("Yaw", &m_camera.Yaw(), 1.0f, 0.0f, 360.0f, "%.1f deg");
	ImGui::DragFloat("Pitch", &m_camera.Pitch(), 1.0f, -90.0f, 90.0f, "%.1f deg");
	ImGui::DragFloat("Move Speed", &m_config->m_camera_speed, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Mouse Sensitive", &m_config->m_mouse_sensitive, 0.01f, 0.0f, 1.0f);
	if(m_lock_camera)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
	else
		cam_update();

	ImGui::End();
}


void OglInterface::ui_path_tracer_settings()
{
	ImGui::Begin("PathTracer Settings");

	Platform::PTConfig &ptc = m_platform->GetPTConfig();

	ImGui::DragInt("Max Bounce", &(ptc.m_max_bounce), 1, 2, 20);
	ImGui::DragInt("Subpixel", &(ptc.m_subpixel), 1, 1, 32);
	ImGui::DragInt("Primary Ray Tmp Lifetime", &(ptc.m_tmp_lifetime), 1, 2, 64);

	ImGui::InputFloat("Ray T-Min", &(ptc.m_ray_tmin), 0.00001f, 0.01f, 8);
	ptc.m_ray_tmin = std::max(ptc.m_ray_tmin, 0.0f);
	ImGui::InputFloat("Color Clamp", &(ptc.m_clamp), 0.01f, 0.1f, 3);
	ptc.m_clamp = std::max(ptc.m_clamp, 1.0f);

	ImGui::DragFloat3("Sun Radiance", ptc.m_sun, 0.1f, 0.0f, 20.0f);

	ImGui::End();
}


void OglInterface::ui_main_menubar()
{
	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("File"))
	{
		if(ImGui::MenuItem("Export to OpenEXR"))
			m_path_tracer.SaveResult(m_model_name.c_str());

		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Info", nullptr, &m_show_info_overlay);

		if(m_path_tracer.IsTracing())
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		ImGui::MenuItem("Camera Settings", nullptr, &m_show_camera_settings);
		ImGui::MenuItem("PathTracer Settings", nullptr, &m_show_path_tracer_settings);
		if(m_path_tracer.IsTracing())
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("View"))
	{
		if(m_path_tracer.IsTracing())
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if(ImGui::MenuItem("Diffuse", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kDiffuse))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kDiffuse;
		if(ImGui::MenuItem("Specular", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kSpecular))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kSpecular;
		if(ImGui::MenuItem("Emissive", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kEmissive))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kEmissive;
		if(ImGui::MenuItem("Normal", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kNormal))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kNormal;
		if(ImGui::MenuItem("Position", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kPosition))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kPosition;
		if(m_path_tracer.IsTracing())
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		if(!m_path_tracer.IsTracing())
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if(ImGui::MenuItem("Radiance", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kPTRadiance))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kPTRadiance;
		if(ImGui::MenuItem("Visibiliy", nullptr, m_path_tracer.ViewerType() == OglPathTracer::ViewerTypes::kPTVisibiliy))
			m_path_tracer.ViewerType() = OglPathTracer::ViewerTypes::kPTVisibiliy;
		if(!m_path_tracer.IsTracing())
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::EndMenu();
	}

	ImGui::Checkbox("Tracing", &m_path_tracer.IsTracing());

	ImGui::EndMainMenuBar();
}

void OglInterface::ui_control()
{
	ui_main_menubar();
	if(m_show_info_overlay) ui_info_overlay();
	if(!m_path_tracer.IsTracing())
	{
		if(m_show_camera_settings) ui_camera_settings();
		if(m_show_path_tracer_settings) ui_path_tracer_settings();
	}
}

OglInterface::~OglInterface()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void OglInterface::cam_update()
{
	m_projection = glm::perspective(glm::radians(m_config->m_fov), m_config->m_width / (float)m_config->m_height, 0.1f,
									glm::length(m_oglscene.GetAABB().GetExtent()));
}

