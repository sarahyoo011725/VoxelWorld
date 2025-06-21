#include "Terrain.h"

Terrain::Terrain(vec3& cam_pos) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()) {
	player_pos = &cam_pos;
}

void Terrain::update() {
	//player's pos in chunk space
	origin = {
		floor(player_pos->x / chunk_size),
		floor(player_pos->z / chunk_size),
	};

	//render chunks around player's pos up to the render dist
	for (int x = origin.x - render_dist; x <= origin.x + render_dist; ++x) {
		for (int z = origin.y - render_dist; z <= origin.y + render_dist; ++z) {
			ivec2 chunk_id = { x, z };
			Chunk* chunk = cm.get_chunk(chunk_id);
			if (!chunk) {
				chunk = cm.create_chunk(chunk_id);	
				spawn_structures(chunk);
				
				auto unloaded_c = cm.unloaded_blocks.find(chunk_id);
				if (unloaded_c != cm.unloaded_blocks.end()) {
					for (const block_data& b : unloaded_c->second) {
						chunk->set_block(b.local_coord, b.type);
					}
					cm.unloaded_blocks.erase(chunk->id);
				}
			}
			visible_chunks.push_back(chunk);
		}
	}

	//TODO: fix mesh crossing chunk
	for (Chunk* c : visible_chunks) {
		if (!c->has_built) {
			c->build_chunk();
			c->update_buffers_data();
		}
		else if (c->should_rebuild) {
			c->rebuild_chunk();
			c->update_buffers_data();
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

void Terrain::spawn_structures(Chunk* chunk) {
	for (int x = 0; x < chunk_size; ++x) {
		for (int z = 0; z < chunk_size; ++z) {
			//convert local coords into world coord
			int wx = chunk->world_position.x + x;
			int wz = chunk->world_position.z + z;
			int h = chunk->height_map[x][z];

			//do not spawn anything in water
			if (h <= water_level) continue;

			int n_tree = rand() % 60;
			int n_grass = rand() % 10;
			if (n_tree == 0) {
				sg.spawn_tree(vec3(wx, h + 1, wz));
			}
			if (n_grass == 0) {
				sg.spawn_grass(vec3(wx, h + 1, wz));
			}
		}
	}
}