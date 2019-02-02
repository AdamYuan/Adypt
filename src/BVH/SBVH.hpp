//
// Created by adamyuan on 10/7/18.
//

#ifndef YART_BVH_HPP
#define YART_BVH_HPP

#include <vector>
#include "../Util/Scene.hpp"

struct SBVHNode //a bvh with only 1 primitive per node
{
	AABB m_aabb;
	int m_tri_idx; //triangle index
	int m_left_idx; //left node index (right node index = current + 1)
};
class SBVH
{
private:
	std::vector<SBVHNode> m_nodes;
	int m_leaves_cnt;

public:
	SBVH() = default;
	const std::vector<SBVHNode> &GetNodes() const { return m_nodes; }
	bool Empty() const { return m_nodes.empty(); }
	bool IsLeaf(int idx) const { return m_nodes[idx].m_left_idx == -1; }
	int GetLeft(int idx) const { return m_nodes[idx].m_left_idx; }
	int GetRight(int idx) const { return idx + 1; }
	int GetTriIdx(int idx) const { return m_nodes[idx].m_tri_idx; }
	const AABB &GetBox(int idx) const { return m_nodes[idx].m_aabb; }

	int GetLeafCount() const { return m_leaves_cnt; }
	friend class SBVHBuilder;
};


#endif //YART_BVH_HPP
