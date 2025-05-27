#include "Terrain.h"

Terrain::Terrain(vec3 &cam_pos) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()) {
	player_pos = &cam_pos;
}

void Terrain::update() {
	//player's pos in chunk space
	origin = {
		static_cast<int> (floor(player_pos->x / chunk_size)),
		static_cast<int> (floor(player_pos->z / chunk_size)),
	};

	//render chunks around player's pos up to the render dist
	for (int x = origin.x - render_dist; x <= origin.x + render_dist; ++x) {
		for (int z = origin.y - render_dist; z <= origin.y + render_dist; ++z) {
			ivec2 chunk_origin = { x, z };
			Chunk* chunk = cm.get_chunk(chunk_origin);
			if (chunk != nullptr) { 
				visible_chunks.push_back(chunk);
			}
			else {
				cm.create_chunk(chunk_origin);
			}
		}
	}

	//TODO: fix mesh crossing chunk
	for (Chunk* c : visible_chunks) {
		if (c->has_built) continue;
		spawn_structures(c);
		c->build_chunk();
		c->bind_buffers();
	}

	for (Chunk* c : visible_chunks) {
		c->draw_opaque_blocks();
	}
	//TODO: sort transparent geometries and draw them in order from furthest to closest to player
	for (Chunk* c : visible_chunks) {
		c->draw_transparent_blocks();
	}

	visible_chunks.clear();
}

void Terrain::spawn_structures(Chunk* chunk) {
	for (int x = 1; x < chunk_size - 1; ++x) {
		for (int z = 1; z < chunk_size - 1; ++z) {
			//convert local coords into world coord
			int wx = chunk->position.x + x;
			int wz = chunk->position.z + z;
			int h = chunk->height_map[x][z];
			//ensure to spawn tree on ground
			int r = rand() % 50;
			if (h <= water_level || r != 0) continue;
			sg.spawn_tree(vec3(wx, h + 1, wz));
		}
	}
}