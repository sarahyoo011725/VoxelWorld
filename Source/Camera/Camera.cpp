#include "Camera.h"

/*
	initializes instance variables, sets up VAOs
*/
Camera::Camera(WindowSetting *setting, vec3 position) 
: cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()), sm(ShaderManager::get_instance()), window_setting(setting) {
	size = vec3(1, 2, 1);
	this->position = position;
	//sets last cursor position to be at the center of the window_setting->window
	last_xpos = window_setting->width / 2;
	last_ypos = window_setting->height / 2;
	mouse_xpos = last_xpos;
	mouse_ypos = last_ypos;
	glfwSetInputMode(window_setting->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	outline_vao.bind();
	outline_vao.link_attrib(outline_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	HUD_vao.bind();
	HUD_vao.link_attrib(HUD_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	HUD_vao.link_attrib(HUD_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));
	
	quad_vao.bind();
	quad_vao.link_attrib(quad_vbo, 0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)0);
	quad_vao.link_attrib(quad_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_2d), (void*)(2 * sizeof(float)));

	sm.frame_buffer_shader.activate();
	sm.frame_buffer_shader.set_uniform_1i("screen_texture", 1);
	sm.frame_buffer_shader.set_uniform_1i("depth_texture", 3);

	fbo.bind();
	fbo.attach_texture(GL_COLOR_ATTACHMENT0, texture_color_buffer.get_id());
	fbo.attach_texture(GL_DEPTH_ATTACHMENT, depth_texture.get_id());
	fbo.unbind();
}

//updates anything required periodically
void Camera::update() {
	float frame = (float)glfwGetTime();
	dt = frame - last_frame;
	last_frame = frame;

	update_matrix();

	sm.default_shader.activate();
	sm.default_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);

	sm.wave_shader.activate();
	sm.wave_shader.set_uniform_1f("time", frame);
	sm.wave_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);
	
	sm.frame_buffer_shader.activate();
	sm.frame_buffer_shader.set_uniform_1i("is_underwater", is_underwater());

	update_other_inputs();
	update_mouse();
	update_movement(dt);
	raycast();
	block_interaction();
}

/*
	handles user inputs and enables/disables modes
*/
void Camera::update_other_inputs() {
	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
		fov_degrees -= 23.0f * dt;
		if (fov_degrees < 10.f)
			fov_degrees = 10.0f;
	}
	else {
		fov_degrees = 45.0f;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		is_running = !is_running;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_2) == GLFW_PRESS) {
		enable_physics = !enable_physics;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_3) == GLFW_PRESS) {
		enable_outline = !enable_outline;
	}
}

void Camera::bind_fbo() {
	fbo.bind();
}

void Camera::unbind_fbo() {
	fbo.unbind();
}

/*
	checks if the player is on ground
*/
bool Camera::is_ground() {
	vec3 below_pos = position;
	below_pos.y -= size.y + 0.1f;
	Block* block = cm.get_block_worldspace(below_pos);
	return block != nullptr && is_solid(block->type);
}

bool Camera::is_underwater() {
	Block* block = cm.get_block_worldspace(position);
	if (block != nullptr && block->type == water) {
		return true;
	}
	return false;
}

/*
	updates camera projection * view matrix 
*/
void Camera::update_matrix() {
	view = lookAt(position, position + direction, up);
	projection = perspective(radians(fov_degrees), (float)window_setting->width / window_setting->height, near_plane, far_plane);
	mat = projection * view;
}

/*
	draws 2D HUD components like crosshair
*/
void Camera::draw_HUDs() {
	sm.HUD_shader.activate();
	sm.HUD_shader.set_uniform_1i("use_texture", GL_FALSE);
	sm.HUD_shader.set_uniform_4f("color", 1, crosshair_color);
	//draw crosshair
	HUD_vao.bind();
	glLineWidth(1.0);
	glDrawArrays(GL_LINES, 0, 4);
}

/*
	draws outlined objects
*/
void Camera::draw_outlines() {
	if (!enable_outline) return;
	//outlines are must be drawn after activating the shader
	sm.outline_shader.activate();
	sm.outline_shader.set_uniform_mat4f("cam_matrix", 1, GL_FALSE, mat);  //must be sent to the shader
	outline_vao.bind();
	outline_hovered_cube();
}

void Camera::post_process() {
	quad_vao.bind();
	texture_color_buffer.set_slot(1);
	texture_color_buffer.bind();
	depth_texture.activate();
	depth_texture.bind();
	glDrawArrays(GL_TRIANGLES, 0, quad_vertices.size());
}

/*
	draws outline of a hovered block
*/
void Camera::outline_hovered_cube() {
	if (hovered_block == nullptr) return;
	//sends cube model and outline color to the outline shader
	mat4 cube_model = translate(mat4(1.0), hovered_block->position);
	sm.outline_shader.set_uniform_mat4f("cube_model", 1, GL_FALSE, cube_model);
	sm.outline_shader.set_uniform_4f("color", 1, hovered_block_outline_color);
	//draw cube outline
	glDisable(GL_DEPTH_TEST); //keeps the cube outline visible
	glLineWidth(outline_thickness);
	glDrawArrays(GL_LINES, 0, cube_edges.size()); //first: index where vertices are inserted, counts: # of vertices to draw the line(s)
	glEnable(GL_DEPTH_TEST);
}

