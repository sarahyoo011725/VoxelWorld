#include "Terrain.h"

Terrain::Terrain(vec3 &cam_pos) {
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
			if (chunks.find(coord) != chunks.end()) { 
				visible_chunks.push_back(&chunks.at(coord));
			}
			else {
				create_chunk(coord);
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

void Terrain::create_chunk(ivec2 chunk_coord) {
	vec3 world_pos = { chunk_coord.x * chunk_size, 0, chunk_coord.y * chunk_size };
	Chunk new_chunk = Chunk(world_pos);
	chunks.insert({ chunk_coord, new_chunk });
}