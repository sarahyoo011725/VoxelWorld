#include "PlayerRenderer.h"
#include "World/StructureGenerator.h"
#include <glm/gtc/matrix_transform.hpp>

PlayerRenderer::PlayerRenderer(WindowSetting* setting)
	: sm(ShaderManager::get_instance()), window_setting(setting), fbo_width(setting->width), fbo_height(setting->height),
	texture_color_buffer(setting->width, setting->height, GL_TEXTURE2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_NEAREST),
	depth_texture(setting->width, setting->height, GL_TEXTURE3, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_LINEAR),
	shadow_map(shadow_resolution, shadow_resolution, GL_TEXTURE4, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_LINEAR) {

	outline_vao.bind();
	outline_vao.link_attrib(outline_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	HUD_vao.bind();
	HUD_vao.link_attrib(HUD_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	HUD_vao.link_attrib(HUD_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));

	quad_vao.bind();
	quad_vao.link_attrib(quad_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	quad_vao.link_attrib(quad_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));

	cloud_vao.bind();
	cloud_vao.link_attrib(cloud_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	cloud_vao.link_attrib(cloud_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
	cloud_vao.link_attrib(cloud_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));

	cloud_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	//frequency high enough that its correlation length is close to one cell width,
	//so adjacent cells aren't almost always occupied together - otherwise every
	//formation spans many contiguous cells and reads as one big overlapping mass
	cloud_noise.SetFrequency(0.08f);

	sm.frame_buffer_shader.activate();
	sm.frame_buffer_shader.set_uniform_1i("screen_texture", 1);
	sm.frame_buffer_shader.set_uniform_1i("depth_texture", 3);

	sm.HUD_shader.activate();
	sm.HUD_shader.set_uniform_1i("texture1", 1);

	sm.default_shader.activate();
	sm.default_shader.set_uniform_1i("shadow_map", 4);
	sm.wave_shader.activate();
	sm.wave_shader.set_uniform_1i("shadow_map", 4);
	sm.foliage_shader.activate();
	sm.foliage_shader.set_uniform_1i("shadow_map", 4);

	fbo.bind();
	fbo.attach_texture(GL_COLOR_ATTACHMENT0, texture_color_buffer.get_id());
	fbo.attach_texture(GL_DEPTH_ATTACHMENT, depth_texture.get_id());
	fbo.unbind();

	//sampling outside the shadow map's coverage should read as "fully lit" (max depth),
	//not "fully shadowed" - clamp-to-border with a white (1.0 depth) border achieves that
	glBindTexture(GL_TEXTURE_2D, shadow_map.get_id());
	float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glBindTexture(GL_TEXTURE_2D, 0);

	shadow_fbo.bind();
	shadow_fbo.attach_texture(GL_DEPTH_ATTACHMENT, shadow_map.get_id());
	glDrawBuffer(GL_NONE); //depth-only framebuffer, no color attachment
	glReadBuffer(GL_NONE);
	shadow_fbo.unbind();
}

void PlayerRenderer::update() {
	if (glfwGetKey(window_setting->window, GLFW_KEY_3) == GLFW_PRESS) {
		enable_outline = !enable_outline;
	}
}

void PlayerRenderer::sync_fbo_size() {
	if (window_setting->width != fbo_width || window_setting->height != fbo_height) {
		fbo_width = window_setting->width;
		fbo_height = window_setting->height;
		texture_color_buffer.resize(fbo_width, fbo_height);
		depth_texture.resize(fbo_width, fbo_height);
	}
}

void PlayerRenderer::bind_fbo() {
	fbo.bind();
	shadow_map.activate();
	shadow_map.bind();
}

void PlayerRenderer::unbind_fbo() {
	fbo.unbind();
}

void PlayerRenderer::bind_shadow_fbo() {
	shadow_fbo.bind();
}

void PlayerRenderer::unbind_shadow_fbo() {
	shadow_fbo.unbind();
}

/*
	draws 2D HUD components like crosshair
*/
void PlayerRenderer::draw_HUDs() {
	sm.HUD_shader.activate();
	sm.HUD_shader.set_uniform_1i("use_texture", GL_FALSE);
	sm.HUD_shader.set_uniform_4f("color", 1, crosshair_color);
	//crosshair vertices are already baked at their final NDC position - identity transform
	sm.HUD_shader.set_uniform_2f("offset", 1, vec2(0.0f));
	sm.HUD_shader.set_uniform_2f("scale", 1, vec2(1.0f));
	HUD_vao.bind();
	glLineWidth(1.0);
	glDrawArrays(GL_LINES, 0, 4);
}

/*
	draws the 9-slot hotbar along the bottom of the screen: a background per slot
	(brighter for the selected one) and an item icon sampled from the same texture
	atlas blocks use
*/
void PlayerRenderer::draw_hotbar(const Inventory& inventory) {
	sm.HUD_shader.activate();
	sm.HUD_shader.set_uniform_1i("texture1", 1);
	quad_vao.bind();

	glDisable(GL_DEPTH_TEST);

	float aspect = (float)window_setting->height / (float)window_setting->width;
	float slot_step = hotbar_slot_size * 2.0f + hotbar_slot_spacing;
	float total_width = Inventory::size * slot_step - hotbar_slot_spacing;
	float start_x = -total_width / 2.0f + hotbar_slot_size;
	vec2 slot_scale = vec2(hotbar_slot_size * aspect, hotbar_slot_size);

	for (int i = 0; i < Inventory::size; ++i) {
		vec2 offset = vec2(start_x + i * slot_step, hotbar_y);

		sm.HUD_shader.set_uniform_1i("use_texture", GL_FALSE);
		sm.HUD_shader.set_uniform_4f("color", 1, (i == inventory.selected_slot) ? hotbar_selected_color : hotbar_slot_color);
		sm.HUD_shader.set_uniform_2f("offset", 1, offset);
		sm.HUD_shader.set_uniform_2f("scale", 1, slot_scale);
		sm.HUD_shader.set_uniform_2f("uv_offset", 1, vec2(0.0f));
		sm.HUD_shader.set_uniform_2f("uv_scale", 1, vec2(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());

		const item_stack& stack = inventory.slots[i];
		if (stack.type == none) continue;

		vec2 texture_coord;
		if (is_nonblock(stack.type)) {
			texture_coord = grass_text_coord;
		}
		else if (texture_map.find(stack.type) != texture_map.end()) {
			texture_coord = texture_map[stack.type][Front];
		}
		else {
			continue;
		}

		vec2 uv_min = vec2((texture_coord.x - 1.0f) / textures_columns, (texture_coord.y - 1.0f) / texture_rows);
		vec2 uv_scale = vec2(1.0f / textures_columns, 1.0f / texture_rows);

		sm.HUD_shader.set_uniform_1i("use_texture", GL_TRUE);
		sm.HUD_shader.set_uniform_2f("offset", 1, offset);
		sm.HUD_shader.set_uniform_2f("scale", 1, slot_scale * hotbar_icon_scale);
		sm.HUD_shader.set_uniform_2f("uv_offset", 1, uv_min);
		sm.HUD_shader.set_uniform_2f("uv_scale", 1, uv_scale);
		glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
	}

	glEnable(GL_DEPTH_TEST);
}

/*
	draws outlined objects
*/
void PlayerRenderer::draw_outlines(mat4 cam_matrix, Block* hovered_block) {
	if (!enable_outline) return;
	sm.outline_shader.activate();
	sm.outline_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, cam_matrix);
	outline_vao.bind();
	outline_hovered_cube(hovered_block);
}

/*
	draws the sun and moon as camera-facing discs, opposite each other in the sky.
	drawn with depth writes off so terrain drawn afterward still occludes them correctly,
	without the discs themselves blocking anything
*/
void PlayerRenderer::draw_sky_discs(mat4 view, mat4 projection, vec3 eye_position, vec3 to_sun) {
	sm.sky_disc_shader.activate();
	sm.sky_disc_shader.set_uniform_mat4f("view", 1, GL_FALSE, view);
	sm.sky_disc_shader.set_uniform_mat4f("projection", 1, GL_FALSE, projection);
	quad_vao.bind();

	glDepthMask(GL_FALSE);

	vec3 sun_center = eye_position + to_sun * sky_disc_distance;
	vec3 moon_center = eye_position - to_sun * sky_disc_distance;

	//glow halos first (larger, soft, behind), then the flat squares on top as the core
	sm.sky_disc_shader.set_uniform_1i("is_glow", GL_TRUE);
	sm.sky_disc_shader.set_uniform_1f("disc_size", sky_glow_size);

	sm.sky_disc_shader.set_uniform_3f("disc_center", 1, sun_center);
	sm.sky_disc_shader.set_uniform_3f("color", 1, sun_glow_color);
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());

	sm.sky_disc_shader.set_uniform_3f("disc_center", 1, moon_center);
	sm.sky_disc_shader.set_uniform_3f("color", 1, moon_glow_color);
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());

	sm.sky_disc_shader.set_uniform_1i("is_glow", GL_FALSE);
	sm.sky_disc_shader.set_uniform_1f("disc_size", sky_disc_size);

	sm.sky_disc_shader.set_uniform_3f("disc_center", 1, sun_center);
	sm.sky_disc_shader.set_uniform_3f("core_color", 1, sun_core_color);
	sm.sky_disc_shader.set_uniform_3f("color", 1, sun_edge_color);
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());

	sm.sky_disc_shader.set_uniform_3f("disc_center", 1, moon_center);
	sm.sky_disc_shader.set_uniform_3f("core_color", 1, moon_core_color);
	sm.sky_disc_shader.set_uniform_3f("color", 1, moon_edge_color);
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());

	glDepthMask(GL_TRUE);
}