/*
	handles placing and breaking block
*/
void Camera::block_interaction() {
	if (hovered_block == nullptr) return;
	
	Chunk* chunk = cm.get_chunk(hovered_block->position);
	ivec3 local_coord = world_to_local_coord(hovered_block->position);

	if (glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (holding_block_type != none && hovered_block->type != holding_block_type && (hovered_block->type == none || hovered_block->type == water)) {
			if (is_nonblock(holding_block_type)) {
				if (hovered_block->type == water && !can_be_placed_underwater(holding_block_type)) {
					return;
				}
				else {
					sg.spawn_nonblock_structure(holding_block_type, hovered_block->position);
					chunk->should_rebuild = true;
				}
			}
			else {
				cm.set_block_manual(chunk->id, local_coord, holding_block_type); //TOOD: selection of block type, inventory 
			}
			audio::play_block_sound_effect(holding_block_type);
		}
	}
	if (glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (hovered_block->type != none) {
			holding_block_type = hovered_block->type;
			if (is_nonblock(hovered_block->type)) {
				chunk->remove_structure(local_coord); 
			}
			audio::play_block_sound_effect(hovered_block->type);
			cm.set_block_manual(chunk->id, local_coord, none);
		}
	}
}

/*
	updates direction vector based on mouse pitch and yaw, converted from 2D mouse coordinates
*/
void Camera::update_mouse() {
	glfwGetCursorPos(window_setting->window, &mouse_xpos, &mouse_ypos);
	//prevents sudden view jump when window_setting->window re-focused
	
	if (window_refocused) {
		mouse_xpos = last_xpos;
		mouse_ypos = last_ypos;
		window_refocused = false;
	}
	
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

/*
	handles physical movements of the player
*/
void Camera::update_movement(float dt) {
	vec3 right = normalize(cross(direction, vec3(0.0, 1.0, 0.0)));
	up = normalize(cross(right, direction));
	vec3 input_dir = vec3(0.0);

	if (glfwGetKey(window_setting->window, GLFW_KEY_W) == GLFW_PRESS) {
		input_dir += direction;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_S) == GLFW_PRESS) {
		input_dir -= direction;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_A) == GLFW_PRESS) {
		input_dir -= right;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_D) == GLFW_PRESS) {
		input_dir += right;
	}

	if (glfwGetKey(window_setting->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		input_dir += up;
	}
	if (glfwGetKey(window_setting->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
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
			if (glfwGetKey(window_setting->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
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

/*
	casts a ray to detect a block, updating hovered_block_type
*/
void Camera::raycast() {
	vec3 origin = position;
	vec3 dir = direction;
	vec3 delta = { //unit step size in x, z, and y axis
		abs(1.0f / dir.x),
		abs(1.0f / dir.y),
		abs(1.0f / dir.z),
	};
	vec3 ray_length, step;
	vec3 current = floor(origin);
	if (dir.x < 0) {
		step.x = -1;
		ray_length.x = (origin.x - current.x) * delta.x; // dist with neighbor
	}
	else {
		step.x = 1;
		ray_length.x = (current.x + 1 - origin.x) * delta.x;
	}
	if (dir.y < 0) {
		step.y = -1;
		ray_length.y = (origin.y - current.y) * delta.y;
	}
	else {
		step.y = 1;
		ray_length.y = (current.y + 1 - origin.y) * delta.y;
	}
	if (dir.z < 0) {
		step.z = -1;
		ray_length.z = (origin.z - current.z) * delta.z;
	}
	else {
		step.z = 1;
		ray_length.z = (current.z + 1 - origin.z) * delta.z;
	}

	float dist = 0.0f;
	while (dist < max_ray_length) {
		Block* block = cm.get_block_worldspace(current);
		if (block != nullptr) {
			hovered_block = block;
			if (block->type != none) {
				return;
			}
		}

		//increment in the direction that ray_length is shorter
		if (ray_length.x < ray_length.y) {
			if (ray_length.x < ray_length.z) {
				//horizontal step in x-axis
				current.x += step.x;
				dist = ray_length.x;
				ray_length.x += delta.x;
			}
			else {
				//horizontal step in z-axis
				current.z += step.z;
				dist = ray_length.z;
				ray_length.z += delta.z;
			}
		}
		else {
			if (ray_length.z < ray_length.y) {
				//horizontal step in z-axis
				current.z += step.z;
				dist = ray_length.z;
				ray_length.z += delta.z;
			}
			else {
				//vertical step in y-axis
				current.y += step.y;
				dist = ray_length.y;
				ray_length.y += delta.y;
			}
		}
	}
}

/*
	handles the player's collision with blocks (AABB collision check)
*/
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