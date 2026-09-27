#include "Goal.h"
#include "Mob.h"

namespace {
	bool roll(Mob& mob, float chance_per_second, float dt) {
		return mob.random_between(0.0f, 1.0f) < chance_per_second * dt;
	}
}

bool WanderGoal::can_start(Mob& mob, const MobContext& context, float dt) {
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
