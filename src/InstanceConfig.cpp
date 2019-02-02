//
// Created by adamyuan on 1/31/19.
//

#include "InstanceConfig.hpp"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <fstream>

bool InstanceConfig::LoadFromFile(const char *filename)
{
	std::ifstream in{filename};
	if(!in.is_open())
		return false;

	//macros to check whether identifier in document "d" is "o" (Object, Int, etc...)
#define CHECK(d, x, o) if(!d[x].Is##o()) { printf("[PARSER]ERR: undefined " #o " " #x " in " #d "\n"); return false; }
#define CHECK_SET(d, x, o, v) { CHECK(d, x, o) v = d[x].Get##o(); }

	rapidjson::Document document;
	{
		std::string src{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
		if(document.Parse(src.c_str()).HasParseError())
		{
			printf("[PARSER]ERR: Failed to parse json\n");
			return false;
		}
	}
	if(!document.IsObject())
	{
		printf("[PARSER]ERR: Failed to parse json\n");
		return false;
	}

	CHECK_SET(document, "width", Uint, m_width)
	CHECK_SET(document, "height", Uint, m_height)

	CHECK(document, "scene", Object)
	{
		const auto &scn_obj = document["scene"].GetObject();
		CHECK_SET(scn_obj, "filename", String, m_obj_filename);
	}


	CHECK(document, "pathTracer", Object)
	{
		const auto &pt_obj = document["pathTracer"].GetObject();
		CHECK_SET(pt_obj, "invocationSize", Uint, m_pt_cfg.m_invocation_size)
		CHECK_SET(pt_obj, "stackSize", Uint, m_pt_cfg.m_stack_size)
		CHECK_SET(pt_obj, "maxBounce", Uint, m_pt_cfg.m_max_bounce)
		CHECK_SET(pt_obj, "subpixel", Uint, m_pt_cfg.m_subpixel)
		CHECK_SET(pt_obj, "tmpLifetime", Uint, m_pt_cfg.m_tmp_lifetime)
		CHECK_SET(pt_obj, "rayTMin", Float, m_pt_cfg.m_ray_tmin)
		CHECK_SET(pt_obj, "clamp", Float, m_pt_cfg.m_clamp)

		CHECK(pt_obj, "sun", Array)
		const auto &sun_arr = pt_obj["sun"].GetArray();
		if(sun_arr.Size() != 3)
		{
			printf("[PARSER]ERR: size of \"sun\" array is not 3\n");
			return false;
		}
		for(int i = 0; i < 3; ++i) CHECK_SET(sun_arr, i, Float, m_pt_cfg.m_sun[i])
	}

	CHECK(document, "bvh", Object)
	{
		const auto &bvh_obj = document["bvh"].GetObject();
		CHECK_SET(bvh_obj, "filename", String, m_bvh_filename);

		CHECK_SET(bvh_obj, "maxSpatialDepth", Uint, m_bvh_cfg.m_max_spatial_depth)
		CHECK_SET(bvh_obj, "triangleSAH", Float, m_bvh_cfg.m_triangle_sah)
		CHECK_SET(bvh_obj, "nodeSAH", Float, m_bvh_cfg.m_node_sah)
	}


	CHECK(document, "camera", Object)
	{
		const auto &cam_obj = document["camera"].GetObject();

		CHECK_SET(cam_obj, "speed", Float, m_cam_cfg.m_speed);
		CHECK_SET(cam_obj, "mouseSensitive", Float, m_cam_cfg.m_mouse_sensitive);
		CHECK_SET(cam_obj, "fov", Float, m_cam_cfg.m_fov);
		CHECK_SET(cam_obj, "yaw", Float, m_cam_cfg.m_yaw);
		CHECK_SET(cam_obj, "pitch", Float, m_cam_cfg.m_pitch);

		CHECK(cam_obj, "position", Array)
		const auto &pos_arr = cam_obj["position"].GetArray();
		if(pos_arr.Size() != 3)
		{
			printf("[PARSER]ERR: size of \"position\" array is not 3\n");
			return false;
		}
		for(int i = 0; i < 3; ++i) CHECK_SET(pos_arr, i, Float, m_cam_cfg.m_position[i])
	}

	return true;

#undef CHECK
#undef CHECK_SET
}

std::string InstanceConfig::GetJson() const
{
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();

	writer.Key("width");
	writer.Int(m_width);
	writer.Key("height");
	writer.Int(m_height);

	//scene object
	writer.Key("scene");
	writer.StartObject();
	{
		writer.Key("filename");
		writer.String(m_obj_filename.c_str());
	}
	writer.EndObject();

	//pathtracer object
	writer.Key("pathTracer");
	writer.StartObject();
	{
		writer.Key("invocationSize");
		writer.Int(m_pt_cfg.m_invocation_size);
		writer.Key("stackSize");
		writer.Int(m_pt_cfg.m_stack_size);
		writer.Key("maxBounce");
		writer.Int(m_pt_cfg.m_max_bounce);
		writer.Key("subpixel");
		writer.Int(m_pt_cfg.m_subpixel);
		writer.Key("tmpLifetime");
		writer.Int(m_pt_cfg.m_tmp_lifetime);
		writer.Key("rayTMin");
		writer.Double(m_pt_cfg.m_ray_tmin);
		writer.Key("clamp");
		writer.Double(m_pt_cfg.m_clamp);
		writer.Key("sun");
		writer.StartArray();
		for (int i = 0; i < 3; ++i)
			writer.Double(m_pt_cfg.m_sun[i]);
		writer.EndArray();
	}
	writer.EndObject();

	//bvh object
	writer.Key("bvh");
	writer.StartObject();
	{
		writer.Key("filename");
		writer.String(m_bvh_filename.c_str());

		writer.Key("maxSpatialDepth");
		writer.Int(m_bvh_cfg.m_max_spatial_depth);
		writer.Key("triangleSAH");
		writer.Double(m_bvh_cfg.m_triangle_sah);
		writer.Key("nodeSAH");
		writer.Double(m_bvh_cfg.m_node_sah);
	}
	writer.EndObject();

	//camera object
	writer.Key("camera");
	writer.StartObject();
	{
		writer.Key("speed");
		writer.Double(m_cam_cfg.m_speed);
		writer.Key("mouseSensitive");
		writer.Double(m_cam_cfg.m_mouse_sensitive);
		writer.Key("fov");
		writer.Double(m_cam_cfg.m_fov);
		writer.Key("yaw");
		writer.Double(m_cam_cfg.m_yaw);
		writer.Key("pitch");
		writer.Double(m_cam_cfg.m_pitch);

		writer.Key("position");
		writer.StartArray();
		for (int i = 0; i < 3; ++i)
			writer.Double(m_cam_cfg.m_position[i]);
		writer.EndArray();
	}
	writer.EndObject();

	writer.EndObject();

	return std::string{buffer.GetString()};
}

void InstanceConfig::SetDefault()
{
	m_pt_cfg = PT{};
	m_bvh_cfg = BVH{};
	m_cam_cfg = Cam{};
	m_width = 1280;
	m_height = 720;
}

bool InstanceConfig::SaveToFile(const char *filename) const
{
	std::ofstream out{filename};
	if(!out.is_open()) return false;

	out << GetJson();
	return true;
}
