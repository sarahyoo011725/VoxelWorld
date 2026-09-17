#include "BlockInteractor.h"
#include "Audio/AudioManager.h"

BlockInteractor::BlockInteractor(WindowSetting* setting)
	: cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()), window_setting(setting) {}

void BlockInteractor::update(vec3 origin, vec3 direction) {
	handle_scroll();
	handle_drop();
	raycast(origin, direction);
	interact();
}

/*
	consumes the accumulated scroll delta to cycle the selected hotbar slot
*/
void BlockInteractor::handle_scroll() {
	if (window_setting->scroll_delta_y > 0.0) {
		inventory.scroll_select(-1);
	}
	else if (window_setting->scroll_delta_y < 0.0) {
		inventory.scroll_select(1);
	}
	window_setting->scroll_delta_y = 0.0;
}

/*
	Q clears the selected slot
*/
void BlockInteractor::handle_drop() {
	if (glfwGetKey(window_setting->window, GLFW_KEY_Q) == GLFW_PRESS) {
		inventory.drop_selected();
	}
}

/*
	casts a ray to detect a block, updating hovered_block (the solid block hit, for
	breaking) and placement_block (the empty cell just before it, for placing - a
	solid hovered_block almost never has type none/water itself, so placement must
	target its neighbor, not the hit block)
*/
void BlockInteractor::raycast(vec3 world_origin, vec3 direction) {
	//blocks are centered on integer coords; get_block_worldspace() floors raw world coords
	vec3 origin = world_origin + vec3(0.5f);
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
	Block* previous = nullptr;
	vec3 previous_position = vec3(0.0f);
	while (dist < max_ray_length) {
		Block* block = cm.get_block_worldspace(current);
		if (block != nullptr) {
			hovered_block = block;
			hovered_position = floor(current);
			if (block->type != none) {
				placement_block = previous;
				placement_position = previous_position;
				return;
			}
			previous = block;
			previous_position = floor(current);
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
	placement_block = nullptr; //nothing solid within reach to place against
}

/*
	handles placing and breaking block
*/
void BlockInteractor::interact() {
	if (hovered_block == nullptr) return;

	Chunk* chunk = cm.get_chunk(hovered_position);
	ivec3 local_coord = world_to_local_coord(hovered_position);

	block_type holding_block_type = inventory.selected_type();

	if (glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		bool target_is_hovered = hovered_block->type == water;
		Block* target = target_is_hovered ? hovered_block : placement_block;
		vec3 target_position = target_is_hovered ? hovered_position : placement_position;

		if (target != nullptr && holding_block_type != none && target->type != holding_block_type) {
			Chunk* target_chunk = cm.get_chunk(target_position);
			ivec3 target_local_coord = world_to_local_coord(target_position);

			if (is_nonblock(holding_block_type)) {
				if (target->type == water && !can_be_placed_underwater(holding_block_type)) {
					return;
				}
				else {
					sg.spawn_nonblock_structure(holding_block_type, target_position);
					target_chunk->should_rebuild = true;
				}
			}
			else {
				cm.set_block_manual(target_chunk->id, target_local_coord, holding_block_type);
			}
			audio::play_block_sound_effect(holding_block_type);
		}
	}
	if (glfwGetMouseButton(window_setting->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (hovered_block->type != none) {
			inventory.add_item(hovered_block->type);
			if (is_nonblock(hovered_block->type)) {
				chunk->remove_structure(local_coord);
			}
			audio::play_block_sound_effect(hovered_block->type);
			cm.set_block_manual(chunk->id, local_coord, none);
		}
	}
}
