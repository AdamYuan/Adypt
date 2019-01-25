//
// Created by adamyuan on 10/7/18.
//

#ifndef YART_SBVHBUILDER_HPP
#define YART_SBVHBUILDER_HPP

#include "../Util/Scene.hpp"
#include "SBVH.hpp"
#include <glm/gtx/compatibility.hpp>
#include <algorithm>

//build an SBVH with only one triangle per leaf
class SBVHBuilder
{
private:
	static constexpr int kSpatialBinNum = 32;

	struct Reference { AABB m_aabb; int m_tri_index; };
	struct NodeSpec { AABB m_aabb; int m_ref_num; };
	struct ObjectSplit { AABB m_left_aabb, m_right_aabb; int m_dim, m_left_num; float m_sah; };
	struct SpatialSplit { int m_dim; float m_pos, m_sah; };
	struct SpatialBin { AABB m_aabb; int m_in, m_out; };

	SBVH *m_bvh;
	const Scene &m_scene;

	SpatialBin m_spatial_bins[kSpatialBinNum];

	std::vector<AABB> m_right_aabbs; //store right aabb result
	std::vector<Reference> m_refstack;

	float m_min_overlap_area;

	//for std::sort
	template<int DIM> inline static bool reference_cmp(const Reference &l, const Reference &r);
	template<int DIM> inline void sort_spec(const NodeSpec &t_spec);
	inline void sort_spec(const NodeSpec &t_spec, int dim);
	inline int get_ref_index(const NodeSpec &t_spec) { return (int)m_refstack.size() - t_spec.m_ref_num; }
	inline int build_leaf(const NodeSpec &t_spec);

	template<int DIM>
	inline void __find_object_split_dim(const NodeSpec &t_spec, ObjectSplit *t_os, float t_node_sah);
	inline void find_object_split(const NodeSpec &t_spec, ObjectSplit *t_os, float t_node_sah);
	inline void perform_object_split(const NodeSpec &t_spec, const ObjectSplit &t_os, NodeSpec *t_left, NodeSpec *t_right);
	inline void split_reference(const Reference &t_ref, int t_dim, float t_pos, Reference *t_left, Reference *t_right);
	template<int DIM>
	inline void __find_spatial_split_dim(const NodeSpec &t_spec, SpatialSplit *t_ss, float t_node_sah);
	inline void find_spatial_split(const NodeSpec &t_spec, SpatialSplit *t_ss, float t_node_sah);
	inline void perform_spatial_split(const NodeSpec &t_spec, const SpatialSplit &t_ss, NodeSpec *t_left, NodeSpec *t_right);
	int build_node(const NodeSpec &t_spec, int t_depth);
	inline int push_node() { m_bvh->m_nodes.emplace_back(); return m_bvh->m_nodes.size() - 1; }

public:
	SBVHBuilder(const Platform::BVHConfig &config, SBVH *bvh, const Scene &scene) : m_scene(scene), m_bvh(bvh)
	{
		m_bvh->m_config = config;
		m_bvh->m_leaves_cnt = 0;
		m_bvh->m_nodes.clear();
	}
	void Run();
};

template<int DIM>
bool SBVHBuilder::reference_cmp(const SBVHBuilder::Reference &l, const SBVHBuilder::Reference &r)
{
	float lc = l.m_aabb.GetCenter()[DIM], rc = r.m_aabb.GetCenter()[DIM];
	return lc < rc || (lc == rc && l.m_tri_index < r.m_tri_index);
}

template<int DIM>
void SBVHBuilder::sort_spec(const SBVHBuilder::NodeSpec &t_spec)
{
	std::sort(m_refstack.data() + m_refstack.size() - t_spec.m_ref_num,
			  m_refstack.data() + m_refstack.size(), reference_cmp<DIM>);
}

void SBVHBuilder::sort_spec(const SBVHBuilder::NodeSpec &t_spec, int dim)
{
	std::sort(m_refstack.data() + m_refstack.size() - t_spec.m_ref_num,
			  m_refstack.data() + m_refstack.size(), dim!=0 ? (dim==1?reference_cmp<1>:reference_cmp<2>) : reference_cmp<0>);
}

