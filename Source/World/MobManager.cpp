#include "MobManager.h"
#include "Chunk/Chunk.h"
#include <algorithm>

MobManager::MobManager() : cm(ChunkManager::get_instance()), rng(std::random_device{}()) {}

//a mob only moves where its chunk is meshed; outside it there are no blocks to stand on
bool MobManager::chunk_ready(vec3 world_position) const {
	Chunk* chunk = cm.get_chunk(world_position);
	return chunk != nullptr && chunk->has_built;
}

//grass on top with two blocks of air above it
bool MobManager::can_stand_at(int x, int z, int& ground_y) const {
	vec3 column = vec3(x, 0, z);
	Chunk* chunk = cm.get_chunk(column);
	if (chunk == nullptr || !chunk->has_built) return false;
	ivec3 local = world_to_local_coord(column);

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

//a weighted draw among the types that spawn in this biome
const MobType* MobManager::pick_type(biome_id biome) {
	int total = 0;
	for (const MobType& type : mob_types()) {
		if (find(type.spawn.biomes.begin(), type.spawn.biomes.end(), biome) != type.spawn.biomes.end()) total += type.spawn.weight;
	}
	if (total == 0) return nullptr;
	int pick = std::uniform_int_distribution<int>(0, total - 1)(rng);
	for (const MobType& type : mob_types()) {
		if (find(type.spawn.biomes.begin(), type.spawn.biomes.end(), biome) == type.spawn.biomes.end()) continue;
		pick -= type.spawn.weight;
		if (pick < 0) return &type;
	}
	return nullptr;
}

void MobManager::try_spawn_herd(vec3 player_position) {
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	float angle = unit(rng) * 6.2831853f;
	float distance = spawn_min_distance + unit(rng) * (spawn_max_distance - spawn_min_distance);
	int cx = (int)std::round(player_position.x + cos(angle) * distance);
	int cz = (int)std::round(player_position.z + sin(angle) * distance);

	int ground_y;
	if (!can_stand_at(cx, cz, ground_y)) return;
	Chunk* chunk = cm.get_chunk(vec3(cx, 0, cz));
	ivec3 local = world_to_local_coord(vec3(cx, 0, cz));
	const MobType* type = pick_type(chunk->get_biome(local.x, local.z));
	if (type == nullptr) return;

	int count = std::uniform_int_distribution<int>(type->spawn.min_group, type->spawn.max_group)(rng);
	for (int i = 0; i < count && (int)mobs.size() < max_mobs; ++i) {
		int x = cx + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		int z = cz + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		if (!can_stand_at(x, z, ground_y)) continue;
		//the ground block spans ground_y - 0.5 to ground_y + 0.5, so feet rest on its top
		vec3 feet = vec3(x, ground_y + 0.5f + 0.01f, z);
		auto mob = make_unique<Mob>(*type, feet, rng());
		if (mob->fits()) mobs.push_back(std::move(mob));
	}
}

Mob* MobManager::pick(vec3 origin, vec3 direction, float max_distance, float& distance) const {
	Mob* nearest = nullptr;
	distance = max_distance;
	for (const auto& m : mobs) {
		if (m->dying()) continue;
		//slab test: the ray is inside the box between its latest entry and earliest exit
		vec3 low = m->position - m->size * 0.5f, high = m->position + m->size * 0.5f;
		float enter = 0.0f, exit = max_distance;
		for (int axis = 0; axis < 3 && enter <= exit; ++axis) {
			if (std::abs(direction[axis]) < 1e-6f) {
				if (origin[axis] < low[axis] || origin[axis] > high[axis]) enter = exit + 1.0f;
				continue;
			}
			float t0 = (low[axis] - origin[axis]) / direction[axis];
			float t1 = (high[axis] - origin[axis]) / direction[axis];
			enter = glm::max(enter, glm::min(t0, t1));
			exit = glm::min(exit, glm::max(t0, t1));
		}
		if (enter <= exit && enter < distance) {
			distance = enter;
			nearest = m.get();
		}
	}
	return nearest;
}

void MobManager::update(float dt, const MobContext& context) {
	vec3 player = context.player_eye;
	mobs.erase(remove_if(mobs.begin(), mobs.end(), [&](const unique_ptr<Mob>& m) {
		vec2 offset = vec2(m->position.x - player.x, m->position.z - player.z);
		return m->finished_dying() || length(offset) > despawn_distance || !chunk_ready(m->position) || m->position.y < -10.0f;
	}), mobs.end());

	for (auto& m : mobs) m->update(dt, context);

	//a few attempts a second, so herds appear gradually as the world loads in
	spawn_timer -= dt;
	if (spawn_timer <= 0.0f && (int)mobs.size() < max_mobs) {
		spawn_timer = 0.25f;
		try_spawn_herd(player);
	}
}
