//
// Created by adamyuan on 12/7/18.
//

#ifndef ADYPT_OGLSCENE_HPP
#define ADYPT_OGLSCENE_HPP

#include <mygl3/buffer.hpp>
#include "../Util/Scene.hpp"
#include "../BVH/WideBVH.hpp"
#include <GL/gl3w.h>
#include <mygl3/vertexarray.hpp>
#include <mygl3/texture.hpp>
#include <map>

struct OglScene
{
private:
	struct GPUMaterial
	{
		GLint m_dtex; GLfloat m_dr, m_dg, m_db;
		GLint m_etex; GLfloat m_er, m_eg, m_eb;
		GLint m_stex; GLfloat m_sr, m_sg, m_sb;
		GLint m_illum = 1;
		GLfloat m_shininess;
		GLfloat m_dissolve;
		GLfloat m_refraction_index;
	};

	//for gpu ray tracing
	AABB m_aabb;
	mygl3::Buffer m_triangles_ssbo, m_bvh_nodes_ssbo, m_tri_indices_ssbo,
			m_tri_matrices_ssbo, m_material_ssbo, m_texture_handle_ubo;
	std::vector<mygl3::Texture2D> m_textures;

	int load_texture(std::vector<mygl3::Texture2D> *textures, std::map<std::string, int> &name_set,
					 const char *filename);
	void init_materials(const Scene &scene, std::vector<GPUMaterial> *materials,
						std::vector<GLuint64> *texture_handles);
	void init_triangles(const Scene &scene, const WideBVH &bvh, std::vector<glm::vec4> *tri_matrices);
	void create_buffers(const Scene &scene, const WideBVH &bvh);

public:
	const AABB &GetAABB() const { return m_aabb; }
	void Initialize(const Scene &scene, const WideBVH &bvh);
	const mygl3::Buffer &GetTriangleBuffer() const { return m_triangles_ssbo; }
	const mygl3::Buffer &GetTriMatrixBuffer() const { return m_tri_matrices_ssbo; }
	const mygl3::Buffer &GetTriIndexBuffer() const { return m_tri_indices_ssbo; }
	const mygl3::Buffer &GetBVHNodeBuffer() const { return m_bvh_nodes_ssbo; }
	const mygl3::Buffer &GetMaterialBuffer() const { return m_material_ssbo; }
	const mygl3::Buffer &GetTextureHandleBuffer() const { return m_texture_handle_ubo; }
	const std::vector<mygl3::Texture2D> &GetTextures() const { return m_textures; }
};


#endif //ADYPT_OGLSCENE_HPP
