#pragma once
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Terrain.h"
#include "AudioManager.h"
#include <GLFW/glfw3.h>

class GameScreen {
private:
	WindowSetting *window_setting;
	Camera cam;
	Terrain terrain;
	Shader shader = Shader("default.vert", "default.frag");
	Texture texture = Texture("texture_atlas.png", GL_TEXTURE_2D, GL_TEXTURE1, GL_RGBA, GL_UNSIGNED_BYTE);
	bool wireframe = false;
	bool enable_music = true;
	bool gl_setting_done = false;
public:
	GameScreen(WindowSetting *setting) 
	: window_setting(setting), cam(Camera(setting, vec3(0, 60, 0))), terrain(Terrain(cam.position)) {
		texture.set_unit(shader, "texture1", 1);
	}

	void gl_settings() {
		if (gl_setting_done) return;
		glEnable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//my vertices are winded in counter wise for the front face.
		//glCullFace is set to cull back face and glFrontFace is CCW by default. So, I tell gl that front faces are winded in counter wise.
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		gl_setting_done = true;
	}

	void draw() {
		//plays music

		if (glfwGetKey(window_setting->window, GLFW_KEY_1)) {
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
		
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		shader.activate();
		texture.activate(GL_TEXTURE1);
		texture.bind();
		terrain.update(); //must called after world shader is activated
		if (window_setting->window_active) {
			cam.update();
		}
		cam.update_matrix(shader.id);
		cam.draw_outlines();
		cam.draw_HUDs();
	}

	void destroy() {
		shader.destroy();
		texture.destroy();
	}
};