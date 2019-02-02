const ivec2 kPixel = ivec2(gl_GlobalInvocationID.xy);
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
//images
layout(rgba32f, binding = 0) uniform image2D uOutImg;
layout(rgba32f, binding = 1) uniform image2D uPrimaryTmpImg;
layout(rg8, binding = 2) uniform image2D uSobolBiasImg;

//bindless textures
#if TEXTURE_COUNT != 0
layout(binding = 3) uniform uuTextures { sampler2D uTextures[TEXTURE_COUNT]; };
#endif

//args
layout(std140, binding = 0) uniform uuCamera
{
	vec4 uOrigin_TMin;
	mat4 uInvProjection;
	mat4 uInvView;
};
//args
layout(std140, binding = 1) uniform uuPT
{
	int uSpp, uSubpixel, uTmpLife, uMaxBounce;
	float uClamp, uSunR, uSunG, uSunB;
};


layout(std430, binding = 3) readonly buffer uuTriangles { Triangle uTriangles[]; };
layout(std430, binding = 4) readonly buffer uuMaterials { Material uMaterials[]; };
layout(std430, binding = 5) buffer uuSobol { vec2 uSobol[]; };

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
vec3 Render(vec4 origin, vec3 dir)
{
	vec2 tri_uv; int tri_idx;

	vec3 ret = vec3(0), color = vec3(1), position, normal, emissive, diffuse, specular;
	Material mtl;
	for(int b = 0; b < uMaxBounce; ++b)
	{
		//use primary ray tmp
		if(b > 0)
			BVHIntersection(origin, dir, tri_idx, tri_uv);
		else
		{
			vec4 tmp;
			if(uSpp % uTmpLife > 0) //primary ray
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

		if(tri_idx == -1)
		{
			ret += color * vec3(uSunR, uSunG, uSunB);
			break;
		}

		FetchInfo(tri_idx, tri_uv, position, normal, emissive, diffuse, specular, mtl);
		origin.xyz = position;
		ret += color * emissive;

		switch(mtl.m_illum)
		{
			case 2: //glossy reflection
				float e = mtl.m_shininess*0.01f;
				if(e > 0.3f)
				{
					vec3 r = reflect(dir, normal), s = SampleHemisphere(b, e);
					dir = AlignDirection(s, r);
					if(dot(dir, normal) < 0.0f) return ret;
					color *= diffuse + specular*pow(dot(dir, r), e);
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
	}
	return ret;
}

vec2 SubPixel()
{
	int subpixel_idx = (uSpp / uTmpLife) % (uSubpixel * uSubpixel);
	const float unit = 1.0f / float(uSubpixel);
	return vec2((subpixel_idx / uSubpixel)*unit, (subpixel_idx % uSubpixel)*unit);
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

	vec3 radiance = min(Render(uOrigin_TMin, Camera( SubPixel() )), vec3(uClamp));
	radiance = (imageLoad(uOutImg, kPixel).xyz*uSpp + radiance) / float(uSpp + 1);
	imageStore(uOutImg, kPixel, vec4(radiance, 1));
}
