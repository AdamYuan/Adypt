//
// Created by adamyuan on 12/7/18.
//

#include <random>
#include "OglPathTracer.hpp"
#include "OglScene.hpp"
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

void OglPathTracer::Initialize(const InstanceConfig::PT *config, const OglScene &scene, int width, int height)
{
	m_config = config;
	m_width = width;
	m_height = height;
	m_texture_count = (int)scene.GetTextures().size();
	m_group_x = (GLuint)glm::ceil(m_width / (float)m_config->m_invocation_size);
	m_group_y = (GLuint)glm::ceil(m_height / (float)m_config->m_invocation_size);

	create_buffers();
	create_shaders();
	bind_buffers(scene);

	update_config_args();
}

void OglPathTracer::SetCamera(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &position)
{
	m_camera_args->m_origin_tmin.xyz = position;
	m_camera_args->m_inv_projection = glm::inverse(projection);
	m_camera_args->m_inv_view = glm::inverse(view);
}

void OglPathTracer::Trace(bool enable_pt)
{
	if(enable_pt) //do path tracing
	{
		//clear image if first entry
		if(m_pt_local_spp == 0)
		{
			update_config_args();
			m_viewer_type = ViewerTypes::kPTRadiance;
			glClearTexImage(m_result_tex.Get(), 0, GL_RGBA, GL_FLOAT, nullptr);
			m_sobol_gen.Reset();
			m_tracing_start_time = std::chrono::high_resolution_clock::now();
		}

		m_sobol_gen.Next(m_sobol_seq);
		m_pt_args->m_spp = m_pt_local_spp ++;

		m_pt_shader.Use();
	}
	else //do primary ray
	{
		if(m_pt_local_spp) m_viewer_type = ViewerTypes::kDiffuse;
		m_pt_local_spp = 0;
		m_primaryray_shader.Use();
	}
	m_viewer_args->m_type = m_viewer_type;
	glDispatchCompute(m_group_x, m_group_y, 1);
}

void OglPathTracer::DrawScreen()
{
	m_screen_shader.Use();
	m_screen_vao.DrawArrays(GL_TRIANGLES);
}

