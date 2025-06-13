#include "Camera.h"

Camera::Camera(GLFWwindow *window, int window_width, int window_height, vec3 position) : cm(ChunkManager::get_instance()) {
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

void Camera::update_matrix(int shader_id) {
	mat4 view = lookAt(position, position + direction, up);
	mat4 projection = perspective(radians(fov_degrees), (float)window_width / window_height, near_plane, far_plane);
	GLuint location = glGetUniformLocation(shader_id, "cam_matrix");
	glUniformMatrix4fv(location, 1, false, value_ptr(projection * view));
}

bool Camera::is_ground() {
	vec3 below_pos = position;
	below_pos.y -= player_size.y + 0.1f;
	Block* block = cm.get_block_worldspace(below_pos);
	return block != nullptr && is_solid(block->type);
}

//updates anything required periodically
void Camera::update() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;

	update_mouse();
	update_movement(dt);
}

void Camera::update_movement(float dt) {
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	up = normalize(cross(right, direction));
	vec3 input_dir = vec3(0.0);

	if (glfwGetKey(window, GLFW_KEY_W)) {
		input_dir += direction;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		input_dir -= direction;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		input_dir -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		input_dir += right;
	}
	
	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		input_dir += up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
		input_dir -= up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT)) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	} else {
		fov_degrees = 45.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
		is_running = !is_running;
	}
	if (glfwGetKey(window, GLFW_KEY_2)) {
		enable_physics = !enable_physics;
	}

	if (is_running) {
		speed = (enable_physics) ? run_speed : run_speed_fly;
	}
	else {
		speed = default_speed;
	}

	if (length(input_dir) > 0.0) {
		input_dir = normalize(input_dir);
	}

	velocity.x = input_dir.x * speed;
	velocity.z = input_dir.z * speed;
	if (!enable_physics) velocity.y = input_dir.y * speed;

	//note: velocity += acceleration * dt, position += velocity * dt, 
	//multiply dt to ensure player moves the same distance per second. (makes the movement frame-rate independent)

	if (enable_physics) {
		if (is_ground()) {
			velocity.y = 0.0f;

			if (glfwGetKey(window, GLFW_KEY_SPACE)) {
				velocity.y = jump_force;
			}
		}
		else {
			velocity.y += gravity * dt;
		}
	}
	position += velocity * dt;
}

void Camera::update_mouse() {
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