#include "Collision.h"
#include "GameObject.h"
#include <limits>

GameObject aabb::get_broad_phase(const GameObject& obj, float dt) {
	GameObject broadphase;
	vec3 half = obj.size * 0.5f;
	vec3 displacement = obj.velocity * dt;

	vec3 expanded_min = min(obj.position - half, obj.position + displacement - half);
	vec3 expanded_max = max(obj.position + half, obj.position + displacement + half);

	broadphase.position = (expanded_min + expanded_max) * 0.5f;
	broadphase.size = expanded_max - expanded_min;
	return broadphase;
}

bool aabb::check_collision(const GameObject& a, const GameObject& b) {
	vec3 a_half = (a.size + a.hitbox_margin) * 0.5f;
	vec3 b_half = (b.size + b.hitbox_margin) * 0.5f;

	bool x_overlap = (a.position.x + a_half.x) > (b.position.x - b_half.x) && (a.position.x - a_half.x) < (b.position.x + b_half.x);
	bool y_overlap = (a.position.y + a_half.y) > (b.position.y - b_half.y) && (a.position.y - a_half.y) < (b.position.y + b_half.y);
	bool z_overlap = (a.position.z + a_half.z) > (b.position.z - b_half.z) && (a.position.z - a_half.z) < (b.position.z + b_half.z);

	return x_overlap && y_overlap && z_overlap;
}

float aabb::get_collision_time(const GameObject& a, const GameObject& b, vec3& normal, float dt) {
	vec3 a_half = (a.size + a.hitbox_margin) * 0.5f;
	vec3 b_half = (b.size + b.hitbox_margin) * 0.5f;
	vec3 displacement = a.velocity * dt;

	vec3 entry_dist, exit_dist;

	if (displacement.x > 0) {
		entry_dist.x = (b.position.x - b_half.x) - (a.position.x + a_half.x);
		exit_dist.x = (b.position.x + b_half.x) - (a.position.x - a_half.x);
	}
	else {
		entry_dist.x = (b.position.x + b_half.x) - (a.position.x - a_half.x);
		exit_dist.x = (b.position.x - b_half.x) - (a.position.x + a_half.x);
	}
	if (displacement.y > 0) {
		entry_dist.y = (b.position.y - b_half.y) - (a.position.y + a_half.y);
		exit_dist.y = (b.position.y + b_half.y) - (a.position.y - a_half.y);
	}
	else {
		entry_dist.y = (b.position.y + b_half.y) - (a.position.y - a_half.y);
		exit_dist.y = (b.position.y - b_half.y) - (a.position.y + a_half.y);
	}
	if (displacement.z > 0) {
		entry_dist.z = (b.position.z - b_half.z) - (a.position.z + a_half.z);
		exit_dist.z = (b.position.z + b_half.z) - (a.position.z - a_half.z);
	}
	else {
		entry_dist.z = (b.position.z + b_half.z) - (a.position.z - a_half.z);
		exit_dist.z = (b.position.z - b_half.z) - (a.position.z + a_half.z);
	}

	vec3 entry_time = vec3(-numeric_limits<float>::infinity());
	vec3 exit_time = vec3(numeric_limits<float>::infinity());

	if (displacement.x != 0) { entry_time.x = entry_dist.x / displacement.x; exit_time.x = exit_dist.x / displacement.x; }
	if (displacement.y != 0) { entry_time.y = entry_dist.y / displacement.y; exit_time.y = exit_dist.y / displacement.y; }
	if (displacement.z != 0) { entry_time.z = entry_dist.z / displacement.z; exit_time.z = exit_dist.z / displacement.z; }

	float first_entry_time = std::max(entry_time.x, std::max(entry_time.y, entry_time.z));
	float last_exit_time = std::min(exit_time.x, std::min(exit_time.y, exit_time.z));

	if (first_entry_time > last_exit_time || first_entry_time < 0.0f || first_entry_time > 1.0f) {
		normal = vec3(0.0f);
		return 1.0f;
	}

	normal.x = (first_entry_time == entry_time.x) ? -sign(entry_dist.x) : 0.0f;
	normal.y = (first_entry_time == entry_time.y) ? -sign(entry_dist.y) : 0.0f;
	normal.z = (first_entry_time == entry_time.z) ? -sign(entry_dist.z) : 0.0f;

	return first_entry_time;
}
