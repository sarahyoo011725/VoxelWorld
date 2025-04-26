#include "Camera.h"

Camera::Camera(GLFWwindow *window, vec3 position) {
	this->window = window;
	this->position = position;
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
		position -= speed * direction * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		position += speed * direction * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) { 
		position -= right * speed * dt;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		position += right * speed * dt;
	}
	process_mouse_inputs();
}

void Camera::update_view(Shader &shader) {
	mat4 view = lookAt(position, position + direction, up);
	GLuint location = glGetUniformLocation(shader.id, "view");
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

	vec3 new_direction;
	new_direction.x = cos(radians(yaw) * cos(radians(pitch)));
	new_direction.y = sin(radians(pitch));
	new_direction.z = sin(radians(yaw)) * cos(radians(pitch));
	direction = normalize(new_direction);
}