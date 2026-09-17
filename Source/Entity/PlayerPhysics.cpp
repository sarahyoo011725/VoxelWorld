#include "PlayerPhysics.h"

PlayerPhysics::PlayerPhysics() : cm(ChunkManager::get_instance()) {}

void PlayerPhysics::integrate(GameObject& target, float dt, bool enabled) {
	if (!enabled) {
		target.position += target.velocity * dt;
		return;
	}

	//mid-step frames are a brief scripted rise, not normal physics - gravity
	//and collision running concurrently on top of it was fighting the rise
	//(re-blocked by the step's own face until fully above it) and never let it
	//complete, so the player fell back down and re-triggered the same step
	//forever. Keep moving horizontally through it, but skip gravity/collision
	//until the rise finishes.
	if (pending_step > 0.0f) {
		float rise = std::min(pending_step, step_speed * dt);
		target.position.y += rise;
		pending_step -= rise;
		target.position += vec3(target.velocity.x, 0.0f, target.velocity.z) * dt;
		if (pending_step <= 0.0f) {
			//land in a clean, known state - a stray residual velocity.y or a
			//height a hair off the true resting position is exactly what turns
			//an adjacent floor tile's side face into a phantom collision
			target.velocity.y = 0.0f;
			on_ground = true;
		}
		return;
	}

	try_auto_step(target, dt);

	on_ground = false;
	target.velocity.y += gravity * dt;

	float remaining_dt = dt;
	vector<vec3> resolved;

	for (int i = 0; i < 8 && remaining_dt > 0.0f; ++i) {
		GameObject broadphase = aabb::get_broad_phase(target, remaining_dt);
		vector<vec3> blocks = gather_candidate_blocks(broadphase);

		float shortest_time = 1.0f;
		vec3 collision_normal = vec3(0.0f);
		bool hit = false;
		vec3 hit_position = vec3(0.0f);

		for (const vec3& block : blocks) {
			if (find(resolved.begin(), resolved.end(), block) != resolved.end()) continue;

			//a block whose top is already at or below the target's current feet
			//can only ever be support, never a wall - without this, walking
			//across a flat multi-tile floor can catch the side face of the next
			//tile over (whose top merely brushes the target's resting height)
			//and register a phantom horizontal collision
			bool underfoot_level = block.y + 0.5f <= target.position.y - target.size.y * 0.5f + 0.01f;
			if (underfoot_level) {
				bool directly_below = abs(block.x - target.position.x) < (target.size.x * 0.5f + 0.5f)
					&& abs(block.z - target.position.z) < (target.size.z * 0.5f + 0.5f);
				if (!directly_below) continue;
			}

			GameObject block_box;
			block_box.position = block;
			block_box.size = vec3(1.0f);

			vec3 normal;
			float t = aabb::get_collision_time(target, block_box, normal, remaining_dt);
			if (t < shortest_time) {
				shortest_time = t;
				collision_normal = normal;
				hit = true;
				hit_position = block;
			}
		}

		vec3 displacement = target.velocity * remaining_dt;

		if (shortest_time < 1.0f && hit) {
			target.position += displacement * shortest_time;
			target.velocity -= dot(target.velocity, collision_normal) * collision_normal;
			if (collision_normal.y > 0.0f) {
				on_ground = true;
			}
			resolved.push_back(hit_position);
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

bool PlayerPhysics::is_position_clear(const GameObject& probe) {
	GameObject bounds = aabb::get_broad_phase(probe, 0.0f);
	for (const vec3& block : gather_candidate_blocks(bounds)) {
		GameObject block_box;
		block_box.position = block;
		block_box.size = vec3(1.0f);
		if (aabb::check_collision(probe, block_box)) return false;
	}
	return true;
}

bool PlayerPhysics::has_solid_ground_below(const GameObject& probe) {
	vec3 below = probe.position;
	below.y -= probe.size.y * 0.5f + 0.1f;
	Block* block = cm.get_block_worldspace(below);
	return block != nullptr && is_solid(block->type);
}

//catching the top edge of a 1-block terrain step is the classic AABB
//snag - a small hop where none should happen. checking whether raising the
//player clears the way, and doing that instead of a normal horizontal
//collision, gives a deliberate step instead of a snag.
void PlayerPhysics::try_auto_step(GameObject& target, float dt) {
	if (!on_ground) return;

	vec3 horizontal_velocity = vec3(target.velocity.x, 0.0f, target.velocity.z);
	if (length(horizontal_velocity) < 0.001f) return;
	vec3 direction = normalize(horizontal_velocity);

	GameObject ahead = target;
	ahead.position += horizontal_velocity * dt;
	if (is_position_clear(ahead)) return; //nothing in the way, no step needed

	GameObject raised_here = target;
	raised_here.position.y += step_height;
	if (!is_position_clear(raised_here)) return; //no headroom to rise into

	//probe past our own leading edge onto the obstacle's top surface, not just
	//this frame's tiny nudge forward, which is still at its vertical face
	GameObject landing = target;
	landing.position += direction * (target.size.x * 0.5f + 0.6f);
	landing.position.y += step_height;
	if (!is_position_clear(landing)) return; //still blocked even stepped up - a real wall

	//a thin obstacle (e.g. a tree trunk) can have clear air above it with
	//nothing to actually stand on - stepping up there would just mean falling
	//straight back down next frame and re-triggering the same step: a bounce
	if (!has_solid_ground_below(landing)) return;

	pending_step = step_height;
}

vector<vec3> PlayerPhysics::gather_candidate_blocks(const GameObject& broadphase) {
	vector<vec3> blocks;
	vec3 half = broadphase.size * 0.5f;
	const float block_half_extent = 0.5f;
	vec3 min_bound = broadphase.position - half - vec3(block_half_extent);
	vec3 max_bound = broadphase.position + half + vec3(block_half_extent);

	for (int x = (int)floor(min_bound.x); x <= (int)floor(max_bound.x); ++x) {
		for (int y = (int)floor(min_bound.y); y <= (int)floor(max_bound.y); ++y) {
			for (int z = (int)floor(min_bound.z); z <= (int)floor(max_bound.z); ++z) {
				vec3 block_position = vec3(x, y, z);
				Block* block = cm.get_block_worldspace(block_position);
				if (block != nullptr && is_solid(block->type)) {
					blocks.push_back(block_position);
				}
			}
		}
	}
	return blocks;
}
