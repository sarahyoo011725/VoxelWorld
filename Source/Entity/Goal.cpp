#include "Goal.h"
#include "Mob.h"

namespace {
	bool roll(Mob& mob, float chance_per_second, float dt) {
		return mob.random_between(0.0f, 1.0f) < chance_per_second * dt;
	}
}

bool PanicGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	return mob.panic_time > 0.0f;
}

bool PanicGoal::keep_going(Mob& mob, const MobContext& context) {
	return mob.panic_time > 0.0f;
}

void PanicGoal::flee(Mob& mob) {
	vec3 away = mob.position - mob.threat;
	float heading = atan2(away.x, away.z);
	mob.target_yaw = mob.wrap_angle(heading + mob.random_between(-0.35f, 0.35f));
	time_left = mob.random_between(0.8f, 1.6f);
}

void PanicGoal::start(Mob& mob, const MobContext& context) {
	mob.move = speed;
	mob.turn_boost = 3.0f;
	mob.target_head_yaw = mob.target_head_pitch = 0.0f;
	flee(mob);
}

void PanicGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	if (mob.blocked) {
		mob.target_yaw = mob.wrap_angle(mob.yaw + mob.random_between(1.6f, 4.7f));
		time_left = mob.random_between(0.8f, 1.6f);
	}
	else if (time_left <= 0.0f) {
		flee(mob);
	}
}

void PanicGoal::stop(Mob& mob) {
	mob.move = 0.0f;
	mob.turn_boost = 1.0f;
}

bool WanderGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	//in the water, a swimmer's own goals take over
	if (mob.in_water && mob.type.lives != habitat::land) return false;
	return roll(mob, chance, dt);
}

void WanderGoal::start(Mob& mob, const MobContext& context) {
	mob.target_yaw = mob.wrap_angle(mob.yaw + mob.random_between(-2.0f, 2.0f));
	mob.move = 1.0f;
	time_left = mob.random_between(2.0f, 5.0f);
}

void WanderGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	if (mob.blocked) mob.target_yaw = mob.wrap_angle(mob.yaw + mob.random_between(1.9f, 4.4f));
}

void WanderGoal::stop(Mob& mob) {
	mob.move = 0.0f;
}

bool GrazeGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	if (!roll(mob, chance, dt)) return false;
	vec3 below = mob.feet() - vec3(0.0f, 0.5f, 0.0f);
	Block* ground = ChunkManager::get_instance().get_block_worldspace(round(below));
	return ground != nullptr && ground->type == dirt_grass;
}

void GrazeGoal::start(Mob& mob, const MobContext& context) {
	mob.target_head_pitch = 0.8f;
	time_left = mob.random_between(2.0f, 6.0f);
}

void GrazeGoal::stop(Mob& mob) {
	mob.target_head_pitch = 0.0f;
}

bool SwimGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	return mob.in_water && roll(mob, chance, dt);
}

bool SwimGoal::keep_going(Mob& mob, const MobContext& context) {
	return time_left > 0.0f && mob.in_water;
}

void SwimGoal::pick_heading(Mob& mob) {
	mob.target_yaw = mob.wrap_angle(mob.yaw + mob.random_between(-2.5f, 2.5f));
	Block* below = ChunkManager::get_instance().get_block_worldspace(round(mob.position - vec3(0.0f, mob.size.y * 0.5f + 1.0f, 0.0f)));
	bool near_bottom = below != nullptr && is_solid(below->type);
	if (mob.water_above(1) == 0) mob.target_pitch = mob.random_between(-0.5f, -0.1f);
	else if (near_bottom) mob.target_pitch = mob.random_between(0.1f, 0.4f);
	else mob.target_pitch = mob.random_between(-0.35f, 0.35f);
}

void SwimGoal::start(Mob& mob, const MobContext& context) {
	pick_heading(mob);
	mob.move = 1.0f;
	time_left = mob.random_between(2.0f, 5.0f);
}

void SwimGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	if (mob.blocked) {
		pick_heading(mob);
		mob.target_yaw = mob.wrap_angle(mob.yaw + mob.random_between(1.6f, 4.7f));
	}
}

void SwimGoal::stop(Mob& mob) {
	mob.move = 0.0f;
	mob.target_pitch = 0.0f;
}

bool SchoolGoal::school_centre(const Mob& mob, const MobContext& context, vec3& centre) const {
	if (context.mobs == nullptr) return false;
	vec3 sum = vec3(0.0f);
	int count = 0;
	for (const auto& other : *context.mobs) {
		if (other.get() == &mob || &other->type != &mob.type || other->dying()) continue;
		if (distance(other->position, mob.position) > range) continue;
		sum += other->position;
		count++;
	}
	if (count == 0) return false;
	centre = sum / (float)count;
	return distance(centre, mob.position) > 2.5f;
}

bool SchoolGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	vec3 centre;
	return mob.in_water && roll(mob, chance, dt) && school_centre(mob, context, centre);
}

void SchoolGoal::start(Mob& mob, const MobContext& context) {
	mob.move = 1.0f;
	time_left = mob.random_between(1.0f, 2.0f);
}

void SchoolGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	vec3 centre;
	if (!mob.in_water || !school_centre(mob, context, centre)) {
		time_left = 0.0f;
		return;
	}
	vec3 to = centre - mob.position;
	mob.target_yaw = atan2(to.x, to.z);
	mob.target_pitch = glm::clamp(atan2(to.y, length(vec2(to.x, to.z))), -0.5f, 0.5f);
}

void SchoolGoal::stop(Mob& mob) {
	mob.move = 0.0f;
	mob.target_pitch = 0.0f;
}

bool BreachGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	return mob.in_water && mob.water_above(5) <= 3 && roll(mob, chance, dt);
}

bool BreachGoal::keep_going(Mob& mob, const MobContext& context) {
	//done once it has been out and splashed back down, or given up
	return time_left > 0.0f && !(left_water && mob.in_water);
}

void BreachGoal::start(Mob& mob, const MobContext& context) {
	leapt = left_water = false;
	mob.target_pitch = 0.7f;
	mob.move = 1.6f;
	time_left = 4.0f;
}

void BreachGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	if (!leapt && mob.in_water && mob.water_above(1) == 0) {
		mob.leap(8.5f, 3.0f);
		leapt = true;
	}
	if (leapt && !mob.in_water) left_water = true;
}

void BreachGoal::stop(Mob& mob) {
	mob.move = 0.0f;
	mob.target_pitch = 0.0f;
}

//dry ground level with the water's surface, beside open water
bool HaulOutGoal::find_shore(const Mob& mob, vec3& found) const {
	ChunkManager& cm = ChunkManager::get_instance();
	auto type_at = [&](int x, int y, int z) {
		Block* b = cm.get_block_worldspace(vec3(x, y, z));
		return b != nullptr ? b->type : bedrock;
	};
	ivec3 at = ivec3(round(mob.position));
	float best = 1e9f;
	for (int dx = -10; dx <= 10; ++dx) {
		for (int dz = -10; dz <= 10; ++dz) {
			int x = at.x + dx, z = at.z + dz;
			int y = water_level;
			if (!is_solid(type_at(x, y, z)) || type_at(x, y + 1, z) != none || type_at(x, y + 2, z) != none) continue;
			bool by_water = type_at(x + 1, y, z) == water || type_at(x - 1, y, z) == water || type_at(x, y, z + 1) == water || type_at(x, y, z - 1) == water;
			if (!by_water) continue;
			float d = (float)(dx * dx + dz * dz);
			if (d < best) {
				best = d;
				found = vec3(x, y + 1, z);
			}
		}
	}
	return best < 1e9f;
}

bool HaulOutGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	return mob.in_water && roll(mob, chance, dt) && find_shore(mob, shore);
}

bool HaulOutGoal::keep_going(Mob& mob, const MobContext& context) {
	return time_left > 0.0f && !(leapt && !mob.in_water);
}

void HaulOutGoal::start(Mob& mob, const MobContext& context) {
	leapt = false;
	mob.move = 1.0f;
	time_left = 12.0f;
}

void HaulOutGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	vec3 to = shore - mob.position;
	mob.target_yaw = atan2(to.x, to.z);
	//up to the surface first, so it arrives at the waterline rather than under the bank
	mob.target_pitch = mob.water_above(1) > 0 ? 0.6f : 0.0f;
	if (!leapt && mob.water_above(1) == 0 && length(vec2(to.x, to.z)) < 1.8f) {
		mob.leap(7.5f, 2.5f);
		leapt = true;
	}
}

void HaulOutGoal::stop(Mob& mob) {
	mob.move = 0.0f;
	mob.target_pitch = 0.0f;
}

bool LookAtPlayerGoal::can_start(Mob& mob, const MobContext& context, float dt) {
	return distance(mob.position, context.player_eye) < range && roll(mob, chance, dt);
}

bool LookAtPlayerGoal::keep_going(Mob& mob, const MobContext& context) {
	return time_left > 0.0f && distance(mob.position, context.player_eye) < range;
}

void LookAtPlayerGoal::start(Mob& mob, const MobContext& context) {
	time_left = mob.random_between(2.0f, 5.0f);
}

void LookAtPlayerGoal::tick(Mob& mob, const MobContext& context, float dt) {
	Goal::tick(mob, context, dt);
	vec3 to_player = context.player_eye - (mob.position + vec3(0.0f, mob.size.y * 0.3f, 0.0f));
	float heading = atan2(to_player.x, to_player.z);
	float relative = mob.wrap_angle(heading - mob.yaw);
	//past what a neck turns, turn the body too
	if (std::abs(relative) > 1.2f) mob.target_yaw = heading;
	mob.target_head_yaw = glm::clamp(relative, -1.2f, 1.2f);
	mob.target_head_pitch = glm::clamp(-atan2(to_player.y, length(vec2(to_player.x, to_player.z))), -0.6f, 0.6f);
}

void LookAtPlayerGoal::stop(Mob& mob) {
	mob.target_head_yaw = 0.0f;
	mob.target_head_pitch = 0.0f;
}
