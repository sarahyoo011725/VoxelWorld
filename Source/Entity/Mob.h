#pragma once
#include "GameObject.h"
#include "Goal.h"
#include "MobType.h"
#include "PlayerPhysics.h"
#include <memory>
#include <random>
#include <string>
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
	//walking: no wall, no drop of more than two blocks and (unless amphibious) no water one step along this direction
	bool path_is_safe(vec3 direction) const;
	//swimming: open water, head to tail, one body length along this direction
	bool water_ahead(vec3 direction) const;
	bool water_at(vec3 p) const;
	//whole blocks of water stacked above the one holding the centre, up to `limit`
	int water_above(int limit) const;
	bool on_ground() const { return physics.on_ground; }
	//a water mob lying on dry land
	bool stranded() const { return type.lives == habitat::water && !in_water && physics.on_ground; }
	//a jump: straight up at `up`, plus a push along the heading that fades like knockback
	void leap(float up, float forward);
	float random_between(float lo, float hi);
	float wrap_angle(float a) const;
	//a hit from something at `source`: knocks the mob away from it and sends it fleeing.
	//false while it is still recovering from the last hit, or already dying
	bool hurt(float amount, vec3 source);
	bool dying() const { return health <= 0.0f; }
	bool finished_dying() const { return death_time >= death_duration; }

	const MobType& type;

	//steering, set by goals
	float target_yaw = 0.0f;
	float target_pitch = 0.0f; //while swimming; positive heads up
	float move = 0.0f; //0 stand, 1 walk or swim at full speed
	float turn_boost = 1.0f; //multiplies the type's turn rate, e.g. to spin round and flee
	float target_head_yaw = 0.0f; //relative to the body
	float target_head_pitch = 0.0f; //positive looks down
	bool blocked = false; //the last step was refused: unsafe ground, a wall, or the edge of the water

	bool in_water = false; //the centre of the hitbox is in water, as of the last update

	//pose, read by the renderer
	float yaw = 0.0f; //radians; 0 faces +z
	float pitch = 0.0f; //positive nose up
	float head_yaw = 0.0f;
	float head_pitch = 0.0f;
	float walk_phase = 0.0f; //drives legs, tails and flippers
	float leg_swing = 0.0f; //0 still, 1 moving at full speed

	float health = 0.0f;
	float hurt_time = 0.0f; //counts down after a hit: the red flash, and no further damage until it ends
	float death_time = 0.0f; //seconds since health ran out
	float panic_time = 0.0f; //counts down after a hit; while it runs the mob flees from `threat`
	vec3 threat = vec3(0.0f);

	static constexpr float invulnerable_time = 0.5f;
	static constexpr float death_duration = 1.0f;
	static constexpr float panic_duration = 5.0f;
	static constexpr float safe_fall = 3.0f; //blocks a mob can drop without harm

	//the sound the mob wants played since the last call, or null
	const string* take_sound() { const string* s = sound; sound = nullptr; return s; }

private:
	void run_goals(float dt, const MobContext& context);
	void locomote(float dt);
	void walk(float dt, float turn);
	void swim(float dt, float turn);
	void dry_out(float dt);
	void take_damage(float amount);
	void track_fall(float y_before);
	const string* pick_sound(const vector<string>& files);

	PlayerPhysics physics;
	std::mt19937 rng;
	vector<unique_ptr<Goal>> goals; //sorted by priority
	Goal* active = nullptr;
	vec3 last_position = vec3(0.0f);
	float stuck_time = 0.0f;
	float ambient_timer = 0.0f;
	vec3 knockback = vec3(0.0f); //horizontal push from the last hit or leap, fading out
	bool was_on_ground = true;
	const string* sound = nullptr;
	float fall_start_y = 0.0f;
	float time_out_of_water = 0.0f;
	float suffocation_timer = 0.0f;
};
