#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <FastNoise/FastNoiseLite.h>
#include "unordered_map"
#include "Chunk.h"
#include "Block.h"

using namespace std;
using namespace glm;

/*
There are two types of coords this engine: world and chunk coord.
- world coord is a coordinate (or position) in the world space and is equal, absolute to all objects in the game world.
- chunk coord is a coordinate (or position) in a chunk space that is relative to a single chunk. Hence, the coord is incorrect to other chunks.
** chunk coord == world_coord / chunk_size
- chunk pos is a vec3 made from ivec2 chunk origin, with only y being 0 by default, for every chunks newly created.	
  Hence, ivec2(chunk_pos.x, chunk_pos.z) is the unique id of a chunk.
- local coord is the index of a block in a chunk.
*/

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
	unordered_map<ivec2, Chunk> chunks;
	void set_block_worldspace(vec3 world_coord, block_type type);
	Block* get_block_worldspace(vec3 world_coord);
	Chunk* get_chunk(ivec2 chunk_coord);
	bool chunk_exists(ivec2 chunk_coord);
	void create_chunk(ivec2 chunk_coord);
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
			static_cast<int> (floor(world_coord.x / chunk_size)),
			static_cast<int> (floor(world_coord.z / chunk_size))
		};
		return chunk_origin;
	}
	ivec3 world_to_chunk_coord(vec3 world_coord) {
		return {
			static_cast<int> (floor(world_coord.x / chunk_size)),
			static_cast<int> (floor(world_coord.y / chunk_size)),
			static_cast<int> (floor(world_coord.z / chunk_size))
		};
	}
	vec3 chunk_to_world_coord(ivec3 chunk_coord) {
		return chunk_coord * chunk_size;
	}
	ivec3 chunk_to_local_coord(ivec3 chunk_coord, vec3 chunk_pos) {
		return {
			chunk_coord.x - chunk_pos.x,
			chunk_coord.y - chunk_pos.y,
			chunk_coord.z - chunk_pos.z
		};
	}
	vec3 local_to_chunk_coord(ivec3 block_index, vec3 chunk_pos) {
		return (vec3)block_index + chunk_pos;
	}
	
	//return range: from 0 to 15 (which is chunk_size - 1)
	ivec3 world_to_local_coord(vec3 world_coord) {
		int wx = floor(world_coord.x);
		int wz = floor(world_coord.z);
		ivec3 local_coord = ivec3(wx % chunk_size, world_coord.y, wz % chunk_size);
		 //cout << "lx: " << local_coord.x << ", lz: " << local_coord.z << endl;
		if (local_coord.x < 0) local_coord.x += chunk_size;
		if (local_coord.z < 0) local_coord.z += chunk_size;
		return local_coord;
	}
}