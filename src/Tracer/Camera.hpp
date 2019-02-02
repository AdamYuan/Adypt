//
// Created by adamyuan on 2/1/19.
//

#ifndef ADYPT_CAMERA_HPP
#define ADYPT_CAMERA_HPP

#endif //ADYPT_CAMERA_HPP

#include <GLFW/glfw3.h>
#include <mygl3/utils/framerate.hpp>
#include "../InstanceConfig.hpp"

class Camera
{
private:
	InstanceConfig::Cam *m_config;
	float m_aspect_ratio;
	void move_forward(float dist, float dir) //degrees
	{
		float rad = glm::radians(m_config->m_yaw + dir);
		m_config->m_position.x -= glm::sin(rad) * dist;
		m_config->m_position.z -= glm::cos(rad) * dist;
	}

public:
	void Initialize(InstanceConfig::Cam *cam_cfg, int width, int height)
	{
		m_config = cam_cfg;
		m_aspect_ratio = width / (float)height;
	}

	glm::mat4 GetView() const;
	glm::mat4 GetProjection() const;

	void Control(GLFWwindow *window, const mygl3::Framerate &fps);
};
