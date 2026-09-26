#include "CaveGenerator.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>

using namespace std;

namespace {
	int floor_div(int a, int b) {
		return (a >= 0 ? a : a - b + 1) / b;
	}

	//terrain fill only ever places ground, water and bedrock, so this is all that separates rock from the rest
	bool carvable(block_type type) {
		return type != none && type != water && type != bedrock;
	}

	//1 well below a ceiling, falling to 0.25 at it, so tunnels meet their roof in an arch instead of a flat cut
	float ceiling_taper(int y, int roof) {
		return glm::clamp((roof - y + 1) / 4.0f, 0.0f, 1.0f);
	}
}

CaveGenerator::CaveGenerator(int world_seed, int sea, const CaveConfig& cfg) : config(cfg), sea_level(sea) {
	tunnel_noise_a.SetSeed(world_seed + 101);
	tunnel_noise_a.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	tunnel_noise_a.SetFrequency(config.tunnel_frequency);

	tunnel_noise_b.SetSeed(world_seed + 102);
	tunnel_noise_b.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	tunnel_noise_b.SetFrequency(config.tunnel_frequency);

	cavern_noise.SetSeed(world_seed + 103);
	cavern_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	cavern_noise.SetFrequency(config.cavern_frequency);
	cavern_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	cavern_noise.SetFractalOctaves(2);

	entrance_noise.SetSeed(world_seed + 104);
	entrance_noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	entrance_noise.SetFrequency(config.entrance_frequency);
}

