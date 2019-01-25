//
// Created by adamyuan on 7/22/18.
//

#ifndef PATHGL_BVH_SHAPE_HPP
#define PATHGL_BVH_SHAPE_HPP

#include <cfloat>
#include <cstdio>
#include <cmath>
#include <glm/glm.hpp>

struct Intersection
{
	glm::vec3 m_normal;
	glm::vec2 m_texcoords;
	int m_texid;
	float t;
};

struct Ray
{
	glm::vec3 m_origin, m_dir;

	inline Ray() : m_origin(.0f, .0f, .0f), m_dir(.0f, 1.0f, .0f) {}
	inline Ray( const Ray &t_ray ) = default;
	inline Ray( const glm::vec3 &t_origin, const glm::vec3 &t_dir ) :
			m_origin(t_origin), m_dir(t_dir) {}

	inline glm::vec3 GetPoint(float t) const { return m_origin + m_dir * t; }
};

struct AABB
{
	glm::vec3 m_min, m_max;

	inline AABB() : m_min{FLT_MAX}, m_max{-FLT_MAX} { } //initialize with empty
	inline AABB(const glm::vec3 &t_min, const glm::vec3 &t_max)
		: m_min{t_min}, m_max{t_max}
	{}

	inline AABB(const AABB &t_a, const AABB &t_b)
		: m_min{glm::min(t_a.m_min, t_b.m_min)}, m_max{glm::max(t_a.m_max, t_b.m_max)}
	{}

	inline void Expand(const glm::vec3 &vec)
	{
		m_min = glm::min(vec, m_min); m_max = glm::max(vec, m_max);
	}
	inline void Expand(const AABB &aabb)
	{
		m_min = glm::min(aabb.m_min, m_min); m_max = glm::max(aabb.m_max, m_max);
	}
	inline void IntersectAABB(const AABB &aabb)
	{
		m_min = glm::max(m_min, aabb.m_min);
		m_max = glm::min(m_max, aabb.m_max);
	}
	inline bool Valid() const { return m_min.x <= m_max.x && m_min.y <= m_max.y && m_min.z <= m_max.z; }
	inline glm::vec3 GetCenter() const { return (m_min + m_max) * 0.5f; }
	inline glm::vec3 GetExtent() const { return m_max - m_min; }
	inline float GetArea() const
	{
		glm::vec3 extent = GetExtent();
		return (extent.x*(extent.y + extent.z) + extent.y*extent.z) * 2.0f;
	}
};

//can be directly put into gpu
struct Triangle
{
	glm::vec3 m_positions[3], m_normals[3];
	glm::vec2 m_texcoords[3]; int m_matid;

	Triangle() = default;
	Triangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
			 const glm::vec3 &n0, const glm::vec3 &n1, const glm::vec3 &n2)
		: m_positions{v0, v1, v2}, m_normals{n0, n1, n2}
	{}

	inline AABB GetAABB() const
	{
		return {
			glm::min(m_positions[0], glm::min(m_positions[1], m_positions[2])),
					glm::max(m_positions[0], glm::max(m_positions[1], m_positions[2]))
		};
	}
};

#endif //PATHGL_BVH_SHAPE_HPP
