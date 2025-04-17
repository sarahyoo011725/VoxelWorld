#include "Camera.h"

Camera::Camera(GLFWwindow *window, vec3 position) {
	this->window = window;
	this->position = position;
	this->target = target;
	direction = normalize(position - target);
	right = normalize(cross(up, direction));
	up = cross(direction, right);
	view = lookAt(position, target, up);
}

void Camera::update() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;
	vec3 forward(1.0, 0.0, 1.0); //TODO:

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
}

void Camera::update_view(Shader &shader, const char* uniform_name) {
	GLuint location = glGetUniformLocation(shader.id, uniform_name);
	glUniformMatrix4fv(location, 1, false, value_ptr(view));
}
