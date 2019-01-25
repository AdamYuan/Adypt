//
// Created by adamyuan on 12/7/18.
//

#include <random>
#include "OglPathTracer.hpp"
#include "OglScene.hpp"

void OglPathTracer::Initialize(const Platform &platform, const OglScene &scene)
{
	m_config = platform.GetPTConfig();
	m_uiwidth = platform.GetUIConfig().m_width;
	m_uiheight = platform.GetUIConfig().m_height;
	m_group_x = (GLuint)glm::ceil(m_uiwidth / (float)m_config.m_invocation_size);
	m_group_y = (GLuint)glm::ceil(m_uiheight / (float)m_config.m_invocation_size);

	create_buffers();
	create_shaders((int)scene.GetTextures().size());
	bind_buffers(scene);
}

void OglPathTracer::UpdateCamera(const OglScene &scene, const glm::mat4 &projection, const glm::mat4 &view,
								 const glm::vec3 &position)
{
	m_args->m_position.xyz = position;
	m_args->m_inv_projection = glm::inverse(projection);
	m_args->m_inv_view = glm::inverse(view);

	//dont do progressive render
	m_args->m_iteration = -1;
	m_local_iteration = -1;

	m_shader.Use();
	glDispatchCompute(m_group_x, m_group_y, 1);
}

void OglPathTracer::Render()
{
	//clear image if first entry
	if(m_local_iteration == -1)
	{
		glClearTexImage(m_result_tex.Get(), 0, GL_RGBA, GL_FLOAT, nullptr);
		m_sobol_gen.Reset();
	}
	//dont do progressive render
	m_sobol_gen.Next(m_sobol_seq);
	m_args->m_iteration = ++m_local_iteration;

	m_shader.Use();
	glDispatchCompute(m_group_x, m_group_y, 1);
}

void OglPathTracer::create_shaders(int texture_count)
{
	//generate glsl code
	std::string pis = std::to_string(m_config.m_invocation_size);
	std::string glsl_src =
			"#version 450 core\n"
			"#extension GL_ARB_bindless_texture : require\n"
			"#define STACK_SIZE " + std::to_string(m_config.m_stack_size) +
			"\n#define TEXTURE_COUNT " + std::to_string(texture_count) +
			"\n#define RAY_T_MIN " + std::to_string(m_config.m_ray_tmin) +
			"\n#define MAX_BOUNCE " + std::to_string(m_config.m_max_bounce) +
			"\nlayout (local_size_x = " + pis + ", local_size_y = " + pis + ") in;\n";
	//load file
	std::ifstream glslin{"shaders/pathtracer.glsl"};
	glsl_src += {std::istreambuf_iterator<char>{glslin}, std::istreambuf_iterator<char>{}};

	m_shader.Initialize();
	m_shader.Load(glsl_src.c_str(), GL_COMPUTE_SHADER);
}

void OglPathTracer::create_buffers()
{
	GLenum map_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	//create buffers
	m_args_buffer.Initialize();
	m_args_buffer.Storage(sizeof(GPUShaderArgs), map_flags);
	m_args = (GPUShaderArgs*)glMapNamedBufferRange(m_args_buffer.Get(), 0, sizeof(GPUShaderArgs), map_flags);
	//set default args
	{
		m_args->m_img_width = m_uiwidth;
		m_args->m_img_height = m_uiheight;
		m_args->m_iteration = -1;
		m_local_iteration = -1;
	}

	m_sobol_gen.Reset(m_config.m_max_bounce * 2u);
	m_sobol_buffer.Initialize();
	m_sobol_buffer.Storage(sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);
	m_sobol_seq = (GLfloat *)glMapNamedBufferRange(m_sobol_buffer.Get(), 0, sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);

	//create textures
	m_result_tex.Initialize();
	m_result_tex.Load(mygl3::ImageInfo(m_uiwidth, m_uiheight, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr));

	//store tmp primary ray result
	m_primary_tmp_tex.Initialize();
	m_primary_tmp_tex.Load(mygl3::ImageInfo(m_uiwidth, m_uiheight, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr));

	//create sobol sequence bias
	m_sobol_bias_tex.Initialize();
	{
		std::vector<GLbyte> seeds(m_uiwidth*m_uiheight*2u);
		std::random_device rd{};
		std::mt19937 gen{rd()};
		for(auto &i : seeds) i = GLbyte(gen());
		m_sobol_bias_tex.Load(mygl3::ImageInfo(m_uiwidth, m_uiheight, 0, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, seeds.data()));
	}
}

void OglPathTracer::bind_buffers(const OglScene &scene)
{
	//create
	glBindImageTexture(0, m_result_tex.Get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, m_primary_tmp_tex.Get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, m_sobol_bias_tex.Get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG8);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_args_buffer.Get());
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, scene.GetTextureHandleBuffer().Get());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.GetBVHNodeBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.GetTriIndexBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.GetTriMatrixBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.GetTriangleBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.GetMaterialBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_sobol_buffer.Get());
}
