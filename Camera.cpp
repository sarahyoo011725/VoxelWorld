#include "Camera.h"

Camera::Camera(GLFWwindow *window, int window_width, int window_height, vec3 position) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()) {
	size = vec3(1, 2, 1);
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

	outline_vao.bind();
	outline_vao.link_attrib(outline_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	HUD_vao.bind();
	HUD_vao.link_attrib(HUD_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
}

//updates anything required periodically
void Camera::update() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;

	update_other_inputs();
	update_mouse();
	update_movement(dt);
	raycast();
	block_interaction();
}

void Camera::update_other_inputs() {
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	}
	else {
		fov_degrees = 45.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		is_running = !is_running;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		enable_physics = !enable_physics;
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		enable_outline = !enable_outline;
	}
}

bool Camera::is_ground() {
	vec3 below_pos = position;
	below_pos.y -= size.y + 0.1f;
	Block* block = cm.get_block_worldspace(below_pos);
	return block != nullptr &&is_solid(block->type);
}

void Camera::raycast() {
	//normally, the conversion from 2d screen into 3d world space would like this:
	//viewport space (2d screen) -> normalized device space (opengl coord system) -> homogenenous clip state -> eye space(origin is cam) -> world space
	//but it's so simple that I can just cast a ray with direction (or forward) vector
	ray = position + direction * ray_length;
}

void Camera::update_matrix(int shader_id) {
	view = lookAt(position, position + direction, up);
	projection = perspective(radians(fov_degrees), (float)window_width / window_height, near_plane, far_plane);
	glUniformMatrix4fv(glGetUniformLocation(shader_id, "cam_matrix"), 1, GL_FALSE, value_ptr(projection * view));
}

void Camera::draw_HUDs() {
	HUD_shader.activate();
	HUD_vao.bind();
	glUniform4fv(glGetUniformLocation(HUD_shader.id, "color"), 1, value_ptr(crosshair_color));
	//draw crosshair
	glLineWidth(1.0);
	glDrawArrays(GL_LINES, 0, 4);
}

void Camera::draw_outlines() {
	if (!enable_outline) return;
	//outlines are must be drawn after activating the shader
	outline_shader.activate();
	outline_vao.bind();
	update_matrix(outline_shader.id);  //must be sendt to the shader
	outline_hovered_cube();
}

void Camera::outline_hovered_cube() {
	if (hovered_block == nullptr) return;
	//sends cube model and outline color to the outline shader
	mat4 cube_model = translate(mat4(1.0), hovered_block->position);
	glUniformMatrix4fv(glGetUniformLocation(outline_shader.id, "cube_model"), 1, GL_FALSE, value_ptr(cube_model));
	glUniform4fv(glGetUniformLocation(outline_shader.id, "color"), 1, value_ptr(hovered_block_outline_color));

	//draw cube outline
	glDisable(GL_DEPTH_TEST); //keeps the cube outline visible
	glLineWidth(outline_thickness);
	glDrawArrays(GL_LINES, 0, cube_edges.size()); //first: index where verticies are inserted, counts: # of vertices to draw the line(s)
	glEnable(GL_DEPTH_TEST);
}