int SBVHBuilder::build_leaf(const SBVHBuilder::NodeSpec &t_spec)
{
	int node = push_node();
	m_bvh->m_nodes[node].m_aabb = t_spec.m_aabb;
	m_bvh->m_nodes[node].m_left_idx = -1; //mark to -1 for leaf
	m_bvh->m_nodes[node].m_tri_idx = m_refstack.back().m_tri_index;
	m_refstack.pop_back();
	printf("\r[SBVH]created %d leaves", ++m_bvh->m_leaves_cnt);
	return node;
}

template<int DIM>
void SBVHBuilder::__find_object_split_dim(const SBVHBuilder::NodeSpec &t_spec, SBVHBuilder::ObjectSplit *t_os,
										  float t_node_sah)
{
	sort_spec<DIM>(t_spec);
	Reference *refs = m_refstack.data() + get_ref_index(t_spec);

	//get the aabb from right
	m_right_aabbs.resize((size_t)t_spec.m_ref_num);
	m_right_aabbs[t_spec.m_ref_num - 1] = refs[t_spec.m_ref_num - 1].m_aabb;
	for(int i = t_spec.m_ref_num - 2; i >= 1; --i)
		m_right_aabbs[i] = AABB(refs[i].m_aabb, m_right_aabbs[i + 1]);

	AABB left_aabb = refs->m_aabb;
	for(int i = 1; i <= t_spec.m_ref_num - 1; ++i)
	{
		float sah = t_node_sah + m_bvh->m_config.GetTriangleCost(i) * left_aabb.GetArea() +
				m_bvh->m_config.GetTriangleCost(t_spec.m_ref_num - i) * m_right_aabbs[i].GetArea();
		if(sah < t_os->m_sah)
		{
			t_os->m_dim = DIM;
			t_os->m_left_num = i;
			t_os->m_left_aabb = left_aabb;
			t_os->m_right_aabb = m_right_aabbs[i];
			t_os->m_sah = sah;
		}

		left_aabb.Expand(refs[i].m_aabb);
	}
}

void
SBVHBuilder::find_object_split(const SBVHBuilder::NodeSpec &t_spec, SBVHBuilder::ObjectSplit *t_os, float t_node_sah)
{
	t_os->m_sah = FLT_MAX;

	__find_object_split_dim<0>(t_spec, t_os, t_node_sah);
	__find_object_split_dim<1>(t_spec, t_os, t_node_sah);
	__find_object_split_dim<2>(t_spec, t_os, t_node_sah);
}

void SBVHBuilder::split_reference(const SBVHBuilder::Reference &t_ref, int t_dim, float t_pos,
								  SBVHBuilder::Reference *t_left, SBVHBuilder::Reference *t_right)
//if the part is invalid, set m_tri_index to -1
{
	t_left->m_aabb = t_right->m_aabb = AABB();
	t_left->m_tri_index = t_right->m_tri_index = t_ref.m_tri_index;

	const Triangle &tri = m_scene.GetTriangles()[t_ref.m_tri_index];
	for(int i = 0; i < 3; ++i)
	{
		const glm::vec3 &v0 = tri.m_positions[i], &v1 = tri.m_positions[(i+1)%3];
		float p0 = v0[t_dim], p1 = v1[t_dim];
		if(p0 <= t_pos) t_left->m_aabb.Expand(v0);
		if(p0 >= t_pos) t_right->m_aabb.Expand(v0);

		if((p0 < t_pos && t_pos < p1) || (p1 < t_pos && t_pos < p0)) //process edge
		{
			glm::vec3 x = glm::lerp(v0, v1, glm::clamp((t_pos - p0) / (p1 - p0), 0.0f, 1.0f));
			t_left->m_aabb.Expand(x);
			t_right->m_aabb.Expand(x);
		}
	}

	t_left->m_aabb.m_max[t_dim] = t_pos;
	t_left->m_aabb.IntersectAABB(t_ref.m_aabb);

	t_right->m_aabb.m_min[t_dim] = t_pos;
	t_right->m_aabb.IntersectAABB(t_ref.m_aabb);
}

