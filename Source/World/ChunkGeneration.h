#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Block/Block.h"
#include "World/Biome.h"
#include "World/CaveGenerator.h"

//sea level + tallest extreme peak, with the underground below
const int chunk_height = 112;

/*
	fills a chunk's blocks, heightmap and biome map from the world seed. arrays
	include the one-block border and are indexed like Chunk: blocks
	(x * height + y) * length + z, columns x * length + z. touches no GL and no
	shared state, so it runs on any thread and in the offline tools unchanged.
*/
CarveResult generate_chunk_blocks(glm::ivec2 chunk_id, int width, int height, int length,
	std::vector<Block>& blocks, std::vector<int>& heights, std::vector<biome_id>& biomes);

//the same without caves, for measuring what carving adds
void fill_chunk_terrain(glm::ivec2 chunk_id, int width, int height, int length,
	std::vector<Block>& blocks, std::vector<int>& heights, std::vector<biome_id>& biomes);
