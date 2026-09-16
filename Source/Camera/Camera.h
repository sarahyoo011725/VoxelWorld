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
* view/projection, mouse-look, and the day/night lighting cycle - movement, physics,
* and interaction live elsewhere
*/
class Camera
{
public:
	Camera(WindowSetting* setting);
	mat4 mat = mat4(0.0f);
	mat4 view = mat4(0.0f);
	mat4 projection = mat4(0.0f);
	vec3 direction = vec3(0.0f, 0.0f, -1.0f);
	mat4 light_space_matrix = mat4(1.0f); //recomputed each frame, centered on the player, for the shadow pass

	//day/night cycle state, exposed so the sun/moon discs can be drawn in the sky
	float time_of_day = 0.2f; //0 = sunrise, 0.25 = noon, 0.5 = sunset, 0.75 = midnight
	vec3 to_sun = vec3(0.0f, 1.0f, 0.0f); //unit direction from the player toward the sun
	vec3 sky_color = vec3(0.0f); //horizon-level sky/fog color
	vec3 sky_zenith_color = vec3(0.0f); //color straight overhead, for the sky gradient
	vec3 light_color = vec3(1.0f);
	float ambient_strength = 0.45f;
	float diffuse_strength = 0.55f;

	void update(vec3 eye_position);
private:
	void update_mouse();
	void update_zoom(float dt);
	void update_matrix(vec3 eye_position);
	void update_light_space_matrix(vec3 eye_position);
	void update_day_night_cycle(float dt);

	WindowSetting* window_setting;
	ShaderManager& sm;

	const float mouse_sensitivity = 0.1f;
	const float near_plane = 0.1f;
	const float far_plane = 180.0f;
	const float fog_start = 90.0f;
	const float fog_end = 160.0f;

	const float day_length = 120.0f; //seconds for a full day/night cycle
	const vec3 day_sky_color = vec3(135.0f / 255, 206.0f / 255, 235.0f / 255);
	const vec3 night_sky_color = vec3(0.03f, 0.03f, 0.1f);
	const vec3 day_zenith_color = vec3(0.25f, 0.45f, 0.85f);
	const vec3 night_zenith_color = vec3(0.0f, 0.0f, 0.02f);
	const vec3 day_light_color = vec3(1.0f, 0.97f, 0.9f);
	const vec3 night_light_color = vec3(0.45f, 0.55f, 0.75f);

	vec3 sun_direction = vec3(0.0f, -1.0f, 0.0f); //direction light currently travels (sun by day, moon by night)
	const float shadow_extent = 60.0f; //half-width of the shadow-covered area around the player
	const float shadow_resolution = 2048.0f; //must match PlayerRenderer::shadow_resolution - used to snap the shadow frustum to whole texels

	double mouse_xpos, mouse_ypos;
	double last_xpos, last_ypos;
	float last_frame = 0.0f;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov_degrees = 45.0f;
};
