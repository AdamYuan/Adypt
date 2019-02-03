#include <GL/gl3w.h>
#include <glm/gtc/type_ptr.hpp>
#include "Application.hpp"
#include "BVH/WideBVH.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>
#include <portable-file-dialogs.h>

Application::~Application()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::Initialize()
{
	init_window();

	glEnable(GL_CULL_FACE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

	pfd::settings::verbose(true);

}

void Application::InitializeFromFile(const char *filename)
{
	Initialize();

	m_instance.reset(new Instance());
	if(!m_instance->InitializeFromFile(filename, m_window))
		m_instance.reset();
}

void Application::Run()
{
	while(!glfwWindowShouldClose(m_window))
	{
		m_fps.Update();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ui_control();

		//render
		glClear(GL_COLOR_BUFFER_BIT);
		if(m_instance) m_instance->Update(m_window, m_fps);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}
}

void Application::init_window()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(1280, 720, "Adypt", nullptr, nullptr);
	glfwMakeContextCurrent(m_window);
	glfwSetWindowUserPointer(m_window, (void*)this);

	gl3wInit();
}

void Application::ui_info_overlay_window()
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

		if(m_instance && m_instance->m_enable_pt_flag)
		{
			ImGui::Text("Render SPP: %d", m_instance->m_path_tracer.GetSPP());
			ImGui::Text("Render Time: %ld sec", m_instance->m_path_tracer.GetPTSec());
		}

		ImGui::End();
	}
	ImGui::PopStyleColor();
}

void Application::ui_general_settings(InstanceConfig *cfg)
{
	static char obj_name_buf[kFilenameBufSize]{}, bvh_name_buf[kFilenameBufSize]{};

	if(strcmp(obj_name_buf, cfg->m_obj_filename.c_str()) != 0)
		strcpy(obj_name_buf, cfg->m_obj_filename.c_str());
	if(strcmp(bvh_name_buf, cfg->m_bvh_filename.c_str()) != 0)
		strcpy(bvh_name_buf, cfg->m_bvh_filename.c_str());

	if(comp_file_open("OBJ Filename", "...##2", obj_name_buf, kFilenameBufSize, "OBJ Filename",
					  {"Wavefront OBJ File (.obj)", "*.obj", "All Files", "*"}))
		cfg->m_obj_filename = obj_name_buf;
	if(comp_file_save("BVH Filename", "...##3", bvh_name_buf, kFilenameBufSize, "BVH Filename",
					  {"BVH File (.bvh)", "*.bvh", "All Files", "*"}))
		cfg->m_bvh_filename = bvh_name_buf;

	ImGui::DragInt("Width", &cfg->m_width, 8, 640, 3840);
	ImGui::DragInt("Height", &cfg->m_height, 8, 480, 2160);
}

void Application::ui_bvh_settings(InstanceConfig::BVH *bvh_cfg)
{
	ImGui::DragInt("Max Spatial Depth", &bvh_cfg->m_max_spatial_depth, 1, 0, 128);
	ImGui::DragFloat("Triangle SAH", &bvh_cfg->m_triangle_sah, 0.05f, 0.0f, 1.0f);
	ImGui::DragFloat("Node SAH", &bvh_cfg->m_node_sah, 0.05f, 0.0f, 1.0f);
}

void Application::ui_camera_settings(InstanceConfig::Cam *cam_cfg)
{
	ImGui::DragFloat("Fov", &cam_cfg->m_fov, 1.0f, 0.0f, 360.0f, "%.1f deg");
	ImGui::InputFloat3("Position", glm::value_ptr(cam_cfg->m_position), 6);
	ImGui::DragFloat("Yaw", &cam_cfg->m_yaw, 1.0f, 0.0f, 360.0f, "%.1f deg");
	ImGui::DragFloat("Pitch", &cam_cfg->m_pitch, 1.0f, -90.0f, 90.0f, "%.1f deg");
	ImGui::DragFloat("Mouse Sensitive", &cam_cfg->m_mouse_sensitive, 0.01f, 0.0f, 1.0f);
	ImGui::InputFloat("Move Speed", &cam_cfg->m_speed, 0.1f);
}


