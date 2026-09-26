#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Block/Block.h"

/*
	light given off by blocks such as lava and glowstone. it starts at max_block_light
	in the glowing block and loses one level per step through see-through blocks, so
	it reaches max_block_light - 1 blocks away, which is less than a chunk. a chunk's
	light therefore depends only on its own 3x3 neighbourhood.
*/
const int max_block_light = 15;

//interior cells (local 1..width-2 / 1..length-2) holding a glowing block
void find_emitters(const std::vector<Block>& blocks, int width, int height, int length, std::vector<glm::ivec3>& emitters);

//one chunk of a 3x3 neighbourhood; blocks is null where that chunk is not loaded, which reads as solid rock
struct LightNeighbour {
	const std::vector<Block>* blocks = nullptr;
	const std::vector<glm::ivec3>* emitters = nullptr;
};

/*
	fills light (indexed like the centre chunk's blocks, border included) with
	block light for neighbourhood[4], the centre of a 3x3 laid out
	[(dx + 1) * 3 + (dz + 1)]. every block is read from the chunk that owns it,
	never from a border copy, so two chunks agree on the light where they meet.
	returns false, leaving light untouched, when no glowing block is near enough.
*/
bool compute_block_light(const LightNeighbour neighbourhood[9], int width, int height, int length, std::vector<unsigned char>& light);
