#include "StructureGenerator.h"
#include "Chunk.h"

void StructureGenerator::spawn_grass(vec3 world_coord) {
	//draw grass
	ivec2 chunk_id = get_chunk_origin(world_coord);
	Chunk* chunk = cm.get_chunk(chunk_id);
	if (chunk != nullptr) {
		ivec3 local_coord = world_to_local_coord(world_coord);
		if (chunk->get_block(local_coord)->type == grass) return;
		vector<vertex> transformed_vertices;
		for (vector<vertex> face : grass_face_vertices) {
			for (int i = 0; i < face.size(); ++i) {
				vec3 position = face[i].position + world_coord;
				vec2 uv_coord = convert_to_uv(i, grass_text_coord);
				transformed_vertices.push_back({ position, uv_coord });
			}
		}
		chunk->add_structure_vertices(transformed_vertices, true); //grass has transparency
		chunk->set_block(local_coord, grass);
	}
}

void StructureGenerator::spawn_tree(vec3 world_coord) {
	//checks if there are any trees 1 block away.
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			int wx = world_coord.x + dx;
			int wz = world_coord.z + dz;
			Block* block = cm.get_block_worldspace(vec3(wx, world_coord.y, wz));
			if (block == nullptr || block != nullptr && block->type == trunk) {
				return;
			} 
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
