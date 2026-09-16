#include "PlayerPhysics.h"

PlayerPhysics::PlayerPhysics() : cm(ChunkManager::get_instance()) {}

void PlayerPhysics::integrate(GameObject& target, float dt, bool enabled) {
	if (!enabled) {
		target.position += target.velocity * dt;
		return;
	}

	on_ground = false;
	target.velocity.y += gravity * dt;

	float remaining_dt = dt;
	vector<Block*> resolved;

	for (int i = 0; i < 8 && remaining_dt > 0.0f; ++i) {
		GameObject broadphase = aabb::get_broad_phase(target, remaining_dt);
		vector<Block*> blocks = gather_candidate_blocks(broadphase);

		float shortest_time = 1.0f;
		vec3 collision_normal = vec3(0.0f);
		Block* hit_block = nullptr;

		for (Block* block : blocks) {
			if (find(resolved.begin(), resolved.end(), block) != resolved.end()) continue;

			GameObject block_box;
			block_box.position = block->position;
			block_box.size = vec3(1.0f);

			vec3 normal;
			float t = aabb::get_collision_time(target, block_box, normal, remaining_dt);
			if (t < shortest_time) {
				shortest_time = t;
				collision_normal = normal;
				hit_block = block;
			}
		}

		vec3 displacement = target.velocity * remaining_dt;

		if (shortest_time < 1.0f && hit_block != nullptr) {
			target.position += displacement * shortest_time;
			target.velocity -= dot(target.velocity, collision_normal) * collision_normal;
			if (collision_normal.y > 0.0f) {
				on_ground = true;
			}
			resolved.push_back(hit_block);
			remaining_dt *= (1.0f - shortest_time);
		}
		else {
			target.position += displacement;
			remaining_dt = 0.0f;
		}
	}
}

bool PlayerPhysics::is_underwater(vec3 position) {
	Block* block = cm.get_block_worldspace(position);
	return block != nullptr && block->type == water;
}

vector<Block*> PlayerPhysics::gather_candidate_blocks(const GameObject& broadphase) {
	vector<Block*> blocks;
	vec3 half = broadphase.size * 0.5f;
	const float block_half_extent = 0.5f;
	vec3 min_bound = broadphase.position - half - vec3(block_half_extent);
	vec3 max_bound = broadphase.position + half + vec3(block_half_extent);

	for (int x = (int)floor(min_bound.x); x <= (int)floor(max_bound.x); ++x) {
		for (int y = (int)floor(min_bound.y); y <= (int)floor(max_bound.y); ++y) {
			for (int z = (int)floor(min_bound.z); z <= (int)floor(max_bound.z); ++z) {
				Block* block = cm.get_block_worldspace(vec3(x, y, z));
				if (block != nullptr && is_solid(block->type)) {
					blocks.push_back(block);
				}
			}
		}
	}
	return blocks;
}