/*
	draws a full-screen horizon-to-zenith gradient with a warm glow near the sun.
	must be drawn with depth test disabled, before anything else, as the scene's backdrop
*/
void PlayerRenderer::draw_sky_background(mat4 view, mat4 projection, vec3 zenith_color, vec3 horizon_color, vec3 to_sun) {
	sm.sky_shader.activate();
	sm.sky_shader.set_uniform_mat4f("inv_view", 1, GL_FALSE, inverse(view));
	sm.sky_shader.set_uniform_mat4f("inv_projection", 1, GL_FALSE, inverse(projection));
	sm.sky_shader.set_uniform_3f("zenith_color", 1, zenith_color);
	sm.sky_shader.set_uniform_3f("horizon_color", 1, horizon_color);
	sm.sky_shader.set_uniform_3f("to_sun", 1, to_sun);
	quad_vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
}

/*
	builds an actual voxel-style cloud mesh: a grid of cells, each either empty or a
	solid box. rebuilding is not cheap enough to do every frame, so it only happens
	when the player has moved far enough that the old mesh no longer covers them (see
	draw_clouds), not on a timer or every frame.
*/
void PlayerRenderer::generate_cloud_mesh(vec2 center) {
	cloud_vertices.clear();
	last_cloud_center = center;

	//snap to the cell grid so regenerating doesn't shift the pattern's alignment
	vec2 snapped_center = vec2(floor(center.x / cloud_cell_size) * cloud_cell_size, floor(center.y / cloud_cell_size) * cloud_cell_size);

	int size = cloud_grid_radius * 2 + 1;
	vector<bool> occupied(static_cast<size_t>(size) * size, false);
	auto cell_index = [size](int x, int z) { return (x + size / 2) * size + (z + size / 2); };

	for (int cx = -cloud_grid_radius; cx <= cloud_grid_radius; ++cx) {
		for (int cz = -cloud_grid_radius; cz <= cloud_grid_radius; ++cz) {
			float wx = snapped_center.x + cx * cloud_cell_size;
			float wz = snapped_center.y + cz * cloud_cell_size;
			occupied[cell_index(cx, cz)] = cloud_noise.GetNoise(wx, wz) > cloud_threshold;
		}
	}

	//a small deterministic per-cell hash, decorrelated from the occupancy noise via
	//an arbitrary seed offset, so no two cloud boxes end up the same size/shape
	auto cell_hash = [](int cx, int cz, float seed) {
		float v = sin((float)cx * 127.1f + (float)cz * 311.7f + seed) * 43758.5453f;
		return v - floor(v);
	};

	float base_half_xz = cloud_cell_size * 0.4f;
	float base_half_y = cloud_thickness * 0.5f;

	for (int cx = -cloud_grid_radius; cx <= cloud_grid_radius; ++cx) {
		for (int cz = -cloud_grid_radius; cz <= cloud_grid_radius; ++cz) {
			if (!occupied[cell_index(cx, cz)]) continue;

			float cell_x = snapped_center.x + cx * cloud_cell_size;
			float cell_z = snapped_center.y + cz * cloud_cell_size;

			//1-3 overlapping sub-boxes per cell, each with its own offset and independently
			//jittered x/z extents, so a cloud unit reads as an irregular cluster of
			//rectangular blocks instead of a single uniformly-scaled cube - kept small and
			//close to the cell center so neighboring cells' clusters don't bleed into each other
			int sub_box_count = 1 + (cell_hash(cx, cz, 201.1f) > 0.65f ? 1 : 0) + (cell_hash(cx, cz, 404.4f) > 0.88f ? 1 : 0);

			for (int i = 0; i < sub_box_count; ++i) {
				float s = (float)i * 91.0f;
				float offset_x = (cell_hash(cx, cz, s + 1.0f) - 0.5f) * cloud_cell_size * 0.25f;
				float offset_z = (cell_hash(cx, cz, s + 2.0f) - 0.5f) * cloud_cell_size * 0.25f;
				float half_x = base_half_xz * (0.45f + cell_hash(cx, cz, s + 3.0f) * 0.5f);
				float half_z = base_half_xz * (0.45f + cell_hash(cx, cz, s + 4.0f) * 0.5f);
				float half_y = base_half_y * (0.5f + cell_hash(cx, cz, s + 5.0f) * 1.0f);
				float height_jitter = (cell_hash(cx, cz, s + 6.0f) - 0.5f) * 3.0f;

				vec3 box_center = vec3(cell_x + offset_x, cloud_base_height + height_jitter, cell_z + offset_z);

				add_cloud_face(Top, box_center, half_x, half_z, half_y);
				add_cloud_face(Bottom, box_center, half_x, half_z, half_y);
				add_cloud_face(Left, box_center, half_x, half_z, half_y);
				add_cloud_face(Right, box_center, half_x, half_z, half_y);
				add_cloud_face(Back, box_center, half_x, half_z, half_y);
				add_cloud_face(Front, box_center, half_x, half_z, half_y);
			}
		}
	}

	cloud_vbo.reset_vertices(cloud_vertices.data(), sizeof(vertex) * cloud_vertices.size(), GL_STATIC_DRAW);
}