void Camera::block_interaction() {
	ivec2 chunk_id = get_chunk_origin(ray);
	ivec3 local_coord = world_to_local_coord(ray);

	Chunk* chunk = cm.get_chunk(chunk_id);
	Block* block = nullptr;
	if (chunk != nullptr) {
		block = chunk->get_block(local_coord);
	}
	
	hovered_block = block;

	if (block == nullptr) return;

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (block->type == none || block->type == water) {
			if (holding_block_type != none) {
				if (is_nonblock(holding_block_type)) {
					sg.spawn_nonblock_structure(holding_block_type, block->position);
					chunk->should_rebuild = true;
				}
				else {
					cm.set_block_manual(chunk_id, local_coord, holding_block_type); //TOOD: selection of block type, inventory 
				}
				audio::play_block_sound_effect(holding_block_type);
			}
		}
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (block->type != none) {
			holding_block_type = hovered_block->type;
			if (is_nonblock(block->type)) {
				chunk->remove_structure(local_coord); 
			}
			audio::play_block_sound_effect(block->type);
			cm.set_block_manual(chunk_id, local_coord, none);
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

void Camera::update_movement(float dt) {
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	up = normalize(cross(right, direction));
	vec3 input_dir = vec3(0.0);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		input_dir += direction;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		input_dir -= direction;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		input_dir -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		input_dir += right;
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		input_dir += up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		input_dir -= up;
	}

	if (length(input_dir) > 0.0) {
		input_dir = normalize(input_dir);
	}

	if (is_running) {
		speed = (enable_physics) ? run_speed : run_speed_fly;
	}
	else {
		speed = default_speed;
	}

	velocity.x = input_dir.x * speed;
	velocity.z = input_dir.z * speed;
	if (!enable_physics) velocity.y = input_dir.y * speed;

	//note: velocity += acceleration * dt, position += velocity * dt, 
	//multiply dt to ensure player moves the same distance per second. (makes the movement frame-rate independent)

	if (enable_physics) {
		velocity.y += gravity * dt;

		//TODO: walking sound effect
		if (on_ground) {
			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
				velocity.y = jump_force;
			}
			if (velocity.x != 0 && velocity.z != 0) {
				audio::play_block_walked(dirt_grass); //TODO: detect what type of block the player is walking on
			} else {
				audio::effect_player2.stop();
			}
		}
		else {
			if (audio::effect_player2.is_playing()) {
				audio::effect_player2.stop();
			}
		}
	}

	position.x += velocity.x * dt;
	collision(velocity.x, 0, 0);
	position.y += velocity.y * dt;
	collision(0, velocity.y, 0);
	position.z += velocity.z * dt;
	collision(0, 0, velocity.z);
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
	if (is_solid(cm.get_block_worldspace(vec3(x1, y1, z1))) ||is_solid(cm.get_block_worldspace(vec3(x2, y1, z1))) ||
		is_solid(cm.get_block_worldspace(vec3(x1, y1, z2))) ||is_solid(cm.get_block_worldspace(vec3(x2, y1, z2)))) {
		if (vy < 0) { //the player is falling down
			position.y = (y1 + 1) * block_size + p_bottom; //locates the player on top of the blocks
			velocity.y = 0;
			on_ground = true;
		}
	}

	//checks if any solid blocks are touching the player's top head
	if (is_solid(cm.get_block_worldspace(vec3(x1, y2, z1))) ||is_solid(cm.get_block_worldspace(vec3(x2, y2, z1))) ||
		is_solid(cm.get_block_worldspace(vec3(x1, y2, z2))) ||is_solid(cm.get_block_worldspace(vec3(x2, y2, z2)))) {
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
	if (is_solid(cm.get_block_worldspace(vec3(x1, y3, z3))) ||is_solid(cm.get_block_worldspace(vec3(x1, y3, z4))) || //left-bottom-back || left-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x1, y4, z3))) ||is_solid(cm.get_block_worldspace(vec3(x1, y4, z4))) || //left-top-back || left-top-front
		is_solid(cm.get_block_worldspace(vec3(x1, y5, z3))) ||is_solid(cm.get_block_worldspace(vec3(x1, y5, z4)))) { //left-middle-back || left-middle-front
		if (vx < 0) { //the player is moving to the left
			position.x = (x1 + 1) * block_size + p_width; //locates the player just right to the blocks
			velocity.x = 0;
		}
	}
	//checks if any solid bloccks are touchign the player's right side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x2, y3, z3))) ||is_solid(cm.get_block_worldspace(vec3(x2, y3, z4))) || //right-bottom-back || right-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x2, y4, z3))) ||is_solid(cm.get_block_worldspace(vec3(x2, y4, z4))) || //right-top-back || right-top-front
		is_solid(cm.get_block_worldspace(vec3(x2, y5, z3))) ||is_solid(cm.get_block_worldspace(vec3(x2, y5, z4)))) { //right-middle-back || right-middle-front
		if (vx > 0) { //the player is moving to the right
			position.x = x2 * block_size - p_width - 0.01; //locates the player just left to the blocks
			velocity.x = 0;
		}
	}
	//checks if any blocks are touching the player's back side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z1))) ||is_solid(cm.get_block_worldspace(vec3(x4, y3, z1))) || //left-bottom-back || rigth-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z1))) ||is_solid(cm.get_block_worldspace(vec3(x3, y4, z1))) || //right-top-back || left-bottom-back
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z1))) ||is_solid(cm.get_block_worldspace(vec3(x4, y5, z1)))) { //left-middle-back || rigth-middle-back
		if (vz < 0) { //the player is moving backward
			position.z = (z1 + 1) * block_size + p_depth; //locates the player just in front of the blocks
			velocity.z = 0;
		}
	}
	//checks if any blocks are touchig the player's front side at any vertical level
	if (is_solid(cm.get_block_worldspace(vec3(x3, y3, z2))) ||is_solid(cm.get_block_worldspace(vec3(x4, y3, z2))) || //left-bottom-front || left-bottom-front
		is_solid(cm.get_block_worldspace(vec3(x4, y4, z2))) ||is_solid(cm.get_block_worldspace(vec3(x3, y4, z2))) || //right-top-front || left-top-front
		is_solid(cm.get_block_worldspace(vec3(x3, y5, z2))) ||is_solid(cm.get_block_worldspace(vec3(x4, y5, z2)))) { //left-middle-front || right-middle-front
		if (vz > 0) { //the player is moving forward
			position.z = z2 * block_size - p_depth - 0.01; //locates the player just behind the blocks
			velocity.z = 0;
		}
	}
}