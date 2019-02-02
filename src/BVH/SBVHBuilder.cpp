//
// Created by adamyuan on 10/7/18.
//

#include "SBVHBuilder.hpp"
#include <chrono>

int SBVHBuilder::build_node(const NodeSpec &t_spec, int t_depth)
{
	if(t_spec.m_ref_num == 1) return build_leaf(t_spec);

	float area = t_spec.m_aabb.GetArea();
	float node_sah = area * m_config.GetNodeCost(2); //C_t * SA(B)

	ObjectSplit object_split;
	find_object_split(t_spec, &object_split, node_sah);

	SpatialSplit spatial_split;
	spatial_split.m_sah = FLT_MAX;
	if(t_depth <= m_config.m_max_spatial_depth)
	{
		AABB overlap = object_split.m_left_aabb;
		overlap.IntersectAABB(object_split.m_right_aabb);
		if(overlap.GetArea() >= m_min_overlap_area)
			find_spatial_split(t_spec, &spatial_split, node_sah);
	}

	int node = push_node(); //alloc new node
	m_bvh->m_nodes[node].m_aabb = t_spec.m_aabb;

	NodeSpec left, right; left.m_ref_num = right.m_ref_num = 0;
	if(spatial_split.m_sah < object_split.m_sah)
		perform_spatial_split(t_spec, spatial_split, &left, &right);

	if(left.m_ref_num == 0 || right.m_ref_num == 0)
		perform_object_split(t_spec, object_split, &left, &right);

	build_node(right, t_depth + 1);
	//use a temp variable to get the return value()
	int lidx = build_node(left, t_depth + 1);
	m_bvh->m_nodes[node].m_left_idx = lidx;

	return node;
}

void SBVHBuilder::Run()
{
	m_right_aabbs.reserve(m_scene.GetTriangles().size());

	//init reference stack
	m_refstack.reserve(m_scene.GetTriangles().size()*2);
	m_refstack.resize(m_scene.GetTriangles().size());
	for(int i = 0; i < m_scene.GetTriangles().size(); ++i)
	{
		m_refstack[i].m_tri_index = i;
		m_refstack[i].m_aabb = m_scene.GetTriangles()[i].GetAABB();
	}

	m_min_overlap_area = m_scene.GetAABB().GetArea() * 1e-5f;

	m_bvh->m_nodes.reserve(m_scene.GetTriangles().size() * 2);

	auto start = std::chrono::steady_clock::now();
	build_node({m_scene.GetAABB(), (int) m_scene.GetTriangles().size()}, 0);

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	printf("\n[SBVH]building lasted %ld ms\n", duration.count());

	m_bvh->m_nodes.shrink_to_fit();
	printf("[SBVH]built with %ld nodes\n", m_bvh->m_nodes.size());
}
