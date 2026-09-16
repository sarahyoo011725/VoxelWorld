#include "Camera.h"

Camera::Camera(WindowSetting* setting) : sm(ShaderManager::get_instance()), window_setting(setting) {
	last_xpos = window_setting->width / 2;
	last_ypos = window_setting->height / 2;
	mouse_xpos = last_xpos;
	mouse_ypos = last_ypos;
	glfwSetInputMode(window_setting->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera::update(vec3 eye_position) {
	float frame = (float)glfwGetTime();
	float dt = frame - last_frame;
	last_frame = frame;

	update_mouse();
	update_zoom(dt);
	update_matrix(eye_position);
	update_day_night_cycle(dt);
	update_light_space_matrix(eye_position);

	sm.default_shader.activate();
	sm.default_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	sm.default_shader.set_uniform_3f("cam_pos", 1, eye_position);
	sm.default_shader.set_uniform_3f("fog_color", 1, sky_color);
	sm.default_shader.set_uniform_1f("fog_start", fog_start);
	sm.default_shader.set_uniform_1f("fog_end", fog_end);
	sm.default_shader.set_uniform_3f("sun_direction", 1, sun_direction);
	sm.default_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, light_space_matrix);
	sm.default_shader.set_uniform_3f("light_color", 1, light_color);
	sm.default_shader.set_uniform_1f("ambient_strength", ambient_strength);
	sm.default_shader.set_uniform_1f("diffuse_strength", diffuse_strength);

	sm.wave_shader.activate();
	sm.wave_shader.set_uniform_1f("time", frame);
	sm.wave_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	sm.wave_shader.set_uniform_3f("cam_pos", 1, eye_position);
	sm.wave_shader.set_uniform_3f("fog_color", 1, sky_color);
	sm.wave_shader.set_uniform_1f("fog_start", fog_start);
	sm.wave_shader.set_uniform_1f("fog_end", fog_end);
	sm.wave_shader.set_uniform_3f("sun_direction", 1, sun_direction);
	sm.wave_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, light_space_matrix);
	sm.wave_shader.set_uniform_3f("light_color", 1, light_color);
	sm.wave_shader.set_uniform_1f("ambient_strength", ambient_strength);
	sm.wave_shader.set_uniform_1f("diffuse_strength", diffuse_strength);

	sm.foliage_shader.activate();
	sm.foliage_shader.set_uniform_1f("time", frame);
	sm.foliage_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	sm.foliage_shader.set_uniform_3f("cam_pos", 1, eye_position);
	sm.foliage_shader.set_uniform_3f("fog_color", 1, sky_color);
	sm.foliage_shader.set_uniform_1f("fog_start", fog_start);
	sm.foliage_shader.set_uniform_1f("fog_end", fog_end);
	sm.foliage_shader.set_uniform_3f("sun_direction", 1, sun_direction);
	sm.foliage_shader.set_uniform_mat4f("light_space_matrix", 1, GL_FALSE, light_space_matrix);
	sm.foliage_shader.set_uniform_3f("light_color", 1, light_color);
	sm.foliage_shader.set_uniform_1f("ambient_strength", ambient_strength);
	sm.foliage_shader.set_uniform_1f("diffuse_strength", diffuse_strength);
}

void Camera::update_zoom(float dt) {
	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	}
	else {
		fov_degrees = 45.0f;
	}
}

/*
	advances the day/night clock and derives the active light (sun by day, its mirror
	position by night, standing in for the moon), its color/strength, and the sky color
*/
void Camera::update_day_night_cycle(float dt) {
	time_of_day += dt / day_length;
	if (time_of_day > 1.0f) time_of_day -= 1.0f;

	const float two_pi = 6.28318530718f;
	float angle = time_of_day * two_pi;
	//a small fixed z tilt keeps the sun from passing exactly through the up vector at
	//noon, which would make lookAt() in update_light_space_matrix degenerate
	to_sun = normalize(vec3(cos(angle), sin(angle), 0.15f));

	vec3 to_light = (to_sun.y >= 0.0f) ? to_sun : -to_sun;
	sun_direction = -to_light;

	float day_factor = clamp(to_sun.y * 2.5f, 0.0f, 1.0f);
	light_color = mix(night_light_color, day_light_color, day_factor);
	ambient_strength = mix(0.12f, 0.45f, day_factor);
	diffuse_strength = mix(0.2f, 0.55f, day_factor);
	sky_color = mix(night_sky_color, day_sky_color, day_factor);
	sky_zenith_color = mix(night_zenith_color, day_zenith_color, day_factor);
}

/*
	sizes and centers the sun's ortho projection on the player, so a fixed-resolution
	shadow map keeps reasonable detail instead of covering the whole render distance
*/
void Camera::update_light_space_matrix(vec3 eye_position) {
	mat4 light_view_at_origin = lookAt(vec3(0.0f), sun_direction, vec3(0.0f, 1.0f, 0.0f));
	float texel_size = (shadow_extent * 2.0f) / shadow_resolution;
	vec3 eye_light_space = vec3(light_view_at_origin * vec4(eye_position, 1.0f));
	eye_light_space.x = floor(eye_light_space.x / texel_size) * texel_size;
	eye_light_space.y = floor(eye_light_space.y / texel_size) * texel_size;
	vec3 snapped_center = vec3(inverse(light_view_at_origin) * vec4(eye_light_space, 1.0f));

	vec3 light_pos = snapped_center - sun_direction * 150.0f;
	mat4 light_view = lookAt(light_pos, snapped_center, vec3(0.0f, 1.0f, 0.0f));
	mat4 light_projection = ortho(-shadow_extent, shadow_extent, -shadow_extent, shadow_extent, 1.0f, 300.0f);
	light_space_matrix = light_projection * light_view;
}

void Camera::update_matrix(vec3 eye_position) {
	vec3 world_up = vec3(0.0f, 1.0f, 0.0f);
	vec3 right = normalize(cross(direction, world_up));
	vec3 up = normalize(cross(right, direction));
	view = lookAt(eye_position, eye_position + direction, up);
	projection = perspective(radians(fov_degrees), (float)window_setting->width / window_setting->height, near_plane, far_plane);
	mat = projection * view;
}

/*
	updates direction vector based on mouse pitch and yaw, converted from 2D mouse coordinates
*/
void Camera::update_mouse() {
	glfwGetCursorPos(window_setting->window, &mouse_xpos, &mouse_ypos);
	//prevents sudden view jump when window_setting->window re-focused
	if (window_refocused) {
		mouse_xpos = last_xpos;
		mouse_ypos = last_ypos;
		window_refocused = false;
	}

	float x_offset = last_xpos - mouse_xpos;
	float y_offset = mouse_ypos - last_ypos;
	last_xpos = mouse_xpos;
	last_ypos = mouse_ypos;

	x_offset *= mouse_sensitivity;
	y_offset *= mouse_sensitivity;

	yaw += x_offset;
	pitch += y_offset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	vec3 new_direction;
	new_direction.x = cos(radians(yaw)) * cos(radians(pitch));
	new_direction.y = sin(radians(pitch));
	new_direction.z = sin(radians(yaw)) * cos(radians(pitch));
	direction = normalize(new_direction);
}
