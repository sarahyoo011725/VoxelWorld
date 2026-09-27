#include "Onsen.h"
#include "Chunk/Chunk.h"
#include "World/ChunkManager.h"
#include "World/TerrainGenerator.h"
#include "World/WorldRandom.h"

namespace {
	const uint32_t onsen_salt = 0x0A5E;
	const uint32_t rock_salt = 0x0A5F;
	const int clear_height = 16; //mountain cut away above the terrace
	const int foundation_depth = 10; //stone under it, so it never hangs off a slope

	//the rim of the pool is an ellipse a little in front of the centre
	float pool_shape(int dx, int dz) {
		float x = dx / 4.5f, z = (dz - 1) / 3.5f;
		return x * x + z * z;
	}

	/*
		places a site's blocks, but only in columns from x0..x1, z0..z1 (one chunk's
		interior); `put` receives everything else and ignores it
	*/
	class Builder {
	public:
		Builder(const OnsenSite& site, Chunk* chunk)
			: cm(ChunkManager::get_instance()), chunk(chunk), c(site.centre) {
			x0 = (int)chunk->world_position.x;
			z0 = (int)chunk->world_position.z;
		}

		bool inside(int dx, int dz) const {
			int x = c.x + dx, z = c.z + dz;
			return x >= x0 && x < x0 + chunk_size && z >= z0 && z < z0 + chunk_size;
		}

		void put(int dx, int dy, int dz, block_type type) {
			int y = c.y + dy;
			if (!inside(dx, dz) || y < 1 || y >= chunk->height - 1) return;
			cm.set_block_worldspace(vec3(c.x + dx, y, c.z + dz), type);
		}

		void surface(int dx, int dz, int top) {
			if (!inside(dx, dz)) return;
			chunk->set_height(c.x + dx - x0 + 1, c.z + dz - z0 + 1, c.y + top);
		}

		void build() {
			terrace();
			pool();
			for (ivec2 at : { ivec2(-6, 4), ivec2(6, 4), ivec2(-6, -2), ivec2(6, -2) }) lantern(at.x, at.y);
			bathhouse();
			torii();
		}

	private:
		//a round shelf cut into the mountain: cleared above, founded below, gravel between
		void terrace() {
			for (int dx = -onsen::radius; dx <= onsen::radius; ++dx) {
				for (int dz = -onsen::radius; dz <= onsen::radius; ++dz) {
					float r = sqrt((float)(dx * dx + dz * dz));
					if (r > onsen::radius + 0.5f) continue;
					for (int dy = 1; dy <= clear_height; ++dy) put(dx, dy, dz, none);
					for (int dy = -foundation_depth; dy < 0; ++dy) put(dx, dy, dz, stone);
					bool path = std::abs(dx) <= 1 && dz >= 5;
					put(dx, 0, dz, path ? stone_brick : r > onsen::radius - 1.5f ? snow : gravel);
					surface(dx, dz, 0);
				}
			}
		}

		//hot water two deep, glowing warm from below, ringed with rough stones
		void pool() {
			WorldRandom rng(get_terrain_generator().config.world_seed, c.x, c.z, rock_salt);
			for (int dx = -7; dx <= 7; ++dx) {
				for (int dz = -5; dz <= 7; ++dz) {
					float shape = pool_shape(dx, dz);
					bool rock = rng.next_int(10) < 6;
					bool mossy = rng.next_int(3) == 0;
					if (shape <= 1.0f) {
						put(dx, 0, dz, water);
						put(dx, -1, dz, water);
						bool glow = (dx == 0 || std::abs(dx) == 2) && dz == 1;
						put(dx, -2, dz, glow ? glowstone : stone);
					}
					else if (shape <= 1.5f) {
						put(dx, 0, dz, stone);
						if (rock) put(dx, 1, dz, mossy ? mossy_stone : stone);
					}
				}
			}
		}

		//a stone lantern: post, light, cap
		void lantern(int dx, int dz) {
			put(dx, 1, dz, stone_brick);
			put(dx, 2, dz, stone_brick);
			put(dx, 3, dz, glowstone);
			put(dx, 4, dz, stone_brick);
		}