void OglPathTracer::create_shaders()
{
	//shader sources
	std::string head, pt_glsl_src, traversal_glsl_src, primaryray_glsl_src, screen_glsl_src;

	{
		std::ifstream in;
		in.open("shaders/pathtracer.glsl");
		pt_glsl_src = {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
		in.close();

		in.open("shaders/traversal.glsl");
		traversal_glsl_src = {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
		in.close();

		in.open("shaders/primaryray.glsl");
		primaryray_glsl_src = {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
		in.close();

		in.open("shaders/screen.glsl");
		screen_glsl_src = {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
		in.close();
	}

	{
		char buffer[1024];
		sprintf(buffer,
				"#version 450 core\n"
				"#extension GL_ARB_bindless_texture : require\n"
				"#define IMG_SIZE ivec2(%d, %d)\n"
				"#define TRAVERSAL_STACK_SIZE %d\n"
				"#define TEXTURE_COUNT %d\n"
				"layout (local_size_x = %d, local_size_y = %d) in;\n",
				m_width, m_height,
				m_config->m_stack_size,
				m_texture_count,
				m_config->m_invocation_size, m_config->m_invocation_size
		);
		head = buffer;
	}

	m_pt_shader.Initialize();
	m_pt_shader.Load((head + traversal_glsl_src + pt_glsl_src).c_str(), GL_COMPUTE_SHADER);

	m_primaryray_shader.Initialize();
	m_primaryray_shader.Load((head + traversal_glsl_src + primaryray_glsl_src).c_str(), GL_COMPUTE_SHADER);

	m_screen_shader.Initialize();
	m_screen_shader.Load(("#version 450 core\n#define VERTEX\n" + screen_glsl_src).c_str(), GL_VERTEX_SHADER);
	m_screen_shader.Load(("#version 450 core\n#define FRAGMENT\n" + screen_glsl_src).c_str(), GL_FRAGMENT_SHADER);
}

void OglPathTracer::create_buffers()
{
	{
		GLenum map_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		m_camera_args_ubo.Initialize();
		m_camera_args_ubo.Storage(sizeof(CameraArgs), map_flags);
		m_camera_args = (CameraArgs*)glMapNamedBufferRange(m_camera_args_ubo.Get(), 0, sizeof(CameraArgs), map_flags);

		m_pt_args_ubo.Initialize();
		m_pt_args_ubo.Storage(sizeof(PathTracerArgs), map_flags);
		m_pt_args = (PathTracerArgs*)glMapNamedBufferRange(m_pt_args_ubo.Get(), 0, sizeof(PathTracerArgs), map_flags);
		m_pt_args->m_spp = 0;

		m_viewer_args_ubo.Initialize();
		m_viewer_args_ubo.Storage(sizeof(ViewerArgs), map_flags);
		m_viewer_args = (ViewerArgs*)glMapNamedBufferRange(m_viewer_args_ubo.Get(), 0, sizeof(ViewerArgs), map_flags);
		m_viewer_args->m_type = 0;

		m_sobol_gen.Reset(m_config->m_max_bounce * 2u);
		m_sobol_ssbo.Initialize();
		m_sobol_ssbo.Storage(sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);
		m_sobol_seq = (GLfloat *)glMapNamedBufferRange(m_sobol_ssbo.Get(), 0, sizeof(GLfloat) * m_sobol_gen.Dim(), map_flags);
	}

	//create textures
	m_result_tex.Initialize();
	m_result_tex.Storage(m_width, m_height, GL_RGBA32F);

	//store tmp primary ray result
	m_primary_tmp_tex.Initialize();
	m_primary_tmp_tex.Storage(m_width, m_height, GL_RGBA32F);

	//create sobol sequence bias
	m_sobol_bias_tex.Initialize();
	m_sobol_bias_tex.Storage(m_width, m_height, GL_RG8);
	{
		std::vector<GLbyte> seeds(m_width*m_height*2u);
		std::random_device rd{};
		std::mt19937 gen{rd()};
		for(auto &i : seeds) i = GLbyte(gen());
		m_sobol_bias_tex.Data(seeds.data(), m_width, m_height, GL_RG, GL_UNSIGNED_BYTE);
	}

	{
		float fw = m_width, fh = m_height;
		float quad_vertices[] {
				-1.0f, -1.0f, 0.0f, fh,
				1.0f, -1.0f, fw, fh,
				1.0f, 1.0f, fw, 0.0f,
				1.0f, 1.0f, fw, 0.0f,
				-1.0f, 1.0f, 0.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, fh };
		m_screen_vbo.Initialize();
		m_screen_vbo.Storage(quad_vertices, quad_vertices + 24, 0);
		m_screen_vao.Initialize(0, 2, 1, 2);
		m_screen_vao.BindVertices(m_screen_vbo);
	}
}

void OglPathTracer::bind_buffers(const OglScene &scene)
{
	glBindImageTexture(0, m_result_tex.Get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, m_primary_tmp_tex.Get(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(2, m_sobol_bias_tex.Get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG8);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_camera_args_ubo.Get());
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_pt_args_ubo.Get());
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_viewer_args_ubo.Get());
	if(m_texture_count) glBindBufferBase(GL_UNIFORM_BUFFER, 3, scene.GetTextureHandleBuffer().Get());

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.GetBVHNodeBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.GetTriIndexBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.GetTriMatrixBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.GetTriangleBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.GetMaterialBuffer().Get());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_sobol_ssbo.Get());
}

void OglPathTracer::SaveResult(const char *filename, bool save_as_fp16)
{
	char *err{nullptr};

	int size = m_width * m_height;
	std::vector<GLfloat> pixels((size_t)size * 3);
	glGetTextureImage(m_result_tex.Get(), 0, GL_RGB, GL_FLOAT, size * 3 * sizeof(GLfloat), pixels.data());

	if(SaveEXR(pixels.data(), m_width, m_height, 3, save_as_fp16, filename, (const char **)&err) < 0)
		printf("[PT]ERR: %s\n", err);
	else
		printf("[PT]INFO: Saved image to %s\n", filename);
	free(err);
}

void OglPathTracer::update_config_args()
{
	m_camera_args->m_origin_tmin.w = m_config->m_ray_tmin;
	m_pt_args->m_subpixel = m_config->m_subpixel;
	m_pt_args->m_tmplife = m_config->m_tmp_lifetime;
	m_pt_args->m_max_bounce = m_config->m_max_bounce;
	m_pt_args->m_clamp = m_config->m_clamp;
	//m_pt_args->m_min_glossy_exp = m_config->m_min_glossy_exp;
	m_pt_args->m_sun[0] = m_config->m_sun[0];
	m_pt_args->m_sun[1] = m_config->m_sun[1];
	m_pt_args->m_sun[2] = m_config->m_sun[2];
}
