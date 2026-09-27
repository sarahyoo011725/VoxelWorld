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

	//riding, the camera and block interaction wait for follow_mount, once the mount has moved
	if (mount != nullptr) {
		steer_mount();
		return;
	}
	update_movement(dt);
	place_camera();
	block_interactor.update(eye_position(), camera.direction);
}

void Player::next_view() {
	view_mode = view_mode == view::first_person ? view::behind : view_mode == view::behind ? view::in_front : view::first_person;
}

void Player::place_camera() {
	view_eye = eye_position();
	camera.look_back = view_mode == view::in_front;
	if (view_mode != view::first_person) {
		//back along the look (or out in front of the face), pulled in rather than through a block
		vec3 away = view_mode == view::behind ? -camera.direction : camera.direction;
		float wanted = (mount != nullptr ? mount->size.x * 2.0f : 0.0f) + 4.0f;
		float allowed = 0.0f;
		ChunkManager& cm = ChunkManager::get_instance();
		for (float d = 0.25f; d <= wanted; d += 0.25f) {
			Block* b = cm.get_block_worldspace(round(view_eye + away * d + vec3(0.0f, d * 0.15f, 0.0f)));
			if (b != nullptr && is_solid(b->type)) break;
			allowed = d;
		}
		float d = glm::max(allowed - 0.3f, 0.0f);
		view_eye = view_eye + away * d + vec3(0.0f, d * 0.15f, 0.0f);
	}
	camera.update(view_eye);
}

void Player::steer_mount() {
	GLFWwindow* w = window_setting->window;
	auto held = [&](int key) { return glfwGetKey(w, key) == GLFW_PRESS; };
	RideInput input;
	input.forward = (held(GLFW_KEY_W) ? 1.0f : 0.0f) - (held(GLFW_KEY_S) ? 1.0f : 0.0f);
	input.strafe = (held(GLFW_KEY_D) ? 1.0f : 0.0f) - (held(GLFW_KEY_A) ? 1.0f : 0.0f);
	input.up = held(GLFW_KEY_SPACE);
	input.down = held(GLFW_KEY_LEFT_SHIFT);
	input.sprint = held(GLFW_KEY_LEFT_CONTROL);
	input.look = camera.direction;
	input.yaw = atan2(camera.direction.x, camera.direction.z);
	mount->steer(input);
}

void Player::rest_mount() {
	if (mount == nullptr) return;
	RideInput input;
	input.yaw = mount->yaw;
	input.look = vec3(sin(mount->yaw), 0.0f, cos(mount->yaw));
	mount->steer(input);
}

void Player::ride(Mob* mob) {
	mount = mob;
	mob->ridden = true;
	velocity = vec3(0.0f);
	block_interactor.consume_clicks();
}

//off to the mount's side, level with its feet
void Player::dismount() {
	if (mount == nullptr) return;
	vec3 side = vec3(-cos(mount->yaw), 0.0f, sin(mount->yaw));
	position = mount->position + side * (mount->size.x * 0.5f + size.x * 0.5f + 0.2f);
	position.y = mount->feet().y + size.y * 0.5f + 0.05f;
	velocity = vec3(0.0f);
	mount->ridden = false;
	mount = nullptr;
	block_interactor.consume_clicks();
}

void Player::follow_mount() {
	if (mount == nullptr) return;
	//sitting: hips on the saddle, so the centre is a little above it
	position = mount->seat() + vec3(0.0f, 0.25f, 0.0f);
	velocity = mount->velocity;
	place_camera();
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
