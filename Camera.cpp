#include "Camera.h"

Camera::Camera(GLFWwindow *window, int window_width, int window_height, vec3 position) : cm(ChunkManager::get_instance()) {
	size = vec3(1, 2, 1);
	hitbox_margin = vec3(1, 0, 1);
	this->position = position;
	this->window = window;
	this->window_width = window_width;
	this->window_height = window_height;
	//sets last cursor position to be at the center of the window
	last_xpos = window_width / 2;
	last_ypos = window_height / 2;
	mouse_xpos = last_xpos;
	mouse_ypos = last_ypos;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera::update_matrix(int shader_id) {
	mat4 view = lookAt(position, position + direction, up);
	mat4 projection = perspective(radians(fov_degrees), (float)window_width / window_height, near_plane, far_plane);
	GLuint location = glGetUniformLocation(shader_id, "cam_matrix");
	glUniformMatrix4fv(location, 1, false, value_ptr(projection * view));
}

bool Camera::is_ground() {
	vec3 below_pos = position;
	below_pos.y -= size.y + 0.1f;
	Block* block = cm.get_block_worldspace(below_pos);
	return block != nullptr && is_solid(block->type);
}

//updates anything required periodically
void Camera::update() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;

	update_mouse();
	update_movement(dt);
}

void Camera::update_movement(float dt) {
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	up = normalize(cross(right, direction));
	vec3 input_dir = vec3(0.0);

	if (glfwGetKey(window, GLFW_KEY_W)) {
		input_dir += direction;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		input_dir -= direction;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		input_dir -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		input_dir += right;
	}
	
	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		input_dir += up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
		input_dir -= up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT)) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	} else {
		fov_degrees = 45.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
		is_running = !is_running;
	}
	if (glfwGetKey(window, GLFW_KEY_2)) {
		enable_physics = !enable_physics;
	}

	if (is_running) {
		speed = (enable_physics) ? run_speed : run_speed_fly;
	}
	else {
		speed = default_speed;
	}

	if (length(input_dir) > 0.0) {
		input_dir = normalize(input_dir);
	}

	velocity.x = input_dir.x * speed;
	velocity.z = input_dir.z * speed;
	if (!enable_physics) velocity.y = input_dir.y * speed;

	//note: velocity += acceleration * dt, position += velocity * dt, 
	//multiply dt to ensure player moves the same distance per second. (makes the movement frame-rate independent)

	if (enable_physics) {
		velocity.y += gravity * dt;
	
		if (on_ground && glfwGetKey(window, GLFW_KEY_SPACE)) {
			velocity.y = jump_force;
		}
	}
	handle_collision_with_block(dt);
}

vector<Block*> Camera::get_potential_blocks_in_collision(GameObject broadsphase) {
	vector<Block*> blocks;

	int min_x = (int)(floor(broadsphase.position.x - broadsphase.size.x / 2));
	int min_y = (int)(floor(broadsphase.position.y - broadsphase.size.y / 2));
	int min_z = (int)(floor(broadsphase.position.z - broadsphase.size.z / 2));
	int max_x = (int)(floor(broadsphase.position.x + broadsphase.size.x / 2));
	int max_y = (int)(floor(broadsphase.position.y + broadsphase.size.y / 2));
	int max_z = (int)(floor(broadsphase.position.z + broadsphase.size.z / 2));

	for (int x = min_x; x <= max_x; ++x) {
		for (int y = min_y; y <= max_y; ++y) {
			for (int z = min_z; z <= max_z; ++z) {
				Block* block = cm.get_block_worldspace({ x, y, z });
				if (block != nullptr && is_solid(block->type)) {
					blocks.push_back(block);
				}
			}
		}
	}

	return blocks;
}

