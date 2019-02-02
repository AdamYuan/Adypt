//
// Created by adamyuan on 1/31/19.
//

#ifndef ADYPT_INSTANCE_HPP
#define ADYPT_INSTANCE_HPP

#include "Tracer/OglPathTracer.hpp"
#include "Tracer/OglScene.hpp"

#include <mygl3/utils/framerate.hpp>
#include <memory>
#include "Tracer/Camera.hpp"
#include "InstanceConfig.hpp"

class Instance
{
private:
	OglScene m_oglscene;
	Camera m_camera;
	bool m_valid = false;

public:
	bool InitializeFromFile(const char *filename, GLFWwindow *window);
	bool Initialize(GLFWwindow *window);
	bool SaveToFile();
	void Update(GLFWwindow *window, const mygl3::Framerate &fps);
	~Instance();

	std::string m_filename;
	OglPathTracer m_path_tracer;
	InstanceConfig m_config;
	bool m_lock_flag = false;
	bool m_enable_pt_flag = false;
};

#endif //ADYPT_INSTANCE_HPP
