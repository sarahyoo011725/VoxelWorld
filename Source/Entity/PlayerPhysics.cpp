#include "PlayerPhysics.h"

PlayerPhysics::PlayerPhysics() : cm(ChunkManager::get_instance()) {}

void PlayerPhysics::integrate(GameObject& target, float dt, bool enabled) {
	if (!enabled) {
		target.position += target.velocity * dt;
		return;
	}

	on_ground = false;
	target.velocity.y += gravity * dt;

	GameObject broadphase = aabb::get_broad_phase(target, dt);
	vector<Block*> blocks = gather_candidate_blocks(broadphase);

	float shortest_time = 1.0f;
	vec3 collision_normal = vec3(0.0f);

	for (Block* block : blocks) {
		GameObject block_box;
		block_box.position = block->position;
		block_box.size = vec3(1.0f);

		vec3 normal;
		float t = aabb::get_collision_time(target, block_box, normal, dt);
		if (t < shortest_time) {
			shortest_time = t;
			collision_normal = normal;
		}
	}

	vec3 displacement = target.velocity * dt;

	if (shortest_time < 1.0f) {
		target.position += displacement * shortest_time;
		target.velocity -= dot(target.velocity, collision_normal) * collision_normal;
		target.position += target.velocity * dt * (1.0f - shortest_time);
		if (collision_normal.y > 0.0f) {
			on_ground = true;
		}
	}
	else {
		target.position += displacement;
	}
}

bool PlayerPhysics::is_underwater(vec3 position) {
	Block* block = cm.get_block_worldspace(position);
	return block != nullptr && block->type == water;
}

vector<Block*> PlayerPhysics::gather_candidate_blocks(const GameObject& broadphase) {
	vector<Block*> blocks;
	vec3 half = broadphase.size * 0.5f;
	vec3 min_bound = broadphase.position - half;
	vec3 max_bound = broadphase.position + half;

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
