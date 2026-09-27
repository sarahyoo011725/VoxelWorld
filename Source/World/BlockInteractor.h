#pragma once
#include "ChunkManager.h"
#include "StructureGenerator.h"
#include "WindowSetting.h"
#include "Entity/Inventory.h"

/*
* raycasts for the hovered block and handles placing/breaking it
*/
class BlockInteractor
{
public:
	BlockInteractor(WindowSetting* setting);
	Block* hovered_block = nullptr;
	Block* placement_block = nullptr;
	vec3 hovered_position = vec3(0.0f);
	vec3 placement_position = vec3(0.0f);
	Inventory inventory;
	void update(vec3 origin, vec3 direction);
	//treats the mouse buttons as already held, so a click used for something else (getting on or off a mount) doesn't also place or break
	void consume_clicks() { left_click_was_down = right_click_was_down = true; }
	float reach() const { return max_ray_length; }
	//blocks past this distance can't be hovered, e.g. behind a mob in the way
	float reach_limit = 4.5f;
private:
	void raycast(vec3 origin, vec3 direction);
	void interact();
	void handle_scroll();
	void handle_drop();

	ChunkManager& cm;
	StructureGenerator& sg;
	WindowSetting* window_setting;
	const float max_ray_length = 4.5f;

	bool left_click_was_down = false;
	bool right_click_was_down = false;
};
