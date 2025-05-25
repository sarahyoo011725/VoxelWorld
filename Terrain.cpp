#include "Terrain.h"

Terrain::Terrain(vec3 &cam_pos) : cm(ChunkManager::get_instance()) {
	player_pos = &cam_pos;
}

void Terrain::update() {
	//converts player's pos into coords on chunk
	vec3 pos = *player_pos;
	ivec2 origin = {
		static_cast<int> (pos.x / chunk_size),
		static_cast<int> (pos.z / chunk_size),
	};

	for (int x = origin.x - render_dist; x <= origin.x + render_dist; ++x) {
		for (int z = origin.y - render_dist; z <= origin.y + render_dist; ++z) {
			ivec2 coord = { x, z };
			Chunk* chunk = cm.get_chunk(coord);
			if (chunk != nullptr) { 
				visible_chunks.push_back(chunk);
			}
			else {
				cm.create_chunk(coord);
			}
		}
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
