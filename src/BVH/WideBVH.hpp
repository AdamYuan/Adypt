//
// Created by adamyuan on 1/15/19.
//

#ifndef ADYPT_WIDEBVH_HPP
#define ADYPT_WIDEBVH_HPP

#include <cinttypes>
#include <vector>
#include "../InstanceConfig.hpp"

//a compressed 80 byte bvh node
struct WideBVHNode
{
	float m_px, m_py, m_pz;
	uint8_t m_ex, m_ey, m_ez, m_imask;
	uint32_t m_child_idx_base; //child node base index
	uint32_t m_tri_idx_base; //triangle base index
	uint8_t m_meta[8];
	uint8_t m_qlox[8];
	uint8_t m_qloy[8];
	uint8_t m_qloz[8];
	uint8_t m_qhix[8];
	uint8_t m_qhiy[8];
	uint8_t m_qhiz[8];
};

class WideBVH
{
private:
	//for saving file
	static constexpr const char *kVersionStr = "CWBVH_1.0";
	std::vector<WideBVHNode> m_nodes;
	std::vector<int32_t> m_tri_indices;
public:
	WideBVH() = default;
	bool LoadFromFile(const char *filename, const InstanceConfig::BVH &expected_config);
	bool SaveToFile(const char *filename, const InstanceConfig::BVH &config);
	const std::vector<WideBVHNode> &GetNodes() const { return m_nodes; }
	const std::vector<int> &GetTriIndices() const { return m_tri_indices; }

	friend class WideBVHBuilder;
};


#endif //ADYPT_WIDEBVH_HPP
