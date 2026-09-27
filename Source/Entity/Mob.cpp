#include "Mob.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

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
	in_water = water_at(position);
	flying = type.hovers;
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

bool Mob::water_at(vec3 p) const {
	Block* b = block_at(p);
	return b != nullptr && b->type == water;
}

int Mob::water_above(int limit) const {
	for (int k = 1; k <= limit; ++k) {
		if (!water_at(position + vec3(0.0f, (float)k, 0.0f))) return k - 1;
	}
	return limit;
}

bool Mob::path_is_safe(vec3 direction) const {
	bool swims = type.lives == habitat::amphibious;
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
		if (below->type == water) return swims;
		if (is_solid(below->type)) {
			Block* at_feet = block_at(ahead + vec3(0.0f, 0.5f - drop, 0.0f));
			return at_feet == nullptr || at_feet->type != water || swims;
		}
	}
	return false;
}

bool Mob::water_ahead(vec3 direction) const {
	vec3 ahead = position + direction * (size.x * 0.5f + 0.6f);
	vec3 half_height = vec3(0.0f, size.y * 0.5f, 0.0f);
	if (type.lives == habitat::amphibious) {
		//a seal swims at the surface too, so only rock is in its way
		Block* b = block_at(ahead);
		return b != nullptr && !is_solid(b->type);
	}
	return water_at(ahead) && water_at(ahead + half_height) && water_at(ahead - half_height);
}

mat4 Mob::body_matrix() const {
	mat4 m = translate(mat4(1.0f), position);
	m = rotate(m, yaw, vec3(0, 1, 0));
	m = rotate(m, -pitch, vec3(1, 0, 0));
	m = rotate(m, bank, vec3(0, 0, 1));
	return translate(m, vec3(0.0f, -size.y * 0.5f, 0.0f));
}

vec3 Mob::seat() const {
	return vec3(body_matrix() * vec4(type.seat / 16.0f, 1.0f));
}

void Mob::leap(float up, float forward) {
	velocity.y = up;
	knockback = vec3(sin(yaw), 0.0f, cos(yaw)) * forward;
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

void Mob::walk(float dt, float turn) {
	vec3 forward = vec3(sin(yaw), 0.0f, cos(yaw));
	float speed = 0.0f;
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

	//in the air, a swimmer's body follows its arc, so a leap reads as a dive back in
	if (type.lives != habitat::land && !physics.on_ground) {
		float along = length(vec2(velocity.x, velocity.z));
		pitch = approach(pitch, glm::clamp(atan2(velocity.y, glm::max(along, 0.5f)), -1.2f, 1.2f), dt * 8.0f);
	}
	else {
		pitch = approach(pitch, 0.0f, dt * 6.0f);
	}

	float stride = type.walk_speed > 0.0f ? speed / type.walk_speed : 0.0f;
	leg_swing = approach(leg_swing, glm::min(stride, 1.4f), dt * 8.0f);
}

void Mob::swim(float dt, float turn) {
	pitch = approach(pitch, target_pitch, dt * 3.0f);
	vec3 heading = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
	float speed = 0.0f;
	if (move > 0.0f) {
		if (water_ahead(heading)) {
			speed = type.swim_speed * move * (std::abs(turn) < 0.8f ? 1.0f : 0.5f);
		}
		else {
			blocked = true;
			//a bank in front needn't stop it rising or sinking, e.g. up to the waterline to climb out
			vec3 vertical = vec3(0.0f, pitch > 0.0f ? 1.0f : -1.0f, 0.0f);
			if (std::abs(pitch) > 0.2f && water_ahead(vertical)) {
				heading = vertical;
				speed = type.swim_speed * move * std::abs(sin(pitch));
			}
		}
	}
	//water resists: the body eases toward the speed it's swimming at rather than jumping to it
	vec3 wanted = heading * speed + knockback;
	velocity = mix(velocity, wanted, glm::min(1.0f, dt * 4.0f));
	physics.integrate(*this, dt, true, 0.0f);

	float stroke = type.swim_speed > 0.0f ? speed / type.swim_speed : 0.0f;
	leg_swing = approach(leg_swing, glm::min(stroke, 1.4f), dt * 6.0f);
}

void Mob::locomote(float dt) {
	in_water = water_at(position);
	if (ridden) target_yaw = control.yaw;
	float turn = wrap_angle(target_yaw - yaw);
	float max_turn = type.turn_rate * (ridden ? 3.0f : turn_boost) * dt;
	yaw = wrap_angle(yaw + glm::clamp(turn, -max_turn, max_turn));

	blocked = false;
	if (ridden) ride(dt);
	else if (flying) fly(dt, turn);
	else if (in_water && type.lives != habitat::land) swim(dt, turn);
	else walk(dt, turn);
	knockback *= exp(-6.0f * dt);
	last_position = position;

	//a swimmer's tail never quite stops
	float idle = type.lives != habitat::land && in_water ? 3.0f : 0.0f;
	walk_phase += dt * (9.0f * leg_swing + idle);

	//wings beat hard climbing, slowly gliding down; a serpent's ripple quickens with speed
	wing_spread = approach(wing_spread, flying ? 1.0f : 0.0f, dt * 3.0f);
	float beat = !flying ? 0.0f : (velocity.y < -1.5f ? 2.5f : 7.0f);
	if (type.hovers) beat = 3.0f + length(velocity) * 0.4f;
	flap_phase += dt * beat;
	//lean into turns in the air
	bank = approach(bank, flying ? glm::clamp(turn * 1.2f, -0.6f, 0.6f) : 0.0f, dt * 3.0f);
}

//unridden flight: toward wherever the goals point, or hovering, or settling to land
void Mob::fly(float dt, float turn) {
	pitch = approach(pitch, target_pitch, dt * 2.0f);
	vec3 heading = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch));
	vec3 wanted = vec3(0.0f);
	if (move > 0.0f) {
		//climb over whatever is in the way rather than stopping against it
		Block* ahead = block_at(position + heading * (size.x * 0.5f + 1.0f));
		if (ahead != nullptr && is_solid(ahead->type)) {
			blocked = true;
			heading = normalize(vec3(heading.x * 0.3f, 1.0f, heading.z * 0.3f));
		}
		wanted = heading * type.fly_speed * 0.6f * move;
	}
	else if (type.hovers) {
		wanted.y = sin(flap_phase * 0.5f) * 0.3f;
	}
	else {
		wanted.y = -2.5f;
	}
	velocity = mix(velocity, wanted + knockback, glm::min(1.0f, dt * 2.5f));
	physics.integrate(*this, dt, true, 0.0f);
	if (!type.hovers && move == 0.0f && physics.on_ground) flying = false;
	leg_swing = approach(leg_swing, 0.0f, dt * 4.0f);
}