template<int DIM>
void SBVHBuilder::__find_spatial_split_dim(const SBVHBuilder::NodeSpec &t_spec, SBVHBuilder::SpatialSplit *t_ss,
										   float t_node_sah)
{
	std::fill(m_spatial_bins, m_spatial_bins + kSpatialBinNum, SpatialBin{AABB(), 0, 0}); //initialize bins

	float bin_width = t_spec.m_aabb.GetExtent()[DIM] / kSpatialBinNum, inv_bin_width = 1.0f / bin_width;
	float bound_base = t_spec.m_aabb.m_min[DIM];
	Reference *refs = m_refstack.data() + get_ref_index(t_spec);
	Reference cur_ref, left_ref, right_ref;

	//put references into bins
	for(int i = 0; i < t_spec.m_ref_num; ++i)
	{
		int bin = glm::clamp(int( (refs[i].m_aabb.m_min[DIM] - bound_base) * inv_bin_width ), 0, kSpatialBinNum - 1);
		int last_bin = glm::clamp(int( (refs[i].m_aabb.m_max[DIM] - bound_base) * inv_bin_width ), 0, kSpatialBinNum - 1);
		m_spatial_bins[bin].m_in ++;
		cur_ref = refs[i];
		for(; bin < last_bin; ++bin)
		{
			split_reference(cur_ref, DIM, (bin + 1)*bin_width + bound_base, &left_ref, &right_ref);
			m_spatial_bins[bin].m_aabb.Expand(left_ref.m_aabb);
			cur_ref = right_ref;
		}
		m_spatial_bins[last_bin].m_aabb.Expand(cur_ref.m_aabb);
		m_spatial_bins[last_bin].m_out ++;
	}

	//get the aabb from right
	m_right_aabbs.resize(kSpatialBinNum);
	m_right_aabbs[kSpatialBinNum - 1] = m_spatial_bins[kSpatialBinNum - 1].m_aabb;
	for(int i = kSpatialBinNum - 2; i >= 1; --i)
		m_right_aabbs[i] = AABB(m_spatial_bins[i].m_aabb, m_right_aabbs[i + 1]);

	//compare sah
	AABB left_aabb = m_spatial_bins[0].m_aabb;
	int left_num = 0, right_num = t_spec.m_ref_num;
	for(int i = 1; i < kSpatialBinNum; ++i)
	{
		left_num += m_spatial_bins[i - 1].m_in;
		right_num -= m_spatial_bins[i - 1].m_out;

		float sah = t_node_sah + m_bvh->m_config.GetTriangleCost(left_num) * left_aabb.GetArea() +
				m_bvh->m_config.GetTriangleCost(right_num) * m_right_aabbs[i].GetArea();
		if(sah < t_ss->m_sah)
		{
			t_ss->m_sah = sah;
			t_ss->m_dim = DIM;
			t_ss->m_pos = bound_base + i*bin_width;
		}

		left_aabb.Expand(m_spatial_bins[i].m_aabb);
	}
}

void
SBVHBuilder::find_spatial_split(const SBVHBuilder::NodeSpec &t_spec, SBVHBuilder::SpatialSplit *t_ss, float t_node_sah)
{
	t_ss->m_sah = FLT_MAX;
	__find_spatial_split_dim<0>(t_spec, t_ss, t_node_sah);
	__find_spatial_split_dim<1>(t_spec, t_ss, t_node_sah);
	__find_spatial_split_dim<2>(t_spec, t_ss, t_node_sah);
}

