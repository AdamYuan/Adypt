//
// Created by adamyuan on 1/17/19.
//

#include <algorithm>
#include "WideBVHBuilder.hpp"

void WideBVHBuilder::Run()
{
	m_costs.resize(m_sbvh.GetNodes().size());
	calculate_cost(0);
	printf("[WideBVH]cost analyzed\n");

	m_wbvh->m_nodes.emplace_back();
	m_wbvh->m_tri_indices.reserve((size_t)m_sbvh.GetLeafCount());
	create_nodes(0, 0);
	printf("[WideBVH]built with %ld nodes\n", m_wbvh->m_nodes.size());

	m_wbvh->m_nodes.shrink_to_fit();
	m_costs.clear();
	m_costs.shrink_to_fit();
}

int WideBVHBuilder::calculate_cost(int node_idx)
{
	float area = m_sbvh.GetBox(node_idx).GetArea();
	//is leaf, then initialize
	if(m_sbvh.IsLeaf(node_idx))
	{
		for (int i = 1; i <= 7; ++i)
		{
			m_costs[node_idx][i].m_sah = m_wbvh->m_config.GetTriangleCost(1) * area;
			m_costs[node_idx][i].m_type = NodeCost::kLeaf;
		}
		return 1;
	}

	int lidx = m_sbvh.GetLeft(node_idx), ridx = m_sbvh.GetRight(node_idx);
	int tri_count = calculate_cost(ridx) + calculate_cost(lidx);

	auto &dp = m_costs[node_idx];

	{ //for i = 1
		float c_leaf = tri_count <= 3 ? area * m_wbvh->m_config.GetTriangleCost(tri_count) : FLT_MAX;
		float c_internal = FLT_MAX;
		float node_sah = area * m_wbvh->m_config.GetNodeCost(8);
		for(int k = 1; k < 8; ++k)
		{
			float r = node_sah + m_costs[lidx][k].m_sah + m_costs[ridx][8 - k].m_sah;
			if(r < c_internal)
			{
				c_internal = r;
				dp[1].m_distribute[0] = k;
				dp[1].m_distribute[1] = 8 - k;
			}
		}
		if(c_leaf < c_internal)
		{
			dp[1].m_sah = c_leaf;
			dp[1].m_type = NodeCost::kLeaf;
		}
		else
		{
			dp[1].m_sah = c_internal;
			dp[1].m_type = NodeCost::kInternal;
		}
	}

	for(int i = 2; i <= 7; ++i)
	{
		float c_distribute = FLT_MAX;
		for(int k = 1; k < i; ++k)
		{
			float r = m_costs[lidx][k].m_sah + m_costs[ridx][i - k].m_sah;
			if(r < c_distribute)
			{
				c_distribute = r;
				dp[i].m_distribute[0] = k;
				dp[i].m_distribute[1] = i - k;
			}
		}
		if(c_distribute < dp[i - 1].m_sah)
		{
			dp[i].m_sah = c_distribute;
			dp[i].m_type = NodeCost::kDistribute;
		}
		else
			dp[i] = dp[i - 1];
	}
	return tri_count;
}

void WideBVHBuilder::fetch_children(int node_idx, int i, int *out_size, int out_node_idx[8])
{
	int cidx[2] = {m_sbvh.GetLeft(node_idx), m_sbvh.GetRight(node_idx)};
	int cdis[2] = {
			m_costs[node_idx][i].m_distribute[0],
			m_costs[node_idx][i].m_distribute[1]
	};
	for(int c = 0; c < 2; ++c)
	{
		const auto &info = m_costs[cidx[c]][cdis[c]];
		if(info.m_type == NodeCost::kDistribute)
			fetch_children(cidx[c], cdis[c], out_size, out_node_idx);
		else
			out_node_idx[(*out_size) ++] = cidx[c];
	}
}

int WideBVHBuilder::fetch_leaves(int node_idx)
{
	if(m_sbvh.IsLeaf(node_idx))
	{
		m_wbvh->m_tri_indices.push_back(m_sbvh.GetTriIdx(node_idx));
		return 1;
	}
	return fetch_leaves(m_sbvh.GetRight(node_idx)) + fetch_leaves(m_sbvh.GetLeft(node_idx));
}

void WideBVHBuilder::hungarian(const float mat[8][8], int n, int order[8])
{
#define INF 1e12f
	static int p[9], way[9];
	static float u[9], v[9], minv[9];
	static bool used[9];

	std::fill(u, u + n + 1, 0.0f);
	std::fill(v, v + 9, 0.0f);
	std::fill(way, way + 9, 0);
	std::fill(p, p + 9, 0);

	for (int i = 1; i <= n; ++i) {
		p[0] = i;
		int j0 = 0;
		std::fill(minv, minv + 9, INF);
		std::fill(used, used + 9, false);
		do {
			used[j0] = true;
			int i0 = p[j0], j1{};
			float delta = INF;
			for (int j = 1; j <= 8; ++j)
				if (!used[j]) {
					float cur = mat[i0 - 1][j - 1] - u[i0] - v[j];
					if (cur < minv[j])
						minv[j] = cur, way[j] = j0;
					if (minv[j] < delta)
						delta = minv[j], j1 = j;
				}
			for (int j = 0; j <= 8; ++j)
				if (used[j]) u[p[j]] += delta, v[j] -= delta;
				else minv[j] -= delta;
			j0 = j1;
		} while (p[j0] != 0);
		do {
			int j1 = way[j0];
			p[j0] = p[j1];
			j0 = j1;
		} while (j0);
	}
	for(int i = 1; i <= 8; ++i)
		order[p[i] - 1] = i - 1;
#undef INF
}

