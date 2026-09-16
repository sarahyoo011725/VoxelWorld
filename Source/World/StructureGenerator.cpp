#include "StructureGenerator.h"
#include "Chunk/Chunk.h"

StructureGenerator::StructureGenerator() : chunk_manager(ChunkManager::get_instance()) {
	terrain_structures = {
		{ 60, [this](vec3 c) { spawn_tree(c); } },
		{ 10, [this](vec3 c) { spawn_grass(c); } },
	};
	placeable_structures = {
		{ grass, [this](vec3 c) { spawn_grass(c); } },
	};
}

/*
	looks up the structure registered for a block type and spawns it
*/
void StructureGenerator::spawn_nonblock_structure(block_type type, vec3 world_coord) {
	const auto& rule = placeable_structures.find(type);
	if (rule != placeable_structures.end()) {
		rule->second(world_coord);
	}
}

/*
	spawns grass at a world coordinate.
	grass is non-block geometry
*/
void StructureGenerator::spawn_grass(vec3 world_coord) {
	//draw grass
	ivec2 chunk_id = get_chunk_origin(world_coord);
	Chunk* chunk = chunk_manager.get_chunk(chunk_id);
	if (chunk != nullptr) {
		ivec3 local_coord = world_to_local_coord(world_coord);
		Block* block = chunk->get_block(local_coord);
		if (block == nullptr || block != nullptr && block->type != none) return; //spawn grass only if there is no structure

		vector<vertex> transformed_vertices;
		for (vector<vertex> face : grass_face_vertices) {
			for (int i = 0; i < face.size(); ++i) {
				vec3 position = face[i].position + world_coord;
				vec2 uv_coord = convert_to_uv(i, grass_text_coord);
				transformed_vertices.push_back({ position, uv_coord });
			}
		}
		chunk->add_nonblock_structure_vertices(local_coord, transformed_vertices); //grass has transparency
		chunk->set_block(local_coord, grass);
	}
}

/*
	spawns a tree structure at a world coordinate.
	a tree is made of blocks
*/
void StructureGenerator::spawn_tree(vec3 world_coord) {
	//checks if there are any trees 1 block away.
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			int wx = world_coord.x + dx;
			int wz = world_coord.z + dz;
			Block* block = chunk_manager.get_block_worldspace(vec3(wx, world_coord.y, wz));
			if (block == nullptr || block != nullptr && block->type == wood) {
				return;
			} 
		}
	}

	//make a stem, height varies a bit so trees aren't all identical
	int trunk_height = 4 + rand() % 3;
	for (int h = 0; h < trunk_height; ++h) {
		chunk_manager.set_block_worldspace(vec3(world_coord.x, world_coord.y + h, world_coord.z), wood);
	}

	//add leaves
	int r = rand() % 2;
	block_type leaf_type = (r == 0) ? leaf_red : leaf_yellow;
	float layer_radius[4] = { 2.2f, 2.2f, 2.0f, 1.6f };
	for (int dx = -2; dx <= 2; ++dx) {
		for (int dz = -2; dz <= 2; ++dz) {
			for (int dy = 0; dy < 4; ++dy) {
				if (dx == 0 && dz == 0 && dy < 2) continue;

				float dist = sqrt((float)(dx * dx + dz * dz));
				if (dist > layer_radius[dy]) continue;
				if (dist > layer_radius[dy] - 0.5f && rand() % 5 == 0) continue;

				int wx = world_coord.x + dx;
				int wy = world_coord.y + trunk_height - 2 + dy;
				int wz = world_coord.z + dz;
				chunk_manager.set_block_worldspace(vec3(wx, wy, wz), leaf_type);
			}
		}
	}

	chunk_manager.set_block_worldspace(vec3(world_coord.x, world_coord.y + trunk_height - 2 + 4, world_coord.z), leaf_type);
}
