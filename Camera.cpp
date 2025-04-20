#include "Camera.h"

Camera::Camera(GLFWwindow *window, vec3 position) {
	this->window = window;
	this->position = position;
	this->target = target;
	direction = normalize(position - target);
	right = normalize(cross(up, direction));
	up = cross(direction, right);
	view = lookAt(position, target, up);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

//updates anything required periodically
void Camera::process_inputs() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;
	
	if (glfwGetKey(window, GLFW_KEY_W)) {
		position -= dt * speed * forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		position += dt * speed * forward;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) { 
		position -= normalize(cross(up, forward)) * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		position += normalize(cross(up, forward)) * speed * dt;
	}

	direction = normalize(position - target);
	right = normalize(cross(vec3(0.0, 1.0, 0.0), direction));
	up = cross(direction, right);
	view = lookAt(position, position + forward, up);
	process_mouse_inputs();
}

void Camera::update_view(Shader &shader, const char* uniform_name) {
	GLuint location = glGetUniformLocation(shader.id, uniform_name);
	glUniformMatrix4fv(location, 1, false, value_ptr(view));
}

void Camera::process_mouse_inputs() {
	glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
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

	vec3 direction;
	direction.x = cos(radians(yaw) * cos(radians(pitch)));
	direction.y = sin(radians(pitch));
	direction.z = sin(radians(yaw)) * cos(radians(pitch));
	forward = normalize(direction);
}