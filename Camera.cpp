#include "Camera.h"

Camera::Camera(GLFWwindow *window, int window_width, int window_height, vec3 position) {
	this->window = window;
	this->window_width = window_width;
	this->window_height = window_height;
	this->position = position;
	//sets last cursor position to be at the center of the window
	last_xpos = window_width / 2;
	last_ypos = window_height / 2;
	mouse_xpos = last_xpos;
	mouse_ypos = last_ypos;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

//updates anything required periodically
void Camera::process_inputs() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;
	
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	up = normalize(cross(right, direction));

	if (glfwGetKey(window, GLFW_KEY_W)) {
		position += speed * direction * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		position -= speed * direction * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) { 
		position -= right * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		position += right * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		position += up * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
		position -= up * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE) {	
		fov_degrees = 45.0f;
	}
	//TODO: check ground
	process_mouse_inputs();
}

void Camera::update_matrix(int shader_id ) {
	mat4 view = lookAt(position, position + direction, up);
	mat4 projection = perspective(radians(fov_degrees), (float)window_width/window_height, near_plane, far_plane);
	GLuint location = glGetUniformLocation(shader_id, "cam_matrix");
	glUniformMatrix4fv(location, 1, false, value_ptr(projection * view));
}

void Camera::process_mouse_inputs() {
	glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
	//prevents sudden view jump when window re-focued
	/*
	if (window_refocused) {
		mouse_xpos = last_xpos;
		mouse_ypos = last_ypos;
		window_refocused = false;
	}
	*/
	float x_offset = last_xpos - mouse_xpos;
	float y_offset = mouse_ypos - last_ypos;
	last_xpos = mouse_xpos;
	last_ypos = mouse_ypos;
	
	x_offset *= mouse_sensitivity;
	y_offset *= mouse_sensitivity;

	yaw += x_offset;
	pitch += y_offset;

	/*
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f) 
		pitch = -89.0f;
	*/

	vec3 new_direction;
	new_direction.x = cos(radians(yaw)) * cos(radians(pitch));
	new_direction.y = sin(radians(pitch));
	new_direction.z = sin(radians(yaw)) * cos(radians(pitch));
	direction = normalize(new_direction);
}