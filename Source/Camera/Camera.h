#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "WindowSetting.h"
#include "Shader/ShaderManager.h"

using namespace glm;

namespace {
	static bool window_refocused = false;
}

/*
* view/projection and mouse-look only - movement, physics, and interaction live elsewhere
*/
class Camera
{
public:
	Camera(WindowSetting* setting);
	mat4 mat = mat4(0.0f);
	vec3 direction = vec3(0.0f, 0.0f, -1.0f);
	mat4 light_space_matrix = mat4(1.0f); //recomputed each frame, centered on the player, for the shadow pass
	void update(vec3 eye_position);
private:
	void update_mouse();
	void update_zoom(float dt);
	void update_matrix(vec3 eye_position);
	void update_light_space_matrix(vec3 eye_position);

	WindowSetting* window_setting;
	ShaderManager& sm;

	const float mouse_sensitivity = 0.1f;
	const float near_plane = 0.1f;
	const float far_plane = 180.0f;
	const vec3 fog_color = vec3(135.0f / 255, 206.0f / 255, 235.0f / 255);
	const float fog_start = 90.0f;
	const float fog_end = 160.0f;

	//fixed directional sun, no day/night cycle - points down and to a side
	const vec3 sun_direction = normalize(vec3(-0.4f, -1.0f, -0.3f));
	const float shadow_extent = 60.0f; //half-width of the shadow-covered area around the player

	double mouse_xpos, mouse_ypos;
	double last_xpos, last_ypos;
	float last_frame = 0.0f;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov_degrees = 45.0f;
	mat4 view = mat4(0.0f);
	mat4 projection = mat4(0.0f);
};
