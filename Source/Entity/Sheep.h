#pragma once
#include "GameObject.h"
#include "PlayerPhysics.h"
#include <random>

/*
	a wandering sheep. it stands, grazes or walks for a few seconds at a time,
	turning smoothly onto a new heading, and turns away instead of walking off
	a drop or into water. position is the centre of its hitbox, like the player.
*/
class Sheep : public GameObject {
public:
	Sheep(vec3 feet_position, uint32_t seed);
	void update(float dt);

	float yaw = 0.0f; //radians; 0 faces +z
	float walk_phase = 0.0f; //drives the leg swing
	float leg_swing = 0.0f; //0 standing still, 1 walking at full speed
	float head_pitch = 0.0f; //radians the head is lowered, for grazing

	vec3 feet() const { return position - vec3(0.0f, size.y * 0.5f, 0.0f); }
	//the hitbox is wider than a block column, so a clear column alone isn't enough to spawn in
	bool fits() { return physics.is_position_clear(*this); }

private:
	enum class activity { standing, grazing, walking };

	void choose_activity();
	bool path_is_safe(vec3 direction) const;
	float random_between(float lo, float hi);

	PlayerPhysics physics;
	std::mt19937 rng;
	activity current = activity::standing;
	float time_left = 0.0f;
	float target_yaw = 0.0f;
	vec3 last_position = vec3(0.0f);
	float stuck_time = 0.0f;

	const float walk_speed = 1.1f; //blocks per second
	const float turn_rate = 2.5f; //radians per second
};
