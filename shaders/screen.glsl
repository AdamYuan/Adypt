#ifdef VERTEX
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexcoords;
out vec2 vTexcoords;
void main() {
	gl_Position = vec4(aPosition, 1.0, 1.0);
	vTexcoords = aTexcoords; 
}
#endif
#ifdef FRAGMENT
out vec4 FragColor;
in vec2 vTexcoords;
layout(rgba32f, binding = 0) uniform image2D uTexture;
layout (std140, binding = 2) uniform uuViewer { int uType; };
void main() { 
	vec4 v = imageLoad(uTexture, ivec2(vTexcoords));
	if(uType <= 3) //diffuse, specular, emissive, pt_radiance
		FragColor = vec4(pow(v.xyz, vec3(1.0 / 2.2)), 1.0f); 
	else if(uType == 4) //pt_radiance
		FragColor = vec4(vec3(pow(v.w, 1.0 / 2.2)), 1.0f);
	else //normal, position
		FragColor = vec4(normalize(v.xyz)*0.5f + 0.5f, 1.0f); 
}
#endif
