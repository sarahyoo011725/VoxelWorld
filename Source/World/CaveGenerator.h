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

	//worms: long wandering tunnels that link up the noise caves. each cell of
	//worm_cell_size blocks may start one, which can run into neighbouring chunks
	int worm_cell_size = 64;
	float worm_chance = 0.45f; //share of cells that start a worm; 0 disables them
	int worm_min_length = 70, worm_max_length = 150; //in blocks travelled
	float worm_min_radius = 1.4f, worm_max_radius = 2.6f;
	int worm_min_y = 8, worm_max_y = 36; //where worms start

	//isolated pockets smaller than this are filled back in; they cost faces and cannot be explored
	int min_pocket_size = 24;

	//cave space at or below this height fills with liquid, forming lakes on deep cavern floors.
	//lava where a large-scale noise is above lava_threshold, water elsewhere
	int liquid_level = 7;
	float lava_frequency = 0.004f;
	float lava_threshold = 0.25f; //-1..1, higher = less lava

	//decoration of exposed cave rock, rolled per 4x4x4 patch so it comes in clusters
	float gravel_chance = 0.3f; //share of floor patches covered in gravel
	float moss_chance = 0.1f; //share of wall and ceiling patches that are mossy
	float moss_chance_near_liquid = 0.45f; //within a few blocks above liquid_level
	float ore_density = 1.0f; //scales every ore's vein chance

	//daylight fades over this many blocks below the ground, so caves grow dark as they go deeper
	float light_falloff = 10.0f;
	float min_light = 0.12f; //the darkest a cave gets; 0 is pitch black
};

struct CarveResult {
	bool opened_surface = false; //some surface block was carved, so sunlight reaches into the caves here
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
	CarveResult carve(std::vector<Block>& blocks, const std::vector<int>& heights,
		int origin_x, int origin_z, int width, int height, int length,
		const TerrainGenerator& terrain) const;

	//horizontal slices, a vertical cut and a stats file for a size x size area, through the same carve() the game uses
	void export_debug_slices(const std::string& path_prefix, const TerrainGenerator& terrain, int center_x, int center_z, int size) const;

	const CaveConfig config;
	const int world_seed;
	const int sea_level;

private:
	static const int cell = 4;

	FastNoiseLite tunnel_noise_a;
	FastNoiseLite tunnel_noise_b;
	FastNoiseLite cavern_noise;
	FastNoiseLite entrance_noise;
	FastNoiseLite lava_noise;
};

const CaveGenerator& get_cave_generator();
