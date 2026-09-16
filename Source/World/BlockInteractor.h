#pragma once
#include "ChunkManager.h"
#include "StructureGenerator.h"
#include "WindowSetting.h"

/*
* raycasts for the hovered block and handles placing/breaking it
*/
class BlockInteractor
{
public:
	BlockInteractor(WindowSetting* setting);
	Block* hovered_block = nullptr;
	void update(vec3 origin, vec3 direction);
private:
	void raycast(vec3 origin, vec3 direction);
	void interact();

	ChunkManager& cm;
	StructureGenerator& sg;
	WindowSetting* window_setting;
	const float max_ray_length = 3.0f;
	block_type holding_block_type = none;
};
