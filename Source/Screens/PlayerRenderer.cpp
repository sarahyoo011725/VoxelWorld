#include "PlayerRenderer.h"
#include <glm/gtc/matrix_transform.hpp>

PlayerRenderer::PlayerRenderer(WindowSetting* setting)
	: sm(ShaderManager::get_instance()), window_setting(setting), fbo_width(setting->width), fbo_height(setting->height),
	texture_color_buffer(setting->width, setting->height, GL_TEXTURE2, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_CLAMP_TO_EDGE, GL_NEAREST),
	depth_texture(setting->width, setting->height, GL_TEXTURE3, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, GL_CLAMP_TO_BORDER, GL_LINEAR) {

	outline_vao.bind();
	outline_vao.link_attrib(outline_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	HUD_vao.bind();
	HUD_vao.link_attrib(HUD_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	HUD_vao.link_attrib(HUD_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));

	quad_vao.bind();
	quad_vao.link_attrib(quad_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	quad_vao.link_attrib(quad_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));

	sm.frame_buffer_shader.activate();
	sm.frame_buffer_shader.set_uniform_1i("screen_texture", 1);
	sm.frame_buffer_shader.set_uniform_1i("depth_texture", 3);

	fbo.bind();
	fbo.attach_texture(GL_COLOR_ATTACHMENT0, texture_color_buffer.get_id());
	fbo.attach_texture(GL_DEPTH_ATTACHMENT, depth_texture.get_id());
	fbo.unbind();
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
}

void PlayerRenderer::unbind_fbo() {
	fbo.unbind();
}

/*
	draws 2D HUD components like crosshair
*/
void PlayerRenderer::draw_HUDs() {
	sm.HUD_shader.activate();
	sm.HUD_shader.set_uniform_1i("use_texture", GL_FALSE);
	sm.HUD_shader.set_uniform_4f("color", 1, crosshair_color);
	HUD_vao.bind();
	glLineWidth(1.0);
	glDrawArrays(GL_LINES, 0, 4);
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
