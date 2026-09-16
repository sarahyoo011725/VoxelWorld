#pragma once
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/FBO.h"
#include "Texture/Texture.h"
#include "Shader/ShaderManager.h"
#include "Entity/Geometries.h"
#include "Block/Block.h"
#include "WindowSetting.h"

/*
* renders the crosshair/HUD, hovered-block outline, and post-process pass
*/
class PlayerRenderer
{
public:
	PlayerRenderer(WindowSetting* setting);
	bool enable_outline = true;
	const int shadow_resolution = 2048;
	void update();
	void sync_fbo_size();
	void bind_fbo();
	void unbind_fbo();
	void bind_shadow_fbo();
	void unbind_shadow_fbo();
	void draw_HUDs();
	void draw_outlines(mat4 cam_matrix, Block* hovered_block);
	void draw_sky_discs(mat4 view, mat4 projection, vec3 eye_position, vec3 to_sun);
	void post_process();
private:
	void outline_hovered_cube(Block* hovered_block);

	WindowSetting* window_setting;
	ShaderManager& sm;
	int fbo_width;
	int fbo_height;

	const float outline_thickness = 2.0f;
	vec4 hovered_block_outline_color = vec4(0.0, 1.0, 1.0, 1.0); //cyan
	vec4 crosshair_color = vec4(1.0); //white

	const float sky_disc_distance = 150.0f; //must stay under Camera's far_plane (180) or it gets clipped
	const float sky_disc_size = 15.0f;
	const float sky_glow_size = 38.0f;
	const vec3 sun_core_color = vec3(1.0f, 0.98f, 0.85f);
	const vec3 sun_edge_color = vec3(1.0f, 0.78f, 0.35f);
	const vec3 sun_glow_color = vec3(1.0f, 0.85f, 0.5f);
	const vec3 moon_core_color = vec3(0.92f, 0.95f, 1.0f);
	const vec3 moon_edge_color = vec3(0.55f, 0.6f, 0.7f);
	const vec3 moon_glow_color = vec3(0.6f, 0.68f, 0.85f);

	VAO outline_vao = VAO();
	VBO outline_vbo = VBO(cube_edges.data(), sizeof(vec3) * cube_edges.size(), GL_STATIC_DRAW);
	VAO HUD_vao = VAO();
	VBO HUD_vbo = VBO(crosshair_vertices.data(), sizeof(vertex_2d) * crosshair_vertices.size(), GL_STATIC_DRAW);
	VAO quad_vao = VAO();
	VBO quad_vbo = VBO(quad_vertices.data(), sizeof(vertex_2d) * quad_vertices.size(), GL_STATIC_DRAW);
	FBO fbo = FBO();
	Texture texture_color_buffer;
	Texture depth_texture;

	FBO shadow_fbo = FBO();
	Texture shadow_map;
};
