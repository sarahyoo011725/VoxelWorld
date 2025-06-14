#include "GameObject.h"

GameObject aabb::get_broad_phase(const GameObject& obj) {
	GameObject broadphase;
	vec3 movement = obj.velocity; // movement this frame (assume per frame already)

	// Calculate the swept bounds: expand the original AABB to include the entire path of motion
	vec3 expanded_min = min(obj.position - obj.size, obj.position + movement - obj.size);
	vec3 expanded_max = max(obj.position + obj.size, obj.position + movement + obj.size);

	// Center position of broadphase box
	broadphase.position = (expanded_min + expanded_max) * 0.5f;
	// Half-size of broadphase box
	broadphase.size = (expanded_max - expanded_min) * 0.5f;

	return broadphase;
}

bool aabb::check_collision(const GameObject& a, const GameObject& b) {		
	vec3 a_size = a.size + a.hitbox_margin;
	vec3 b_size = b.size + b.hitbox_margin;

	// Calculate min/max corners
	float a_min_x = a.position.x - a_size.x;
	float a_max_x = a.position.x + a_size.x;
	float b_min_x = b.position.x - b_size.x;
	float b_max_x = b.position.x + b_size.x;

	float a_min_y = a.position.y - a_size.y;
	float a_max_y = a.position.y + a_size.y;
	float b_min_y = b.position.y - b_size.y;
	float b_max_y = b.position.y + b_size.y;

	float a_min_z = a.position.z - a_size.z;
	float a_max_z = a.position.z + a_size.z;
	float b_min_z = b.position.z - b_size.z;
	float b_max_z = b.position.z + b_size.z;

	// Check for overlap on all axes
	bool x_overlap = a_max_x > b_min_x && a_min_x < b_max_x;
	bool y_overlap = a_max_y > b_min_y && a_min_y < b_max_y;
	bool z_overlap = a_max_z > b_min_z && a_min_z < b_max_z;

	return x_overlap && y_overlap && z_overlap;
}

float aabb::get_collision_time(const GameObject& a, const GameObject& b, vec3& normal) {
	vec3 a_size = a.size + a.hitbox_margin;
	vec3 b_size = b.size + b.hitbox_margin;

	vec3 entry_dist; //distance from a to b on each axis when it first makes contact
	vec3 exit_dist; // distance from a to b on each axis when it exists contact
	
	//find entry and exit distances in x direction
	if (a.velocity.x > 0) { //if moving to the right
		entry_dist.x = b.position.x - (a.position.x + a_size.x); //b_left - a_right
		exit_dist.x = (b.position.x + b_size.x) - a.position.x;  //b_right - a_left
	}
	else { //if moving to the left
		entry_dist.x = (b.position.x + b_size.x) - a.position.x; //b_right - a_left
		exit_dist.x = b.position.x - (a.position.x + a_size.x);  //b_left - a_right
	}
	//find entry and exit distances in y direction
	if (a.velocity.y > 0) { //if moving up
		entry_dist.y = b.position.y - (a.position.y); //b_bottom - a_top
		exit_dist.y = (b.position.y + b_size.y) - (a.position.y - a_size.y);	 //b_top - a_bottom
	}
	else { //if moving down (falling)
		entry_dist.y = (b.position.y + b_size.y) - (a.position.y - a_size.y); //b_top - a_bottom
		exit_dist.y = b.position.y - (a.position.y);  //b_bottom - a_top
	}
	//find entry and exit distances in z direction
	if (a.velocity.z > 0) { //if moving forward
		entry_dist.z = b.position.z - (a.position.z + a_size.z); //b_back - a_front
		exit_dist.z = (b.position.z + b_size.z) - a.position.z;  //b_front - a_back
	}
	else { //if moving backward
		entry_dist.z = (b.position.z + b_size.z) - a.position.z; //b_front - a_back
		exit_dist.z = b.position.z - (a.position.z + a_size.z);  //b_back - a_front
	}

	//if object's velocity is zero, it does not move. So the entry and exit times are infinite (never validated).
	vec3 entry_time = vec3(-numeric_limits<float>::infinity()); //time of collision for each axis
	vec3 exit_time = vec3(numeric_limits<float>::infinity());   //time of leaving for each axis
	//find entry and exit times. note that time = distance / speed
	if (a.velocity.x != 0) {
		entry_time.x = entry_dist.x / a.velocity.x;
		exit_time.x = exit_dist.x / a.velocity.x;
	}
	if (a.velocity.y != 0) {
		entry_time.y = entry_dist.y / a.velocity.y;
		exit_time.y = exit_dist.y / a.velocity.y;
	}
	if (a.velocity.z != 0) {
		entry_time.z = entry_dist.z / a.velocity.z;
		exit_time.z = exit_dist.z / a.velocity.z;
	}

	//find the time when the collision first occurred
	float first_entry_time = std::max(entry_time.x, std::max(entry_time.y, entry_time.z));
	//find the time when a stopped making contact with b
	float last_exit_time = std::min(exit_time.x, std::min(exit_time.y, exit_time.z));

	if (first_entry_time > last_exit_time || first_entry_time < 0.0f || first_entry_time > 1.0f) {
		//there was no collision if the collision time is not within 0 and 1
		normal = vec3(0.0f);
		return 1; //return 1 to signal that there was no collision
	}
	else { //if there was a collision
		//calculate the normal (directio) of collided surface
		normal.x = (first_entry_time == entry_time.x) ? -sign(entry_dist.x) : 0;
		normal.y = (first_entry_time == entry_time.y) ? -sign(entry_dist.y) : 0;
		normal.z = (first_entry_time == entry_time.z) ? -sign(entry_dist.z) : 0;

		return first_entry_time;
	}
}