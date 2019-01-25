#version 450 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexcoords;

out vec2 vTexcoords;

void main()
{
	gl_Position = vec4(aPosition, 1.0, 1.0);
	vTexcoords = aTexcoords;
}
