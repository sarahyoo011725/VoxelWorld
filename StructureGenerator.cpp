#include "StructureGenerator.h"
#include "Chunk.h"

void StructureGenerator::spawn_tree(vec3 world_coord) {
	//checks if there are any trees 1 block away.
	ivec2 offsets[] = { {0, 0}, {-1, 0}, {-1, -1}, {-1, 1}, {1, 0}, {1, -1}, {1, 1}, {0, -1}, {0, 1} };
	for (ivec2 off : offsets) {
		int wx = world_coord.x + off.x;
		int wz = world_coord.z + off.y;
		Block* block = cm.get_block_worldspace(vec3(wx, world_coord.y, wz));
		if (block != nullptr && block->type == trunk) {
			return;
		} 
	}
	//make a stem of height 5
	for (int h = 0; h < 5; ++h) {
		cm.set_block_worldspace(vec3(world_coord.x, world_coord.y + h, world_coord.z), trunk);
	}

	//add leaves
	int r = rand() % 2;
	block_type leaf_type = (r == 0) ? leaf_red : leaf_yellow;
	for (int dx = -2; dx <= 2; ++dx) {
		for (int dz = -2; dz <= 2; ++dz) {
			for (int dy = 0; dy < 4; ++dy) {
				if (dx == 0 && dz == 0 && dy < 2) continue;
				int wx = world_coord.x + dx;
				int wy = world_coord.y + 3 + dy; //add leaves 3 trunks above
				int wz = world_coord.z + dz;
				cm.set_block_worldspace(vec3(wx, wy, wz), leaf_type);
			}
		}
	}
}
