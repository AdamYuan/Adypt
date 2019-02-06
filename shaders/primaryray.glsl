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
layout(std140, binding = 2) uniform uuViewer { int uType; };

layout(std430, binding = 3) readonly buffer uuTriangles { Triangle uTriangles[]; };
layout(std430, binding = 4) readonly buffer uuMaterials { Material uMaterials[]; };

vec3 Camera()
{
	vec2 scrpos = 2.0f*vec2(kPixel)/vec2(IMG_SIZE) - 1.0f;
	scrpos.y = -scrpos.y;
	return normalize(mat3(uInvView) * (uInvProjection * vec4(scrpos, 1.0f, 1.0f)).xyz);
}

void main()
{
	if(kPixel.x >= IMG_SIZE.x || kPixel.y >= IMG_SIZE.y) return;

	vec2 tri_uv; int tri_idx;
	BVHIntersection(uOrigin_TMin, Camera(), tri_idx, tri_uv);
	if(tri_idx != -1)
	{
		Triangle tri = uTriangles[tri_idx];
		Material mtl = uMaterials[tri.m_material_id];
		vec3 color;
		if(uType == 0) //diffuse
		{
#if TEXTURE_COUNT != 0
			if(mtl.m_dtex != -1)
			{
				vec2 texcoords = 
					vec2(tri.m_tc1[0], tri.m_tc1[1])*tri_uv.x +
					vec2(tri.m_tc2[0], tri.m_tc2[1])*tri_uv.y + 
					vec2(tri.m_tc3[0], tri.m_tc3[1])*(1.0 - tri_uv.x - tri_uv.y);
				color = texture(uTextures[mtl.m_dtex], texcoords).rgb;
			}
			else
#endif
				color = vec3(mtl.m_dr, mtl.m_dg, mtl.m_db);
		}
		else if(uType == 1) //specular
			color = vec3(mtl.m_sr, mtl.m_sg, mtl.m_sb);
		else if(uType == 2) //emissive
			color = vec3(mtl.m_er, mtl.m_eg, mtl.m_eb);
		else if(uType == 4) //normal
		{
			color = normalize(
				vec3(tri.m_n1[0], tri.m_n1[1], tri.m_n1[2])*tri_uv.x +
				vec3(tri.m_n2[0], tri.m_n2[1], tri.m_n2[2])*tri_uv.y + 
				vec3(tri.m_n3[0], tri.m_n3[1], tri.m_n3[2])*(1.0 - tri_uv.x - tri_uv.y));
		}
		else if(uType == 5) //position
		{
			color = 
				vec3(tri.m_p1[0], tri.m_p1[1], tri.m_p1[2])*tri_uv.x +
				vec3(tri.m_p2[0], tri.m_p2[1], tri.m_p2[2])*tri_uv.y + 
				vec3(tri.m_p3[0], tri.m_p3[1], tri.m_p3[2])*(1.0 - tri_uv.x - tri_uv.y);
		}
		imageStore(uOutImg, kPixel, vec4(color, 1.0f));
	}
	else
		imageStore(uOutImg, kPixel, vec4(0, 0, 0, 1.0f));
}
