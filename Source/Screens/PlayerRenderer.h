#pragma once
#include <FastNoise/FastNoiseLite.h>
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/FBO.h"
#include "Texture/Texture.h"
#include "Shader/ShaderManager.h"
#include "Entity/Geometries.h"
#include "Entity/Inventory.h"
#include "Block/Block.h"
#include "WindowSetting.h"
#include "BitmapFont.h"
#include "World/PerfStats.h"

/*
* renders the crosshair/HUD, hovered-block outline, and post-process pass
*/
class PlayerRenderer
{
public:
	PlayerRenderer(WindowSetting* setting);
	const int shadow_resolution = 2048;
	void sync_fbo_size();
	void bind_fbo();
	void unbind_fbo();
	void bind_shadow_fbo();
	void unbind_shadow_fbo();
	void draw_HUDs();
	void draw_hotbar(const Inventory& inventory);
	void draw_outlines(mat4 cam_matrix, Block* hovered_block, vec3 hovered_position);
	void draw_sky_discs(mat4 view, mat4 projection, vec3 eye_position, vec3 to_sun);
	void draw_sky_background(mat4 view, mat4 projection, vec3 zenith_color, vec3 horizon_color, vec3 to_sun);
	void draw_clouds(mat4 cam_matrix, vec2 player_xz, float time, vec3 light_color);
	void draw_perf_overlay(const PerfStats& stats);
	void post_process();
private:
	void draw_text(const string& text, vec2 top_left, float pixel_size, vec4 color);
	void outline_hovered_cube(Block* hovered_block, vec3 hovered_position);
	void generate_cloud_mesh(vec2 center);
	void add_cloud_face(block_face face, vec3 center, float half_x, float half_z, float half_y);

	WindowSetting* window_setting;
	ShaderManager& sm;
	int fbo_width;
	int fbo_height;

	const float outline_thickness = 2.0f;
	vec4 hovered_block_outline_color = vec4(0.0f, 0.0f, 0.0f, 0.4f); 
	vec4 crosshair_color = vec4(1.0); //white

	const float overlay_pixel_size = 0.009f; //one dot of the 5x7 font, in ndc

	const float hotbar_slot_size = 0.09f; //ndc half-size before aspect correction
	const float hotbar_slot_spacing = 0.02f;
	const float hotbar_y = -0.88f;
	const float hotbar_icon_scale = 0.68f;
	const vec4 hotbar_slot_color = vec4(0.15f, 0.15f, 0.15f, 0.6f);
	const vec4 hotbar_selected_color = vec4(0.9f, 0.9f, 0.9f, 0.75f);

	const float sky_disc_distance = 150.0f; //must stay under Camera's far_plane (180) or it gets clipped
	const float sky_disc_size = 15.0f;
	const float sky_glow_size = 38.0f;
	const vec3 sun_core_color = vec3(1.0f, 0.98f, 0.85f);
	const vec3 sun_edge_color = vec3(1.0f, 0.78f, 0.35f);
	const vec3 sun_glow_color = vec3(1.0f, 0.85f, 0.5f);
	const vec3 moon_core_color = vec3(0.92f, 0.95f, 1.0f);
	const vec3 moon_edge_color = vec3(0.55f, 0.6f, 0.7f);
	const vec3 moon_glow_color = vec3(0.6f, 0.68f, 0.85f);

	const float cloud_cell_size = 16.0f;
	const float cloud_thickness = 2.5f;
	const float cloud_base_height = 70.0f; //above the terrain's max height (50)
	const int cloud_grid_radius = 13; 
	const float cloud_threshold = 0.42f; //higher = sparser clouds
	const float cloud_wind_speed = 0.3f;
	const float cloud_regen_distance = 80.0f;
	FastNoiseLite cloud_noise;
	vector<vertex> cloud_vertices;
	vec2 last_cloud_center = vec2(1e9f); 

	VAO outline_vao = VAO();
	VBO outline_vbo = VBO(cube_edges.data(), sizeof(vec3) * cube_edges.size(), GL_STATIC_DRAW);
	VAO HUD_vao = VAO();
	VBO HUD_vbo = VBO(crosshair_vertices.data(), sizeof(vertex_2d) * crosshair_vertices.size(), GL_STATIC_DRAW);
	VAO quad_vao = VAO();
	VBO quad_vbo = VBO(quad_vertices.data(), sizeof(vertex_2d) * quad_vertices.size(), GL_STATIC_DRAW);
	VAO cloud_vao = VAO();
	VBO cloud_vbo = VBO(nullptr, sizeof(vertex) * 0, GL_STATIC_DRAW);
	FBO fbo = FBO();
	Texture texture_color_buffer;
	Texture depth_texture;

	FBO shadow_fbo = FBO();
	Texture shadow_map;
};