void Application::ui_path_tracer_settings(InstanceConfig::PT *pt_cfg)
{
	ImGui::DragInt("Max Bounce", &pt_cfg->m_max_bounce, 1, 2, 20);
	ImGui::DragInt("Subpixel", &pt_cfg->m_subpixel, 1, 1, 32);
	ImGui::DragInt("Primary Ray Tmp Lifetime", &pt_cfg->m_tmp_lifetime, 1, 2, 64);

	ImGui::InputFloat("Ray T-Min", &pt_cfg->m_ray_tmin, 0.00001f, 0.01f, 8);
	pt_cfg->m_ray_tmin = std::max(pt_cfg->m_ray_tmin, 0.0f);
	ImGui::InputFloat("Color Clamp", &pt_cfg->m_clamp, 0.01f, 0.1f, 3);
	pt_cfg->m_clamp = std::max(pt_cfg->m_clamp, 1.0f);

	ImGui::DragFloat3("Sun Radiance", glm::value_ptr(pt_cfg->m_sun), 0.1f, 0.0f, 20.0f);
}


void Application::ui_main_menubar()
{
	ImGui::BeginMainMenuBar();

	bool export_openexr_popup = false;
	bool new_instance_popup = false;
	bool open_instance_popup = false;

	if(ImGui::BeginMenu("File"))
	{
		if(ImGui::MenuItem("New Instance"))
			new_instance_popup = true;
		if(ImGui::MenuItem("Open Instance"))
			open_instance_popup = true;

		if(m_instance)
		{
			if(ImGui::MenuItem("Reload Instance"))
			{
				std::string filename = m_instance->m_filename;
				m_instance.reset(new Instance());
				m_instance->InitializeFromFile(filename.c_str(), m_window);
			}
			if(ImGui::MenuItem("Save Instance"))
				m_instance->SaveToFile();
			if(ImGui::MenuItem("Export OpenEXR"))
				export_openexr_popup = true;
		}

		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Info", nullptr, &m_show_info_overlay);

		if(m_instance)
		{
			if(m_instance->m_enable_pt_flag) ui_push_disable();
			ImGui::MenuItem("Instance Settings", nullptr, &m_show_instance_settings);
			if(m_instance->m_enable_pt_flag) ui_pop_disable();
		}

		ImGui::EndMenu();
	}
	if(m_instance)
	{
		if(ImGui::BeginMenu("View"))
		{
			if(m_instance->m_enable_pt_flag) ui_push_disable();
			if(ImGui::MenuItem("Diffuse", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kDiffuse))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kDiffuse;
			if(ImGui::MenuItem("Specular", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kSpecular))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kSpecular;
			if(ImGui::MenuItem("Emissive", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kEmissive))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kEmissive;
			if(ImGui::MenuItem("Normal", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kNormal))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kNormal;
			if(ImGui::MenuItem("Position", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kPosition))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kPosition;
			if(m_instance->m_enable_pt_flag) ui_pop_disable();

			if(!m_instance->m_enable_pt_flag) ui_push_disable();
			if(ImGui::MenuItem("Radiance", nullptr, m_instance->m_path_tracer.m_viewer_type == OglPathTracer::ViewerTypes::kPTRadiance))
				m_instance->m_path_tracer.m_viewer_type = OglPathTracer::ViewerTypes::kPTRadiance;
			if(!m_instance->m_enable_pt_flag) ui_pop_disable();

			ImGui::EndMenu();
		}

		ImGui::Separator();

		if(m_instance->m_lock_flag) ui_push_disable();
		ImGui::Checkbox("Render", &m_instance->m_enable_pt_flag);
		if(m_instance->m_lock_flag) ui_pop_disable();

		ImGui::Checkbox("Lock", &m_instance->m_lock_flag);
	}

	ImGui::EndMainMenuBar();

	if(export_openexr_popup)
		ImGui::OpenPopup("Export OpenEXR");
	if(new_instance_popup)
		ImGui::OpenPopup("New Instance");
	if(open_instance_popup)
		ImGui::OpenPopup("Open Instance");
}

