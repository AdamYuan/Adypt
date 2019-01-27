#define SHOW_BVH 0
const ivec2 kPixel = ivec2(gl_GlobalInvocationID.xy);

struct Ray { vec4 m_origin, m_dir; };
struct Node //20 * 4 bytes for inner node
{
	vec4 m_head;
	uvec4 m_base_meta, m_lox_loy, m_loz_hix, m_hiy_hiz;
};

struct Triangle //25*4 bytes
{
	float m_p1[3], m_p2[3], m_p3[3];
	float m_n1[3], m_n2[3], m_n3[3];
	float m_tc1[2], m_tc2[2], m_tc3[2];
	int m_material_id;
};
struct Material
{
	int m_dtex; float m_dr, m_dg, m_db;
	int m_etex; float m_er, m_eg, m_eb; 
	int m_stex; float m_sr, m_sg, m_sb;
	int m_illum; 
	float m_shininess;
	float m_dissolve;
	float m_ior;
};
struct Woop { vec4 m0, m1, m2; };
//images
layout(rgba32f, binding = 0) uniform image2D uOutImg;
layout(rgba32f, binding = 1) uniform image2D uPrimaryTmpImg;
layout(rg8, binding = 2) uniform image2D uSobolBiasImg;
#if TEXTURE_COUNT != 0
//bindless textures
layout(binding = 1) uniform uuTextures { sampler2D uTextures[TEXTURE_COUNT]; };
#endif

//buffers
layout(std430, binding = 0) readonly buffer uuBVHNodes { Node uBVHNodes[]; };
layout(std430, binding = 1) readonly buffer uuTriIndices { int uTriIndices[]; };
layout(std430, binding = 2) readonly buffer uuTriMatrices { Woop uTriMatrices[]; };
layout(std430, binding = 3) readonly buffer uuTriangles { Triangle uTriangles[]; };
layout(std430, binding = 4) readonly buffer uuMaterials { Material uMaterials[]; };
layout(std430, binding = 5) buffer uuSobol { vec2 uSobol[]; };

//args
layout(std140, binding = 0) uniform uuArgs
{
	int uIteration; 
	float uPosX, uPosY, uPosZ;
	mat4 uInvProjection;
	mat4 uInvView;
};

uvec2 stack[STACK_SIZE];

