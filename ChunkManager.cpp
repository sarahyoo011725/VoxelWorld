#include "ChunkManager.h"

bool ChunkManager::chunk_exists(ivec2 chunk_coord) {
	return chunks.find(chunk_coord) != chunks.end();
}

Chunk* ChunkManager::get_chunk(ivec2 chunk_coord) {
	if (chunk_exists(chunk_coord)) {
		return &chunks.at(chunk_coord);
	}
	return nullptr;
}

Block* ChunkManager::get_block_worldspace(vec3 world_coord) {
	ivec2 chunk_coord = {
		static_cast<int> (world_coord.x / chunk_size),
		static_cast<int> (world_coord.z / chunk_size)
	};
	Chunk* chunk = get_chunk(chunk_coord);
	if (chunk == nullptr) {
		cout << "Chunk has not been created." << endl;
		return nullptr;
	}
	int local_x = world_coord.x - chunk_coord.x;
	int local_y = world_coord.y;
	int local_z = world_coord.z - chunk_coord.y;
	return chunk->get_block(local_x, local_y, local_z);
}

void ChunkManager::create_chunk(ivec2 chunk_coord) {
	vec3 world_coord = vec3(chunk_coord.x * chunk_size, 0, chunk_coord.y * chunk_size);
	Chunk new_chunk = Chunk(world_coord);
	chunks.insert({ chunk_coord, new_chunk });
}