#include "Sheep.h"

namespace {
	const float half_turn = 3.14159265f; //pi radians

	//the block that contains a point: blocks are centred on integer coordinates
	Block* block_at(vec3 p) {
		return ChunkManager::get_instance().get_block_worldspace(vec3(std::round(p.x), std::round(p.y), std::round(p.z)));
	}

	float wrap_angle(float a) {
		while (a > half_turn) a -= 2.0f * half_turn;
		while (a < -half_turn) a += 2.0f * half_turn;
		return a;
	}
}

Sheep::Sheep(vec3 feet_position, uint32_t seed) : rng(seed) {
	size = vec3(0.9f, 1.3f, 0.9f);
	position = feet_position + vec3(0.0f, size.y * 0.5f, 0.0f);
	yaw = target_yaw = random_between(-half_turn, half_turn);
	last_position = position;
	choose_activity();
}

float Sheep::random_between(float lo, float hi) {
	return std::uniform_real_distribution<float>(lo, hi)(rng);
}

void Sheep::choose_activity() {
	float roll = random_between(0.0f, 1.0f);
	if (roll < 0.45f) {
		current = activity::walking;
		target_yaw = wrap_angle(yaw + random_between(-2.0f, 2.0f));
		time_left = random_between(2.0f, 5.0f);
	}
	else if (roll < 0.75f) {
		current = activity::grazing;
		time_left = random_between(2.0f, 6.0f);
	}
	else {
		current = activity::standing;
		time_left = random_between(1.0f, 4.0f);
	}
}

/*
	looks one step ahead: the ground there must be no more than two blocks down,
	and neither that ground nor the space at the sheep's feet may be water
*/
bool Sheep::path_is_safe(vec3 direction) const {
	vec3 ahead = feet() + direction * 0.9f;
	for (int drop = 0; drop <= 2; ++drop) {
		Block* below = block_at(ahead - vec3(0.0f, 0.5f + drop, 0.0f));
		if (below == nullptr) return false;
		if (below->type == water) return false;
		if (is_solid(below->type)) {
			Block* at_feet = block_at(ahead + vec3(0.0f, 0.5f - drop, 0.0f));
			return at_feet == nullptr || at_feet->type != water;
		}
	}
	return false;
}

void Sheep::update(float dt) {
	time_left -= dt;
	if (time_left <= 0.0f) choose_activity();

	float turn = wrap_angle(target_yaw - yaw);
	float max_turn = turn_rate * dt;
	yaw = wrap_angle(yaw + glm::clamp(turn, -max_turn, max_turn));

	vec3 forward = vec3(sin(yaw), 0.0f, cos(yaw));
	float speed = 0.0f;
	if (current == activity::walking) {
		if (path_is_safe(forward)) {
			//slow down while still swinging round, so turns read as turns
			speed = walk_speed * (std::abs(turn) < 0.6f ? 1.0f : 0.35f);
		}
		else {
			target_yaw = wrap_angle(yaw + random_between(half_turn * 0.6f, half_turn * 1.4f));
		}
	}
	velocity.x = forward.x * speed;
	velocity.z = forward.z * speed;
	physics.integrate(*this, dt, true);

	//walked into something the step-up could not climb: pick another way
	if (speed > 0.0f && length(vec2(position.x - last_position.x, position.z - last_position.z)) < speed * dt * 0.2f) {
		stuck_time += dt;
		if (stuck_time > 0.5f) {
			target_yaw = wrap_angle(yaw + random_between(half_turn * 0.5f, half_turn * 1.5f));
			stuck_time = 0.0f;
		}
	}
	else {
		stuck_time = 0.0f;
	}
	last_position = position;

	float swing_target = speed > 0.0f ? speed / walk_speed : 0.0f;
	leg_swing += (swing_target - leg_swing) * glm::min(1.0f, dt * 8.0f);
	walk_phase += dt * 9.0f * leg_swing;
	float pitch_target = current == activity::grazing ? 0.7f : 0.0f;
	head_pitch += (pitch_target - head_pitch) * glm::min(1.0f, dt * 4.0f);
}
