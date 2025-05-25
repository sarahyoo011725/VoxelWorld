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

//a singleton class for manaiging chunks
class ChunkManager
{
protected:
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
	vec3 convert_to_chunk_coord(vec3 world_coord) {
		return world_coord / (float)chunk_size;
	}
	vec3 convert_to_world_coord(vec3 chunk_coord) {
		return chunk_coord * (float)chunk_size;
	}
	vec3 convert_to_block_index(vec3 chunk_coord, vec3 chunk_pos) {
		return chunk_coord - chunk_pos;
	}
	vec3 convert_to_chunk_coord(vec3 block_index, vec3 chunk_pos) {
		return block_index + chunk_pos;
	}
}