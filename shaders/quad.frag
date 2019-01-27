#version 450 core
out vec4 FragColor;
in vec2 vTexcoords;
layout (binding = 0) uniform sampler2D uTexture;
void main() { FragColor = vec4(pow(texture(uTexture, vTexcoords).rgb, vec3(1.0 / 2.2)), 1.0f); }
