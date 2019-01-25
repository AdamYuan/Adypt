//
// Created by adamyuan on 1/17/19.
//

#ifndef ADYPT_WIDEBVHBUILDER_HPP
#define ADYPT_WIDEBVHBUILDER_HPP

#include "SBVH.hpp"
#include "WideBVH.hpp"
#include "../Util/Platform.hpp"

//build wide bvh from a binary sbvh
class WideBVHBuilder
{
private:
	const SBVH &m_sbvh;
	WideBVH *m_wbvh;

	struct NodeCost
	{
		float m_sah;
		enum Type { kInternal = 0, kLeaf, kDistribute };
		int32_t m_type;
		int32_t m_distribute[2];
	};
	struct NodeCostGroup
	{
		NodeCost m_arr[7];
		NodeCost &operator[] (int i){ return m_arr[i-1]; }
	};


	std::vector<NodeCostGroup> m_costs;
	int calculate_cost(int node_idx);

	//{node_idx, i} are the two dimensions of dp array
	//out_size describes the number of children
	//out_idx  stores the children index
	void fetch_children(int node_idx, int i, int *out_size, int out_node_idx[8]);

	//
	int fetch_leaves(int node_idx);

	//hungarian algorithm to solve the min-assignment problem
	void hungarian(const float mat[8][8], int n, int order[8]);

	void create_nodes(int wbvh_node_idx, int sbvh_node_idx);
public:
	WideBVHBuilder(const Platform::BVHConfig &config, WideBVH *wbvh, const SBVH &sbvh) : m_wbvh(wbvh), m_sbvh(sbvh)
	{
		m_wbvh->m_nodes.clear();
		m_wbvh->m_tri_indices.clear();
		m_wbvh->m_config = config;
	}
	void Run();
};


#endif //ADYPT_WIDEBVHBUILDER_HPP
