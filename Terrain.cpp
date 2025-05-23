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
			ivec2 chunk_coord = { x, z };
			if (chunks.find(chunk_coord) != chunks.end()) { //chunk found
				Chunk c = chunks.at(chunk_coord);
				if (c.active) {
					c.render(); //no need to remove chunks since we draw only active chunk lol
				}
			}
			else { 
				create_chunk(chunk_coord);
			}
		}
	}
}

void Terrain::create_chunk(ivec2 chunk_coord) {
	vec3 world_pos = { chunk_coord.x * chunk_size, 0, chunk_coord.y * chunk_size };
	Chunk new_chunk = Chunk(world_pos);
	chunks.insert({ chunk_coord, new_chunk });
}