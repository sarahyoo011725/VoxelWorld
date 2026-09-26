#pragma once
#include "Entity/Sheep.h"
#include "World/ChunkManager.h"
#include <memory>
#include <random>
#include <vector>

/*
	keeps a population of sheep around the player: spawns small herds on open
	grass in grassy biomes, updates them, and drops any that wander out of
	range or whose chunk is unloaded
*/
class MobManager {
public:
	MobManager();
	void update(float dt, vec3 player_position);
	const vector<unique_ptr<Sheep>>& sheep() const { return herd; }

	int max_sheep = 16;
	float spawn_min_distance = 20.0f; //out of the player's immediate view
	float spawn_max_distance = 64.0f;
	float despawn_distance = 112.0f;

private:
	void try_spawn_herd(vec3 player_position);
	bool can_stand_at(int x, int z, int& ground_y) const;
	bool chunk_ready(vec3 world_position) const;

	ChunkManager& cm;
	vector<unique_ptr<Sheep>> herd;
	std::mt19937 rng;
	float spawn_timer = 0.0f;
};
