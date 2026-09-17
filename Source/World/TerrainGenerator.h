#pragma once
#include <FastNoise/FastNoiseLite.h>
#include <glm/glm.hpp>
#include <string>

struct TerrainConfig {
	int world_seed = 1337;
	int sea_level = 10;

	float continental_frequency = 0.0012f;
	int continental_octaves = 3;
	float coast_inner_edge = -0.28f; //below this: fully ocean floor
	float coast_outer_edge = -0.03f; //above this: fully normal land
	float domain_warp_strength = 260.0f;
	float domain_warp_scale = 0.0016f;

	float mountain_frequency = 0.006f;
	int mountain_octaves = 5;
	float mountain_strength = 38.0f;
	float mountain_range_scale = 0.0022f;
	float mountain_range_edge0 = 0.45f; //mask value where a range starts rising out of the plains
	float mountain_range_edge1 = 0.68f; //mask value where a range reaches full height

	float extreme_peak_frequency = 0.0006f;
	float extreme_peak_strength = 34.0f;
	float extreme_peak_threshold = 0.62f;

	float hill_frequency = 0.02f;
	int hill_octaves = 4;
	float hill_strength = 4.0f;

	float terrain_detail_strength = 0.6f;
	float terrain_detail_frequency = 0.05f;

	float river_density = 0.02f;
	float river_min_length = 0.028f; //half-width of the carved channel at its narrowest
	float river_source_elevation = 40.0f; //rivers don't carve above this
	float river_mouth_widening = 2.5f;

	float lake_basin_frequency = 0.004f;
	float lake_density = 0.72f; //0..1 basin-mask threshold: higher means rarer lakes
	float lake_min_size = 3.0f; //how far below sea level a fully-formed lake bed sits

	float moisture_frequency = 0.004f;
	float temperature_frequency = 0.0018f;
};

enum class terrain_feature {
	ocean,
	river,
	lake,
	land
};

//kept separate from the elevation math so new biomes can key off these
//without touching height generation
struct TerrainSample {
	int elevation = 0;
	terrain_feature feature = terrain_feature::land;
	float slope = 0.0f;
	float moisture = 0.5f; //0..1
	float temperature = 0.5f; //0..1
};

/*
	Layered, domain-warped noise. Everything is a pure function of world (x, z)
	and the seed, so there is no chunk-local state and no seam artifacts.

	NOT implemented: true flow-accumulation hydrology (D8 flow direction,
	drainage basins, watershed delineation, river merging/erosion). That needs a
	finite heightmap rasterized into a grid with graph algorithms run over it,
	which doesn't fit an infinite, chunk-streamed, pure-function world without a
	precompute/caching layer. Rivers are a geometric approximation instead - see
	compute_elevation().
*/
class TerrainGenerator {
public:
	explicit TerrainGenerator(const TerrainConfig& config = TerrainConfig());

	TerrainSample sample(int x, int z) const;
	int sample_height(int x, int z) const;

	//PPM rather than PNG so this needs no image-writing dependency
	void export_debug_maps(const std::string& path_prefix, int center_x, int center_z, int size) const;

	const TerrainConfig config;

private:
	struct elevation_result {
		float height;
		terrain_feature feature;
	};

	elevation_result compute_elevation(int x, int z) const;

	FastNoiseLite continental_noise;
	FastNoiseLite continental_warp;
	FastNoiseLite range_mask_noise;
	FastNoiseLite mountain_noise;
	FastNoiseLite mountain_warp;
	FastNoiseLite extreme_peak_noise;
	FastNoiseLite hill_noise;
	FastNoiseLite detail_noise;
	FastNoiseLite river_noise;
	FastNoiseLite lake_basin_noise;
	FastNoiseLite moisture_noise;
	FastNoiseLite temperature_noise;
};
