//
// Created by adamyuan on 1/7/19.
//

#ifndef ADYPT_PLATFORM_HPP
#define ADYPT_PLATFORM_HPP

#include <string>
#include <cctype>
#include <map>

class Platform
{
public:
//bvh configuration
	struct BVHConfig
	{
		int32_t m_max_spatial_depth = 48; float m_triangle_sah = 0.3, m_node_sah = 1.0f;
		inline float GetTriangleCost(int count) { return m_triangle_sah * count; }
		inline float GetNodeCost(int count) { return m_node_sah * count; }
		bool operator== (const BVHConfig &r) const
		{
			return m_max_spatial_depth == r.m_max_spatial_depth &&
				   m_triangle_sah == r.m_triangle_sah &&
				   m_node_sah == r.m_node_sah;
		}
		bool operator!= (const BVHConfig &r) const
		{
			return m_max_spatial_depth != r.m_max_spatial_depth ||
				   m_triangle_sah != r.m_triangle_sah ||
				   m_node_sah != r.m_node_sah;
		}
	};
//gpu path tracer configuration
	struct PTConfig
	{
		int32_t m_invocation_size = 8, m_stack_size = 24, m_max_bounce = 5,
			m_subpixel = 8, m_tmp_lifetime = 16; //each TMP_LIFETIME frame, refresh primary ray buffer
		float m_ray_tmin = 0.0001f, m_min_glossy_exp = 4.0f, m_clamp = 4.0f; //enable glossy reflection only if exp exceeds MIN_GLOSSY_EXP
		float m_sun_r = 6.0f, m_sun_g = 5.6f, m_sun_b = 5.2f; //ambient color of the sun
	};
//interface configuration
	struct UIConfig
	{ int32_t m_width = 1280, m_height = 720; float m_camera_speed = 0.05f, m_mouse_sensitive = 0.3f, m_fov = 45.0f; };

private:
	BVHConfig m_bvh_config;
	PTConfig m_pt_config;
	UIConfig m_ui_config;
	std::string m_obj_filename, m_bvh_filename, m_name;
	static void estimate_blanks(std::string *str);
	static void parse_int(std::map<std::string, std::string> *dict, const std::string &key, int *val);
	static void parse_float(std::map<std::string, std::string> *dict, const std::string &key, float *val);
	static void parse_string(std::map<std::string, std::string> *dict, const std::string &key, std::string *val);
public:
	Platform() = default;
	Platform(const char *filename);
	void Load(const char *filename);
	const BVHConfig &GetBVHConfig() const { return m_bvh_config; }
	const PTConfig &GetPTConfig() const { return m_pt_config; }
	const UIConfig &GetUIConfig() const { return m_ui_config; }
	const char *GetObjFilename() const { return m_obj_filename.c_str(); }
	const char *GetBVHFilename() const { return m_bvh_filename.c_str(); }
	const char *GetName() const { return m_name.c_str(); }
	bool Invalid() const { return m_bvh_filename.empty() || m_obj_filename.empty(); }
};


#endif //ADYPT_PLATFORM_HPP