//the rider's controls: on the ground like walking, in the air toward where they look
void Mob::ride(float dt) {
	vec3 forward = vec3(sin(yaw), 0.0f, cos(yaw));
	vec3 right = vec3(-cos(yaw), 0.0f, sin(yaw));
	if (type.flies && !flying && control.up) {
		flying = true;
		velocity.y = 6.0f;
	}
	if (flying) {
		vec3 wanted = control.look * control.forward + right * control.strafe;
		wanted.y += (control.up ? 1.0f : 0.0f) - (control.down ? 1.0f : 0.0f);
		if (length(wanted) > 0.01f) wanted = normalize(wanted) * type.fly_speed * (control.sprint ? 1.8f : 1.0f);
		velocity = mix(velocity, wanted + knockback, glm::min(1.0f, dt * 2.5f));
		physics.integrate(*this, dt, true, 0.0f);
		//touching down with descent held lands a flier that can land
		if (!type.hovers && physics.on_ground && control.down) flying = false;
		float along = length(vec2(velocity.x, velocity.z));
		pitch = approach(pitch, glm::clamp(atan2(velocity.y, glm::max(along, 1.0f)), -0.8f, 0.8f), dt * 4.0f);
		leg_swing = approach(leg_swing, 0.0f, dt * 4.0f);
		return;
	}
	float speed = type.ride_speed * (control.sprint ? 1.5f : 1.0f);
	vec3 wanted = forward * control.forward + right * control.strafe;
	if (length(wanted) > 0.01f) wanted = normalize(wanted) * speed;
	velocity.x = wanted.x + knockback.x;
	velocity.z = wanted.z + knockback.z;
	if (control.up && physics.on_ground && !type.flies) velocity.y = type.jump_speed;
	physics.integrate(*this, dt, true);
	pitch = approach(pitch, 0.0f, dt * 6.0f);
	float stride = type.walk_speed > 0.0f ? length(vec2(wanted.x, wanted.z)) / type.walk_speed : 0.0f;
	leg_swing = approach(leg_swing, glm::min(stride * 0.5f, 1.4f), dt * 8.0f);
}

//a water mob on land flops about, and past its dry_out_time loses a point of health a second
void Mob::dry_out(float dt) {
	if (type.lives != habitat::water || in_water) {
		time_out_of_water = 0.0f;
		suffocation_timer = 0.0f;
		return;
	}
	time_out_of_water += dt;
	if (dying()) return;
	if (physics.on_ground && random_between(0.0f, 1.0f) < 1.5f * dt) {
		yaw = target_yaw = random_between(-half_turn, half_turn);
		leap(4.0f, random_between(0.5f, 1.5f));
	}
	if (type.dry_out_time >= 0.0f && time_out_of_water > type.dry_out_time) {
		suffocation_timer += dt;
		if (suffocation_timer >= 1.0f) {
			suffocation_timer -= 1.0f;
			take_damage(1.0f);
		}
	}
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
		target_pitch = 0.0f;
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

//landing after a drop longer than safe_fall costs a point of health per extra block; water breaks any fall
void Mob::track_fall(float y_before) {
	bool supported = physics.on_ground || in_water || flying;
	if (!supported) {
		fall_start_y = was_on_ground ? y_before : glm::max(fall_start_y, position.y);
	}
	else if (!was_on_ground && physics.on_ground && !in_water) {
		float fell = fall_start_y - position.y;
		if (fell > safe_fall + 0.5f && !dying()) take_damage(std::floor(fell - safe_fall + 0.01f));
	}
	was_on_ground = supported;
}

void Mob::update(float dt, const MobContext& context) {
	hurt_time = glm::max(0.0f, hurt_time - dt);
	panic_time = glm::max(0.0f, panic_time - dt);

	if (dying()) {
		death_time += dt;
		flying = false;
	}
	else if (ridden) {
		if (active != nullptr) active->stop(*this);
		active = nullptr;
	}
	else run_goals(dt, context);
	float y_before = position.y;
	locomote(dt);
	track_fall(y_before);
	dry_out(dt);

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
