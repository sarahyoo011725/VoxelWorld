#pragma once
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/EBO.h"
#include "Texture/Texture.h"
#include "Shader/ShaderManager.h"
#include "Entity/Geometries.h"
#include "Audio/AudioManager.h"
#include "WindowSetting.h"
#include <vector>
#include <map>
#include <array>

using namespace std;

namespace {
	//a tiny hand-authored 5x7 dot-matrix font, just the letters needed to spell
	//START/EXIT - there's no real text-rendering system in this engine, so button
	//labels are drawn as a grid of small quads instead of sampled glyphs
	const static map<char, array<int, 7>> menu_font_glyphs = {
		{'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}},
		{'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
		{'A', {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
		{'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}},
		{'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}},
		{'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}},
		{'I', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}},
	};
}

/*
* a screen displaying the start menu, with mouse-clickable start/exit buttons
*/
class StartScreen {
public:
	enum class Action { None, Start, Exit };

	/*
	* calls initial setups for the screen, often links buffers attributes
	*/
	StartScreen() : sm(ShaderManager::get_instance()) {
		vao.bind();
		vao.link_attrib(vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
		vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));
		music::moog_city2.play();
	};

	/*
	* draws the start menu background. this must be called every frame
	*/
	void draw() {
		if (music::moog_city2.is_playing()) {
			music::moog_city2.update_buffer_stream();
		}
		glDisable(GL_DEPTH_TEST);
		sm.HUD_shader.activate();
		//the hotbar (PlayerRenderer, constructed after this screen) also uses
		//HUD_shader and sets its own texture1 unit - reset it every frame instead
		//of only once in the constructor, or whichever set it last wins
		sm.HUD_shader.set_uniform_1i("texture1", 0);
		vao.bind();

		texture.activate();
		texture.bind();
		sm.HUD_shader.set_uniform_1i("use_texture", GL_TRUE);
		set_identity_transform();
		glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
	};

	/*
	* draws the start/exit buttons and hit-tests the mouse against them - the
	* caller is expected to keep the cursor visible (not GLFW_CURSOR_DISABLED)
	* while this screen is up, or clicking them isn't practical
	*/
	Action poll_buttons(WindowSetting* window_setting) {
		double mx, my;
		glfwGetCursorPos(window_setting->window, &mx, &my);
		vec2 mouse_ndc = vec2(
			(float)(mx / window_setting->width) * 2.0f - 1.0f,
			1.0f - (float)(my / window_setting->height) * 2.0f
		);
		bool clicked = glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		float aspect = (float)window_setting->height / (float)window_setting->width;

		vec2 button_half = vec2(button_half_size.x * aspect, button_half_size.y);
		bool hover_start = in_bounds(mouse_ndc, start_button_center, button_half);
		bool hover_exit = in_bounds(mouse_ndc, exit_button_center, button_half);

		sm.HUD_shader.activate();
		sm.HUD_shader.set_uniform_1i("use_texture", GL_FALSE);
		vao.bind();

		draw_button(start_button_center, button_half, aspect, hover_start);
		draw_text("START", start_button_center, aspect, hover_start ? text_hover_color : text_color);
		draw_button(exit_button_center, button_half, aspect, hover_exit);
		draw_text("EXIT", exit_button_center, aspect, hover_exit ? text_hover_color : text_color);

		if (clicked && hover_start) return Action::Start;
		if (clicked && hover_exit) return Action::Exit;
		return Action::None;
	}

private:
	ShaderManager& sm;

	VAO vao = VAO();
	VBO vbo = VBO(quad_vertices.data(), sizeof(vertex_2d) * quad_vertices.size(), GL_STATIC_DRAW);
	Texture texture = Texture("Resources/Textures/menu_background.png", GL_TEXTURE0, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	const vec2 start_button_center = vec2(0.0f, -0.2f);
	const vec2 exit_button_center = vec2(0.0f, -0.45f);
	const vec2 button_half_size = vec2(0.22f, 0.08f);
	const float button_border_thickness = 0.012f;

	//Minecraft-style flat grey button: dark border, mid-grey fill, lighter on hover
	const vec4 button_border_color = vec4(0.15f, 0.15f, 0.15f, 1.0f);
	const vec4 button_fill_color = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	const vec4 button_hover_fill_color = vec4(0.65f, 0.65f, 0.65f, 1.0f);
	const vec4 text_color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	const vec4 text_hover_color = vec4(1.0f, 1.0f, 0.5f, 1.0f); //Minecraft-style yellow hover text

	const float text_pixel_size = 0.014f;

	void set_identity_transform() {
		sm.HUD_shader.set_uniform_2f("offset", 1, vec2(0.0f));
		sm.HUD_shader.set_uniform_2f("scale", 1, vec2(1.0f));
		sm.HUD_shader.set_uniform_2f("uv_offset", 1, vec2(0.0f));
		sm.HUD_shader.set_uniform_2f("uv_scale", 1, vec2(1.0f));
	}

	bool in_bounds(vec2 p, vec2 center, vec2 half_size) {
		return abs(p.x - center.x) <= half_size.x && abs(p.y - center.y) <= half_size.y;
	}

	void draw_quad(vec2 center, vec2 half_size, vec4 color) {
		sm.HUD_shader.set_uniform_4f("color", 1, color);
		sm.HUD_shader.set_uniform_2f("offset", 1, center);
		sm.HUD_shader.set_uniform_2f("scale", 1, half_size);
		sm.HUD_shader.set_uniform_2f("uv_offset", 1, vec2(0.0f));
		sm.HUD_shader.set_uniform_2f("uv_scale", 1, vec2(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
	}

	void draw_button(vec2 center, vec2 half_size, float aspect, bool hovered) {
		draw_quad(center, half_size, button_border_color);
		vec2 inner_half = vec2(half_size.x - button_border_thickness * aspect, half_size.y - button_border_thickness);
		draw_quad(center, inner_half, hovered ? button_hover_fill_color : button_fill_color);
	}

	//draws each "on" bit of the 5x7 glyphs as its own small quad, centered as a block
	void draw_text(const string& text, vec2 center, float aspect, vec4 color) {
		float px = text_pixel_size;
		float glyph_w = 5.0f * px;
		float glyph_gap = px;
		float total_w = text.size() * glyph_w + (text.size() > 0 ? (text.size() - 1) * glyph_gap : 0.0f);
		float start_x = center.x - total_w / 2.0f * aspect;
		float start_y = center.y + 3.5f * px;

		sm.HUD_shader.set_uniform_4f("color", 1, color);

		for (size_t li = 0; li < text.size(); ++li) {
			auto it = menu_font_glyphs.find(text[li]);
			if (it == menu_font_glyphs.end()) continue;
			float letter_x = start_x + li * (glyph_w + glyph_gap) * aspect;

			for (int row = 0; row < 7; ++row) {
				for (int col = 0; col < 5; ++col) {
					if (!(it->second[row] & (1 << (4 - col)))) continue;
					vec2 pixel_center = vec2(
						letter_x + (col + 0.5f) * px * aspect,
						start_y - (row + 0.5f) * px
					);
					sm.HUD_shader.set_uniform_2f("offset", 1, pixel_center);
					sm.HUD_shader.set_uniform_2f("scale", 1, vec2(px * 0.5f * aspect, px * 0.5f));
					glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
				}
			}
		}
	}
};
