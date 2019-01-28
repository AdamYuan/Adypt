//
// Created by adamyuan on 12/7/18.
//

#include <random>
#include "OglPathTracer.hpp"
#include "OglScene.hpp"
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

void OglPathTracer::Initialize(const Platform &platform, const OglScene &scene)
{
	m_config = platform.GetPTConfig();
	m_uiwidth = platform.GetUIConfig().m_width;
	m_uiheight = platform.GetUIConfig().m_height;
	m_texture_count = (int)scene.GetTextures().size();
	m_group_x = (GLuint)glm::ceil(m_uiwidth / (float)m_config.m_invocation_size);
	m_group_y = (GLuint)glm::ceil(m_uiheight / (float)m_config.m_invocation_size);

	create_buffers();
	create_shaders();
	bind_buffers(scene);

}

void OglPathTracer::UpdateCamera(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &position)
{
	m_args->m_position = position;
	m_args->m_inv_projection = glm::inverse(projection);
	m_args->m_inv_view = glm::inverse(view);
}

void OglPathTracer::Render()
{
	//clear image if first entry
	if(m_path_tracing_flag)
	{
		if(m_local_spp == -1)
		{
			glClearTexImage(m_result_tex.Get(), 0, GL_RGBA, GL_FLOAT, nullptr);
			m_sobol_gen.Reset();
			m_path_tracing_start_time = std::chrono::high_resolution_clock::now();
		}
		//dont do progressive render
		m_sobol_gen.Next(m_sobol_seq);
		m_args->m_spp = ++m_local_spp;
	}
	else
	{
		//dont do progressive render
		m_args->m_spp = -1;
		m_local_spp = -1;
	}

	m_shader.Use();
	glDispatchCompute(m_group_x, m_group_y, 1);
}

void OglPathTracer::create_shaders()
{
	//generate glsl code
	std::string pis = std::to_string(m_config.m_invocation_size);
	std::string glsl_src = generate_shader_head();
	printf("%s\n", glsl_src.c_str());
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
	m_args->m_spp = -1;
	m_local_spp = -1;

	m_sobol_gen.Reset(m_config.m_max_bounce * 2u);
	m_sobol_buffer.Initialize();
	m_sobol_buffer.Storage(sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);
	m_sobol_seq = (GLfloat *)glMapNamedBufferRange(m_sobol_buffer.Get(), 0, sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);

	//create textures
	m_result_tex.Initialize();
	m_result_tex.Load(mygl3::ImageInfo(m_uiwidth, m_uiheight, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr));

	//store tmp primary ray result
	if(m_config.m_tmp_lifetime > 1)
	{
		m_primary_tmp_tex.Initialize();
		m_primary_tmp_tex.Load(mygl3::ImageInfo(m_uiwidth, m_uiheight, 0, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr));
	}

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
	if(m_config.m_tmp_lifetime > 1)
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

std::string OglPathTracer::generate_shader_head()
{
	char buffer[1024];
	sprintf(buffer,
			"#version 450 core\n"
			"#extension GL_ARB_bindless_texture : require\n"
			"#define IMG_SIZE ivec2(%d, %d)\n"
			"#define STACK_SIZE %d\n"
			"#define TEXTURE_COUNT %d\n"
			"#define MAX_BOUNCE %d\n"
			"#define SUB_PIXEL %d\n"
			"#define TMP_LIFETIME %d\n"
			"#define RAY_TMIN %lf\n"
			"#define MIN_GLOSSY_EXP %lf\n"
			"#define CLAMP %lf\n"
			"#define SUN vec3(%lf, %lf, %lf)\n"
			"layout (local_size_x = %d, local_size_y = %d) in;\n",
			m_uiwidth, m_uiheight,
			m_config.m_stack_size,
			m_texture_count,
			m_config.m_max_bounce,
			m_config.m_subpixel,
			m_config.m_tmp_lifetime,
			m_config.m_ray_tmin,
			m_config.m_min_glossy_exp,
			m_config.m_clamp,
			m_config.m_sun_r, m_config.m_sun_g, m_config.m_sun_b,
			m_config.m_invocation_size, m_config.m_invocation_size
			);
	return {buffer};
}

void OglPathTracer::SaveResult(const char *name)
{
	static char filename[64], err[1024];

	int size = m_uiwidth * m_uiheight;
	std::vector<GLfloat> pixels((size_t)size * 3);
	glGetTextureImage(m_result_tex.Get(), 0, GL_RGB, GL_FLOAT, size * 3 * sizeof(GLfloat), pixels.data());

	sprintf(filename, "%s-%dspp.exr", name, m_local_spp);

	if(SaveEXR(pixels.data(), m_uiwidth, m_uiheight, 3, true, filename, (const char**)(&err)) < 0)
		printf("[PT]ERR: %s\n", err);
	else
		printf("[PT]INFO: Saved image to %s\n", filename);
}
