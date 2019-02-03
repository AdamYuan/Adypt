#ifndef OGLTRACER_H
#define OGLTRACER_H

#include <GL/gl3w.h>

#include <memory>
#include <functional>

#include <GLFW/glfw3.h>
#include <mygl3/utils/framerate.hpp>

#include "Instance.hpp"

class Application
{
private:
	static constexpr size_t kFilenameBufSize = 1024;
	GLFWwindow *m_window;

	mygl3::Framerate m_fps;

	std::unique_ptr<Instance> m_instance;

	bool m_show_info_overlay = true;
	bool m_show_instance_settings = false;
	void ui_control();
	void ui_export_openexr_modal();
	void ui_new_instance_modal();
	void ui_open_instance_modal();
	void ui_info_overlay_window();
	void ui_main_menubar();
	void ui_general_settings(InstanceConfig *cfg);
	void ui_bvh_settings(InstanceConfig::BVH *bvh_cfg);
	void ui_camera_settings(InstanceConfig::Cam *cam_cfg);
	void ui_path_tracer_settings(InstanceConfig::PT *pt_cfg);
	void ui_push_disable();
	void ui_pop_disable();
	bool comp_file_open(const char *label, const char *btn, char *buf, size_t buf_size, const char *title,
						const std::vector<std::string> &filters);
	bool comp_file_save(const char *label, const char *btn, char *buf, size_t buf_size, const char *title,
						const std::vector<std::string> &filters);
	void init_window();

public:
	void Initialize();
	void InitializeFromFile(const char *filename);
	void Run();
	~Application();
};

#endif // OGLTRACER_H