		//an open-fronted wooden bathhouse behind the pool, under a two-tier tiled roof with lifted corners
		void bathhouse() {
			for (int dx = -4; dx <= 4; ++dx) {
				for (int dz = -9; dz <= -5; ++dz) {
					put(dx, 0, dz, planks);
					bool corner_post = (std::abs(dx) == 4 && (dz == -9 || dz == -5)) || (dx == 0 && dz == -9);
					for (int dy = 1; dy <= 4; ++dy) {
						if (corner_post) put(dx, dy, dz, wood);
						else if ((dz == -9 || std::abs(dx) == 4) && dy <= 3) put(dx, dy, dz, planks);
					}
				}
			}
			for (int dx = -5; dx <= 5; ++dx) {
				for (int dz = -10; dz <= -4; ++dz) {
					put(dx, 5, dz, roof_tile);
					if (std::abs(dx) <= 4 && dz >= -9 && dz <= -5) put(dx, 6, dz, roof_tile);
				}
				if (std::abs(dx) <= 4) put(dx, 7, -7, roof_tile);
			}
			for (int sx : { -5, 5 }) {
				for (int sz : { -10, -4 }) put(sx, 6, sz, roof_tile);
			}
		}

		//a vermilion gate over the path in, with a dark lintel
		void torii() {
			const int z = 9;
			for (int sx : { -3, 3 }) {
				for (int dy = 1; dy <= 5; ++dy) put(sx, dy, z, red_lacquer);
			}
			for (int dx = -2; dx <= 2; ++dx) put(dx, 4, z, red_lacquer);
			for (int dx = -4; dx <= 4; ++dx) put(dx, 6, z, red_lacquer);
			for (int dx = -5; dx <= 5; ++dx) put(dx, 7, z, roof_tile);
		}

		ChunkManager& cm;
		Chunk* chunk;
		ivec3 c;
		int x0 = 0, z0 = 0;
	};
}

namespace onsen {
	bool site_in_cell(ivec2 cell, OnsenSite& site) {
		const TerrainGenerator& gen = get_terrain_generator();
		WorldRandom rng(gen.config.world_seed, cell.x, cell.y, onsen_salt);
		//only now and then: most cells with a snowy peak are left bare, so an onsen stays a find
		bool allowed = rng.next_int(4) == 0;
		int margin = radius + 8;
		for (int attempt = 0; attempt < 6; ++attempt) {
			int x = cell.x * cell_size + margin + rng.next_int(cell_size - 2 * margin);
			int z = cell.y * cell_size + margin + rng.next_int(cell_size - 2 * margin);
			ClimateSample climate = gen.sample_climate(x, z);
			if (select_biome(climate, gen.config.sea_level) != biome_id::snowy_peak) continue;
			//room above for the building and the cut into the slope
			if (climate.elevation + clear_height >= 110) continue;
			if (!allowed) return false;
			site.centre = ivec3(x, climate.elevation, z);
			return true;
		}
		return false;
	}

	std::vector<OnsenSite> sites_near(vec3 position, float range) {
		std::vector<OnsenSite> found;
		ivec2 lo = ivec2(floor((vec2(position.x, position.z) - range) / (float)cell_size));
		ivec2 hi = ivec2(floor((vec2(position.x, position.z) + range) / (float)cell_size));
		for (int cx = lo.x; cx <= hi.x; ++cx) {
			for (int cz = lo.y; cz <= hi.y; ++cz) {
				OnsenSite site;
				if (!site_in_cell(ivec2(cx, cz), site)) continue;
				if (length(vec2(site.centre.x - position.x, site.centre.z - position.z)) <= range) found.push_back(site);
			}
		}
		return found;
	}

	bool nearest(vec3 position, float range, OnsenSite& site) {
		float best = range;
		bool any = false;
		for (const OnsenSite& s : sites_near(position, range)) {
			float d = length(vec2(s.centre.x - position.x, s.centre.z - position.z));
			if (d <= best) {
				best = d;
				site = s;
				any = true;
			}
		}
		return any;
	}

	void build_in_chunk(Chunk* chunk) {
		vec3 middle = chunk->world_position + vec3(chunk_size * 0.5f, 0.0f, chunk_size * 0.5f);
		//any site whose terrace could reach this chunk
		for (const OnsenSite& site : sites_near(middle, chunk_size * 0.75f + radius + 1.0f)) {
			Builder(site, chunk).build();
		}
	}
}
