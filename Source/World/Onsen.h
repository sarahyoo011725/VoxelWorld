#pragma once
#include <glm/glm.hpp>
#include <vector>

using namespace glm;

class Chunk;

//a Japanese hot spring built into a snowy peak: where its pool is, and the level of the terrace around it
struct OnsenSite {
	ivec3 centre = ivec3(0);
};

/*
	onsen are placed from the world seed alone: the world is cut into cells, and a
	cell holds one if a spot in it lands on a snowy peak and a roll lets it. any
	chunk can therefore work out which onsen overlap it and build just its own
	part, so a chunk unloaded and regenerated gets its part back whatever its
	neighbours are doing
*/
namespace onsen {
	const int cell_size = 192;
	const int radius = 10; //half the width of the terrace

	bool site_in_cell(ivec2 cell, OnsenSite& site);
	std::vector<OnsenSite> sites_near(vec3 position, float range);
	bool nearest(vec3 position, float range, OnsenSite& site);
	//writes the blocks of every onsen that overlaps this newly generated chunk, inside it only
	void build_in_chunk(Chunk* chunk);
}
