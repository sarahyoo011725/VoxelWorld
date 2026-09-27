#pragma once
#include "Entity/Mob.h"
#include "World/ChunkManager.h"
#include <memory>
#include <random>
#include <vector>

/*
	keeps a population of animals around the player: spawns small herds on land and
	schools in water, of types that suit the ground, depth and biome, updates them,
	and drops any that wander out of range or whose chunk is unloaded
*/
class MobManager {
public:
	MobManager();
	void update(float dt, const MobContext& context);
	const vector<unique_ptr<Mob>>& all() const { return mobs; }
	//places a mob directly, outside the spawn rules and caps; null if it doesn't fit there
	Mob* add(const MobType& type, vec3 feet);
	//the nearest living mob whose hitbox the ray enters within max_distance, or null
	Mob* pick(vec3 origin, vec3 direction, float max_distance, float& distance) const;

	//kept apart, so a sea full of fish can't crowd the animals out of the fields
	int max_land_mobs = 20;
	int max_water_mobs = 16;
	float spawn_min_distance = 20.0f; //out of the player's immediate view
	float spawn_max_distance = 64.0f;
	float despawn_distance = 112.0f;

private:
	void try_spawn_herd(vec3 player_position);
	void try_spawn_school(vec3 player_position);
	const MobType* pick_type(biome_id biome, bool in_water, block_type ground, int depth);
	bool can_stand_at(int x, int z, int& ground_y, block_type& ground_type) const;
	int water_depth(int x, int z, int& floor_y) const;
	bool near_water(int x, int z, int distance) const;
	biome_id biome_at(int x, int z) const;
	ivec2 random_column(vec3 player_position);
	int count(bool water_mobs) const;
	bool chunk_ready(vec3 world_position) const;

	ChunkManager& cm;
	vector<unique_ptr<Mob>> mobs;
	std::mt19937 rng;
	float spawn_timer = 0.0f;
};