void Camera::handle_collision_with_block(float dt) {	
		on_ground = false;

		GameObject broadsphase = aabb::get_broad_phase(*this);
		vector<Block*> blocks = get_potential_blocks_in_collision(broadsphase);
		
		float shortest_collision_time = 1.0f;
		vec3 collision_normal = vec3(0.0f);

		for (Block* block : blocks) {
			if (aabb::check_collision(*this, *block)) { //quickly determines if there is a collision for a performance-wise
				vec3 normal;
				float collision_time = aabb::get_collision_time(*this, *block, normal);

				if (collision_time <  shortest_collision_time) {
					shortest_collision_time = collision_time;
					collision_normal = normal;
				}
			}
		}

		if (shortest_collision_time < 1.0f) { //if time is between 0 and 1, there is a collision
			position += velocity * shortest_collision_time * dt;
			float remaining_time = 1.0f - shortest_collision_time;
			//performs collision responses during the remaining time
			//slides player if collides. This uses a vector projection to simply find the equivalent player's position on the edge
			//calculates the projected movement vector onto the sliding plane and then apply this to the player's position
			vec3 slide_vel = velocity - (dot(velocity, collision_normal) / dot(collision_normal, collision_normal)) * collision_normal;
			slide_vel = normalize(slide_vel) * length(velocity);
			position += slide_vel * remaining_time * dt;
			//velocity = slide_vel;
			if (collision_normal.y == 1) {
				on_ground = true;
			}
		}
		else {
			position += velocity * dt;
		}
}


//handles player's collision with blocks (aabb collision logic)
void Camera::collision(float vx, float vy, float vz) {
	if (!enable_physics) return;

	if (vy != 0) on_ground = false;

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
			position.y = y2 * block_size - p_top - 0.01; //locates the plyaer just below the blocks
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
	//checks if any solid bloccks are touchign the player's right side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x2, y3, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y3, z4))) || //right-bottom-back || right-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x2, y4, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y4, z4))) || //right-top-back || right-top-front
		is_solid(cm.get_block_worldspace(vec3(x2, y5, z3))) || is_solid(cm.get_block_worldspace(vec3(x2, y5, z4)))) { //right-middle-back || right-middle-front
		if (vx > 0) { //the player is moving to the right
			position.x = x2 * block_size - p_width - 0.01; //locates the player just left to the blocks
			velocity.x = 0;
		}
	}
	//checks if any blocks are touching the player's back side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z1))) || is_solid(cm.get_block_worldspace(vec3(x4, y3, z1))) || //left-bottom-back || rigth-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z1))) || is_solid(cm.get_block_worldspace(vec3(x3, y4, z1))) || //right-top-back || left-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z1))) || is_solid(cm.get_block_worldspace(vec3(x4, y5, z1)))) { //left-middle-back || rigth-middle-back
		if (vz < 0) { //the player is moving backward
			position.z = (z1 + 1) * block_size + p_depth; //locates the player just in front of the blocks
			velocity.z = 0;
		}
	}
	//checks if any blocks are touchig the player's front side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z2))) || is_solid(cm.get_block_worldspace(vec3(x4, y3, z2))) || //left-bottom-front || left-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z2))) || is_solid(cm.get_block_worldspace(vec3(x3, y4, z2))) || //right-top-front || left-top-front
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z2))) || is_solid(cm.get_block_worldspace(vec3(x4, y5, z2)))) { //left-middle-front || right-middle-front
		if (vz > 0) { //the player is moving forward
			position.z = z2 * block_size - p_depth - 0.01; //locates the player just behind the blocks
			velocity.z = 0;
		}
	}
}

void Camera::update_mouse() {
	glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
	//prevents sudden view jump when window re-focued
	/*
	if (window_refocused) {
		mouse_xpos = last_xpos;
		mouse_ypos = last_ypos;
		window_refocused = false;
	}
	*/
	float x_offset = last_xpos - mouse_xpos;
	float y_offset = mouse_ypos - last_ypos;
	last_xpos = mouse_xpos;
	last_ypos = mouse_ypos;
	
	x_offset *= mouse_sensitivity;
	y_offset *= mouse_sensitivity;

	yaw += x_offset;
	pitch += y_offset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f) 
		pitch = -89.0f;

	vec3 new_direction;
	new_direction.x = cos(radians(yaw)) * cos(radians(pitch));
	new_direction.y = sin(radians(pitch));
	new_direction.z = sin(radians(yaw)) * cos(radians(pitch));
	direction = normalize(new_direction);
}