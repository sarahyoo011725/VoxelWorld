#include "MobManager.h"
#include "Chunk/Chunk.h"
#include <algorithm>

namespace {
	bool grassy(biome_id biome) {
		switch (biome) {
		case biome_id::plains:
		case biome_id::savanna:
		case biome_id::forest:
		case biome_id::autumn_forest:
		case biome_id::taiga:
			return true;
		default:
			return false;
		}
	}
}

MobManager::MobManager() : cm(ChunkManager::get_instance()), rng(std::random_device{}()) {}

//a sheep only moves where its chunk is meshed; outside it there are no blocks to stand on
bool MobManager::chunk_ready(vec3 world_position) const {
	Chunk* chunk = cm.get_chunk(world_position);
	return chunk != nullptr && chunk->has_built;
}

//grass on top with two blocks of air above it, in a biome where sheep graze
bool MobManager::can_stand_at(int x, int z, int& ground_y) const {
	vec3 column = vec3(x, 0, z);
	Chunk* chunk = cm.get_chunk(column);
	if (chunk == nullptr || !chunk->has_built) return false;
	ivec3 local = world_to_local_coord(column);
	if (!grassy(chunk->get_biome(local.x, local.z))) return false;

	int h = chunk->get_height(local.x, local.z);
	Block* ground = chunk->get_block(ivec3(local.x, h, local.z));
	Block* body = chunk->get_block(ivec3(local.x, h + 1, local.z));
	Block* head = chunk->get_block(ivec3(local.x, h + 2, local.z));
	if (ground == nullptr || ground->type != dirt_grass) return false;
	if (body == nullptr || is_solid(body->type) || body->type == water) return false;
	if (head == nullptr || is_solid(head->type)) return false;
	ground_y = h;
	return true;
}

void MobManager::try_spawn_herd(vec3 player_position) {
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	float angle = unit(rng) * 6.2831853f;
	float distance = spawn_min_distance + unit(rng) * (spawn_max_distance - spawn_min_distance);
	int cx = (int)std::round(player_position.x + cos(angle) * distance);
	int cz = (int)std::round(player_position.z + sin(angle) * distance);

	int ground_y;
	if (!can_stand_at(cx, cz, ground_y)) return;

	int count = 2 + (int)(unit(rng) * 3.0f);
	for (int i = 0; i < count && (int)herd.size() < max_sheep; ++i) {
		int x = cx + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		int z = cz + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		if (!can_stand_at(x, z, ground_y)) continue;
		//the ground block spans ground_y - 0.5 to ground_y + 0.5, so feet rest on its top
		vec3 feet = vec3(x, ground_y + 0.5f + 0.01f, z);
		auto sheep = make_unique<Sheep>(feet, rng());
		if (sheep->fits()) herd.push_back(std::move(sheep));
	}
}

void MobManager::update(float dt, vec3 player_position) {
	herd.erase(remove_if(herd.begin(), herd.end(), [&](const unique_ptr<Sheep>& s) {
		vec2 offset = vec2(s->position.x - player_position.x, s->position.z - player_position.z);
		return length(offset) > despawn_distance || !chunk_ready(s->position) || s->position.y < -10.0f;
	}), herd.end());

	for (auto& s : herd) s->update(dt);

	//a few attempts a second, so herds appear gradually as the world loads in
	spawn_timer -= dt;
	if (spawn_timer <= 0.0f && (int)herd.size() < max_sheep) {
		spawn_timer = 0.25f;
		try_spawn_herd(player_position);
	}
}
