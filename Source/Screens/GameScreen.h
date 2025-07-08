#pragma once
#include "Shader/ShaderManager.h"
#include "Texture/Texture.h"
#include "Camera/Camera.h"
#include "World/Terrain.h"
#include "Audio/AudioManager.h"
#include <GLFW/glfw3.h>

/*
* a screen where voxel engine game is run
*/
class GameScreen {
private:
	ShaderManager& sm;

	WindowSetting *window_setting;
	Camera cam;
	Terrain terrain;
	Texture texture = Texture("Resources/Textures/texture_atlas.png", GL_TEXTURE1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	bool wireframe = false;
	bool enable_music = true;
	bool gl_setting_done = false;
public:
	GameScreen(WindowSetting *setting) 
	: window_setting(setting), cam(Camera(setting, vec3(0, 60, 0))), terrain(Terrain(cam.position)), sm(ShaderManager::get_instance()) {
		sm.default_shader.set_uniform_1i("texture1", 1);
		sm.wave_shader.set_uniform_1i("texture1", 1);
	}

	/*
	* initializes GL settings for the game. This must be called only once before drawing the screen
	*/
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
	* draws game scene and updates terrain and camera. this must be called every frame
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

		//first render pass: mirror texture
		cam.bind_fbo();
		glClearColor((GLfloat)135/255, (GLfloat)206/255, (GLfloat)235/255, 1.0); //add sky color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		sm.default_shader.activate();
		texture.activate();
		texture.bind();
		terrain.update(); 
		if (window_setting->window_active) {
			cam.update();
		}
		cam.draw_outlines();
		cam.draw_HUDs();
		
		//second render pass: draw as normal
		cam.unbind_fbo();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		sm.frame_buffer_shader.activate();
		cam.post_process();
	}
};