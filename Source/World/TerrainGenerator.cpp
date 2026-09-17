#include "TerrainGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

using namespace glm;

namespace {
	void configure(FastNoiseLite& noise, int seed, float frequency, int octaves) {
		noise.SetSeed(seed);
		noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		noise.SetFrequency(frequency);
		if (octaves > 1) {
			noise.SetFractalType(FastNoiseLite::FractalType_FBm);
			noise.SetFractalOctaves(octaves);
		}
	}

	//a warp source, used via its own DomainWarp(x, z) - never sampled for a value
	void configure_warp(FastNoiseLite& warp, int seed, float frequency, float amplitude) {
		warp.SetSeed(seed);
		warp.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
		warp.SetFrequency(frequency);
		warp.SetDomainWarpAmp(amplitude);
	}
}

TerrainGenerator::TerrainGenerator(const TerrainConfig& cfg) : config(cfg) {
	//distinct offsets so every layer gets an independent field from one seed
	configure(continental_noise, config.world_seed + 1, config.continental_frequency, config.continental_octaves);
	configure_warp(continental_warp, config.world_seed + 2, config.domain_warp_scale, config.domain_warp_strength);

	configure(range_mask_noise, config.world_seed + 3, config.mountain_range_scale, 2);

	configure(mountain_noise, config.world_seed + 4, config.mountain_frequency, config.mountain_octaves);
	mountain_noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
	//shared with the river layer, which is what ties valleys to their range
	configure_warp(mountain_warp, config.world_seed + 5, config.domain_warp_scale * 1.4f, config.domain_warp_strength * 0.85f);
	configure(extreme_peak_noise, config.world_seed + 11, config.extreme_peak_frequency, 2);

	configure(hill_noise, config.world_seed + 6, config.hill_frequency, config.hill_octaves);
	configure(detail_noise, config.world_seed + 7, config.terrain_detail_frequency, 1);
	configure(river_noise, config.world_seed + 8, config.river_density, 2);
	configure(lake_basin_noise, config.world_seed + 12, config.lake_basin_frequency, 2);
	configure(moisture_noise, config.world_seed + 9, config.moisture_frequency, 3);
	configure(temperature_noise, config.world_seed + 10, config.temperature_frequency, 2);
}

TerrainGenerator::elevation_result TerrainGenerator::compute_elevation(int x, int z) const {
	float fx = (float)x, fz = (float)z;

	float wx = fx, wz = fz;
	continental_warp.DomainWarp(wx, wz);
	float continent = continental_noise.GetNoise(wx, wz); //-1..1

	//a saturating mask, not a multiplier: scaling height by the raw mask left
	//even the core of a range at a fraction of its intended height
	float range_raw = range_mask_noise.GetNoise(fx, fz) * 0.5f + 0.5f;
	float range = smoothstep(config.mountain_range_edge0, config.mountain_range_edge1, range_raw);

	float mx = fx, mz = fz;
	mountain_warp.DomainWarp(mx, mz);
	//FastNoiseLite's ridged fractal returns roughly -1..1, not 0..1 - used raw,
	//its negative half digs pits through the middle of a mountain range
	float ridge = mountain_noise.GetNoise(mx, mz) * 0.5f + 0.5f;
	float mountains = ridge * range;

	//scaled by the mountain height already present: requiring several
	//independent 0..1 factors to peak at the same spot meant the bonus
	//almost never materialized
	float peak_mask = extreme_peak_noise.GetNoise(fx, fz) * 0.5f + 0.5f;
	float peak_t = clamp((peak_mask - config.extreme_peak_threshold) / (1.0f - config.extreme_peak_threshold), 0.0f, 1.0f);
	float extreme_bonus = peak_t * mountains * config.extreme_peak_strength;

	//hills taper out where the mountain mask is strong so foothills blend
	float hills = hill_noise.GetNoise(fx, fz);
	float hill_contribution = hills * config.hill_strength * (1.0f - range * 0.6f);

	float detail = detail_noise.GetNoise(fx, fz) * config.terrain_detail_strength;

	const float baseline = (float)config.sea_level + 2.0f;
	float land_height = baseline + mountains * config.mountain_strength + extreme_bonus + hill_contribution + detail;

	const float ocean_floor = 3.0f;
	float ocean_t = clamp((continent - config.coast_inner_edge) / (config.coast_outer_edge - config.coast_inner_edge), 0.0f, 1.0f);
	float height = mix(ocean_floor, land_height, ocean_t);

	terrain_feature feature = terrain_feature::land;
	bool is_ocean = ocean_t < 0.5f;
	if (is_ocean) feature = terrain_feature::ocean;

	//zero-crossings of a noise field sampled at the SAME warped coordinates as
	//the mountains, so a river is geometrically tied to the range that fed it
	if (!is_ocean && height > (float)config.sea_level && height < config.river_source_elevation) {
		float river_v = river_noise.GetNoise(mx, mz);
		//the un-clamped continent value stands in for "how far inland" - far
		//cheaper than a real distance-to-coast field, but only locally accurate
		float coastal_proximity = 1.0f - clamp((continent - config.coast_outer_edge) / 0.5f, 0.0f, 1.0f);
		float half_width = config.river_min_length * (1.0f + coastal_proximity * config.river_mouth_widening);
		float river_t = clamp(std::abs(river_v) / half_width, 0.0f, 1.0f);
		if (river_t < 1.0f) {
			float river_bed = (float)config.sea_level - 1.0f;
			height = mix(river_bed, height, river_t);
			feature = terrain_feature::river;
		}
	}

	//a dedicated basin mask rather than comparing height against a smoothed
	//trend: ordinary hill noise dips several blocks below any trend that
	//doesn't contain those same hills, so every rolling dip became a lake
	if (feature == terrain_feature::land) {
		float basin = lake_basin_noise.GetNoise(fx, fz) * 0.5f + 0.5f;
		float basin_t = clamp((basin - config.lake_density) / (1.0f - config.lake_density), 0.0f, 1.0f);
		if (basin_t > 0.0f) {
			//eased so shorelines slope in instead of forming a cliff ring
			float depth_t = basin_t * basin_t * (3.0f - 2.0f * basin_t);
			float lake_bed = (float)config.sea_level - config.lake_min_size;
			height = mix(height, min(height, lake_bed), depth_t);
			if (height <= (float)config.sea_level) feature = terrain_feature::lake;
		}
	}

	return { height, feature };
}

