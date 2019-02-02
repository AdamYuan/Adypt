//
// Created by adamyuan on 1/15/19.
//

#include "WideBVH.hpp"
#include <fstream>
#include <cstring>

bool WideBVH::SaveToFile(const char *filename, const InstanceConfig::BVH &config)
{
	std::ofstream os{filename, std::ios::binary};
	if(!os.is_open())
		return false;

	os.write(kVersionStr, strlen(kVersionStr) + 1);
	os.write((char *)&config, sizeof(InstanceConfig::BVH));
	auto tri_indeices_size = (uint32_t)m_tri_indices.size();
	os.write((char *)&tri_indeices_size, sizeof(uint32_t));
	os.write((char *)m_tri_indices.data(), m_tri_indices.size() * sizeof(int32_t));
	os.write((char *)m_nodes.data(), m_nodes.size() * sizeof(WideBVHNode));

	return true;
}

bool WideBVH::LoadFromFile(const char *filename, const InstanceConfig::BVH &expected_config)
{
	std::ifstream is{filename, std::ios::binary};
	if(!is.is_open()) return false;

	std::vector<char> buffer{std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};
	char *ptr = buffer.data(), *end = buffer.data() + buffer.size();

	//compare cwbvh version
	if(strcmp(ptr, kVersionStr) != 0)
		return false;

	ptr += strlen(kVersionStr) + 1;

	auto config = *(InstanceConfig::BVH *)ptr;
	ptr += sizeof(InstanceConfig::BVH);

	if(config.m_node_sah != expected_config.m_node_sah ||
	   config.m_triangle_sah != expected_config.m_triangle_sah ||
	   config.m_max_spatial_depth != expected_config.m_max_spatial_depth)
		return false;


	uint32_t tri_indices_size = *(uint32_t *)ptr;
	ptr += sizeof(uint32_t);

	//read triangle indices
	for(int i = 0; i < tri_indices_size && ptr < end; ++i)
	{
		m_tri_indices.push_back(*(int32_t *)ptr);
		ptr += sizeof(int32_t);
	}

	//read node
	while(ptr < end)
	{
		m_nodes.push_back(*(WideBVHNode *)ptr);
		ptr += sizeof(WideBVHNode);
	}

	return true;
}
