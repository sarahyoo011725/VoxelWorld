#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <FastNoise/FastNoiseLite.h>
#include "unordered_map"
#include "Chunk/Chunk.h"
#include "Block/Block.h"

using namespace std;
using namespace glm;

/*
There are two types of coords this engine: world and chunk coord.
- world coord is a coordinate (or position) in the world space and is equal, absolute to all objects in the game world.
- chunk coord is a coordinate (or position) in a chunk space that is relative to a single chunk. Hence, the coord is invalid in other chunks.
	--> chunk coord = world_coord / chunk_size
- chunk origin is a vec3 of ivec2 chunk origin, with only y being 0 by default --> so, chunk_origin = vec3(chunk_id.x, 0, chunk_id.y);
- local coord is the index of a block in a chunk.
*/

struct block_data {
	ivec3 local_coord;
	block_type type;
};

class ChunkManager
{
private:
	ChunkManager() {};
	//prevents creating copies
	ChunkManager(const ChunkManager&) = delete;
	ChunkManager& operator=(const ChunkManager&) = delete;
public:
	static ChunkManager& get_instance() {
		static ChunkManager instance;
		return instance;
	}
	unordered_map<ivec2, vector<block_data>> unloaded_blocks;
	unordered_map<ivec2, Chunk> chunks;
	bool set_block_manual(ivec2 chunk_id, ivec3 local_coord, block_type type);
	bool set_block_worldspace(vec3 world_coord, block_type type);
	Block* get_block_worldspace(vec3 world_coord);
	Chunk* get_chunk(vec3 world_coord);
	Chunk* get_chunk(ivec2 chunk_id);
	bool chunk_exists(ivec2 chunk_id);
	Chunk* create_chunk(ivec2 chunk_id);
};

namespace {
	static FastNoiseLite m_noise;
	const static int chunk_size = 16;
	const static int water_level = 10;

	float get_noise(int x, int z) {
		return m_noise.GetNoise((float)x, (float)z);
	}
	ivec2 get_chunk_origin(vec3 world_coord) {
		ivec2 chunk_origin = {
			floor(world_coord.x / chunk_size),
			floor(world_coord.z / chunk_size)
		};
		return chunk_origin;
	}
	ivec3 world_to_chunk_coord(vec3 world_coord) {
		return {
			floor(world_coord.x / chunk_size),
			floor(world_coord.y / chunk_size),
			floor(world_coord.z / chunk_size)
		};
	}
	vec3 chunk_to_world_coord(ivec3 chunk_coord) {
		return chunk_coord * chunk_size;
	}
	vec3 local_to_world_coord(ivec3 block_index, vec3 chunk_pos) {
		return (vec3)block_index + chunk_pos;
	}
	//return range: from 0 to 15 (which is chunk_size - 1)
	ivec3 world_to_local_coord(vec3 world_coord) {
		ivec2 chunk_origin = get_chunk_origin(world_coord);
		ivec3 local_coord  = {
			static_cast<int> (floor(world_coord.x - chunk_origin.x * 16 )), 
			floor(world_coord.y),
			static_cast<int> (floor(world_coord.z - chunk_origin.y * 16 ))
		};
		return local_coord;
	}
}