//reuses the standard cube face template (block_face.h) so cloud boxes wind exactly
//like normal opaque blocks - correctly front/back-face culled, no special-casing needed
void PlayerRenderer::add_cloud_face(block_face face, vec3 center, float half_x, float half_z, float half_y) {
	static const int order[6] = { 0, 1, 2, 2, 3, 0 };
	vec3 scale = vec3(half_x * 2.0f, half_y * 2.0f, half_z * 2.0f);
	vec3 normal = face_normal(face);
	const vector<vertex>& verts = cw_face_map.at(face);
	for (int idx : order) {
		vec3 pos = verts[idx].position * scale + center;
		cloud_vertices.push_back({ pos, vec2(0.0f), normal });
	}
}

/*
	draws the cloud mesh, regenerating it first if the player has wandered far from
	where it was last centered. wind is a pure visual drift applied in the vertex
	shader, independent of regeneration.
*/
void PlayerRenderer::draw_clouds(mat4 cam_matrix, vec2 player_xz, float time, vec3 light_color) {
	if (distance(player_xz, last_cloud_center) > cloud_regen_distance) {
		generate_cloud_mesh(player_xz);
	}

	sm.clouds_shader.activate();
	sm.clouds_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, cam_matrix);
	sm.clouds_shader.set_uniform_1f("time", time);
	sm.clouds_shader.set_uniform_1f("wind_speed", cloud_wind_speed);
	sm.clouds_shader.set_uniform_3f("light_color", 1, light_color);

	cloud_vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, cloud_vertices.size());
}

void PlayerRenderer::post_process() {
	quad_vao.bind();
	texture_color_buffer.set_slot(1);
	texture_color_buffer.bind();
	depth_texture.activate();
	depth_texture.bind();
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
}

/*
	draws outline of a hovered block
*/
void PlayerRenderer::outline_hovered_cube(Block* hovered_block) {
	if (hovered_block == nullptr) return;
	mat4 cube_model = translate(mat4(1.0), hovered_block->position);
	sm.outline_shader.set_uniform_mat4f("cube_model", 1, GL_FALSE, cube_model);
	sm.outline_shader.set_uniform_4f("color", 1, hovered_block_outline_color);
	glDisable(GL_DEPTH_TEST); //keeps the cube outline visible
	glLineWidth(outline_thickness);
	glDrawArrays(GL_LINES, 0, cube_edges.size());
	glEnable(GL_DEPTH_TEST);
}
