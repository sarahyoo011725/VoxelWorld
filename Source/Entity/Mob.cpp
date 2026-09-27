#include "Mob.h"
#include <algorithm>

namespace {
	const float half_turn = 3.14159265f; //pi radians

	//the block that contains a point: blocks are centred on integer coordinates
	Block* block_at(vec3 p) {
		return ChunkManager::get_instance().get_block_worldspace(vec3(std::round(p.x), std::round(p.y), std::round(p.z)));
	}

	float approach(float value, float target, float rate) {
		return value + (target - value) * glm::min(1.0f, rate);
	}
}

Mob::Mob(const MobType& type, vec3 feet_position, uint32_t seed) : type(type), rng(seed) {
	size = type.hitbox;
	position = feet_position + vec3(0.0f, size.y * 0.5f, 0.0f);
	yaw = target_yaw = random_between(-half_turn, half_turn);
	last_position = position;
	ambient_timer = random_between(0.3f, 1.7f) * type.sounds.ambient_interval;
	for (const auto& make : type.goals) goals.push_back(make());
	stable_sort(goals.begin(), goals.end(), [](const unique_ptr<Goal>& a, const unique_ptr<Goal>& b) { return a->priority < b->priority; });
}

float Mob::random_between(float lo, float hi) {
	return std::uniform_real_distribution<float>(lo, hi)(rng);
}

float Mob::wrap_angle(float a) const {
	while (a > half_turn) a -= 2.0f * half_turn;
	while (a < -half_turn) a += 2.0f * half_turn;
	return a;
}

bool Mob::path_is_safe(vec3 direction) const {
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

void Mob::run_goals(float dt, const MobContext& context) {
	if (active != nullptr && !active->keep_going(*this, context)) {
		active->stop(*this);
		active = nullptr;
	}
	for (auto& goal : goals) {
		if (active != nullptr && goal->priority >= active->priority) break;
		if (goal->can_start(*this, context, dt)) {
			if (active != nullptr) active->stop(*this);
			active = goal.get();
			active->start(*this, context);
			break;
		}
	}
	if (active != nullptr) active->tick(*this, context, dt);
}

void Mob::locomote(float dt) {
	float turn = wrap_angle(target_yaw - yaw);
	float max_turn = type.turn_rate * dt;
	yaw = wrap_angle(yaw + glm::clamp(turn, -max_turn, max_turn));

	vec3 forward = vec3(sin(yaw), 0.0f, cos(yaw));
	float speed = 0.0f;
	blocked = false;
	if (move > 0.0f) {
		if (path_is_safe(forward)) {
			//slow down while still swinging round, so turns read as turns
			speed = type.walk_speed * move * (std::abs(turn) < 0.6f ? 1.0f : 0.35f);
		}
		else {
			blocked = true;
		}
	}
	velocity.x = forward.x * speed;
	velocity.z = forward.z * speed;
	physics.integrate(*this, dt, true);

	//walked into something the step-up could not climb
	if (speed > 0.0f && length(vec2(position.x - last_position.x, position.z - last_position.z)) < speed * dt * 0.2f) {
		stuck_time += dt;
		if (stuck_time > 0.5f) {
			blocked = true;
			stuck_time = 0.0f;
		}
	}
	else {
		stuck_time = 0.0f;
	}
	last_position = position;

	leg_swing = approach(leg_swing, speed > 0.0f ? speed / type.walk_speed : 0.0f, dt * 8.0f);
	walk_phase += dt * 9.0f * leg_swing;
}

void Mob::update(float dt, const MobContext& context) {
	run_goals(dt, context);
	locomote(dt);

	head_yaw = approach(head_yaw, target_head_yaw, dt * 6.0f);
	head_pitch = approach(head_pitch, target_head_pitch, dt * 4.0f);

	ambient_sound = -1;
	if (!type.sounds.ambient.empty()) {
		ambient_timer -= dt;
		if (ambient_timer <= 0.0f) {
			ambient_sound = (int)(random_between(0.0f, 1.0f) * type.sounds.ambient.size()) % (int)type.sounds.ambient.size();
			ambient_timer = random_between(0.5f, 1.5f) * type.sounds.ambient_interval;
		}
	}
}
