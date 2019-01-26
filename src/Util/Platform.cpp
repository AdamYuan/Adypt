//
// Created by adamyuan on 1/7/19.
//

#include "Platform.hpp"
#include <fstream>
#include <map>
#include <cstring>

void Platform::Load(const char *filename)
{
	//parse name
	{
		const char *s = filename + strlen(filename);
		while(*(s - 1) != '/' && *(s - 1) != '\\' && s > filename) s --;
		m_name = {s, filename + strlen(filename)};

		auto p = m_name.find_last_of('.');
		m_name = m_name.substr(0, p);
	}
	std::map<std::string, std::string> dict;

	//parse file to dict
	{
		std::ifstream in{filename};
		if(!in.is_open())
		{
			printf("[PARSER]ERR: failed to load config file %s\n", filename);
			return;
		}
		std::string line, left, right;
		while(getline(in, line))
		{
			auto loc = line.find('=');
			if(loc == std::string::npos)
				continue;
			left = line.substr(0, loc);
			right = line.substr(loc + 1);
			estimate_blanks(&left);
			estimate_blanks(&right);
			for(auto &i : left) i = (char)tolower(i);
			dict[left] = right;
		}
	}

	//bvh
	parse_int(&dict, "bvh.maxspatialdepth", &m_bvh_config.m_max_spatial_depth);
	parse_float(&dict, "bvh.trianglesah", &m_bvh_config.m_triangle_sah);
	parse_float(&dict, "bvh.nodesah", &m_bvh_config.m_node_sah);

	//pt
	parse_int(&dict, "pt.stacksize", &m_pt_config.m_stack_size);
	parse_int(&dict, "pt.invocationsize", &m_pt_config.m_invocation_size);
	parse_int(&dict, "pt.maxbounce", &m_pt_config.m_max_bounce);
	parse_int(&dict, "pt.subpixel", &m_pt_config.m_subpixel);
	parse_int(&dict, "pt.tmplifetime", &m_pt_config.m_tmp_lifetime);
	parse_float(&dict, "pt.raytmin", &m_pt_config.m_ray_tmin);
	parse_float(&dict, "pt.minglossyexp", &m_pt_config.m_min_glossy_exp);
	parse_float(&dict, "pt.sun.r", &m_pt_config.m_sun_r);
	parse_float(&dict, "pt.sun.g", &m_pt_config.m_sun_g);
	parse_float(&dict, "pt.sun.b", &m_pt_config.m_sun_b);

	//ui
	parse_int(&dict, "ui.width", &m_ui_config.m_width);
	parse_int(&dict, "ui.height", &m_ui_config.m_height);
	parse_float(&dict, "ui.cameraspeed", &m_ui_config.m_camera_speed);
	parse_float(&dict, "ui.mousesensitive", &m_ui_config.m_mouse_sensitive);
	parse_float(&dict, "ui.fov", &m_ui_config.m_fov);

	//files
	parse_string(&dict, "objfile", &m_obj_filename);
	parse_string(&dict, "bvhfile", &m_bvh_filename);

	for(auto &i : dict)
		printf("[PARSER]WARN: invalid key %s\n", i.first.c_str());
}

Platform::Platform(const char *filename)
{
	Load(filename);
}

void Platform::parse_string(std::map<std::string, std::string> *dict, const std::string &key, std::string *val)
{
	auto it = dict->find(key);
	if(it == dict->end())
	{
		printf("[PARSER]ERR: STRING %s not found\n", key.c_str());
		return;
	}
	*val = it->second;
	dict->erase(it);
}

void Platform::parse_float(std::map<std::string, std::string> *dict, const std::string &key, float *val)
{
	auto it = dict->find(key);
	if(it == dict->end())
	{
		printf("[PARSER]INFO: FLOAT %s not found, use %f as default\n", key.c_str(), *val);
		return;
	}
	float v;
	try { v = std::stof(it->second); }
	catch(...) { printf("[PARSER]ERR: failed to parse %s = %s, expected FLOAT value\n", key.c_str(), it->second.c_str()); return; }
	*val = v;
	dict->erase(it);
}

void Platform::parse_int(std::map<std::string, std::string> *dict, const std::string &key, int *val)
{
	auto it = dict->find(key);
	if(it == dict->end())
	{
		printf("[PARSER]INFO: INT %s not found, use %d as default\n", key.c_str(), *val);
		return;
	}
	int v;
	try { v = std::stoi(it->second); }
	catch(...) { printf("[PARSER]ERR: failed to parse %s = %s, expected INT value\n", key.c_str(), it->second.c_str()); return; }
	*val = v;
	dict->erase(it);
}

void Platform::estimate_blanks(std::string *str)
{
	size_t l = (int)str->size(), r = 0;
	for(size_t i = 0; i < str->size(); ++i)
	{
		if(!isblank(str->at(i)))
		{
			l = std::min(l, i);
			r = std::max(r, i + 1);
		}
	}
	*str = str->substr(l, r - l);
}