void BVHIntersection(in const vec3 origin, vec3 dir, inout int o_hit_tri_idx, inout vec2 o_hit_uv)
{
	const float ooeps = exp2(-64.0f);
	dir.x = abs(dir.x) > ooeps ? dir.x : (dir.x >= 0 ? ooeps : -ooeps);
	dir.y = abs(dir.y) > ooeps ? dir.y : (dir.y >= 0 ? ooeps : -ooeps);
	dir.z = abs(dir.z) > ooeps ? dir.z : (dir.z >= 0 ? ooeps : -ooeps);
	dir = normalize(dir);
	vec3 idir = 1.0f / dir;
	uint octinv = 7u - ((dir.x < 0 ? 1 : 0) | (dir.y < 0 ? 2 : 0) | (dir.z < 0 ? 4 : 0));
	uint octinv4 = octinv * 0x01010101u;

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

#if SHOW_BVH == 1
	int it = 0;
#endif
	while(true)
	{
#if SHOW_BVH == 1
		it ++;
#endif
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

				ctmin = max(max(txmin[0], tymin[0]), max(tzmin[0], RAY_TMIN));
				ctmax = min(min(txmax[0], tymax[0]), min(tzmax[0], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4       ) & 0xffu) << ((bit_index4       ) & 0xffu);

				ctmin = max(max(txmin[1], tymin[1]), max(tzmin[1], RAY_TMIN));
				ctmax = min(min(txmax[1], tymax[1]), min(tzmax[1], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >>  8u) & 0xffu) << ((bit_index4 >>  8u) & 0xffu);

				ctmin = max(max(txmin[2], tymin[2]), max(tzmin[2], RAY_TMIN));
				ctmax = min(min(txmax[2], tymax[2]), min(tzmax[2], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 16u) & 0xffu) << ((bit_index4 >> 16u) & 0xffu);

				ctmin = max(max(txmin[3], tymin[3]), max(tzmin[3], RAY_TMIN));
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

				ctmin = max(max(txmin[0], tymin[0]), max(tzmin[0], RAY_TMIN));
				ctmax = min(min(txmax[0], tymax[0]), min(tzmax[0], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4       ) & 0xffu) << ((bit_index4       ) & 0xffu);

				ctmin = max(max(txmin[1], tymin[1]), max(tzmin[1], RAY_TMIN));
				ctmax = min(min(txmax[1], tymax[1]), min(tzmax[1], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >>  8u) & 0xffu) << ((bit_index4 >>  8u) & 0xffu);

				ctmin = max(max(txmin[2], tymin[2]), max(tzmin[2], RAY_TMIN));
				ctmax = min(min(txmax[2], tymax[2]), min(tzmax[2], hit_t));
				if(ctmin <= ctmax) hitmask |= ((child_bits4 >> 16u) & 0xffu) << ((bit_index4 >> 16u) & 0xffu);

				ctmin = max(max(txmin[3], tymin[3]), max(tzmin[3], RAY_TMIN));
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

			toz = tv00.w - dot(origin, tv00.xyz);
			tidz = 1.0 / dot(dir, tv00.xyz);
			tt = toz * tidz;

			if(tt > RAY_TMIN && tt < hit_t)
			{
				tox = tv11.w + dot(origin, tv11.xyz);
				tdx = dot(dir, tv11.xyz);
				tu = tox + tt*tdx;

				if(tu >= 0.0 && tu <= 1.0)
				{
					tv22 = uTriMatrices[tridx].m2;
					toy = tv22.w + dot(origin, tv22.xyz);
					tdy = dot(dir, tv22.xyz);
					tv = toy + tt*tdy;

					if(tv >= 0.0 && tu + tv <= 1.0)
					{
						hit_t = tt;
						o_hit_uv = vec2(tu, tv);
						o_hit_tri_idx = int(tridx);
					}
				}
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
#if SHOW_BVH == 1
	imageStore(uOutImg, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(it / 100.0f), 1.0f));
#endif
}

//random number generator
vec2 Sobol(in const int i) { return fract(uSobol[i] + imageLoad(uSobolBiasImg, kPixel).xy); }

#define TWO_PI 6.28318530718f
vec3 SampleHemisphere(in const int b, in const float e)
{
	vec2 r = Sobol(b);
	r.x *= TWO_PI;
	float cos_phi = cos(r.x);
	float sin_phi = sin(r.x);
	float cos_theta = pow((1.0f - r.y), 1.0f/(e + 1.0f));
	float sin_theta = sqrt(1.0f - cos_theta*cos_theta);
	float pu = sin_theta * cos_phi;
	float pv = sin_theta * sin_phi;
	float pw = cos_theta;
	return normalize(vec3(pu, pv, pw)); 
}

vec3 AlignDirection(in const vec3 dir, in const vec3 target)
{
	vec3 u = normalize(cross(abs(target.x) > .01 ? vec3(0, 1, 0) : vec3(1, 0, 0), target));
	vec3 v = cross(target, u);
	return dir.x*u + dir.y*v + dir.z*target;
}

void FetchInfo(in const int tri_idx, in const vec2 tri_uv, out vec3 position, out vec3 normal, 
			   out vec3 emissive, out vec3 diffuse, out vec3 specular, inout Material mtl)
{
	Triangle tri = uTriangles[tri_idx];
	mtl = uMaterials[tri.m_material_id];
	normal = normalize(
		vec3(tri.m_n1[0], tri.m_n1[1], tri.m_n1[2])*tri_uv.x +
		vec3(tri.m_n2[0], tri.m_n2[1], tri.m_n2[2])*tri_uv.y + 
		vec3(tri.m_n3[0], tri.m_n3[1], tri.m_n3[2])*(1.0 - tri_uv.x - tri_uv.y));
	position = 
		vec3(tri.m_p1[0], tri.m_p1[1], tri.m_p1[2])*tri_uv.x +
		vec3(tri.m_p2[0], tri.m_p2[1], tri.m_p2[2])*tri_uv.y + 
		vec3(tri.m_p3[0], tri.m_p3[1], tri.m_p3[2])*(1.0 - tri_uv.x - tri_uv.y);
	emissive = vec3(mtl.m_er, mtl.m_eg, mtl.m_eb);
#if TEXTURE_COUNT != 0
	vec2 texcoords = 
		vec2(tri.m_tc1[0], tri.m_tc1[1])*tri_uv.x +
		vec2(tri.m_tc2[0], tri.m_tc2[1])*tri_uv.y + 
		vec2(tri.m_tc3[0], tri.m_tc3[1])*(1.0 - tri_uv.x - tri_uv.y);
	if(mtl.m_dtex != -1)
		diffuse = texture(uTextures[mtl.m_dtex], texcoords).rgb;
	else
#endif
		diffuse = vec3(mtl.m_dr, mtl.m_dg, mtl.m_db);
	specular = vec3(mtl.m_sr, mtl.m_sg, mtl.m_sb);
}
vec3 Render(vec3 origin, vec3 dir)
{
	vec2 tri_uv; int tri_idx;

	vec3 ret = vec3(0), color = vec3(1), normal, emissive, diffuse, specular;
	Material mtl;
	for(int b = 0; b < MAX_BOUNCE; ++b)
	{
#if TMP_LIFETIME > 1
		//use primary ray tmp
		if(b > 0)
			BVHIntersection(origin, dir, tri_idx, tri_uv);
		else
		{
			vec4 tmp;
			if(uIteration % TMP_LIFETIME > 0) //primary ray
			{
				tmp = imageLoad(uPrimaryTmpImg, kPixel);
				tri_idx = floatBitsToInt(tmp.x);
				tri_uv = tmp.zw;
			}
			else
			{
				BVHIntersection(origin, dir, tri_idx, tri_uv);
				tmp.x = intBitsToFloat(tri_idx);
				tmp.zw = tri_uv;
				imageStore(uPrimaryTmpImg, kPixel, tmp);
			}
		}
#else
		BVHIntersection(origin, dir, tri_idx, tri_uv);
#endif

		if(tri_idx == -1)
		{
			ret += color * SUN;
			break;
		}

		FetchInfo(tri_idx, tri_uv, origin, normal, emissive, diffuse, specular, mtl);
		ret += color * emissive;
		switch(mtl.m_illum)
		{
			case 2: //glossy reflection
				float e = mtl.m_shininess*0.01f;
				if(e > MIN_GLOSSY_EXP)
				{
					vec3 r = reflect(dir, normal), s = SampleHemisphere(b, e);
					dir = AlignDirection(s, r);
					if(dot(dir, normal) < 0.0f) return ret;
					color *= diffuse + specular*pow(dot(dir, r), e);
					origin += r*RAY_TMIN;
					break;
				} //else do diffuse 
			case 1: //diffuse only
				dir = AlignDirection(SampleHemisphere(b, 0), normal);
				color *= diffuse;
				break;
			case 3: case 4: case 5: //mirror reflection
				color *= specular;
				dir = reflect(dir, normal);
				break;
			case 6: case 7: //refraction, fresnel
				float eta = mtl.m_ior;

				float cosi = dot(dir, normal); 
				float fresnel;
				{
					float etai, etat;
					if(cosi > 0)
						etai = eta, etat = 1.0;
					else
					{
						etai = 1.0, etat = eta;
						normal = -normal;
						cosi = -cosi;
					}
					eta = etai / etat;
					// Compute sini using Snell's law
					float sint = etai / etat * sqrt(max(0.f, 1.0f - cosi*cosi));
					// Total internal reflection
					if (sint >= 1.0)
						fresnel = 1.0f;
					else {
						float cost = sqrt(max(0.f, 1 - sint * sint));
						float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
						float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
						fresnel = (Rs*Rs + Rp*Rp)*0.5f;
					}
				}
				float cos2 = 1.0 - eta*eta * (1.0 - cosi*cosi);
				if(cos2 > 0 && Sobol(b).x >= fresnel)
				{
					dir = normalize(dir*eta + normal * (eta*cosi + sqrt(cos2)));
					normal = -normal;
				}
				else
					dir = reflect(dir, normal);
				break;
		}
		origin += normal*RAY_TMIN;
	}
	return ret;
}

vec2 SubPixel()
{
	int subpixel_idx = (uIteration / TMP_LIFETIME) % (SUB_PIXEL * SUB_PIXEL);
	const float unit = 1.0f / float(SUB_PIXEL);
	return vec2((subpixel_idx / SUB_PIXEL)*unit, (subpixel_idx % SUB_PIXEL)*unit);
}

vec3 Camera(in const vec2 bias)
{
	vec2 scrpos = 2.0f*(vec2(kPixel) + bias)/vec2(IMG_SIZE) - 1.0f;
	scrpos.y = -scrpos.y;
	return normalize(mat3(uInvView) * (uInvProjection * vec4(scrpos, 1.0f, 1.0f)).xyz);
}

void main()
{
	if(kPixel.x >= IMG_SIZE.x || kPixel.y >= IMG_SIZE.y) return;

	vec3 origin = vec3(uPosX, uPosY, uPosZ);
	if(uIteration == -1) //only do primary rays
	{
		vec2 tri_uv; int tri_idx;
		BVHIntersection(origin, Camera( vec2(0.5f) ), tri_idx, tri_uv);
#if SHOW_BVH == 0
		if(tri_idx != -1)
		{
			vec3 position, normal, emissive, diffuse, specular;
			Material mtl;
			FetchInfo(tri_idx, tri_uv, position, normal, emissive, diffuse, specular, mtl);
			imageStore(uOutImg, kPixel, vec4(diffuse, 1.0f));
		}
		else
			imageStore(uOutImg, kPixel, vec4(0, 0, 0, 1.0f));
#endif
	}
	else //progress rendering
	{
		vec3 color = clamp(Render(origin, Camera( SubPixel() )), vec3(0.0), vec3(CLAMP));
		color = (imageLoad(uOutImg, kPixel).xyz*uIteration + color) / float(uIteration + 1);
		imageStore(uOutImg, kPixel, vec4(color, 1.0f));
	}
}
