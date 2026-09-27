#pragma once
#include <glm/glm.hpp>

using namespace glm;

class Mob;

//what a mob can see of the world beyond the blocks around it
struct MobContext {
	vec3 player_eye = vec3(0.0f);
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
