#include "Player.h"

Player::Player(WindowSetting* setting, vec3 position)
	: camera(setting), window_setting(setting), block_interactor(setting) {
	size = vec3(0.6f, 1.8f, 0.6f);
	this->position = position;
}

void Player::update() {
	float frame = (float)glfwGetTime();
	float dt = frame - last_frame;
	last_frame = frame;

	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		is_running = !is_running;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_2) == GLFW_PRESS) {
		enable_physics = !enable_physics;
	}

	update_movement(dt);
	camera.update(eye_position());
	block_interactor.update(eye_position(), camera.direction);
}

/*
	handles physical movements of the player
*/
void Player::update_movement(float dt) {
	vec3 look = camera.direction;
	vec3 direction = enable_physics ? normalize(vec3(look.x, 0.0f, look.z)) : look;
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	vec3 up = enable_physics ? vec3(0.0f, 1.0f, 0.0f) : normalize(cross(right, direction));
	vec3 input_dir = vec3(0.0);

	if (glfwGetKey(window_setting->window, GLFW_KEY_W) == GLFW_PRESS) {
		input_dir += direction;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_S) == GLFW_PRESS) {
		input_dir -= direction;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_A) == GLFW_PRESS) {
		input_dir -= right;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_D) == GLFW_PRESS) {
		input_dir += right;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		input_dir += up;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		input_dir -= up;
	}

	if (length(input_dir) > 0.0) {
		input_dir = normalize(input_dir);
	}

	if (is_running) {
		speed = (enable_physics) ? run_speed : run_speed_fly;
	}
	else {
		speed = default_speed;
	}

	velocity.x = input_dir.x * speed;
	velocity.z = input_dir.z * speed;
	if (!enable_physics) velocity.y = input_dir.y * speed;

	if (enable_physics && physics.on_ground && glfwGetKey(window_setting->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		velocity.y = jump_force;
	}

	physics.integrate(*this, dt, enable_physics);
}
