#include "ChunkManager.h"

bool ChunkManager::chunk_exists(ivec2 chunk_origin) {
	return chunks.find(chunk_origin) != chunks.end();
}

Chunk* ChunkManager::get_chunk(ivec2 chunk_origin) {
	auto e = chunks.find(chunk_origin);
	if (e != chunks.end()) {
		return &e->second;
	}
	return nullptr;
}

void ChunkManager::set_block_worldspace(vec3 world_coord, block_type type) {
	ivec2 chunk_origin = get_chunk_origin(world_coord);
	ivec3 local_coord = world_to_local_coord(world_coord);
	Chunk* chunk = get_chunk(chunk_origin);
	
	if (chunk == nullptr) {
		//draw the block later once the chunk is created
		unloaded_blocks[chunk_origin].push_back({ local_coord, type });
	}
	else {
		chunk->set_block(local_coord, type);	
		chunk->should_rebuild = true;
	}

	//TODO: fix structure generation across chunks edge issue
	if (local_coord.x == 0 || local_coord.x == 1 || local_coord.x == 15 || local_coord.x == 16 ||
			local_coord.z == 0 || local_coord.z == 1 || local_coord.z == 15 || local_coord.z == 16) {
		ivec2 offset = ivec2(0.0);
		offset.x = (local_coord.x == 0 || local_coord.x == 1) ? -1 : (local_coord.x == 15 || local_coord.x == 16) ? 1 : 0;
		offset.y = (local_coord.z == 0 || local_coord.z == 1) ? -1 : (local_coord.z == 15 || local_coord.z == 16) ? 1 : 0;

		//if (offset.x != 0 && offset.y != 0) return;

		ivec2 adj_id = chunk_origin + offset;

		ivec3 adj_local_coord;
		adj_local_coord.x = (local_coord.x == 0) ? 15 : (local_coord.x == 1) ? 16 : (local_coord.x == 15) ? 0 : (local_coord.x == 16) ? 1 : local_coord.x;
		adj_local_coord.y = local_coord.y;
		adj_local_coord.z = (local_coord.z == 0) ? 15 : (local_coord.z == 1) ? 16 : (local_coord.z == 15) ? 0 : (local_coord.z == 16) ? 1 : local_coord.z;

		auto adj = chunks.find(adj_id);
		if (adj != chunks.end()) {
			adj->second.set_block(adj_local_coord, type);
			adj->second.should_rebuild = true;
		}
		else {
			unloaded_blocks[adj_id].push_back({ adj_local_coord, type });
		}
	}
	
}

Block* ChunkManager::get_block_worldspace(vec3 world_coord) {
	ivec2 chunk_origin = get_chunk_origin(world_coord);
	Chunk* chunk = get_chunk(chunk_origin);
	if (chunk == nullptr) {
		return nullptr;
	}
	ivec3 local_coord = world_to_local_coord(world_coord);
	return chunk->get_block(local_coord);
}

Chunk* ChunkManager::create_chunk(ivec2 chunk_origin) {
	Chunk new_chunk = Chunk(chunk_origin);
	auto inserted = chunks.insert({chunk_origin, new_chunk});
	return &(inserted.first->second);
}