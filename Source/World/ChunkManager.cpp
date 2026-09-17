#include "ChunkManager.h"

/*
	checks if a chunk has been created with an id (= chunk_coord.x, chunk_coord.z)
*/
bool ChunkManager::chunk_exists(ivec2 chunk_id) {
	return chunks.find(chunk_id) != chunks.end();
}

/*
	creates a new chunk, providing it with an id.
	constructs the Chunk directly inside the map (try_emplace) instead of
	building a temporary and copying it in - Chunk owns raw block/height-map
	storage and GL buffer handles that a shallow copy would alias, not duplicate.
*/
Chunk* ChunkManager::create_chunk(ivec2 chunk_id) {
	auto inserted = chunks.try_emplace(chunk_id, chunk_id);
	return &(inserted.first->second);
}

/*
	finds and returns the pointer of a chunk with a world coordinate
*/
Chunk* ChunkManager::get_chunk(vec3 world_coord) {
	ivec2 chunk_origin = get_chunk_origin(world_coord);
	return get_chunk(chunk_origin);
}

/*
	finds and returns the pointer of a chunk with an id
*/
Chunk* ChunkManager::get_chunk(ivec2 chunk_id) {
	const auto &e = chunks.find(chunk_id);
	if (e != chunks.end()) {
		return &e->second;
	}
	return nullptr;
}

/*
	sets the type of a block at a world coordinate
*/
bool ChunkManager::set_block_worldspace(vec3 world_coord, block_type type, bool player_edit) {
	ivec2 chunk_id = get_chunk_origin(world_coord);
	ivec3 local_coord = world_to_local_coord(world_coord);
	return set_block_manual(chunk_id, local_coord, type, player_edit);
}

/*
	remembers a change so it can be replayed if the chunk is unloaded and later
	regenerated from noise
*/
void ChunkManager::record_player_edit(ivec2 chunk_id, ivec3 local_coord, block_type type) {
	player_edits[chunk_id][local_coord] = type;
}

/*
	sets the type of a block at a local coord in a chunk of provided id
*/
bool ChunkManager::set_block_manual(ivec2 chunk_id, ivec3 local_coord, block_type type, bool player_edit) {
	Chunk* chunk = get_chunk(chunk_id);

	if (player_edit) record_player_edit(chunk_id, local_coord, type);

	if (chunk == nullptr) {
		//draw the block later once the chunk is created
		unloaded_blocks[chunk_id].push_back({ local_coord, type });
	}
	else {
		Block* block = chunk->get_block(local_coord);
		if (block == nullptr || block->type == type) {
			return false;
		}
		chunk->set_block(local_coord, type);
		if (chunk->has_built) {
			chunk->should_rebuild = true;
		}
	}

	if (local_coord.x == 1 || local_coord.x == 16) {
		ivec2 offset = ivec2(0.0);
		offset.x = (local_coord.x == 1) ? -1 : 1;

		ivec2 adj_id = chunk_id + offset;

		ivec3 adj_local_coord = local_coord;
		adj_local_coord.x = (local_coord.x == 1) ? 17 : 0;

		//the neighbour keeps a copy of this block in its border column for
		//meshing, so the edit has to be remembered there too, or a seam
		//appears when only one of the two chunks is regenerated
		if (player_edit) record_player_edit(adj_id, adj_local_coord, type);

		auto adj = chunks.find(adj_id);
		if (adj != chunks.end()) {
			adj->second.set_block(adj_local_coord, type);
			adj->second.should_rebuild = true;
		}
		else {
			unloaded_blocks[adj_id].push_back({ adj_local_coord, type });
		}
	}
	
	if (local_coord.z == 1 || local_coord.z == 16) {
		ivec2 offset = ivec2(0.0);
		offset.y = (local_coord.z == 1) ? -1 : 1;

		ivec2 adj_id = chunk_id + offset;

		ivec3 adj_local_coord = local_coord;
		adj_local_coord.z = (local_coord.z == 1) ? 17 : 0;

		if (player_edit) record_player_edit(adj_id, adj_local_coord, type);

		auto adj = chunks.find(adj_id);
		if (adj != chunks.end()) {
			adj->second.set_block(adj_local_coord, type);
			adj->second.should_rebuild = true;
		}
		else {
			unloaded_blocks[adj_id].push_back({ adj_local_coord, type });
		}
	}

	return true;
}

/*
	finds and returns the pointer of a block at a world coordinate
*/
Block* ChunkManager::get_block_worldspace(vec3 world_coord) {
	ivec2 chunk_origin = get_chunk_origin(world_coord);
	Chunk* chunk = get_chunk(chunk_origin);
	if (chunk == nullptr) {
		return nullptr;
	}
	ivec3 local_coord = world_to_local_coord(world_coord);
	return chunk->get_block(local_coord);
}