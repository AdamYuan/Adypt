//
// Created by adamyuan on 4/18/18.
//

#include "Scene.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void Scene::Load(const char *filename)
{
	//get base dir
	{
		const char *s = filename + strlen(filename);
		while(*(s - 1) != '/' && *(s - 1) != '\\' && s > filename) s --;
		m_base_dir = {filename, s};
	}
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;

	std::string err;
	if(!tinyobj::LoadObj(&attrib, &shapes, &m_materials, &err, filename, m_base_dir.c_str()))
	{
		printf("[SCENE]Failed to load %s\n", filename);
		return;
	}

	if (!err.empty())
		printf("%s\n", err.c_str());

	// Loop over shapes
	for(const auto &shape : shapes)
	{
		size_t index_offset = 0, face = 0;

		// Loop over faces(polygon)
		for(const auto &num_face_vertex : shape.mesh.num_face_vertices)
		{
			// Loop over triangles in the face.
			for (size_t v = 0; v < num_face_vertex; v += 3)
			{
				m_triangles.emplace_back(); Triangle &tri = m_triangles.back();

				{
					tri.m_matid = shape.mesh.material_ids[face];

					tinyobj::index_t index = shape.mesh.indices[index_offset + v];
					tri.m_positions[0] = {
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
					};
					if(~index.normal_index)
						tri.m_normals[0] = {
								attrib.normals[3 * index.normal_index + 0],
								attrib.normals[3 * index.normal_index + 1],
								attrib.normals[3 * index.normal_index + 2]
						};
					if(~index.texcoord_index)
						tri.m_texcoords[0] = {
								attrib.texcoords[2 * index.texcoord_index + 0],
								1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
					index = shape.mesh.indices[index_offset + v + 1];
					tri.m_positions[1] = {
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
					};
					if(~index.normal_index)
						tri.m_normals[1] = {
								attrib.normals[3 * index.normal_index + 0],
								attrib.normals[3 * index.normal_index + 1],
								attrib.normals[3 * index.normal_index + 2]
						};
					if(~index.texcoord_index)
						tri.m_texcoords[1] = {
								attrib.texcoords[2 * index.texcoord_index + 0],
								1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};

					index = shape.mesh.indices[index_offset + v + 2];
					tri.m_positions[2] = {
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
					};
					if(~index.normal_index)
						tri.m_normals[2] = {
								attrib.normals[3 * index.normal_index + 0],
								attrib.normals[3 * index.normal_index + 1],
								attrib.normals[3 * index.normal_index + 2]
						};
					else //generate normal
					{
						tri.m_normals[2] = tri.m_normals[0] = tri.m_normals[1] =
								glm::normalize(glm::cross(tri.m_positions[1] - tri.m_positions[0], tri.m_positions[2] - tri.m_positions[0]));
					}
					if(~index.texcoord_index)
						tri.m_texcoords[2] = {
								attrib.texcoords[2 * index.texcoord_index + 0],
								1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
				}
				m_aabb.Expand(tri.GetAABB());
			}
			index_offset += num_face_vertex;
			face ++;
		}
	}
	printf("%ld triangles loaded from %s\n", m_triangles.size(), filename);
}
