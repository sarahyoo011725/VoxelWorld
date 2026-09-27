#pragma once
#include "GameObject.h"
#include "Goal.h"
#include "MobType.h"
#include "PlayerPhysics.h"
#include <memory>
#include <random>
#include <vector>

/*
	any animal: the type says what it looks like and how it behaves, goals decide
	where it wants to go, and this class turns that into movement, physics and
	animation. position is the centre of its hitbox, like the player.
*/
class Mob : public GameObject {
public:
	Mob(const MobType& type, vec3 feet_position, uint32_t seed);
	void update(float dt, const MobContext& context);

	vec3 feet() const { return position - vec3(0.0f, size.y * 0.5f, 0.0f); }
	//the hitbox can be wider than a block column, so a clear column alone isn't enough to spawn in
	bool fits() { return physics.is_position_clear(*this); }
	//no drop of more than two blocks and no water one step along this direction
	bool path_is_safe(vec3 direction) const;
	float random_between(float lo, float hi);
	float wrap_angle(float a) const;

	const MobType& type;

	//steering, set by goals
	float target_yaw = 0.0f;
	float move = 0.0f; //0 stand, 1 walk at full speed
	float target_head_yaw = 0.0f; //relative to the body
	float target_head_pitch = 0.0f; //positive looks down
	bool blocked = false; //the last step was refused: unsafe ground or a wall

	//pose, read by the renderer
	float yaw = 0.0f; //radians; 0 faces +z
	float head_yaw = 0.0f;
	float head_pitch = 0.0f;
	float walk_phase = 0.0f;
	float leg_swing = 0.0f; //0 standing still, 1 walking at full speed

	//sound requests, collected by the manager each frame
	int ambient_sound = -1;

private:
	void run_goals(float dt, const MobContext& context);
	void locomote(float dt);

	PlayerPhysics physics;
	std::mt19937 rng;
	vector<unique_ptr<Goal>> goals; //sorted by priority
	Goal* active = nullptr;
	vec3 last_position = vec3(0.0f);
	float stuck_time = 0.0f;
	float ambient_timer = 0.0f;
};