int TerrainGenerator::sample_height(int x, int z) const {
	return (int)compute_elevation(x, z).height;
}

TerrainSample TerrainGenerator::sample(int x, int z) const {
	elevation_result center = compute_elevation(x, z);

	//four extra samples for the central-difference slope, which is why
	//heightmap generation calls sample_height() instead of this
	float e_px = compute_elevation(x + 1, z).height;
	float e_nx = compute_elevation(x - 1, z).height;
	float e_pz = compute_elevation(x, z + 1).height;
	float e_nz = compute_elevation(x, z - 1).height;

	TerrainSample result;
	result.elevation = (int)center.height;
	result.feature = center.feature;
	result.slope = (std::abs(e_px - e_nx) + std::abs(e_pz - e_nz)) * 0.5f;

	bool near_water = center.feature != terrain_feature::land;
	float fx = (float)x, fz = (float)z;
	result.moisture = clamp(moisture_noise.GetNoise(fx, fz) * 0.5f + 0.5f + (near_water ? 0.3f : 0.0f), 0.0f, 1.0f);

	float altitude_above_sea = max(0.0f, center.height - (float)config.sea_level);
	result.temperature = clamp(temperature_noise.GetNoise(fx, fz) * 0.5f + 0.5f - altitude_above_sea * 0.01f, 0.0f, 1.0f);

	return result;
}

namespace {
	void write_ppm(const std::string& path, int size, const std::vector<unsigned char>& rgb) {
		std::ofstream out(path, std::ios::binary);
		if (!out) return;
		out << "P6\n" << size << " " << size << "\n255\n";
		out.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
	}
}

void TerrainGenerator::export_debug_maps(const std::string& path_prefix, int center_x, int center_z, int size) const {
	std::vector<unsigned char> elevation_img(static_cast<size_t>(size) * size * 3);
	std::vector<unsigned char> feature_img(static_cast<size_t>(size) * size * 3);
	std::vector<unsigned char> moisture_img(static_cast<size_t>(size) * size * 3);
	std::vector<unsigned char> temperature_img(static_cast<size_t>(size) * size * 3);

	int half = size / 2;
	for (int row = 0; row < size; ++row) {
		for (int col = 0; col < size; ++col) {
			int wx = center_x - half + col;
			int wz = center_z - half + row;
			TerrainSample s = sample(wx, wz);
			size_t idx = (static_cast<size_t>(row) * size + col) * 3;

			unsigned char e = (unsigned char)clamp((float)s.elevation / 90.0f * 255.0f, 0.0f, 255.0f);
			elevation_img[idx + 0] = e;
			elevation_img[idx + 1] = e;
			elevation_img[idx + 2] = e;

			//feature mask: blue = ocean, cyan = river, teal = lake, green = land
			switch (s.feature) {
			case terrain_feature::ocean: feature_img[idx + 0] = 20; feature_img[idx + 1] = 40; feature_img[idx + 2] = 160; break;
			case terrain_feature::river: feature_img[idx + 0] = 60; feature_img[idx + 1] = 200; feature_img[idx + 2] = 230; break;
			case terrain_feature::lake: feature_img[idx + 0] = 40; feature_img[idx + 1] = 140; feature_img[idx + 2] = 140; break;
			default: feature_img[idx + 0] = 60; feature_img[idx + 1] = 150; feature_img[idx + 2] = 60; break;
			}

			unsigned char m = (unsigned char)(s.moisture * 255.0f);
			moisture_img[idx + 0] = 40; moisture_img[idx + 1] = 40; moisture_img[idx + 2] = m;

			unsigned char t = (unsigned char)(s.temperature * 255.0f);
			temperature_img[idx + 0] = t; temperature_img[idx + 1] = 40; temperature_img[idx + 2] = (unsigned char)(255 - t);
		}
	}

	write_ppm(path_prefix + "_elevation.ppm", size, elevation_img);
	write_ppm(path_prefix + "_feature.ppm", size, feature_img);
	write_ppm(path_prefix + "_moisture.ppm", size, moisture_img);
	write_ppm(path_prefix + "_temperature.ppm", size, temperature_img);
}
