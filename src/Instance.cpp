//
// Created by adamyuan on 1/31/19.
//

#include "Instance.hpp"
#include "BVH/WideBVH.hpp"
#include "BVH/SBVHBuilder.hpp"
#include "BVH/WideBVHBuilder.hpp"

bool Instance::Initialize(GLFWwindow *window)
{
	Scene scene;
	if(!scene.LoadFromFile(m_config.m_obj_filename.c_str()))
	{
		printf("[INSTANCE]Err: Unable to load scene %s\n", m_config.m_obj_filename.c_str());
		return (m_valid = false);
	}

	WideBVH wbvh;
	if(!wbvh.LoadFromFile(m_config.m_bvh_filename.c_str(), m_config.m_bvh_cfg))
	{
		//if failed to load, or parameter updated, generate new bvh
		SBVH sbvh;
		SBVHBuilder{m_config.m_bvh_cfg, &sbvh, scene}.Run();
		WideBVHBuilder{m_config.m_bvh_cfg, &wbvh, sbvh}.Run();
		if(!wbvh.SaveToFile(m_config.m_bvh_filename.c_str(), m_config.m_bvh_cfg))
		{
			printf("[INSTANCE]Err: Unable to load bvh %s\n", m_config.m_bvh_filename.c_str());
			return (m_valid = false);
		}
	}

	m_oglscene.Initialize(scene, wbvh);
	m_path_tracer.Initialize(&m_config.m_pt_cfg, m_oglscene, m_config.m_width, m_config.m_height);
	m_camera.Initialize(&m_config.m_cam_cfg, m_config.m_width, m_config.m_height);

	glfwSetWindowSize(window, m_config.m_width, m_config.m_height);
	glViewport(0, 0, m_config.m_width, m_config.m_height);
	printf("[INSTANCE]Info: Initialized from %s\n", m_filename.c_str());

	return (m_valid = true);
}

void Instance::Update(GLFWwindow *window, const mygl3::Framerate &fps)
{
	//update camera if not in rendering
	if(!m_lock_flag)
	{
		if(!m_enable_pt_flag)
		{
			m_camera.Control(window, fps);
			m_path_tracer.SetCamera(m_camera.GetProjection(), m_camera.GetView(), m_config.m_cam_cfg.m_position);
		}
		m_path_tracer.Trace(m_enable_pt_flag);
	}
	m_path_tracer.DrawScreen();
}

bool Instance::InitializeFromFile(const char *filename, GLFWwindow *window)
{
	m_filename = filename;
	if(!m_config.LoadFromFile(filename))
	{
		printf("[INSTANCE]Err: Invalid instance %s\n", filename);
		return (m_valid = false);
	}
	printf("[INSTANCE]Info: Instance loaded from %s\n", filename);
	return (m_valid = Initialize(window));
}

bool Instance::SaveToFile()
{
	if(!m_valid) return false;
	const char *filename = m_filename.c_str();
	if(m_config.SaveToFile(filename))
	{
		printf("[INSTANCE]Info: %s saved\n", filename);
		return true;
	}
	return false;
}

Instance::~Instance()
{
	SaveToFile();
}

