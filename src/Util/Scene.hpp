#ifndef PRIMITIVE_HPP
#define PRIMITIVE_HPP

#include <vector>
#include <tiny_obj_loader.h>
#include "Shape.hpp"

//load some basic components
struct Scene
{
private:
	std::vector<Triangle> m_triangles;
	AABB m_aabb{};
	std::vector<tinyobj::material_t> m_materials;
	std::string m_base_dir;

public:
	Scene() = default;
	Scene(const char *t_filename) { Load(t_filename); }
	void Load(const char *filename);

	const std::vector<Triangle> &GetTriangles() const { return m_triangles; }
	const std::vector<tinyobj::material_t> &GetTinyobjMaterials() const { return m_materials; }
	const std::string &GetBasePath() const { return m_base_dir; };
	const AABB &GetAABB() const { return m_aabb; }
	//void Clear() { m_triangles.clear(); m_triangles.shrink_to_fit(); m_aabb = AABB(); }
};


#endif
