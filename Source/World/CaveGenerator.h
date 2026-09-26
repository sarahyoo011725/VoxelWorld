#pragma once
#include <FastNoise/FastNoiseLite.h>
#include <string>
#include <vector>
#include "Block/Block.h"
#include "World/TerrainGenerator.h"

/*
	every tunable of cave generation. noise values are in FastNoiseLite's
	-1..1 range; heights are in blocks.
*/
struct CaveConfig {
	bool enabled = true;

	int floor_y = 3; //nothing at or below this is carved, so bedrock is never exposed from below
	int roof_thickness = 5; //rock kept between a tunnel and the lowest surface or water bed around it
	int cavern_roof_thickness = 9; //caverns are wide, so they are held deeper to keep their ceilings intact

	//tunnels form where two independent fields are both near zero, which traces winding tubes
	float tunnel_frequency = 0.016f; //higher = more tunnels, shorter and twistier
	float tunnel_radius = 0.095f; //tunnel width; ~0.06 is a crawlway, ~0.12 a wide passage
	float tunnel_depth_growth = 0.5f; //extra width at the bottom of the world, as a fraction of tunnel_radius
	float tunnel_flatten = 1.4f; //stretches the field vertically so tunnels wander level instead of plunging

	//caverns are the high points of one smooth field
	float cavern_frequency = 0.011f; //lower = bigger, further-apart caverns
	float cavern_threshold = 0.64f; //higher = rarer and smaller caverns
	float cavern_depth_bonus = 0.22f; //threshold drop at the bottom of the world, so deep caverns are more common and larger
	float cavern_flatten = 2.2f; //squashes caverns into wide chambers with walkable floors

	//tunnels may only break the surface inside these rare patches
	float entrance_frequency = 0.009f;
	float entrance_threshold = 0.88f; //0..1, higher = fewer openings
};

/*
	carves caves out of already-filled terrain. every decision is a pure function
	of world position and seed, so neighbouring chunks agree on their shared
	border without either one seeing the other, and chunks can be carved on any
	thread in any order.

	noise is sampled on a coarse world-aligned lattice and interpolated rather
	than evaluated per block: the fields are smooth enough that this is visually
	the same, at roughly 1/30 of the cost of sampling every block.
*/
class CaveGenerator {
public:
	explicit CaveGenerator(int world_seed, int sea_level, const CaveConfig& config = CaveConfig());

	/*
		blocks is indexed (x * height + y) * length + z and heights x * length + z,
		with local (0, 0) at world (origin_x, origin_z). neighbour heights outside
		the region are sampled from terrain.
	*/
	void carve(std::vector<Block>& blocks, const std::vector<int>& heights,
		int origin_x, int origin_z, int width, int height, int length,
		const TerrainGenerator& terrain) const;

	//horizontal slices, a vertical cut and a stats file for a size x size area, through the same carve() the game uses
	void export_debug_slices(const std::string& path_prefix, const TerrainGenerator& terrain, int center_x, int center_z, int size) const;

	const CaveConfig config;
	const int sea_level;

private:
	static const int cell = 4;

	FastNoiseLite tunnel_noise_a;
	FastNoiseLite tunnel_noise_b;
	FastNoiseLite cavern_noise;
	FastNoiseLite entrance_noise;
};

const CaveGenerator& get_cave_generator();
