#pragma once
#include "Shader/ShaderManager.h"
#include "Texture/Texture.h"
#include "Entity/Player.h"
#include "World/Terrain.h"
#include "World/CaveGenerator.h"
#include "PlayerRenderer.h"
#include "MobRenderer.h"
#include "World/MobManager.h"
#include "World/Commands.h"
#include "Audio/MobSoundPlayer.h"
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
	MobManager mobs;
	MobRenderer mob_renderer;
	MobSoundPlayer mob_sounds;
	bool attack_was_down = false;
	bool use_was_down = false;
	const float attack_damage = 2.0f;

	//the console: T or / opens it, Enter runs the line, Esc closes it
	bool console_open = false;
	string console_text;
	string console_message;
	double console_message_until = 0.0;
	bool open_key_was_down = false, slash_key_was_down = false, enter_key_was_down = false, backspace_key_was_down = false, escape_key_was_down = false;
	bool camera_key_was_down = false;
	unique_ptr<Mob> rider; //the player as others would see them, drawn in the saddle in third person
	double last_mob_update = 0.0;
	Texture texture = Texture("Resources/Textures/texture_atlas_blocks.png", GL_TEXTURE1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	bool wireframe = false;
	bool enable_music = true;
	bool gl_setting_done = false;
	bool debug_export_key_was_down = false;
	bool show_perf_overlay = false;
	bool perf_key_was_down = false;
	bool occlusion_key_was_down = false;
	double last_frame_time = 0.0;
	float smoothed_frame_ms = 16.7f;
	int frames_since_shadow_update = 1000;
	const int shadow_update_interval = 3;
public:
	/*
	* initializes GL settings for the game. This must be called only once before drawing the screen
	*/
	GameScreen(WindowSetting *setting)
	: sm(ShaderManager::get_instance()), window_setting(setting), player(setting, vec3(0, 122, 0)), terrain(player.position), renderer(setting) {
		sm.default_shader.activate();
		sm.default_shader.set_uniform_1i("texture1", 1);
		sm.wave_shader.activate();
		sm.wave_shader.set_uniform_1i("texture1", 1);
		sm.foliage_shader.activate();
		sm.foliage_shader.set_uniform_1i("texture1", 1);
		sm.mob_shader.activate();
		sm.mob_shader.set_uniform_1i("texture1", MobRenderer::skin_unit);
		sm.mob_shader.set_uniform_1i("part_matrices", MobRenderer::matrix_unit);
		sm.mob_shadow_shader.activate();
		sm.mob_shadow_shader.set_uniform_1i("part_matrices", MobRenderer::matrix_unit);
	}

	/*
		a mob under the crosshair takes the click instead of the block behind it: the
		block ray is cut short at the mob, a fresh left click hits it, and a right click
		climbs onto it if it can be ridden. while riding, a right click gets off
	*/
	void update_attack() {
		float distance;
		Mob* target = mobs.pick(player.eye_position(), player.camera.direction, player.reach(), distance, player.mount);
		player.limit_reach(target != nullptr ? distance : player.reach());

		bool attack_down = glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		if (target != nullptr && attack_down && !attack_was_down && target->hurt(attack_damage, player.position)) {
			audio::play_attack(target->dying());
		}
		attack_was_down = attack_down;

		bool use_down = glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		if (use_down && !use_was_down) {
			if (player.mount != nullptr) player.dismount();
			else if (target != nullptr && target->type.rideable && !target->dying()) player.ride(target);
		}
		use_was_down = use_down;

		bool camera_key_down = glfwGetKey(window_setting->window, GLFW_KEY_F5) == GLFW_PRESS;
		if (camera_key_down && !camera_key_was_down) player.third_person = !player.third_person;
		camera_key_was_down = camera_key_down;
	}

	void show_message(const string& text) {
		console_message = text;
		console_message_until = glfwGetTime() + 6.0;
	}

	void update_console() {
		GLFWwindow* w = window_setting->window;
		auto pressed = [&](int key, bool& was_down) {
			bool down = glfwGetKey(w, key) == GLFW_PRESS;
			bool edge = down && !was_down;
			was_down = down;
			return edge;
		};
		bool open_key = pressed(GLFW_KEY_T, open_key_was_down);
		bool slash_key = pressed(GLFW_KEY_SLASH, slash_key_was_down);
		bool enter_key = pressed(GLFW_KEY_ENTER, enter_key_was_down);
		bool backspace_key = pressed(GLFW_KEY_BACKSPACE, backspace_key_was_down);
		bool escape_key = pressed(GLFW_KEY_ESCAPE, escape_key_was_down);

		if (!console_open) {
			//the key that opens it arrives as a typed character too; that one is dropped here
			if (window_setting->window_active && (open_key || slash_key)) {
				console_open = true;
				console_text = slash_key ? "/" : "";
			}
			window_setting->typed_text.clear();
			return;
		}
		console_text += window_setting->typed_text;
		window_setting->typed_text.clear();
		if (backspace_key && !console_text.empty()) console_text.pop_back();
		if (escape_key) {
			console_open = false;
			return;
		}
		if (enter_key) {
			vec3 teleport = vec3(NAN);
			string reply = run_command(console_text, mobs, player.eye_position(), player.camera.direction, &teleport);
			if (!reply.empty()) show_message(reply);
			if (!isnan(teleport.x)) {
				player.dismount();
				player.position = teleport;
				player.velocity = vec3(0.0f);
			}
			console_open = false;
		}
	}

	//the rider model sits in the mount's saddle and turns its head to where the player looks
	const Mob* rider_to_draw() {
		if (player.mount == nullptr || !player.third_person) return nullptr;
		if (rider == nullptr) rider = make_unique<Mob>(rider_type(), vec3(0.0f), 0u);
		const Mob& mount = *player.mount;
		rider->position = mount.seat() + vec3(0.0f, rider->size.y * 0.5f, 0.0f);
		rider->yaw = mount.yaw;
		rider->pitch = mount.pitch;
		rider->bank = mount.bank;
		vec3 look = player.camera.direction;
		rider->head_yaw = glm::clamp(rider->wrap_angle(atan2(look.x, look.z) - mount.yaw), -1.2f, 1.2f);
		rider->head_pitch = glm::clamp(-asin(glm::clamp(look.y, -1.0f, 1.0f)) - mount.pitch, -0.9f, 0.9f);
		return rider.get();
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
		update_console();
		//typing a command mustn't also toggle things bound to the same keys
		bool keys_free = !console_open;

		if (keys_free && glfwGetKey(window_setting->window, GLFW_KEY_1) == GLFW_PRESS) {
			wireframe = !wireframe;
		}
		if (keys_free && glfwGetKey(window_setting->window, GLFW_KEY_4) == GLFW_PRESS) {
			enable_music = !enable_music;
			if (enable_music == false && audio::current_music != nullptr) {
				audio::current_music->stop();
			}
		}
		bool perf_key_down = keys_free && glfwGetKey(window_setting->window, GLFW_KEY_F3) == GLFW_PRESS;
		if (perf_key_down && !perf_key_was_down) {
			show_perf_overlay = !show_perf_overlay;
		}
		perf_key_was_down = perf_key_down;

		bool occlusion_key_down = keys_free && glfwGetKey(window_setting->window, GLFW_KEY_F4) == GLFW_PRESS;
		if (occlusion_key_down && !occlusion_key_was_down) {
			terrain.occlusion_culling = !terrain.occlusion_culling;
		}
		occlusion_key_was_down = occlusion_key_down;

		//smoothed so the readout is legible instead of flickering every frame
		double now = glfwGetTime();
		if (last_frame_time > 0.0) {
			float dt_ms = (float)((now - last_frame_time) * 1000.0);
			smoothed_frame_ms = smoothed_frame_ms * 0.9f + dt_ms * 0.1f;
		}
		last_frame_time = now;

		//edge-detected, unlike the toggles above: this writes files to disk
		bool debug_export_key_down = keys_free && glfwGetKey(window_setting->window, GLFW_KEY_9) == GLFW_PRESS;
		if (debug_export_key_down && !debug_export_key_was_down) {
			ivec3 p = ivec3(player.position);
			get_terrain_generator().export_debug_maps("terrain_debug", p.x, p.z, 512);
			get_cave_generator().export_debug_slices("cave_debug", get_terrain_generator(), p.x, p.z, 256);
		}
		debug_export_key_was_down = debug_export_key_down;

		if (enable_music) {
			audio::play_random_music();
		}

		renderer.sync_fbo_size();

		terrain.update_chunks();

		//before anything is drawn, so this frame shows this frame's input rather than the last one's
		if (window_setting->window_active && keys_free) {
			update_attack();
			player.update();
		}
		else {
			player.rest_mount();
		}
		if (window_setting->window_active) {
			audio::update_water(player.feet_in_water(), player.is_underwater(), player.velocity.y);
			sm.frame_buffer_shader.activate();
			sm.frame_buffer_shader.set_uniform_1i("is_underwater", player.is_underwater());
		}

		//capped so a stall (a window drag, a debugger) can't launch mobs through the ground
		double mob_now = glfwGetTime();
		float mob_dt = last_mob_update > 0.0 ? (float)std::min(mob_now - last_mob_update, 0.1) : 0.0f;
		last_mob_update = mob_now;
		MobContext mob_context;
		mob_context.player_eye = player.eye_position();
		mobs.update(mob_dt, mob_context);
		//the mount moved with the mobs; the player's seat and view follow it
		if (player.mount != nullptr && player.mount->dying()) player.dismount();
		player.follow_mount();
		for (const auto& m : mobs.all()) {
			if (const string* sound = m->take_sound()) mob_sounds.play(*sound, m->position, player.camera.view, 0.8f + 0.4f * (rand() / (float)RAND_MAX));
		}
		mob_renderer.build(mobs.all(), rider_to_draw());

		//shadow pass: render opaque + foliage geometry depth-only from the sun's POV.
		//refreshed on an interval, or immediately when geometry changed
		bool geometry_changed = terrain.stats.chunks_built > 0;
		if (++frames_since_shadow_update >= shadow_update_interval || geometry_changed) {
			frames_since_shadow_update = 0;
			player.camera.commit_shadow_matrix();

			//push the committed matrix to everything that samples the map, so the
			//map and the matrix reading it always describe the same light view
			mat4 shadow_matrix = player.camera.shadow_matrix;
			sm.default_shader.activate();
			sm.default_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_matrix);
			sm.wave_shader.activate();
			sm.wave_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_matrix);
			sm.foliage_shader.activate();
			sm.foliage_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_matrix);
			sm.mob_shader.activate();
			sm.mob_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_matrix);

			renderer.bind_shadow_fbo();
			glViewport(0, 0, renderer.shadow_resolution, renderer.shadow_resolution);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			sm.shadow_shader.activate();
			sm.shadow_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, shadow_matrix);
			sm.shadow_shader.set_uniform_1f("time", (float)glfwGetTime());
			terrain.draw_shadow_casters(shadow_matrix);
			mob_renderer.draw_depth(shadow_matrix);
			renderer.unbind_shadow_fbo();
			glViewport(0, 0, window_setting->width, window_setting->height);
		}

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
		terrain.draw_opaque(player.camera.mat);
		//before water, so a mob under the surface is seen through it rather than drawn over it
		mob_renderer.draw();
		terrain.draw_translucent();
		renderer.draw_outlines(player.view_matrix(), player.hovered_block(), player.hovered_position());
		renderer.draw_HUDs();
		renderer.draw_hotbar(player.inventory());
		renderer.draw_console(console_text, console_open, glfwGetTime() < console_message_until ? console_message : "");

		if (show_perf_overlay) {
			terrain.stats.frame_ms = smoothed_frame_ms;
			terrain.stats.fps = smoothed_frame_ms > 0.0f ? 1000.0f / smoothed_frame_ms : 0.0f;
			terrain.stats.player_x = (int)player.position.x;
			terrain.stats.player_y = (int)player.position.y;
			terrain.stats.player_z = (int)player.position.z;
			//answers "what biome am I in, and what climate produced it"
			ClimateSample climate = get_terrain_generator().sample_climate(
				terrain.stats.player_x, terrain.stats.player_z);
			terrain.stats.biome_name = biome_of(select_biome(climate, water_level)).name;
			terrain.stats.temperature = climate.temperature;
			terrain.stats.moisture = climate.moisture;
			renderer.draw_perf_overlay(terrain.stats);
		}

		//second render pass: draw as normal
		renderer.unbind_fbo();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		sm.frame_buffer_shader.activate();
		renderer.post_process();
	}
};