void WideBVHBuilder::create_nodes(int wbvh_node_idx, int sbvh_node_idx)
{
#define CUR (m_wbvh->m_nodes[wbvh_node_idx])

	int ch_idx_arr[8], ch_cnt = 0;
	fetch_children(sbvh_node_idx, 1, &ch_cnt, ch_idx_arr);

	const AABB &cur_box = m_sbvh.GetBox(sbvh_node_idx);
	glm::vec3 cell; //cell size
	{
		//fetch lo position
		CUR.m_px = cur_box.m_min.x;
		CUR.m_py = cur_box.m_min.y;
		CUR.m_pz = cur_box.m_min.z;

		constexpr auto kBase = float(1.0 / double((1 << 8) - 1));
		cell = (cur_box.m_max - cur_box.m_min)*kBase;
		int ex = cell.x == 0 ? -128 : (int)std::ceil(std::log2(cell.x));
		int ey = cell.y == 0 ? -128 : (int)std::ceil(std::log2(cell.y));
		int ez = cell.z == 0 ? -128 : (int)std::ceil(std::log2(cell.z));
		cell.x = exp2f(ex);
		cell.y = exp2f(ey);
		cell.z = exp2f(ez);

		//store the 8 exponent bits
		CUR.m_ex = (uint8_t)(*(uint32_t*)(&cell.x) >> 23);
		CUR.m_ey = (uint8_t)(*(uint32_t*)(&cell.y) >> 23);
		CUR.m_ez = (uint8_t)(*(uint32_t*)(&cell.z) >> 23);
	}

	//ordering the children with hungarian assignment algorithm
	int ch_slot_arr[8];

	{
		static float ch_cost_mat[8][8];
		glm::vec3 dist;
		for(int i = 0; i < ch_cnt; ++i)
			for (int j = 0; j < 8; ++j)
			{
				dist = m_sbvh.GetBox(ch_idx_arr[i]).GetCenter() - m_sbvh.GetBox(sbvh_node_idx).GetCenter();
				ch_cost_mat[i][j] = ((j & 1) ? -dist.x : dist.x) + ((j & 2) ? -dist.y : dist.y)
								+ ((j & 4) ? -dist.z : dist.z); //project to diagonal ray
			}
		hungarian(ch_cost_mat, ch_cnt, ch_slot_arr);
	}

	int ch_ranked_idx_arr[8];
	std::fill(ch_ranked_idx_arr, ch_ranked_idx_arr + 8, -1);
	for(int i = 0; i < ch_cnt; ++i)
		ch_ranked_idx_arr[ch_slot_arr[i]] = ch_idx_arr[i];

	//set values
	CUR.m_imask = 0;
	CUR.m_child_idx_base = (uint32_t)m_wbvh->m_nodes.size();
	CUR.m_tri_idx_base = (uint32_t)m_wbvh->m_tri_indices.size();

	for(int i = 0; i < 8; ++i)
	{
		int sidx = ch_ranked_idx_arr[i];
		if(~sidx)
		{
			glm::uvec3 qlow = glm::floor((m_sbvh.GetBox(sidx).m_min - cur_box.m_min) / cell);
			glm::uvec3 qhigh = glm::ceil((m_sbvh.GetBox(sidx).m_max - cur_box.m_min) / cell);
			qlow = glm::min(qlow, glm::uvec3(UINT8_MAX));
			qhigh = glm::min(qhigh, glm::uvec3(UINT8_MAX));
			qlow = glm::max(qlow, glm::uvec3(0));
			qhigh = glm::max(qhigh, glm::uvec3(0));

			CUR.m_qlox[i] = (uint8_t)qlow.x;
			CUR.m_qloy[i] = (uint8_t)qlow.y;
			CUR.m_qloz[i] = (uint8_t)qlow.z;
			CUR.m_qhix[i] = (uint8_t)qhigh.x;
			CUR.m_qhiy[i] = (uint8_t)qhigh.y;
			CUR.m_qhiz[i] = (uint8_t)qhigh.z;

			if(m_costs[sidx][1].m_type == NodeCost::kLeaf)
			{
				auto tidx = m_wbvh->m_tri_indices.size() - CUR.m_tri_idx_base;
				int tri_cnt = fetch_leaves(sidx);

				//bbb...... head
				if(tri_cnt == 1) CUR.m_meta[i] = 0b00100000;
				if(tri_cnt == 2) CUR.m_meta[i] = 0b01100000;
				if(tri_cnt == 3) CUR.m_meta[i] = 0b11100000;
				//...index
				CUR.m_meta[i] |= unsigned(tidx);
			}
			else if (m_costs[sidx][1].m_type == NodeCost::kInternal)
			{
				auto widx = m_wbvh->m_nodes.size() - CUR.m_child_idx_base;
				m_wbvh->m_nodes.emplace_back();
				//001..... head
				CUR.m_meta[i] |= (1 << 5);
				//...index
				CUR.m_meta[i] |= unsigned(widx) + 24u;

				//mark as internal
				CUR.m_imask |= (1u << widx);
			}
		}
		else
			CUR.m_meta[i] = 0;
	}

	for(int i = 0; i < ch_cnt; ++i)
		if (m_costs[ch_idx_arr[i]][1].m_type == NodeCost::kInternal)
			create_nodes(CUR.m_child_idx_base + (CUR.m_meta[ch_slot_arr[i]] & 0x1fu) - 24u, ch_idx_arr[i]);
#undef CUR
}