void SBVHBuilder::perform_spatial_split(const SBVHBuilder::NodeSpec &t_spec, const SBVHBuilder::SpatialSplit &t_ss,
										SBVHBuilder::NodeSpec *t_left, SBVHBuilder::NodeSpec *t_right)
{
	t_left->m_aabb = t_right->m_aabb = AABB();

	int refs = get_ref_index(t_spec);
	//separate the bound into 3 parts
	//[left_begin, left_end) - totally left part
	//[left_end, right_begin) - the part to split
	//[right_begin, right_end) - totally right part
	int left_begin = 0, left_end = 0, right_begin = t_spec.m_ref_num, right_end = t_spec.m_ref_num;
	for(int i = left_begin; i < right_begin; ++i)
	{
		//put to left
		if(m_refstack[refs + i].m_aabb.m_max[t_ss.m_dim] <= t_ss.m_pos)
		{
			t_left->m_aabb.Expand(m_refstack[refs + i].m_aabb);
			std::swap(m_refstack[refs + i], m_refstack[refs + (left_end ++)]);
		}
		else if(m_refstack[refs + i].m_aabb.m_min[t_ss.m_dim] >= t_ss.m_pos)
		{
			t_right->m_aabb.Expand(m_refstack[refs + i].m_aabb);
			std::swap(m_refstack[refs + (i--)], m_refstack[refs + (--right_begin)]);
		}
	}

	Reference left_ref, right_ref;

	AABB lub; // Unsplit to left:     new left-hand bounds.
	AABB rub; // Unsplit to right:    new right-hand bounds.
	AABB ldb; // Duplicate:           new left-hand bounds.
	AABB rdb; // Duplicate:           new right-hand bounds.

	while(left_end < right_begin)
	{
		split_reference(m_refstack[refs + left_end], t_ss.m_dim, t_ss.m_pos, &left_ref, &right_ref);

		lub = ldb = t_left->m_aabb;
		rub = rdb = t_right->m_aabb;

		lub.Expand(m_refstack[refs + left_end].m_aabb);
		rub.Expand(m_refstack[refs + left_end].m_aabb);
		ldb.Expand(left_ref.m_aabb);
		rdb.Expand(right_ref.m_aabb);

		float lac = m_bvh->m_config.GetTriangleCost(left_end - left_begin);
		float rac = m_bvh->m_config.GetTriangleCost(right_end - right_begin);
		float lbc = m_bvh->m_config.GetTriangleCost(1 + left_end - left_begin);
		float rbc = m_bvh->m_config.GetTriangleCost(1 + right_end - right_begin);

		float unsplit_left_sah = lub.GetArea() * lbc + t_right->m_aabb.GetArea() * rac;
		float unsplit_right_sah = t_left->m_aabb.GetArea() * lac + rub.GetArea() * rbc;
		float duplicate_sah = ldb.GetArea() * lbc + rdb.GetArea() * rbc;

		if(unsplit_left_sah < unsplit_right_sah && unsplit_left_sah < duplicate_sah) //unsplit left
		{
			t_left->m_aabb = lub;
			left_end ++;
		}
		else if(unsplit_right_sah < duplicate_sah) //unsplit right
		{
			t_right->m_aabb = rub;
			std::swap(m_refstack[refs + left_end], m_refstack[refs + (--right_begin)]);
		}
		else //duplicate
		{
			m_refstack.emplace_back();
			t_left->m_aabb = ldb;
			t_right->m_aabb = rdb;
			m_refstack[refs + (left_end ++)] = left_ref;
			m_refstack[refs + (right_end ++)] = right_ref;
		}
	}

	t_left->m_ref_num = left_end - left_begin;
	t_right->m_ref_num = right_end - right_begin;
}

void SBVHBuilder::perform_object_split(const SBVHBuilder::NodeSpec &t_spec, const SBVHBuilder::ObjectSplit &t_os,
									   SBVHBuilder::NodeSpec *t_left, SBVHBuilder::NodeSpec *t_right)
{
	sort_spec(t_spec, t_os.m_dim);

	t_left->m_ref_num = t_os.m_left_num;
	t_left->m_aabb = t_os.m_left_aabb;

	t_right->m_ref_num = t_spec.m_ref_num - t_os.m_left_num;
	t_right->m_aabb = t_os.m_right_aabb;
}


#endif //YART_SBVHBUILDER_HPP
