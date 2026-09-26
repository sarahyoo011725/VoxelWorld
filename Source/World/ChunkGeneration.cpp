#include "ChunkGeneration.h"
#include "World/TerrainGenerator.h"
#include "World/CaveGenerator.h"
#include "World/WorldRandom.h"

static const uint32_t bedrock_salt = 0xBED0;

unsigned char daylight_level(int column_height, int y) {
	int depth = column_height - y;
	if (depth < 0) return 15;
	const CaveConfig& caves = get_cave_generator().config;
	float light = glm::clamp(1.0f - depth / caves.light_falloff, caves.min_light, 1.0f);
	return (unsigned char)glm::round(light * 15.0f);
}

CarveResult generate_chunk_blocks(glm::ivec2 chunk_id, int width, int height, int length,
	std::vector<Block>& blocks, std::vector<int>& heights, std::vector<biome_id>& biomes) {
	fill_chunk_terrain(chunk_id, width, height, length, blocks, heights, biomes);
	const int chunk_width = width - 2;
	return get_cave_generator().carve(blocks, heights, chunk_id.x * chunk_width - 1, chunk_id.y * chunk_width - 1,
		width, height, length, get_terrain_generator());
}

void fill_chunk_terrain(glm::ivec2 chunk_id, int width, int height, int length,
	std::vector<Block>& blocks, std::vector<int>& heights, std::vector<biome_id>& biomes) {
	const TerrainGenerator& generator = get_terrain_generator();
	const TerrainConfig& config = generator.config;
	const int sea_level = config.sea_level;
	const int chunk_width = width - 2;
	const int origin_x = chunk_id.x * chunk_width - 1;
	const int origin_z = chunk_id.y * chunk_width - 1;

	heights.resize((size_t)width * length);
	biomes.resize((size_t)width * length);
	blocks.assign((size_t)width * height * length, Block());

	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			//one climate sample feeds both the height and the biome
			ClimateSample climate = generator.sample_climate(origin_x + x, origin_z + z);
			heights[(size_t)x * length + z] = glm::clamp(climate.elevation, 0, height);
			biomes[(size_t)x * length + z] = select_biome(climate, sea_level);
		}
	}

	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			int h = heights[(size_t)x * length + z];
			const BiomeDefinition& biome = biome_of(biomes[(size_t)x * length + z]);
			WorldRandom rng(config.world_seed, origin_x + x, origin_z + z, bedrock_salt);
			int bedrock_top = config.bedrock_height - 1 - rng.next_int(2);

			for (int y = 0; y < height; ++y) {
				block_type type = none;
				if (y > h && y <= sea_level) {
					type = water;
				}
				if (y == h) {
					type = biome.surface;
				}
				if (y < h) {
					type = biome.subsurface;
					if (y < h - biome.subsurface_depth) {
						type = stone;
					}
				}
				//anything at the waterline is beach regardless of biome, so
				//shores read as shores instead of grass running into the sea
				if (y <= h && y >= h - 2 && y + 1 < height && y + 1 <= sea_level) {
					type = sand;
				}
				if (y <= bedrock_top) {
					type = bedrock;
				}
				blocks[((size_t)x * height + y) * length + z].type = type;
			}
		}
	}
}
