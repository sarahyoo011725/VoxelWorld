#include "MobManager.h"
#include "Chunk/Chunk.h"
#include "World/Onsen.h"
#include <algorithm>

namespace {
	bool is_water_mob(const MobType& type) {
		return type.lives == habitat::water;
	}

	template <class T>
	bool contains(const vector<T>& list, T value) {
		return find(list.begin(), list.end(), value) != list.end();
	}
}

MobManager::MobManager() : cm(ChunkManager::get_instance()), rng(std::random_device{}()) {}

//a mob only moves where its chunk is meshed; outside it there are no blocks to stand on
bool MobManager::chunk_ready(vec3 world_position) const {
	Chunk* chunk = cm.get_chunk(world_position);
	return chunk != nullptr && chunk->has_built;
}

//dry ground on top with two blocks of air above it
bool MobManager::can_stand_at(int x, int z, int& ground_y, block_type& ground_type) const {
	vec3 column = vec3(x, 0, z);
	Chunk* chunk = cm.get_chunk(column);
	if (chunk == nullptr || !chunk->has_built) return false;
	ivec3 local = world_to_local_coord(column);

	int h = chunk->get_height(local.x, local.z);
	//the height map is the sea floor under a floe; the ice on the surface is what's stood on
	if (h < water_level) {
		Block* surface = chunk->get_block(ivec3(local.x, water_level, local.z));
		if (surface != nullptr && is_solid(surface->type)) h = water_level;
	}
	Block* ground = chunk->get_block(ivec3(local.x, h, local.z));
	Block* body = chunk->get_block(ivec3(local.x, h + 1, local.z));
	Block* head = chunk->get_block(ivec3(local.x, h + 2, local.z));
	if (ground == nullptr || !is_solid(ground->type)) return false;
	if (body == nullptr || is_solid(body->type) || body->type == water) return false;
	if (head == nullptr || is_solid(head->type)) return false;
	ground_y = h;
	ground_type = ground->type;
	return true;
}

//open water over this column, from the floor up to the surface
int MobManager::water_depth(int x, int z, int& floor_y) const {
	vec3 column = vec3(x, 0, z);
	Chunk* chunk = cm.get_chunk(column);
	if (chunk == nullptr || !chunk->has_built) return 0;
	ivec3 local = world_to_local_coord(column);
	floor_y = chunk->get_height(local.x, local.z);
	int depth = 0;
	for (int y = floor_y + 1; y <= water_level; ++y) {
		Block* b = chunk->get_block(ivec3(local.x, y, local.z));
		if (b == nullptr || b->type != water) break;
		depth++;
	}
	return depth;
}

/*
	a weighted draw among the types that could spawn here: in this biome, and
	either standing on this ground or swimming in water this deep
*/
const MobType* MobManager::pick_type(biome_id biome, bool in_water, block_type ground, int depth) {
	auto fits_here = [&](const MobType& type) {
		if (!contains(type.spawn.biomes, biome) || is_water_mob(type) != in_water) return false;
		return in_water ? depth >= type.spawn.min_depth : contains(type.spawn.ground, ground);
	};
	int total = 0;
	for (const MobType& type : mob_types()) {
		if (fits_here(type)) total += type.spawn.weight;
	}
	if (total == 0) return nullptr;
	int pick = std::uniform_int_distribution<int>(0, total - 1)(rng);
	for (const MobType& type : mob_types()) {
		if (!fits_here(type)) continue;
		pick -= type.spawn.weight;
		if (pick < 0) return &type;
	}
	return nullptr;
}

bool MobManager::near_water(int x, int z, int distance) const {
	for (int dx = -distance; dx <= distance; ++dx) {
		for (int dz = -distance; dz <= distance; ++dz) {
			Block* b = cm.get_block_worldspace(vec3(x + dx, water_level, z + dz));
			if (b != nullptr && b->type == water) return true;
		}
	}
	return false;
}

biome_id MobManager::biome_at(int x, int z) const {
	vec3 column = vec3(x, 0, z);
	ivec3 local = world_to_local_coord(column);
	return cm.get_chunk(column)->get_biome(local.x, local.z);
}

ivec2 MobManager::random_column(vec3 player_position) {
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	float angle = unit(rng) * 6.2831853f;
	float distance = spawn_min_distance + unit(rng) * (spawn_max_distance - spawn_min_distance);
	return ivec2((int)std::round(player_position.x + cos(angle) * distance), (int)std::round(player_position.z + sin(angle) * distance));
}

int MobManager::count(bool water_mobs) const {
	//pets and residents aren't part of the wild population the caps keep in check
	return (int)count_if(mobs.begin(), mobs.end(), [&](const unique_ptr<Mob>& m) { return !m->owned && !m->has_home && is_water_mob(m->type) == water_mobs; });
}

