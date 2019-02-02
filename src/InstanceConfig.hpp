//
// Created by adamyuan on 1/31/19.
//

#ifndef ADYPT_INSTANCECONFIG_HPP
#define ADYPT_INSTANCECONFIG_HPP

#include <cinttypes>
#include <glm/glm.hpp>
#include <string>

struct InstanceConfig
{
//bvh configuration
	struct BVH
	{
		int32_t m_max_spatial_depth = 48; float m_triangle_sah = 0.3, m_node_sah = 1.0f;
		inline float GetTriangleCost(int count) const { return m_triangle_sah * count; }
		inline float GetNodeCost(int count) const { return m_node_sah * count; }
	};
//gpu path tracer configuration
	struct PT
	{
		int32_t m_invocation_size = 8, m_stack_size = 12, m_max_bounce = 5,
				m_subpixel = 8, m_tmp_lifetime = 16; //each TMP_LIFETIME frame, refresh primary ray buffer
		float m_ray_tmin = 0.0001f, m_clamp = 4.0f; //enable glossy reflection only if exp exceeds MIN_GLOSSY_EXP
		glm::vec3 m_sun;
	};
//camera configuration
	struct Cam
	{
		float m_speed = 1.0f, m_mouse_sensitive = 0.3f, m_fov = 45.0f,
				m_yaw{}, m_pitch{};
		glm::vec3 m_position;
	};

	int m_width = 1280, m_height = 720;
	BVH m_bvh_cfg;
	PT m_pt_cfg;
	Cam m_cam_cfg;

	std::string m_obj_filename, m_bvh_filename;

	bool LoadFromFile(const char *filename);
	bool SaveToFile(const char *filename) const;
	std::string GetJson() const;
	void SetDefault();
};


#endif //ADYPT_INSTANCECONFIG_HPP
