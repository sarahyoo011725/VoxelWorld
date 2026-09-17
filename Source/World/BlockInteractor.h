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
	//the world coords the raycast found them at - a Block no longer stores its own
	vec3 hovered_position = vec3(0.0f);
	vec3 placement_position = vec3(0.0f);
	Inventory inventory;
	void update(vec3 origin, vec3 direction);
private:
	void raycast(vec3 origin, vec3 direction);
	void interact();
	void handle_scroll();
	void handle_drop();

	ChunkManager& cm;
	StructureGenerator& sg;
	WindowSetting* window_setting;
	const float max_ray_length = 3.0f;
};
