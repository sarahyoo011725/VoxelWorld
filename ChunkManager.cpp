#include "ChunkManager.h"

bool ChunkManager::chunk_exists(ivec2 chunk_origin) {
	return chunks.find(chunk_origin) != chunks.end();
}

Chunk* ChunkManager::get_chunk(ivec2 chunk_origin) {
	if (chunk_exists(chunk_origin)) {
		return &chunks.at(chunk_origin);
	}
	return nullptr;
}

void ChunkManager::set_block_worldspace(vec3 world_coord, block_type type) {
	ivec2 chunk_origin = get_chunk_origin(world_coord);
	Chunk* chunk = get_chunk(chunk_origin);
	if (chunk == nullptr) return;

	ivec3 local_coord = world_to_local_coord(world_coord);
	chunk->set_block(local_coord, type);	
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

void ChunkManager::create_chunk(ivec2 chunk_origin) {
	vec3 world_coord = vec3(chunk_origin.x * chunk_size, 0, chunk_origin.y * chunk_size);
	Chunk new_chunk = Chunk(world_coord);
	chunks.insert({ chunk_origin, new_chunk });
}