void CaveGenerator::carve(vector<Block>& blocks, const vector<int>& heights,
	int origin_x, int origin_z, int width, int height, int length,
	const TerrainGenerator& terrain) const {
	if (!config.enabled) return;

	auto height_at = [&](int x, int z) {
		if (x >= 0 && x < width && z >= 0 && z < length) return heights[(size_t)x * length + z];
		return glm::clamp(terrain.sample_height(origin_x + x, origin_z + z), 0, height);
	};

	thread_local vector<int> roof, cavern_roof, top;
	thread_local vector<float> entrance;
	size_t columns = (size_t)width * length;
	roof.resize(columns);
	cavern_roof.resize(columns);
	top.resize(columns);
	entrance.resize(columns);

	/*
		a column's ceiling is set by the lowest surface among it and its four
		neighbours. that keeps every carved block below the ground on all sides,
		so a cave can never open into a river bank, lake or sea - water here is
		static and would hang in the air beside it.
	*/
	const int y_lo = config.floor_y + 1;
	int y_hi = -1;
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			size_t i = (size_t)x * length + z;
			int lowest = std::min({ heights[i], height_at(x - 1, z), height_at(x + 1, z), height_at(x, z - 1), height_at(x, z + 1) });
			roof[i] = lowest - config.roof_thickness;
			cavern_roof[i] = lowest - config.cavern_roof_thickness;

			float e = 0.0f;
			if (lowest > sea_level) {
				float v = entrance_noise.GetNoise((float)(origin_x + x), (float)(origin_z + z)) * 0.5f + 0.5f;
				e = glm::smoothstep(config.entrance_threshold, config.entrance_threshold + 0.05f, v);
			}
			entrance[i] = e;
			top[i] = std::min(e > 0.0f ? heights[i] : roof[i], height - 1);
			y_hi = std::max(y_hi, top[i]);
		}
	}
	if (y_hi < y_lo) return;

	int gx0 = floor_div(origin_x, cell);
	int gz0 = floor_div(origin_z, cell);
	int gy0 = floor_div(y_lo, cell);
	int nx = floor_div(origin_x + width - 1, cell) - gx0 + 2;
	int nz = floor_div(origin_z + length - 1, cell) - gz0 + 2;
	int ny = floor_div(y_hi, cell) - gy0 + 2;

	//how high each lattice column is actually read, so mountains and seas only pay for the rock they have
	thread_local vector<int> tunnel_need, cavern_need;
	tunnel_need.assign((size_t)nx * nz, -1);
	cavern_need.assign((size_t)nx * nz, -1);
	for (int x = 0; x < width; ++x) {
		int ix = floor_div(origin_x + x, cell) - gx0;
		for (int z = 0; z < length; ++z) {
			size_t i = (size_t)x * length + z;
			if (top[i] < y_lo) continue;
			int iz = floor_div(origin_z + z, cell) - gz0;
			int t = floor_div(top[i], cell) - gy0 + 1;
			int c = floor_div(std::min(cavern_roof[i], top[i]), cell) - gy0 + 1;
			for (int dx = 0; dx < 2; ++dx) {
				for (int dz = 0; dz < 2; ++dz) {
					size_t k = (size_t)(ix + dx) * nz + iz + dz;
					tunnel_need[k] = std::max(tunnel_need[k], t);
					cavern_need[k] = std::max(cavern_need[k], c);
				}
			}
		}
	}

	thread_local vector<float> lattice;
	lattice.resize((size_t)3 * nx * ny * nz);
	auto at = [&](int field, int ix, int iy, int iz) -> float& {
		return lattice[(((size_t)field * nx + ix) * nz + iz) * ny + iy];
	};
	for (int ix = 0; ix < nx; ++ix) {
		float wx = (float)((gx0 + ix) * cell);
		for (int iz = 0; iz < nz; ++iz) {
			float wz = (float)((gz0 + iz) * cell);
			size_t k = (size_t)ix * nz + iz;
			for (int iy = 0; iy <= tunnel_need[k]; ++iy) {
				float wy = (float)((gy0 + iy) * cell);
				at(0, ix, iy, iz) = tunnel_noise_a.GetNoise(wx, wy * config.tunnel_flatten, wz);
				at(1, ix, iy, iz) = tunnel_noise_b.GetNoise(wx, wy * config.tunnel_flatten, wz);
			}
			for (int iy = 0; iy <= cavern_need[k]; ++iy) {
				float wy = (float)((gy0 + iy) * cell);
				at(2, ix, iy, iz) = cavern_noise.GetNoise(wx, wy * config.cavern_flatten, wz);
			}
		}
	}

	//depth only varies with y, so its effect on tunnel width and cavern rarity is looked up rather than recomputed per block
	thread_local vector<float> tunnel_r2, cavern_threshold;
	tunnel_r2.resize(height);
	cavern_threshold.resize(height);
	const float depth_span = (float)std::max(sea_level - config.floor_y, 1);
	for (int y = y_lo; y <= y_hi; ++y) {
		float depth_t = glm::clamp((sea_level - y) / depth_span, 0.0f, 1.0f);
		float r = config.tunnel_radius * (1.0f + depth_t * config.tunnel_depth_growth);
		tunnel_r2[y] = r * r;
		cavern_threshold[y] = config.cavern_threshold - depth_t * config.cavern_depth_bonus;
	}
	const float cavern_taper_strength = 0.3f;

	thread_local vector<float> column;
	column.resize((size_t)3 * ny);

	//block_type is a char type, so every write to a block may alias anything; plain locals keep the
	//compiler from reloading these through thread-local storage on every block
	const float* r2_of = tunnel_r2.data();
	const float* cavern_threshold_of = cavern_threshold.data();
	float* column_values = column.data();
	Block* block_data = blocks.data();
	for (int x = 0; x < width; ++x) {
		int wx = origin_x + x;
		int ix = floor_div(wx, cell) - gx0;
		float fx = (float)(wx - (gx0 + ix) * cell) / cell;

		for (int z = 0; z < length; ++z) {
			size_t i = (size_t)x * length + z;
			int col_top = top[i];
			if (col_top < y_lo) continue;
			int col_roof = roof[i];
			int col_cavern_roof = std::min(cavern_roof[i], col_top);
			float col_entrance = entrance[i];

			int wz = origin_z + z;
			int iz = floor_div(wz, cell) - gz0;
			float fz = (float)(wz - (gz0 + iz) * cell) / cell;

			int rows[3];
			rows[0] = rows[1] = floor_div(col_top, cell) - gy0 + 1;
			rows[2] = col_cavern_roof >= y_lo ? floor_div(col_cavern_roof, cell) - gy0 + 1 : -1;
			for (int f = 0; f < 3; ++f) {
				const float* p00 = &at(f, ix, 0, iz);
				const float* p10 = &at(f, ix + 1, 0, iz);
				const float* p01 = &at(f, ix, 0, iz + 1);
				const float* p11 = &at(f, ix + 1, 0, iz + 1);
				float* out = column_values + f * ny;
				for (int iy = 0; iy <= rows[f]; ++iy) {
					float near_z = p00[iy] + (p10[iy] - p00[iy]) * fx;
					float far_z = p01[iy] + (p11[iy] - p01[iy]) * fx;
					out[iy] = near_z + (far_z - near_z) * fz;
				}
			}

			Block* column_blocks = block_data + (size_t)x * height * length + z;
			for (int iy = 0; (gy0 + iy) * cell <= col_top; ++iy) {
				int base = (gy0 + iy) * cell;
				int y_start = std::max(y_lo, base);
				int y_end = std::min(col_top, base + cell - 1);
				const float a0 = column_values[iy], a1 = column_values[iy + 1];
				const float b0 = column_values[ny + iy], b1 = column_values[ny + iy + 1];
				const float c0 = column_values[2 * ny + iy];
				const float c1 = column_values[2 * ny + iy + 1];

				//values are linear across the cell, so its closest approach to a tunnel and its peak cavern
				//value are known from the ends alone; most cells are solid rock and are skipped whole
				float a_min = a0 * a1 <= 0.0f ? 0.0f : std::min(std::abs(a0), std::abs(a1));
				float b_min = b0 * b1 <= 0.0f ? 0.0f : std::min(std::abs(b0), std::abs(b1));
				bool may_tunnel = a_min * a_min + b_min * b_min < r2_of[y_start];
				bool may_cavern = y_start <= col_cavern_roof && std::max(c0, c1) > cavern_threshold_of[y_start];
				if (!may_tunnel && !may_cavern) continue;

				for (int y = y_start; y <= y_end; ++y) {
					block_type& type = column_blocks[(size_t)y * length].type;
					if (!carvable(type)) continue;

					float fy = (float)(y - base) / cell;
					float va = a0 + (a1 - a0) * fy;
					float vb = b0 + (b1 - b0) * fy;
					float reach = y > col_roof ? col_entrance : std::max(ceiling_taper(y, col_roof), col_entrance);
					bool carve = va * va + vb * vb < r2_of[y] * reach;

					if (!carve && y <= col_cavern_roof) {
						float vc = c0 + (c1 - c0) * fy;
						carve = vc > cavern_threshold_of[y] + (1.0f - ceiling_taper(y, col_cavern_roof)) * cavern_taper_strength;
					}

					if (carve) type = none;
				}
			}
		}
	}
}

