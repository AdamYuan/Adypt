//
// Created by adamyuan on 12/7/18.
//

#include "OglScene.hpp"
#include "../BVH/WideBVH.hpp"
#include <stack>
#include <stb_image.h>

int OglScene::load_texture(std::vector<mygl3::Texture2D> *textures,
						   std::map<std::string, int> &name_set,
						   const char *filename)
{
	if(name_set.count(filename)) return name_set[filename];
	textures->emplace_back();
	auto &cur = textures->back();
	cur.Initialize();
	auto loader = mygl3::ImageLoader(filename, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 4);
	if(loader.GetInfo().data == nullptr)
	{
		printf("Unable to load texture %s\n", filename);
		return -1;
	}
	printf("Loaded texture %s\n", filename);
	cur.Load(loader.GetInfo());
	cur.SetWrapFilter(GL_REPEAT);
	cur.SetSizeFilter(GL_LINEAR, GL_LINEAR);
	int idx = (int)textures->size() - 1;
	return name_set[filename] = idx;
}


void OglScene::Initialize(const Scene &scene, const WideBVH &bvh)
{
	m_aabb = scene.GetAABB();
	create_buffers(scene, bvh);
}

void OglScene::init_materials(const Scene &scene, std::vector<GPUMaterial> *materials,
							  std::vector<GLuint64> *texture_handles)
{
	std::map<std::string, int> tex_name_set;
	for (const auto &ml : scene.GetTinyobjMaterials())
	{
		materials->emplace_back();
		GPUMaterial &gml = materials->back();

		if(!ml.diffuse_texname.empty())
			gml.m_dtex = load_texture(&m_textures, tex_name_set, (scene.GetBasePath() + ml.diffuse_texname).c_str());
		else
		{
			gml.m_dtex = -1;
			gml.m_dr = ml.diffuse[0];
			gml.m_dg = ml.diffuse[1];
			gml.m_db = ml.diffuse[2];
		}

		gml.m_er = ml.emission[0];
		gml.m_eg = ml.emission[1];
		gml.m_eb = ml.emission[2];

		gml.m_sr = ml.specular[0];
		gml.m_sg = ml.specular[1];
		gml.m_sb = ml.specular[2];

		gml.m_illum = ml.illum;
		gml.m_shininess = ml.shininess;
		gml.m_dissolve = ml.dissolve;
		gml.m_refraction_index = ml.ior;
	}

	GLuint64 handle;
	for(const auto &t : m_textures)
	{
		handle = glGetTextureHandleARB(t.Get());
		texture_handles->push_back(handle);
		glMakeTextureHandleResidentARB(handle);
	}
}

void OglScene::init_triangles(const Scene &scene, const WideBVH &bvh, std::vector<glm::vec4> *tri_matrices)
{
	tri_matrices->reserve(bvh.GetTriIndices().size() * 3u);
	for(int32_t t : bvh.GetTriIndices())
	{
		const Triangle &tri = scene.GetTriangles()[t];
		const glm::vec3 &v0 = tri.m_positions[0], &v1 = tri.m_positions[1], &v2 = tri.m_positions[2];
		glm::vec4 c0{v0 - v2, 0.0f};
		glm::vec4 c1{v1 - v2, 0.0f};
		glm::vec4 c2{glm::cross(v0 - v2, v1 - v2), 0.0f};
		glm::vec4 c3{v2, 1.0f};
		glm::mat4 mtx {
				c0.x, c1.x, c2.x, c3.x,
				c0.y, c1.y, c2.y, c3.y,
				c0.z, c1.z, c2.z, c3.z,
				c0.w, c1.w, c2.w, c3.w
		};
		mtx = glm::inverse(mtx);
		//generate triangle matrices
		tri_matrices->emplace_back(mtx[2][0], mtx[2][1], mtx[2][2], -mtx[2][3]);
		tri_matrices->emplace_back(mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3]);
		tri_matrices->emplace_back(mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3]);
	}
}

void OglScene::create_buffers(const Scene &scene, const WideBVH &bvh)
{
	std::vector<GPUMaterial> materials;
	std::vector<GLuint64> texture_handles;
	init_materials(scene, &materials, &texture_handles);
	std::vector<glm::vec4> tri_matrices;
	init_triangles(scene, bvh, &tri_matrices);
	m_triangles_ssbo.Initialize();
	m_triangles_ssbo.Storage(scene.GetTriangles().data(), scene.GetTriangles().data() + scene.GetTriangles().size(), 0);
	m_bvh_nodes_ssbo.Initialize();
	m_bvh_nodes_ssbo.Storage(bvh.GetNodes().data(), bvh.GetNodes().data() + bvh.GetNodes().size(), 0);
	m_tri_indices_ssbo.Initialize();
	m_tri_indices_ssbo.Storage(bvh.GetTriIndices().data(), bvh.GetTriIndices().data() + bvh.GetTriIndices().size(), 0);
	m_tri_matrices_ssbo.Initialize();
	m_tri_matrices_ssbo.Storage(tri_matrices.data(), tri_matrices.data() + tri_matrices.size(), 0);
	m_material_ssbo.Initialize();
	m_material_ssbo.Storage(materials.data(), materials.data() + materials.size(), 0);
	m_texture_handle_ubo.Initialize();
	m_texture_handle_ubo.Storage(texture_handles.data(), texture_handles.data() + texture_handles.size(), 0);
}

