#version 450 core

out vec4 FragColor;
in vec2 vTexcoords;
layout (binding = 0) uniform sampler2D uTexture;

void main()
{
	vec3 c3 = texture(uTexture, vTexcoords).rgb;
	c3 = pow(c3, vec3(1.0f / 2.2f));
	FragColor = vec4(c3, 1.0f);
}