void MobManager::try_spawn_herd(vec3 player_position) {
	ivec2 centre = random_column(player_position);
	int ground_y;
	block_type ground;
	if (!can_stand_at(centre.x, centre.y, ground_y, ground)) return;
	const MobType* type = pick_type(biome_at(centre.x, centre.y), false, ground, 0);
	if (type == nullptr) return;
	if (type->spawn.shore_distance >= 0 && !near_water(centre.x, centre.y, type->spawn.shore_distance)) return;

	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	int group = std::uniform_int_distribution<int>(type->spawn.min_group, type->spawn.max_group)(rng);
	for (int i = 0, land = count(false); i < group && land < max_land_mobs; ++i) {
		int x = centre.x + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		int z = centre.y + (int)std::round((unit(rng) - 0.5f) * 6.0f);
		if (!can_stand_at(x, z, ground_y, ground) || !contains(type->spawn.ground, ground)) continue;
		//the ground block spans ground_y - 0.5 to ground_y + 0.5, so feet rest on its top
		vec3 feet = vec3(x, ground_y + 0.5f + 0.01f, z);
		auto mob = make_unique<Mob>(*type, feet, rng());
		if (mob->fits()) {
			mobs.push_back(std::move(mob));
			land++;
		}
	}
}

//Haku lives at every onsen: one hovering over the pool of each spring near the player
void MobManager::settle_residents(vec3 player_position) {
	const MobType* haku = find_mob_type("eastern_dragon");
	if (haku == nullptr) return;
	for (const OnsenSite& site : onsen::sites_near(player_position, 80.0f)) {
		vec3 home = vec3(site.centre) + vec3(0.0f, 4.0f, 0.0f);
		if (!chunk_ready(home)) continue;
		bool living_there = any_of(mobs.begin(), mobs.end(), [&](const unique_ptr<Mob>& m) {
			return m->has_home && distance(m->home, home) < 0.5f;
		});
		if (living_there) continue;
		auto mob = make_unique<Mob>(*haku, home - vec3(0.0f, haku->hitbox.y * 0.5f, 0.0f), rng());
		mob->has_home = true;
		mob->home = home;
		mobs.push_back(std::move(mob));
	}
}

void MobManager::try_spawn_school(vec3 player_position) {
	ivec2 centre = random_column(player_position);
	int floor_y;
	int depth = water_depth(centre.x, centre.y, floor_y);
	if (depth < 2) return;
	const MobType* type = pick_type(biome_at(centre.x, centre.y), true, none, depth);
	if (type == nullptr) return;

	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	int group = std::uniform_int_distribution<int>(type->spawn.min_group, type->spawn.max_group)(rng);
	for (int i = 0, swimming = count(true); i < group && swimming < max_water_mobs; ++i) {
		int x = centre.x + (int)std::round((unit(rng) - 0.5f) * 5.0f);
		int z = centre.y + (int)std::round((unit(rng) - 0.5f) * 5.0f);
		if (water_depth(x, z, floor_y) < type->spawn.min_depth) continue;
		//anywhere in the column with the whole body under the surface
		float lowest = floor_y + 0.5f + 0.05f;
		float highest = water_level + 0.5f - type->hitbox.y - 0.05f;
		if (highest < lowest) continue;
		vec3 feet = vec3(x, lowest + unit(rng) * (highest - lowest), z);
		auto mob = make_unique<Mob>(*type, feet, rng());
		if (mob->fits() && mob->in_water) {
			mobs.push_back(std::move(mob));
			swimming++;
		}
	}
}

Mob* MobManager::add(const MobType& type, vec3 feet) {
	auto mob = make_unique<Mob>(type, feet, rng());
	if (!mob->fits()) return nullptr;
	mobs.push_back(std::move(mob));
	return mobs.back().get();
}

Mob* MobManager::pick(vec3 origin, vec3 direction, float max_distance, float& distance, const Mob* ignore) const {
	Mob* nearest = nullptr;
	distance = max_distance;
	for (const auto& m : mobs) {
		if (m->dying() || m.get() == ignore) continue;
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
		if (m->finished_dying()) return true;
		//a pet, or anything being ridden, stays with the player however far they go
		if (m->owned || m->ridden) return false;
		return length(offset) > despawn_distance || !chunk_ready(m->position) || m->position.y < -10.0f;
	}), mobs.end());

	MobContext shared = context;
	shared.mobs = &mobs;
	for (auto& m : mobs) m->update(dt, shared);

	resident_timer -= dt;
	if (resident_timer <= 0.0f) {
		resident_timer = 1.0f;
		settle_residents(player);
	}

	//a few attempts a second, so herds and schools appear gradually as the world loads in
	spawn_timer -= dt;
	if (spawn_timer <= 0.0f) {
		spawn_timer = 0.25f;
		if (count(false) < max_land_mobs) try_spawn_herd(player);
		if (count(true) < max_water_mobs) try_spawn_school(player);
	}
}
