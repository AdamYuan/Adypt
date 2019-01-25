#ifndef MATH_HPP
#define MATH_HPP

#include <glm/glm.hpp>

inline glm::vec2 SpheremapEncode(const glm::vec3 &n) { return glm::vec2(n.xy)/glm::sqrt(n.z*8+8) + 0.5f; }
inline glm::vec3 SpheremapDecode(const glm::vec2 &enc)
{
	glm::vec4 nn = glm::vec4(enc,0,0)*glm::vec4(2,2,0,0) + glm::vec4(-1,-1,1,-1);
	nn.z = dot(glm::vec3(nn.xyz), -glm::vec3(nn.xyw));
	float sn = glm::sqrt(nn.z);
	nn.x *= sn; nn.y *= sn;
	return glm::vec3(nn.xyz)*2.0f + glm::vec3(0,0,-1);
}

#endif // MATH_HPP
