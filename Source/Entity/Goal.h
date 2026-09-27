#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

using namespace glm;

class Mob;

//what a mob can see of the world beyond the blocks around it
struct MobContext {
	vec3 player_eye = vec3(0.0f);
	const std::vector<std::unique_ptr<Mob>>* mobs = nullptr; //everything alive, for flocking
};

/*
	one reusable behaviour. a mob runs at most one goal at a time: each frame the
	first goal, by priority, that wants to start takes over, but only from a
	running goal with a larger priority number. ambient goals share a priority so
	they finish before another begins; an urgent goal (fleeing, say) gets a
	smaller number and cuts in
*/
class Goal {
public:
	explicit Goal(int priority) : priority(priority) {}
	virtual ~Goal() = default;

	virtual bool can_start(Mob& mob, const MobContext& context, float dt) = 0;
	virtual bool keep_going(Mob& mob, const MobContext& context) { return time_left > 0.0f; }
	virtual void start(Mob& mob, const MobContext& context) {}
	virtual void tick(Mob& mob, const MobContext& context, float dt) { time_left -= dt; }
	virtual void stop(Mob& mob) {}

	const int priority;

protected:
	float time_left = 0.0f;
};

//after being hurt, runs from whatever hit it, weaving a little and turning off walls and edges
class PanicGoal : public Goal {
public:
	PanicGoal(int priority, float speed = 1.8f) : Goal(priority), speed(speed) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	void flee(Mob& mob);
	float speed;
};

//walks a few blocks on a random heading, turning away from drops, water and walls
class WanderGoal : public Goal {
public:
	WanderGoal(int priority, float chance_per_second = 0.35f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float chance;
};

//lowers the head to the grass it stands on for a few seconds
class GrazeGoal : public Goal {
public:
	GrazeGoal(int priority, float chance_per_second = 0.12f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	void start(Mob& mob, const MobContext& context) override;
	void stop(Mob& mob) override;

private:
	float chance;
};

//swims a few blocks on a random heading, keeping off the surface and the bottom
class SwimGoal : public Goal {
public:
	SwimGoal(int priority, float chance_per_second = 0.6f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	void pick_heading(Mob& mob);
	float chance;
};

//heads back toward the middle of nearby mobs of the same kind when it strays from them
class SchoolGoal : public Goal {
public:
	SchoolGoal(int priority, float range = 8.0f, float chance_per_second = 0.8f) : Goal(priority), range(range), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	//false when it's alone, or already close enough to the others
	bool school_centre(const Mob& mob, const MobContext& context, vec3& centre) const;
	float range, chance;
};

//near the surface, swims up hard and leaps clear of the water
class BreachGoal : public Goal {
public:
	BreachGoal(int priority, float chance_per_second = 0.3f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float chance;
	bool leapt = false;
	bool left_water = false;
};

//swims to the nearest shore at the waterline and heaves itself out onto it
class HaulOutGoal : public Goal {
public:
	HaulOutGoal(int priority, float chance_per_second = 0.25f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	bool find_shore(const Mob& mob, vec3& shore) const;
	float chance;
	vec3 shore = vec3(0.0f);
	bool leapt = false;
};

//a pet heading back to the player once it's strayed too far, flying if it can
class FollowOwnerGoal : public Goal {
public:
	FollowOwnerGoal(int priority, float start_distance = 10.0f, float stop_distance = 4.0f)
		: Goal(priority), start_distance(start_distance), stop_distance(stop_distance) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float start_distance, stop_distance;
};

//a mob with a home heading back once it's strayed too far from it, flying if it can
class ReturnHomeGoal : public Goal {
public:
	ReturnHomeGoal(int priority, float leash = 14.0f) : Goal(priority), leash(leash) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float leash;
};

//a hovering flier drifting about in the air, keeping a few blocks off the ground
class DriftGoal : public Goal {
public:
	DriftGoal(int priority, float chance_per_second = 0.5f) : Goal(priority), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float chance;
};

//turns the head to follow a nearby player
class LookAtPlayerGoal : public Goal {
public:
	LookAtPlayerGoal(int priority, float range = 8.0f, float chance_per_second = 0.3f) : Goal(priority), range(range), chance(chance_per_second) {}
	bool can_start(Mob& mob, const MobContext& context, float dt) override;
	bool keep_going(Mob& mob, const MobContext& context) override;
	void start(Mob& mob, const MobContext& context) override;
	void tick(Mob& mob, const MobContext& context, float dt) override;
	void stop(Mob& mob) override;

private:
	float range, chance;
};
