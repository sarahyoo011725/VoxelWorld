#pragma once
#include "Block/BlockType.h"
#include <glm/glm.hpp>

using namespace glm;

//what kind of world feature occupies a column, decided by terrain generation
//before any biome is chosen
enum class terrain_feature {
	ocean,
	river,
	lake,
	land
};

enum class biome_id {
	ocean,
	beach,
	desert,
	plains,
	savanna,
	forest,
	autumn_forest,
	swamp,
	taiga,
	tundra,
	mountain,
	snowy_peak,
	count
};

/*
	the environment at a column, sampled before any biome is chosen. these are
	the only inputs biome selection is allowed to read, which keeps selection a
	pure function and stops it from depending on anything chunk-local.
*/
struct ClimateSample {
	float temperature = 0.5f; //0 = polar, 1 = tropical
	float moisture = 0.5f;    //0 = arid, 1 = swamp
	//a second, unrelated field so two biomes can share one climate cell -
	//forest and autumn forest sit in the same temperature/moisture box
	float variation = 0.5f;
	int elevation = 0;
	/*
		elevation with the fine hill/detail noise and river carving left out, so
		it describes the landform rather than every bump on it. biome thresholds
		read this: thresholding the raw height instead made columns sitting near
		a cutoff flip back and forth and speckled the map.
	*/
	int landform_elevation = 0;
	terrain_feature feature = terrain_feature::land;
};

/*
	everything the generator needs to know about a biome. adding one means
	adding an enum value and a row in biome_table() - no other file changes.
*/
struct BiomeDefinition {
	biome_id id = biome_id::plains;
	const char* name = "plains";

	block_type surface = dirt_grass;    //the top block
	block_type subsurface = dirt;       //the few blocks under it
	int subsurface_depth = 4;

	//multiplied onto the greyscale grass/leaf tiles. non-tinted blocks ignore it
	vec3 grass_tint = vec3(0.45f, 0.72f, 0.31f);
	vec3 foliage_tint = vec3(0.35f, 0.62f, 0.25f);

	//1 in N per column, 0 = never. indexes match StructureGenerator's registry
	int tree_chance = 0;
	int ground_cover_chance = 0;
	int flower_chance = 0;
	block_type flower = flower_red; //which flower grows here
};

//the definition for a biome, by id
const BiomeDefinition& biome_of(biome_id id);

/*
	picks a biome from the environment alone, so the same column always resolves
	the same way no matter which chunk or thread asks. water and altitude are
	decided first because they override climate entirely - the rest is a
	temperature/moisture climate grid.
*/
biome_id select_biome(const ClimateSample& climate, int sea_level);
