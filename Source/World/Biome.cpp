#include "Biome.h"

namespace {
	/*
		one row per biome, in biome_id order. tints are the colour the greyscale
		grass/leaf tiles get multiplied by, so they are what actually makes a
		biome read as hot, cold or lush on screen.
	*/
	const BiomeDefinition table[(int)biome_id::count] = {
		//               id                         name          surface     subsurf depth leaf          grass tint                   foliage tint                 tree cover flower  flower type
		{ biome_id::ocean,        "ocean",        sand,       sand,   4, leaf_red,    vec3(0.35f,0.58f,0.36f), vec3(0.30f,0.52f,0.30f),   0,   0,   0, flower_white  },
		{ biome_id::beach,        "beach",        sand,       sand,   5, leaf_red,    vec3(0.50f,0.72f,0.37f), vec3(0.42f,0.64f,0.30f),   0,  60,   0, flower_white  },
		{ biome_id::desert,       "desert",       sand,       sand,   6, leaf_yellow, vec3(0.74f,0.71f,0.33f), vec3(0.66f,0.63f,0.28f),   0, 140, 260, flower_yellow },
		{ biome_id::plains,       "plains",       dirt_grass, dirt,   4, leaf_red,    vec3(0.57f,0.79f,0.33f), vec3(0.47f,0.70f,0.27f), 140,   9,  22, flower_yellow },
		{ biome_id::savanna,      "savanna",      dirt_grass, dirt,   4, leaf_yellow, vec3(0.74f,0.72f,0.30f), vec3(0.80f,0.68f,0.18f),  70,  12,  34, flower_yellow },
		{ biome_id::forest,       "forest",       dirt_grass, dirt,   4, leaf_transp, vec3(0.42f,0.71f,0.29f), vec3(0.30f,0.60f,0.22f),  14,   7,  30, flower_white  },
		{ biome_id::autumn_forest,"autumn forest",dirt_grass, dirt,   4, leaf_yellow, vec3(0.55f,0.62f,0.26f), vec3(0.78f,0.33f,0.12f),  14,   8,  26, flower_red    },
		{ biome_id::swamp,        "swamp",        dirt_grass, dirt,   4, leaf_transp, vec3(0.42f,0.55f,0.27f), vec3(0.36f,0.48f,0.22f),  38,   5,  70, flower_purple },
		{ biome_id::taiga,        "taiga",        dirt_grass, dirt,   4, leaf_red,    vec3(0.34f,0.58f,0.40f), vec3(0.24f,0.44f,0.32f),  18,  14,  90, flower_purple },
		{ biome_id::tundra,       "tundra",       snow,       dirt,   4, leaf_red,    vec3(0.50f,0.62f,0.50f), vec3(0.42f,0.55f,0.45f), 220,  40, 160, flower_white  },
		{ biome_id::mountain,     "mountain",     stone,      stone,  6, leaf_red,    vec3(0.45f,0.65f,0.38f), vec3(0.38f,0.56f,0.32f), 300,  60, 220, flower_purple },
		{ biome_id::snowy_peak,   "snowy peak",   snow,       stone,  5, leaf_red,    vec3(0.55f,0.66f,0.58f), vec3(0.48f,0.58f,0.52f),   0,   0,   0, flower_white  },
	};
}

const BiomeDefinition& biome_of(biome_id id) {
	int i = (int)id;
	if (i < 0 || i >= (int)biome_id::count) i = (int)biome_id::plains;
	return table[i];
}

biome_id select_biome(const ClimateSample& c, int sea_level) {
	//water and altitude ignore climate: an ocean is an ocean at any temperature,
	//and nothing grows on a peak however wet it is
	if (c.feature == terrain_feature::ocean) return biome_id::ocean;

	int above_sea = c.landform_elevation - sea_level;
	if (above_sea >= 34) return biome_id::snowy_peak;
	if (above_sea >= 22) return c.temperature < 0.3f ? biome_id::snowy_peak : biome_id::mountain;

	//only the waterline itself, not merely low ground: the land baseline sits
	//just above sea level, so a wider band would classify whole lowland plains
	//as beach
	if (above_sea <= 0 && c.feature == terrain_feature::land) {
		return c.temperature < 0.22f ? biome_id::tundra : biome_id::beach;
	}

	//river and lake banks stay lush, which is what makes water read as an oasis
	//in an otherwise dry region
	bool bankside = c.feature == terrain_feature::river || c.feature == terrain_feature::lake;

	/*
		the climate grid. temperature runs cold -> hot, moisture dry -> wet:

		        moisture ->
		 hot |  desert   plains   swamp
		     |  plains   forest   forest
		cold |  tundra   taiga    taiga
	*/
	if (c.temperature < 0.28f) {
		return c.moisture < 0.35f ? biome_id::tundra : biome_id::taiga;
	}
	if (c.temperature > 0.72f) {
		if (c.moisture < 0.32f && !bankside) return biome_id::desert;
		if (c.moisture > 0.68f) return biome_id::swamp;
		return biome_id::savanna; //hot grassland, golden trees
	}
	if (c.moisture < 0.34f) return biome_id::plains;
	if (c.moisture > 0.62f && c.temperature > 0.55f) return biome_id::swamp;
	//same climate cell, split by variation so both kinds of wood appear in
	//broad patches rather than alternating block by block
	return c.variation > 0.55f ? biome_id::autumn_forest : biome_id::forest;
}
