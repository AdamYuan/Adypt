//
// Created by adamyuan on 1/15/19.
//

#include "WideBVH.hpp"
#include <fstream>
#include <cstring>

void WideBVH::Save(const char *filename)
{
	std::ofstream os{filename, std::ios::binary};
	os.write(kVersionStr, strlen(kVersionStr) + 1);
	os.write((char *)&m_config, sizeof(Platform::BVHConfig));
	auto tri_indeices_size = (uint32_t)m_tri_indices.size();
	os.write((char *)&tri_indeices_size, sizeof(uint32_t));
	os.write((char *)m_tri_indices.data(), m_tri_indices.size() * sizeof(int32_t));
	os.write((char *)m_nodes.data(), m_nodes.size() * sizeof(WideBVHNode));
}

void WideBVH::Load(const char *filename)
{
	std::ifstream is{filename, std::ios::binary};
	if(!is.is_open()) return;
	std::vector<char> buffer{std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};
	char *ptr = buffer.data(), *end = buffer.data() + buffer.size();

	//compare cwbvh version
	if(strcmp(ptr, kVersionStr) != 0)
		return;
	ptr += strlen(kVersionStr) + 1;

	m_config = *(Platform::BVHConfig *)ptr;
	ptr += sizeof(Platform::BVHConfig);


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
}
