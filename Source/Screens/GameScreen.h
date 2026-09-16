#pragma once
#include "Shader/ShaderManager.h"
#include "Texture/Texture.h"
#include "Entity/Player.h"
#include "World/Terrain.h"
#include "PlayerRenderer.h"
#include "Audio/AudioManager.h"
#include <GLFW/glfw3.h>

/*
* a screen where voxel engine game is run
*/
class GameScreen {
private:
	ShaderManager& sm;

	WindowSetting *window_setting;
	Player player;
	Terrain terrain;
	PlayerRenderer renderer;
	Texture texture = Texture("Resources/Textures/texture_atlas.png", GL_TEXTURE1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	bool wireframe = false;
	bool enable_music = true;
	bool gl_setting_done = false;
public:
	/*
	* initializes GL settings for the game. This must be called only once before drawing the screen
	*/
	GameScreen(WindowSetting *setting)
	: sm(ShaderManager::get_instance()), window_setting(setting), player(setting, vec3(0, 60, 0)), terrain(player.position), renderer(setting) {
		sm.default_shader.activate();
		sm.default_shader.set_uniform_1i("texture1", 1);
		sm.wave_shader.activate();
		sm.wave_shader.set_uniform_1i("texture1", 1);
		sm.foliage_shader.activate();
		sm.foliage_shader.set_uniform_1i("texture1", 1);
	}

	void gl_settings() {
		if (gl_setting_done) return;
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		/*
			my front face vertices are winded in clock wise (CW).
			glCullFace culls back face with glFrontFace being counter-clock wise (CCW) by default.
			So, I tell GL to wind front faces in CW and cull back faces winded in CCW.
		*/
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		gl_setting_done = true;
	}

	/*
	* draws game scene and updates terrain and player. this must be called every frame
	*/
	void draw() {
		if (glfwGetKey(window_setting->window, GLFW_KEY_1) == GLFW_PRESS) {
			wireframe = !wireframe;
		}
		if (glfwGetKey(window_setting->window, GLFW_KEY_4) == GLFW_PRESS) {
			enable_music = !enable_music;
			if (enable_music == false && audio::current_music != nullptr) {
				audio::current_music->stop();
			}
		}

		if (enable_music) {
			audio::play_random_music();
		}

		renderer.sync_fbo_size();

		terrain.update_chunks();

		//shadow pass: render opaque + foliage geometry depth-only from the sun's POV
		renderer.bind_shadow_fbo();
		glViewport(0, 0, renderer.shadow_resolution, renderer.shadow_resolution);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sm.shadow_shader.activate();
		sm.shadow_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, player.camera.light_space_matrix);
		sm.shadow_shader.set_uniform_1f("time", (float)glfwGetTime());
		terrain.draw_shadow_casters();
		renderer.unbind_shadow_fbo();
		glViewport(0, 0, window_setting->width, window_setting->height);

		//first render pass: mirror texture
		renderer.bind_fbo();
		vec3 sky = player.camera.sky_color;
		glClearColor((GLfloat)sky.r, (GLfloat)sky.g, (GLfloat)sky.b, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//sky gradient backdrop, drawn depth-tested-off so it always covers the whole screen
		glDisable(GL_DEPTH_TEST);
		renderer.draw_sky_background(player.camera.view, player.camera.projection, player.camera.sky_zenith_color, player.camera.sky_color, player.camera.to_sun);

		glEnable(GL_DEPTH_TEST);
		renderer.draw_sky_discs(player.camera.view, player.camera.projection, player.eye_position(), player.camera.to_sun);
		renderer.draw_clouds(player.camera.mat, vec2(player.position.x, player.position.z), (float)glfwGetTime(), player.camera.light_color);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		sm.default_shader.activate();
		texture.activate();
		texture.bind();
		terrain.draw();
		if (window_setting->window_active) {
			player.update();
			renderer.update();
			sm.frame_buffer_shader.activate();
			sm.frame_buffer_shader.set_uniform_1i("is_underwater", player.is_underwater());
		}
		renderer.draw_outlines(player.view_matrix(), player.hovered_block());
		renderer.draw_HUDs();

		//second render pass: draw as normal
		renderer.unbind_fbo();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //the composited fullscreen quad must always be solid, even in wireframe mode
		sm.frame_buffer_shader.activate();
		renderer.post_process();
	}
};
