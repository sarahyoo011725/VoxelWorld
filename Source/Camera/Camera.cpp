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

	update_matrix(eye_position);

	sm.default_shader.activate();
	sm.default_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	sm.default_shader.set_uniform_3f("cam_pos", 1, eye_position);
	sm.default_shader.set_uniform_3f("fog_color", 1, fog_color);
	sm.default_shader.set_uniform_1f("fog_start", fog_start);
	sm.default_shader.set_uniform_1f("fog_end", fog_end);

	sm.wave_shader.activate();
	sm.wave_shader.set_uniform_1f("time", frame);
	sm.wave_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	sm.wave_shader.set_uniform_3f("cam_pos", 1, eye_position);
	sm.wave_shader.set_uniform_3f("fog_color", 1, fog_color);
	sm.wave_shader.set_uniform_1f("fog_start", fog_start);
	sm.wave_shader.set_uniform_1f("fog_end", fog_end);

	update_zoom(dt);
	update_mouse();
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
