#include "PlayerPhysics.h"

PlayerPhysics::PlayerPhysics() : cm(ChunkManager::get_instance()) {}

void PlayerPhysics::integrate(GameObject& target, float dt, bool enabled) {
	if (enabled) {
		target.velocity.y += gravity * dt;
	}

	target.position.x += target.velocity.x * dt;
	collision(target, target.velocity.x, 0, 0, enabled);
	target.position.y += target.velocity.y * dt;
	collision(target, 0, target.velocity.y, 0, enabled);
	target.position.z += target.velocity.z * dt;
	collision(target, 0, 0, target.velocity.z, enabled);
}

bool PlayerPhysics::is_underwater(vec3 position) {
	Block* block = cm.get_block_worldspace(position);
	return block != nullptr && block->type == water;
}

/*
	handles the player's collision with blocks (AABB collision check)
*/
void PlayerPhysics::collision(GameObject& target, float vx, float vy, float vz, bool enabled) {
	if (!enabled) return;

	if (vy != 0) on_ground = false;

	vec3& position = target.position;
	vec3& velocity = target.velocity;
	vec3& size = target.size;

	//distances from player's center to the edge of their hitbox
	float p_width = size.x / 2;
	float p_depth = size.z / 2;
	float p_top = 0.5;
	float p_bottom = size.y;

	//a block is a 1 x 1 x 1 cube
	int block_size = 1;

	//vertical collisions
	float x1 = floor((position.x - p_width) / block_size); //left
	float y1 = floor((position.y - p_bottom) / block_size);//bottom
	float z1 = floor((position.z - p_depth) / block_size); //back
	float x2 = floor((position.x + p_width) / block_size); //right
	float y2 = floor((position.y + p_top) / block_size);   //top
	float z2 = floor((position.z + p_depth) / block_size); //front

	//TODO: automatic go up if stuck

	//checks if any solid blocks are touching player's bottom feet
	if (is_solid(cm.get_block_worldspace(vec3(x1, y1, z1))) || is_solid(cm.get_block_worldspace(vec3(x2, y1, z1))) ||
		is_solid(cm.get_block_worldspace(vec3(x1, y1, z2))) || is_solid(cm.get_block_worldspace(vec3(x2, y1, z2)))) {
		if (vy < 0) { //the player is falling down
			position.y = (y1 + 1) * block_size + p_bottom; //locates the player on top of the blocks
			velocity.y = 0;
			on_ground = true;
		}
	}

	//checks if any solid blocks are touching the player's top head
	if (is_solid(cm.get_block_worldspace(vec3(x1, y2, z1))) || is_solid(cm.get_block_worldspace(vec3(x2, y2, z1))) ||
		is_solid(cm.get_block_worldspace(vec3(x1, y2, z2))) || is_solid(cm.get_block_worldspace(vec3(x2, y2, z2)))) {
		if (vy > 0) { //the player is jumping
			position.y = y2 * block_size - p_top - 0.01; //locates the player just below the blocks
			velocity.y = 0;
		}
	}

	//horizontal collisions
	//subtracts velocity components vx, vy, and vz to detect collision based on where the player was before the current movement
	//so these are approximations of the player's previous x, y, and z positions
	float x3 = floor((position.x - vx - p_width) / block_size); //left
	float y3 = floor((position.y - vy - p_bottom) / block_size);//bottom
	float z3 = floor((position.z - vz - p_depth) / block_size); //back
	float x4 = floor((position.x - vx + p_width) / block_size); //right
	float y4 = floor((position.y - vy + p_top) / block_size);	//top
	float z4 = floor((position.z - vz + p_depth) / block_size); //front
	float y5 = round(y3 + (y4 - y3) / 2); //previous y position (the middle height between y3 and y4)

	//checks if any solid blocks are touching the player's left side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x1, y3, z3))) || is_solid(cm.get_block_worldspace(vec3(x1, y3, z4))) || //left-bottom-back || left-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x1, y4, z3))) || is_solid(cm.get_block_worldspace(vec3(x1, y4, z4))) || //left-top-back || left-top-front
		is_solid(cm.get_block_worldspace(vec3(x1, y5, z3))) || is_solid(cm.get_block_worldspace(vec3(x1, y5, z4)))) { //left-middle-back || left-middle-front
		if (vx < 0) { //the player is moving to the left
			position.x = (x1 + 1) * block_size + p_width; //locates the player just right to the blocks
			velocity.x = 0;
		}
	}
	//checks if any solid blocks are touching the player's right side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x2, y3, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y3, z4))) || //right-bottom-back || right-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x2, y4, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y4, z4))) || //right-top-back || right-top-front
		is_solid(cm.get_block_worldspace(vec3(x2, y5, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y5, z4)))) { //right-middle-back || right-middle-front
		if (vx > 0) { //the player is moving to the right
			position.x = x2 * block_size - p_width - 0.01; //locates the player just left to the blocks
			velocity.x = 0;
		}
	}
	//checks if any blocks are touching the player's back side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z1))) || is_solid(cm.get_block_worldspace(vec3(x4, y3, z1))) || //left-bottom-back || right-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z1))) || is_solid(cm.get_block_worldspace(vec3(x3, y4, z1))) || //right-top-back || left-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z1))) || is_solid(cm.get_block_worldspace(vec3(x4, y5, z1)))) { //left-middle-back || right-middle-back
		if (vz < 0) { //the player is moving backward
			position.z = (z1 + 1) * block_size + p_depth; //locates the player just in front of the blocks
			velocity.z = 0;
		}
	}
	//checks if any blocks are touching the player's front side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z2))) || is_solid(cm.get_block_worldspace(vec3(x4, y3, z2))) || //left-bottom-front || left-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z2))) || is_solid(cm.get_block_worldspace(vec3(x3, y4, z2))) || //right-top-front || left-top-front
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z2))) || is_solid(cm.get_block_worldspace(vec3(x4, y5, z2)))) { //left-middle-front || right-middle-front
		if (vz > 0) { //the player is moving forward
			position.z = z2 * block_size - p_depth - 0.01; //locates the player just behind the blocks
			velocity.z = 0;
		}
	}
}