void Application::ui_export_openexr_modal()
{
	if(!m_instance) return;

	if (ImGui::BeginPopupModal("Export OpenEXR", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char exr_name_buf[kFilenameBufSize]{};
		static bool save_as_fp16{false};
		comp_file_save("OpenEXR Filename", "...##0", exr_name_buf, kFilenameBufSize, "Export OpenEXR",
					   {"OpenEXR File (.exr)", "*.exr", "All Files", "*"});

		ImGui::Checkbox("Export As FP16", &save_as_fp16);

		{
			if (ImGui::Button("Export", ImVec2(256, 0)))
			{
				m_instance->m_path_tracer.SaveResult(exr_name_buf, save_as_fp16);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(256, 0)))
				ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Application::ui_open_instance_modal()
{
	if (ImGui::BeginPopupModal("Open Instance", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char name_buf[kFilenameBufSize]{};

		comp_file_open("Instance Filename", "...##5", name_buf, kFilenameBufSize, "Instance Filename",
					   {"JSON File (.json)", "*.json", "All Files", "*"});

		{
			if (ImGui::Button("Open", ImVec2(256, 0)))
			{
				std::unique_ptr<Instance> ptr{new Instance()};
				if(ptr->InitializeFromFile(name_buf, m_window))
					m_instance = std::move(ptr);

				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(256, 0)))
				ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Application::ui_new_instance_modal()
{
	if (ImGui::BeginPopupModal("New Instance", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static InstanceConfig cfg{};
		static char name_buf[kFilenameBufSize]{};

		comp_file_save("Instance Filename", "...##1", name_buf, kFilenameBufSize, "Instance Filename",
					   {"JSON File (.json)", "*.json", "All Files", "*"});

		if(ImGui::CollapsingHeader("General Settings", ImGuiTreeNodeFlags_DefaultOpen))
			ui_general_settings(&cfg);

		if(ImGui::CollapsingHeader("BVH Settings"))
			ui_bvh_settings(&cfg.m_bvh_cfg);

		if(ImGui::CollapsingHeader("PathTracer Settings"))
			ui_path_tracer_settings(&cfg.m_pt_cfg);

		if(ImGui::CollapsingHeader("Camera Settings"))
			ui_camera_settings(&cfg.m_cam_cfg);

		{
			if (ImGui::Button("Create", ImVec2(256, 0)))
			{
				std::unique_ptr<Instance> ptr{new Instance()};
				ptr->m_config = cfg;
				ptr->m_filename = name_buf;
				if(ptr->Initialize(m_window))
					m_instance = std::move(ptr);

				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(256, 0)))
				ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}


void Application::ui_control()
{
	if(!m_instance)
		m_show_instance_settings = false;
	ui_main_menubar();
	ui_export_openexr_modal();
	ui_new_instance_modal();
	ui_open_instance_modal();

	if(m_show_info_overlay) ui_info_overlay_window();

	bool tracing = m_instance ? m_instance->m_enable_pt_flag : true;
	if(!tracing && m_show_instance_settings)
	{
		ImGui::Begin("Instance Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if(ImGui::CollapsingHeader("General Settings"))
		{
			ImGui::Text("Required reload to apply");
			ui_general_settings(&m_instance->m_config);
		}

		if(ImGui::CollapsingHeader("BVH Settings"))
		{
			ImGui::Text("Required reload to regenerate BVH");
			ui_bvh_settings(&m_instance->m_config.m_bvh_cfg);
		}

		if(ImGui::CollapsingHeader("PathTracer Settings"))
			ui_path_tracer_settings(&m_instance->m_config.m_pt_cfg);

		if(ImGui::CollapsingHeader("Camera Settings"))
			ui_camera_settings(&m_instance->m_config.m_cam_cfg);
		ImGui::End();
	}
}

void Application::ui_push_disable()
{
	ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

void Application::ui_pop_disable()
{
	ImGui::PopItemFlag();
	ImGui::PopStyleVar();
}

bool
Application::comp_file_open(const char *label, const char *btn, char *buf, size_t buf_size, const char *title,
							 const std::vector<std::string> &filters)
{
	bool ret = ImGui::InputText(label, buf, buf_size);
	ImGui::SameLine();

	if(ImGui::Button(btn))
	{
		auto file_dialog = pfd::open_file(title, "", filters, false);
		if(!file_dialog.result().empty()) strcpy(buf, file_dialog.result().front().c_str());
		ret = true;
	}
	return ret;
}

bool
Application::comp_file_save(const char *label, const char *btn, char *buf, size_t buf_size, const char *title,
							 const std::vector<std::string> &filters)
{
	bool ret = ImGui::InputText(label, buf, buf_size);
	ImGui::SameLine();

	if(ImGui::Button(btn))
	{
		auto file_dialog = pfd::save_file(title, "", filters, true);
		strcpy(buf, file_dialog.result().c_str());
		ret = true;
	}
	return ret;
}

