struct Node //20 * 4 bytes for inner node
{
	vec4 m_head;
	uvec4 m_base_meta, m_lox_loy, m_loz_hix, m_hiy_hiz;
};
struct Woop { vec4 m0, m1, m2; };
//bvh buffers
layout(std430, binding = 0) readonly buffer uuBVHNodes { Node uBVHNodes[]; };
layout(std430, binding = 1) readonly buffer uuTriIndices { int uTriIndices[]; };
layout(std430, binding = 2) readonly buffer uuTriMatrices { Woop uTriMatrices[]; };

void BVHIntersection(in const vec4 origin_tmin, vec3 dir, inout int o_hit_tri_idx, inout vec2 o_hit_uv)
{
	uvec2 stack[TRAVERSAL_STACK_SIZE];

	const float ooeps = exp2(-64.0f);
	dir.x = abs(dir.x) > ooeps ? dir.x : (dir.x >= 0 ? ooeps : -ooeps);
	dir.y = abs(dir.y) > ooeps ? dir.y : (dir.y >= 0 ? ooeps : -ooeps);
	dir.z = abs(dir.z) > ooeps ? dir.z : (dir.z >= 0 ? ooeps : -ooeps);
	dir = normalize(dir);
	vec3 idir = 1.0f / dir;
	uint octinv = 7u - ((dir.x < 0 ? 1 : 0) | (dir.y < 0 ? 2 : 0) | (dir.z < 0 ? 4 : 0));
	uint octinv4 = octinv * 0x01010101u;
	
	vec3 origin = origin_tmin.xyz;

	float hit_tmin = origin_tmin.w;
	float hit_t = 1e9f;
	o_hit_tri_idx = -1;

	//stack
	int stack_ptr = 0;

	//traversal states
	uvec2 tri_group, node_group = uvec2(0u, 0x80000000u);

	//aabb intersection variables
	float txmin[4], tymin[4], tzmin[4], txmax[4], tymax[4], tzmax[4], ctmin, ctmax;

	//triangle intersection variables
	vec4 tv00, tv11, tv22;
	float tdx, tdy, tidz, tox, toy, toz;
	float tt, tu, tv;

	while(true)
	{
		if(node_group.y > 0x00ffffffu)
		{
			// G represents a node group
			// n <- GetClosestNode(G, r)
			uint imask = node_group.y;
			uint child_bit_index = findMSB(node_group.y);
			uint child_node_base_index = node_group.x;

			// Clear corresponding bit in hits field
			// G <- G / n
			node_group.y &= ~(1u << child_bit_index);

			if(node_group.y > 0x00ffffffu)
				stack[stack_ptr ++] = node_group;

			// Intersect with n
			// G, Gt <- IntersectChildren(n, r)
			uint slot_index = (child_bit_index - 24) ^ octinv;
			uint relative_index = bitCount(imask & ~(0xffffffffu << slot_index));
			uint child_node_index = child_node_base_index + relative_index;

			//read node data
			vec4 head       = uBVHNodes[child_node_index].m_head;
			uint head_w     = floatBitsToUint(head.w);
			uvec4 base_meta = uBVHNodes[child_node_index].m_base_meta;
			uvec4 lox_loy   = uBVHNodes[child_node_index].m_lox_loy;
			uvec4 loz_hix   = uBVHNodes[child_node_index].m_loz_hix;
			uvec4 hiy_hiz   = uBVHNodes[child_node_index].m_hiy_hiz;

			float adjusted_idir_x = uintBitsToFloat(((head_w       ) & 0xffu) << 23u) * idir.x;
			float adjusted_idir_y = uintBitsToFloat(((head_w >>  8u) & 0xffu) << 23u) * idir.y;
			float adjusted_idir_z = uintBitsToFloat(((head_w >> 16u) & 0xffu) << 23u) * idir.z;
			vec3 adjusted_origin = (head.xyz - origin) * idir;

			node_group.x = base_meta.x;
			tri_group.x = base_meta.y;
			tri_group.y = 0u;

			uint hitmask = 0u;
			{
				uint meta4 = base_meta.z;
				uint is_inner4 = (meta4 & (meta4 << 1u)) & 0x10101010u;
				uint bit_index4 = (meta4 ^ (octinv4 & (is_inner4>>4u)*0xffu)) & 0x1f1f1f1fu;
				uint child_bits4 = (meta4 >> 5u) & 0x07070707u;

				uint swizzled_lox = (idir.x < 0) ? loz_hix.z : lox_loy.x;
				uint swizzled_hix = (idir.x < 0) ? lox_loy.x : loz_hix.z;

				uint swizzled_loy = (idir.y < 0) ? hiy_hiz.x : lox_loy.z;
				uint swizzled_hiy = (idir.y < 0) ? lox_loy.z : hiy_hiz.x;

				uint swizzled_loz = (idir.z < 0) ? hiy_hiz.z : loz_hix.x;
				uint swizzled_hiz = (idir.z < 0) ? loz_hix.x : hiy_hiz.z;

				txmin[0] = float((swizzled_lox      ) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[1] = float((swizzled_lox >>  8) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[2] = float((swizzled_lox >> 16) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[3] = float((swizzled_lox >> 24) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				tymin[0] = float((swizzled_loy      ) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[1] = float((swizzled_loy >>  8) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[2] = float((swizzled_loy >> 16) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[3] = float((swizzled_loy >> 24) & 0xffu) * adjusted_idir_y + adjusted_origin.y;

				tzmin[0] = float((swizzled_loz      ) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[1] = float((swizzled_loz >>  8) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[2] = float((swizzled_loz >> 16) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[3] = float((swizzled_loz >> 24) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				txmax[0] = float((swizzled_hix      ) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[1] = float((swizzled_hix >>  8) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[2] = float((swizzled_hix >> 16) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[3] = float((swizzled_hix >> 24) & 0xffu) * adjusted_idir_x + adjusted_origin.x;

				tymax[0] = float((swizzled_hiy      ) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[1] = float((swizzled_hiy >>  8) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[2] = float((swizzled_hiy >> 16) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[3] = float((swizzled_hiy >> 24) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tzmax[0] = float((swizzled_hiz      ) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[1] = float((swizzled_hiz >>  8) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[2] = float((swizzled_hiz >> 16) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[3] = float((swizzled_hiz >> 24) & 0xffu) * adjusted_idir_z + adjusted_origin.z;

				ctmin = max(max(txmin[0], tymin[0]), max(tzmin[0], hit_tmin));
				ctmax = min(min(txmax[0], tymax[0]), min(tzmax[0], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4       ) & 0xffu) << ((bit_index4       ) & 0xffu);

				ctmin = max(max(txmin[1], tymin[1]), max(tzmin[1], hit_tmin));
				ctmax = min(min(txmax[1], tymax[1]), min(tzmax[1], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >>  8u) & 0xffu) << ((bit_index4 >>  8u) & 0xffu);

				ctmin = max(max(txmin[2], tymin[2]), max(tzmin[2], hit_tmin));
				ctmax = min(min(txmax[2], tymax[2]), min(tzmax[2], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 16u) & 0xffu) << ((bit_index4 >> 16u) & 0xffu);

				ctmin = max(max(txmin[3], tymin[3]), max(tzmin[3], hit_tmin));
				ctmax = min(min(txmax[3], tymax[3]), min(tzmax[3], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 24u) & 0xffu) << ((bit_index4 >> 24u) & 0xffu);
			}

			{
				uint meta4 = base_meta.w;
				uint is_inner4 = (meta4 & (meta4 << 1u)) & 0x10101010u;
				uint bit_index4 = (meta4 ^ (octinv4 & (is_inner4>>4u)*0xffu)) & 0x1f1f1f1fu;
				uint child_bits4 = (meta4 >> 5u) & 0x07070707u;

				uint swizzled_lox = (idir.x < 0) ? loz_hix.w : lox_loy.y;
				uint swizzled_hix = (idir.x < 0) ? lox_loy.y : loz_hix.w;

				uint swizzled_loy = (idir.y < 0) ? hiy_hiz.y : lox_loy.w;
				uint swizzled_hiy = (idir.y < 0) ? lox_loy.w : hiy_hiz.y;

				uint swizzled_loz = (idir.z < 0) ? hiy_hiz.w : loz_hix.y;
				uint swizzled_hiz = (idir.z < 0) ? loz_hix.y : hiy_hiz.w;

				txmin[0] = float((swizzled_lox      ) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[1] = float((swizzled_lox >>  8) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[2] = float((swizzled_lox >> 16) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmin[3] = float((swizzled_lox >> 24) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				tymin[0] = float((swizzled_loy      ) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[1] = float((swizzled_loy >>  8) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[2] = float((swizzled_loy >> 16) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymin[3] = float((swizzled_loy >> 24) & 0xffu) * adjusted_idir_y + adjusted_origin.y;

				tzmin[0] = float((swizzled_loz      ) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[1] = float((swizzled_loz >>  8) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[2] = float((swizzled_loz >> 16) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmin[3] = float((swizzled_loz >> 24) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				txmax[0] = float((swizzled_hix      ) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[1] = float((swizzled_hix >>  8) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[2] = float((swizzled_hix >> 16) & 0xffu) * adjusted_idir_x + adjusted_origin.x;
				txmax[3] = float((swizzled_hix >> 24) & 0xffu) * adjusted_idir_x + adjusted_origin.x;

				tymax[0] = float((swizzled_hiy      ) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[1] = float((swizzled_hiy >>  8) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[2] = float((swizzled_hiy >> 16) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tymax[3] = float((swizzled_hiy >> 24) & 0xffu) * adjusted_idir_y + adjusted_origin.y;
				tzmax[0] = float((swizzled_hiz      ) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[1] = float((swizzled_hiz >>  8) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[2] = float((swizzled_hiz >> 16) & 0xffu) * adjusted_idir_z + adjusted_origin.z;
				tzmax[3] = float((swizzled_hiz >> 24) & 0xffu) * adjusted_idir_z + adjusted_origin.z;

				ctmin = max(max(txmin[0], tymin[0]), max(tzmin[0], hit_tmin));
				ctmax = min(min(txmax[0], tymax[0]), min(tzmax[0], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4       ) & 0xffu) << ((bit_index4       ) & 0xffu);

				ctmin = max(max(txmin[1], tymin[1]), max(tzmin[1], hit_tmin));
				ctmax = min(min(txmax[1], tymax[1]), min(tzmax[1], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >>  8u) & 0xffu) << ((bit_index4 >>  8u) & 0xffu);

				ctmin = max(max(txmin[2], tymin[2]), max(tzmin[2], hit_tmin));
				ctmax = min(min(txmax[2], tymax[2]), min(tzmax[2], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 16u) & 0xffu) << ((bit_index4 >> 16u) & 0xffu);

				ctmin = max(max(txmin[3], tymin[3]), max(tzmin[3], hit_tmin));
				ctmax = min(min(txmax[3], tymax[3]), min(tzmax[3], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 24u) & 0xffu) << ((bit_index4 >> 24u) & 0xffu);
			}

			node_group.y = (hitmask & 0xff000000u) | ((head_w >> 24u) & 0xffu);
			tri_group.y = hitmask & 0x00ffffffu;
		}
		else
		{
			tri_group = node_group;
			node_group = uvec2(0);
		}

		while(tri_group.y != 0)
		{
			uint tridx = findLSB(tri_group.y);
			tri_group.y &= ~(1u << tridx);
			tridx += tri_group.x;

			tv00 = uTriMatrices[tridx].m0;
			tv11 = uTriMatrices[tridx].m1;
			tv22 = uTriMatrices[tridx].m2;

			toz = tv00.w - dot(origin, tv00.xyz);
			tidz = 1.0 / dot(dir, tv00.xyz);
			tt = toz * tidz;

			tox = tv11.w + dot(origin, tv11.xyz);
			tdx = dot(dir, tv11.xyz);
			tu = tox + tt*tdx;

			toy = tv22.w + dot(origin, tv22.xyz);
			tdy = dot(dir, tv22.xyz);
			tv = toy + tt*tdy;

			if(tt > hit_tmin && tt < hit_t)
				if(tu >= 0.0 && tu <= 1.0)
					if(tv >= 0.0 && tu + tv <= 1.0)
					{
						hit_t = tt;
						o_hit_uv = vec2(tu, tv);
						o_hit_tri_idx = int(tridx);
					}
		}

		if(node_group.y <= 0x00ffffffu)
		{
			if(stack_ptr == 0)
				break;
			node_group = stack[--stack_ptr];
		}
	}

	if(o_hit_tri_idx != -1) 
		o_hit_tri_idx = uTriIndices[o_hit_tri_idx];
}
