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
	health = type.max_health;
	fall_start_y = position.y;
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
	auto solid_at = [&](float up) {
		Block* b = block_at(ahead + vec3(0.0f, up, 0.0f));
		return b != nullptr && is_solid(b->type);
	};
	//something at foot level is only fine as a one-block step with room above it
	if (solid_at(0.5f)) {
		int headroom = (int)std::ceil(size.y);
		for (int h = 1; h <= headroom; ++h) {
			if (solid_at(0.5f + h)) return false;
		}
		return true;
	}
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
	float max_turn = type.turn_rate * turn_boost * dt;
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
	velocity.x = forward.x * speed + knockback.x;
	velocity.z = forward.z * speed + knockback.z;
	physics.integrate(*this, dt, true);
	knockback *= exp(-6.0f * dt);

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

	leg_swing = approach(leg_swing, speed > 0.0f ? glm::min(speed / type.walk_speed, 1.4f) : 0.0f, dt * 8.0f);
	walk_phase += dt * 9.0f * leg_swing;
}

const string* Mob::pick_sound(const vector<string>& files) {
	if (files.empty()) return nullptr;
	return &files[std::uniform_int_distribution<size_t>(0, files.size() - 1)(rng)];
}

void Mob::take_damage(float amount) {
	health -= amount;
	hurt_time = invulnerable_time;
	sound = pick_sound(dying() ? type.sounds.death : type.sounds.hurt);
	if (dying()) {
		if (active != nullptr) active->stop(*this);
		active = nullptr;
		move = 0.0f;
		target_head_yaw = target_head_pitch = 0.0f;
	}
}

bool Mob::hurt(float amount, vec3 source) {
	if (dying() || hurt_time > 0.0f) return false;
	vec3 away = vec3(position.x - source.x, 0.0f, position.z - source.z);
	away = length(away) > 0.001f ? normalize(away) : vec3(sin(yaw), 0.0f, cos(yaw));
	knockback = away * 5.0f;
	if (physics.on_ground) velocity.y = 5.5f;
	threat = source;
	panic_time = panic_duration;
	take_damage(amount);
	return true;
}

//landing after a drop longer than safe_fall costs a point of health per extra block
void Mob::track_fall(float y_before) {
	bool on_ground = physics.on_ground;
	if (!on_ground) {
		fall_start_y = was_on_ground ? y_before : glm::max(fall_start_y, position.y);
	}
	else if (!was_on_ground) {
		float fell = fall_start_y - position.y;
		if (fell > safe_fall + 0.5f && !dying()) take_damage(std::floor(fell - safe_fall + 0.01f));
	}
	was_on_ground = on_ground;
}

void Mob::update(float dt, const MobContext& context) {
	hurt_time = glm::max(0.0f, hurt_time - dt);
	panic_time = glm::max(0.0f, panic_time - dt);

	if (dying()) death_time += dt;
	else run_goals(dt, context);
	float y_before = position.y;
	locomote(dt);
	track_fall(y_before);

	head_yaw = approach(head_yaw, target_head_yaw, dt * 6.0f);
	head_pitch = approach(head_pitch, target_head_pitch, dt * 4.0f);

	if (!dying() && sound == nullptr && !type.sounds.ambient.empty()) {
		ambient_timer -= dt;
		if (ambient_timer <= 0.0f) {
			sound = pick_sound(type.sounds.ambient);
			ambient_timer = random_between(0.5f, 1.5f) * type.sounds.ambient_interval;
		}
	}
}