namespace {
	void write_ppm(const std::string& path, int w, int h, const vector<unsigned char>& rgb) {
		std::ofstream file(path, std::ios::binary);
		file << "P6\n" << w << " " << h << "\n255\n";
		file.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
	}
}

void CaveGenerator::export_debug_slices(const std::string& path_prefix, const TerrainGenerator& terrain, int center_x, int center_z, int size) const {
	const int chunks = std::max(size / 16, 1);
	const int span = chunks * 16;
	const int x0 = center_x - span / 2, z0 = center_z - span / 2;

	vector<int> surface((size_t)span * span);
	int top = sea_level + 1;
	for (int x = 0; x < span; ++x) {
		for (int z = 0; z < span; ++z) {
			int h = std::max(terrain.sample_height(x0 + x, z0 + z), 0);
			surface[(size_t)x * span + z] = h;
			top = std::max(top, h + 1);
		}
	}

	enum cell_kind : unsigned char { air, rock, wet, cave };
	vector<unsigned char> volume((size_t)span * top * span);
	auto vi = [&](int x, int y, int z) { return ((size_t)x * top + y) * span + z; };

	const int w = 18, l = 18;
	vector<Block> blocks((size_t)w * top * l);
	vector<int> heights((size_t)w * l);
	for (int cx = 0; cx < chunks; ++cx) {
		for (int cz = 0; cz < chunks; ++cz) {
			int ox = x0 + cx * 16 - 1, oz = z0 + cz * 16 - 1;
			for (int x = 0; x < w; ++x) {
				for (int z = 0; z < l; ++z) {
					int h = glm::clamp(terrain.sample_height(ox + x, oz + z), 0, top);
					heights[(size_t)x * l + z] = h;
					for (int y = 0; y < top; ++y) {
						block_type t = y == 0 ? bedrock : y <= h ? stone : y <= sea_level ? water : none;
						blocks[((size_t)x * top + y) * l + z].type = t;
					}
				}
			}
			carve(blocks, heights, ox, oz, w, top, l, terrain);

			for (int x = 1; x < w - 1; ++x) {
				for (int z = 1; z < l - 1; ++z) {
					int rx = cx * 16 + x - 1, rz = cz * 16 + z - 1;
					int h = heights[(size_t)x * l + z];
					for (int y = 0; y < top; ++y) {
						block_type t = blocks[((size_t)x * top + y) * l + z].type;
						unsigned char k = t == water ? wet : t != none ? rock : y <= h ? cave : air;
						volume[vi(rx, y, rz)] = k;
					}
				}
			}
		}
	}

	auto paint = [](unsigned char k, unsigned char* px) {
		static const unsigned char colors[4][3] = { {150, 200, 255}, {110, 110, 110}, {40, 80, 200}, {235, 60, 40} };
		px[0] = colors[k][0]; px[1] = colors[k][1]; px[2] = colors[k][2];
	};

	const int levels[] = { config.floor_y + 5, sea_level / 2, sea_level - 5 };
	for (int y : levels) {
		vector<unsigned char> img((size_t)span * span * 3);
		for (int x = 0; x < span; ++x)
			for (int z = 0; z < span; ++z)
				paint(volume[vi(x, y, z)], &img[((size_t)z * span + x) * 3]);
		write_ppm(path_prefix + "_y" + std::to_string(y) + ".ppm", span, span, img);
	}

	vector<unsigned char> cut((size_t)span * top * 3);
	for (int x = 0; x < span; ++x)
		for (int y = 0; y < top; ++y)
			paint(volume[vi(x, y, span / 2)], &cut[((size_t)(top - 1 - y) * span + x) * 3]);
	write_ppm(path_prefix + "_cut.ppm", span, top, cut);

	long long underground = 0, carved = 0, openings = 0;
	long long band_rock[4] = {}, band_cave[4] = {};
	for (int x = 0; x < span; ++x) {
		for (int z = 0; z < span; ++z) {
			int h = surface[(size_t)x * span + z];
			if (volume[vi(x, h, z)] == cave) openings++;
			for (int y = config.floor_y + 1; y < h && y < top; ++y) {
				unsigned char k = volume[vi(x, y, z)];
				if (k != rock && k != cave) continue;
				int band = std::min(3, y * 4 / std::max(sea_level, 1));
				underground++; band_rock[band]++;
				if (k == cave) { carved++; band_cave[band]++; }
			}
		}
	}

	std::ofstream stats(path_prefix + "_stats.txt");
	stats << std::fixed << std::setprecision(2);
	stats << "area " << span << "x" << span << " centred on (" << center_x << ", " << center_z << ")\n";
	stats << "underground carved: " << 100.0 * carved / std::max(underground, 1LL) << "%\n";
	for (int b = 0; b < 4; ++b) {
		stats << "  y " << b * sea_level / 4 << "+: " << 100.0 * band_cave[b] / std::max(band_rock[b], 1LL) << "%\n";
	}
	stats << "surface columns opened: " << openings << "\n";
}

const CaveGenerator& get_cave_generator() {
	static const CaveGenerator instance(TerrainConfig().world_seed, TerrainConfig().sea_level);
	return instance;
}
