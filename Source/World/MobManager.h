#pragma once
#include "Entity/Mob.h"
#include "World/ChunkManager.h"
#include <memory>
#include <random>
#include <vector>

/*
	keeps a population of animals around the player: spawns small herds of a type
	that suits the ground and biome, updates them, and drops any that wander out
	of range or whose chunk is unloaded
*/
class MobManager {
public:
	MobManager();
	void update(float dt, const MobContext& context);
	const vector<unique_ptr<Mob>>& all() const { return mobs; }
	//the nearest living mob whose hitbox the ray enters within max_distance, or null
	Mob* pick(vec3 origin, vec3 direction, float max_distance, float& distance) const;

	int max_mobs = 20;
	float spawn_min_distance = 20.0f; //out of the player's immediate view
	float spawn_max_distance = 64.0f;
	float despawn_distance = 112.0f;

private:
	void try_spawn_herd(vec3 player_position);
	const MobType* pick_type(biome_id biome);
	bool can_stand_at(int x, int z, int& ground_y) const;
	bool chunk_ready(vec3 world_position) const;

	ChunkManager& cm;
	vector<unique_ptr<Mob>> mobs;
	std::mt19937 rng;
	float spawn_timer = 0.0f;
};
