/*
	offline checks for world generation and cave culling. builds chunks through
	the game's own generate_chunk_blocks(), with no window or GL, and exits
	non-zero if any check fails.

	usage: worldgen_check [caves|occlusion|all] [region=<chunks per side>]
	caves also writes cave_slice_y*.ppm and cave_cut.ppm to the working directory.
*/
#include "World/BlockLight.h"
#include "World/ChunkGeneration.h"
#include "World/CaveGenerator.h"
#include "World/SectionVisibility.h"
#include "World/TerrainGenerator.h"
#include "World/WorldRandom.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

namespace {
	const int CS = 16, W = CS + 2, L = CS + 2, H = chunk_height;
	int failures = 0;

	struct ChunkData {
		vector<Block> blocks;
		vector<int> heights;
		vector<biome_id> biomes;
		vector<glm::ivec3> emitters;
		vector<unsigned char> light;
		bool lit = false;
		block_type at(int x, int y, int z) const { return blocks[((size_t)x * H + y) * L + z].type; }
		unsigned char light_at(int x, int y, int z) const { return lit ? light[((size_t)x * H + y) * L + z] : 0; }
	};

	void generate(int cx, int cz, ChunkData& c) {
		generate_chunk_blocks(glm::ivec2(cx, cz), W, H, L, c.blocks, c.heights, c.biomes);
	}

	void fill_only(int cx, int cz, ChunkData& c) {
		fill_chunk_terrain(glm::ivec2(cx, cz), W, H, L, c.blocks, c.heights, c.biomes);
	}

	double now_ms() {
		return chrono::duration<double, milli>(chrono::steady_clock::now().time_since_epoch()).count();
	}

	void check(const char* what, bool ok, const string& detail = "") {
		printf("  %-52s %s %s\n", what, ok ? "PASS" : "FAIL", detail.c_str());
		if (!ok) failures++;
	}

	bool opaque(block_type t) { return !is_see_through(t) && !is_foliage(t); }

	//mirrors Chunk::light_key
	uint16_t face_light(const ChunkData& c, block_type t, int x, int y, int z, const int* dir) {
		if (is_emissive(t)) return 0x100;
		int fx = x + dir[0], fy = y + dir[1], fz = z + dir[2];
		unsigned char glow = fy >= 0 && fy < H ? c.light_at(fx, fy, fz) : 0;
		return (uint16_t)(daylight_level(c.heights[(size_t)fx * L + fz], fy) | (glow << 4));
	}

	//border copies only feed face culling, which cares whether a block is air, a liquid or solid;
	//which ore or gravel an opaque block is only matters in the chunk that owns and draws it
	int render_class(block_type t) { return t == none ? 0 : t == water ? 1 : t == lava ? 2 : 3; }

	//the mesher's neighbour test: leaves hide faces behind them even though culling looks through them
	bool face_exposed(block_type neighbour) { return neighbour == none || has_transparency(neighbour); }

	//mirrors Chunk::build_opaque_mesh: same-type faces merged into rectangles within each section
	long long greedy_quads(const ChunkData& c) {
		static const int d[6][3] = { {0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
		long long quads = 0;
		vector<unsigned char> mask;
		vector<uint16_t> light;
		for (int y_lo = 0; y_lo < H; y_lo += section_size) {
			int y_hi = std::min(y_lo + section_size, H);
			for (int f = 0; f < 6; ++f) {
				int s0, s1, a0, a1, b0, b1;
				if (f < 2) { s0 = 1; s1 = L - 1; a0 = 1; a1 = W - 1; b0 = y_lo; b1 = y_hi; }
				else if (f < 4) { s0 = 1; s1 = W - 1; a0 = 1; a1 = L - 1; b0 = y_lo; b1 = y_hi; }
				else { s0 = y_lo; s1 = y_hi; a0 = 1; a1 = W - 1; b0 = 1; b1 = L - 1; }
				int aw = a1 - a0, bh = b1 - b0;
				mask.assign((size_t)aw * bh, 0);
				light.assign((size_t)aw * bh, 0);
				for (int s = s0; s < s1; ++s) {
					for (int a = 0; a < aw; ++a)
						for (int b = 0; b < bh; ++b) {
							int x, y, z;
							if (f < 2) { x = a0 + a; y = b0 + b; z = s; }
							else if (f < 4) { z = a0 + a; y = b0 + b; x = s; }
							else { x = a0 + a; z = b0 + b; y = s; }
							block_type t = c.at(x, y, z);
							unsigned char m = 0;
							int ny = y + d[f][1];
							if (opaque(t) && ny >= 0 && ny < H && face_exposed(c.at(x + d[f][0], ny, z + d[f][2]))) m = (unsigned char)t;
							mask[(size_t)a * bh + b] = m;
							light[(size_t)a * bh + b] = m ? face_light(c, t, x, y, z, d[f]) : 0;
						}
					for (int a = 0; a < aw; ++a)
						for (int b = 0; b < bh; ) {
							unsigned char t = mask[(size_t)a * bh + b];
							if (!t) { ++b; continue; }
							uint16_t l = light[(size_t)a * bh + b];
							auto same = [&](size_t k) { return mask[k] == t && light[k] == l; };
							int rh = 1;
							while (b + rh < bh && same((size_t)a * bh + b + rh)) ++rh;
							int rw = 1;
							bool grow = true;
							while (a + rw < aw && grow) {
								for (int k = 0; k < rh; ++k) if (!same((size_t)(a + rw) * bh + b + k)) { grow = false; break; }
								if (grow) ++rw;
							}
							for (int i = 0; i < rw; ++i) for (int k = 0; k < rh; ++k) mask[(size_t)(a + i) * bh + b + k] = 0;
							quads++;
							b += rh;
						}
				}
			}
		}
		return quads;
	}

	void write_ppm(const string& path, int w, int h, const vector<unsigned char>& rgb) {
		ofstream file(path, ios::binary);
		file << "P6\n" << w << " " << h << "\n255\n";
		file.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
	}

	//a square of real chunks stitched together, with each block's pre-cave state kept alongside
	struct Region {
		int chunks, size, cx0, cz0;
		vector<unsigned char> blocks, filled;
		vector<int> surface;
		vector<ChunkData> parts;
		size_t at(int x, int y, int z) const { return ((size_t)x * H + y) * size + z; }
		ChunkData& part(int i, int k) { return parts[(size_t)i * chunks + k]; }

		explicit Region(int n) : chunks(n), size(n * CS), cx0(-n / 2), cz0(-n / 2) {
			blocks.resize((size_t)size * H * size);
			filled.resize(blocks.size());
			surface.resize((size_t)size * size);
			parts.resize((size_t)n * n);
			ChunkData f;
			for (int i = 0; i < n; ++i)
				for (int k = 0; k < n; ++k) {
					ChunkData& c = part(i, k);
					generate(cx0 + i, cz0 + k, c);
					find_emitters(c.blocks, W, H, L, c.emitters);
					fill_only(cx0 + i, cz0 + k, f);
					for (int x = 1; x < W - 1; ++x)
						for (int z = 1; z < L - 1; ++z) {
							int rx = i * CS + x - 1, rz = k * CS + z - 1;
							surface[(size_t)rx * size + rz] = c.heights[(size_t)x * L + z];
							for (int y = 0; y < H; ++y) {
								blocks[at(rx, y, rz)] = c.at(x, y, z);
								filled[at(rx, y, rz)] = f.at(x, y, z);
							}
						}
				}
		}

		//cave space: rock the terrain fill placed that is now air or cave liquid
		bool carved(size_t i) const {
			block_type before = (block_type)filled[i], after = (block_type)blocks[i];
			return before != none && before != water && (after == none || (is_liquid(after) && before != after));
		}
	};

	void run_caves(int region_chunks) {
		const CaveConfig& cc = get_cave_generator().config;
		const int sea = get_terrain_generator().config.sea_level;

		printf("caves: performance (single thread, 400 chunks)\n");
		{
			ChunkData c;
			const int N = 400;
			for (int i = 0; i < 20; ++i) generate(i, 0, c);
			double best_fill = 1e9, best_full = 1e9;
			for (int rep = 0; rep < 3; ++rep) {
				double t0 = now_ms();
				for (int i = 0; i < N; ++i) fill_only(i % 20 - 10, i / 20 - 10, c);
				best_fill = std::min(best_fill, (now_ms() - t0) / N);
				t0 = now_ms();
				for (int i = 0; i < N; ++i) generate(i % 20 - 10, i / 20 - 10, c);
				best_full = std::min(best_full, (now_ms() - t0) / N);
			}
			long long q_fill = 0, q_full = 0;
			for (int i = 0; i < N; ++i) {
				fill_only(i % 20 - 10, i / 20 - 10, c); q_fill += greedy_quads(c);
				generate(i % 20 - 10, i / 20 - 10, c); q_full += greedy_quads(c);
			}
			printf("  terrain only:  %.3f ms/chunk\n", best_fill);
			printf("  with caves:    %.3f ms/chunk (+%.3f ms)\n", best_full, best_full - best_fill);
			printf("  opaque quads:  %.0f -> %.0f per chunk\n", (double)q_fill / N, (double)q_full / N);
		}

		printf("\ncaves: chunk borders\n");
		{
			ChunkData a, b;
			long long compared = 0, mismatched = 0;
			for (int cx = -6; cx < 6; ++cx)
				for (int cz = -6; cz < 6; ++cz) {
					generate(cx, cz, a);
					generate(cx + 1, cz, b);
					for (int z = 1; z < L - 1; ++z)
						for (int y = 0; y < H; ++y) {
							compared += 2;
							if (render_class(a.at(W - 1, y, z)) != render_class(b.at(1, y, z))) mismatched++;
							if (render_class(a.at(W - 2, y, z)) != render_class(b.at(0, y, z))) mismatched++;
						}
					generate(cx, cz + 1, b);
					for (int x = 1; x < W - 1; ++x)
						for (int y = 0; y < H; ++y) {
							compared += 2;
							if (render_class(a.at(x, y, L - 1)) != render_class(b.at(x, y, 1))) mismatched++;
							if (render_class(a.at(x, y, L - 2)) != render_class(b.at(x, y, 0))) mismatched++;
						}
				}
			check("border copies agree with their owner on air/liquid/solid", mismatched == 0,
				"(" + to_string(mismatched) + " of " + to_string(compared) + " differ)");
			generate(3, -5, a);
			generate(3, -5, b);
			check("same chunk regenerates identically", memcmp(a.blocks.data(), b.blocks.data(), a.blocks.size()) == 0);
		}

		Region r(region_chunks);
		const int S = r.size;
		printf("\ncaves: safety (%dx%d)\n", S, S);
		{
			long long wet = 0, below_floor = 0, bedrock_lost = 0;
			static const int d[6][3] = { {0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
			for (int x = 1; x < S - 1; ++x)
				for (int z = 1; z < S - 1; ++z)
					for (int y = 0; y < H - 1; ++y) {
						size_t i = r.at(x, y, z);
						if (y == 0 && r.blocks[i] != bedrock) bedrock_lost++;
						if (!r.carved(i)) continue;
						if (y <= cc.floor_y) below_floor++;
						for (auto& o : d) {
							int ny = y + o[1];
							if (ny < 0) continue;
							if (r.filled[r.at(x + o[0], ny, z + o[2])] == water) { wet++; break; }
						}
					}
			check("no cave touches sea, river or lake water", wet == 0, "(" + to_string(wet) + ")");
			check("nothing carved at or below floor_y", below_floor == 0);
			check("bottom row is all bedrock", bedrock_lost == 0);
		}

		printf("\ncaves: shape\n");
		{
			long long rock = 0, holes = 0, band_rock[4] = {}, band_hole[4] = {};
			long long floors = 0, standable = 0, cave_water = 0, cave_lava = 0;
			long long ores[5] = {}, gravel_count = 0, moss_count = 0;
			for (int x = 0; x < S; ++x)
				for (int z = 0; z < S; ++z) {
					int h = r.surface[(size_t)x * S + z];
					for (int y = cc.floor_y + 1; y < H; ++y) {
						size_t i = r.at(x, y, z);
						block_type t = (block_type)r.blocks[i];
						if (t == coal_ore) ores[0]++;
						if (t == iron_ore) ores[1]++;
						if (t == gold_ore) ores[2]++;
						if (t == redstone_ore) ores[3]++;
						if (t == diamond_ore) ores[4]++;
						if (t == gravel) gravel_count++;
						if (t == mossy_stone) moss_count++;
						bool cv = r.carved(i);
						if (cv && t == water) cave_water++;
						if (cv && t == lava) cave_lava++;
						if (y >= h || (r.filled[i] == none || r.filled[i] == water)) continue;
						int band = std::min(3, y * 4 / sea);
						rock++; band_rock[band]++;
						if (cv) { holes++; band_hole[band]++; }
						if (cv && t == none && y + 1 < H && !is_see_through((block_type)r.blocks[r.at(x, y - 1, z)])) {
							floors++;
							if (r.blocks[r.at(x, y + 1, z)] == none) standable++;
						}
					}
				}
			printf("  underground carved: %.2f%%  (", 100.0 * holes / std::max(rock, 1LL));
			for (int b = 0; b < 4; ++b) printf("%sy%d+ %.1f%%", b ? ", " : "", b * sea / 4, 100.0 * band_hole[b] / std::max(band_rock[b], 1LL));
			printf(")\n");
			printf("  cave floor with standing room: %.1f%%\n", 100.0 * standable / std::max(floors, 1LL));
			printf("  cave water %lld, lava %lld blocks\n", cave_water, cave_lava);
			printf("  ores: coal %lld, iron %lld, gold %lld, redstone %lld, diamond %lld; gravel %lld, mossy stone %lld\n",
				ores[0], ores[1], ores[2], ores[3], ores[4], gravel_count, moss_count);

			long long land = 0, openings = 0;
			vector<unsigned char> open((size_t)S * S, 0);
			for (int x = 0; x < S; ++x)
				for (int z = 0; z < S; ++z) {
					int h = r.surface[(size_t)x * S + z];
					if (h > sea) land++;
					if (h < H && r.carved(r.at(x, h, z))) open[(size_t)x * S + z] = 1;
				}
			vector<int> stack;
			for (int s = 0; s < S * S; ++s) {
				if (open[s] != 1) continue;
				openings++;
				open[s] = 2;
				stack.push_back(s);
				while (!stack.empty()) {
					int p = stack.back(); stack.pop_back();
					for (int dx = -2; dx <= 2; ++dx)
						for (int dz = -2; dz <= 2; ++dz) {
							int qx = p / S + dx, qz = p % S + dz;
							if (qx < 0 || qz < 0 || qx >= S || qz >= S) continue;
							int q = qx * S + qz;
							if (open[q] == 1) { open[q] = 2; stack.push_back(q); }
						}
				}
			}
			long long dark = 0, dim = 0, lit = 0;
			const unsigned char darkest = daylight_level(H, 0);
			for (int x = 0; x < S; ++x)
				for (int z = 0; z < S; ++z)
					for (int y = 0; y < H; ++y) {
						size_t i = r.at(x, y, z);
						if (!r.carved(i) || r.blocks[i] != none) continue;
						unsigned char level = daylight_level(r.surface[(size_t)x * S + z], y);
						if (level <= darkest) dark++;
						else if (level < 15) dim++;
						else lit++;
					}
			long long air = std::max(dark + dim + lit, 1LL);
			printf("  cave air daylight: %.1f%% at the darkest level, %.1f%% dim, %.1f%% fully lit\n",
				100.0 * dark / air, 100.0 * dim / air, 100.0 * lit / air);

			printf("  surface openings: %lld (one per %.0fx%.0f blocks of land)\n", openings,
				sqrt((double)land / std::max(openings, 1LL)), sqrt((double)land / std::max(openings, 1LL)));

			vector<unsigned char> seen(r.blocks.size(), 0);
			vector<long long> sizes;
			vector<size_t> queue;
			for (size_t s = 0; s < r.blocks.size(); ++s) {
				if (seen[s] || !r.carved(s)) continue;
				long long n = 0;
				queue.clear();
				queue.push_back(s);
				seen[s] = 1;
				while (!queue.empty()) {
					size_t p = queue.back(); queue.pop_back(); n++;
					int px = (int)(p / ((size_t)H * S)), py = (int)((p / S) % H), pz = (int)(p % S);
					const int o[6][3] = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1} };
					for (auto& v : o) {
						int nx = px + v[0], ny = py + v[1], nz = pz + v[2];
						if (nx < 0 || ny < 0 || nz < 0 || nx >= S || ny >= H || nz >= S) continue;
						size_t t = r.at(nx, ny, nz);
						if (!seen[t] && r.carved(t)) { seen[t] = 1; queue.push_back(t); }
					}
				}
				sizes.push_back(n);
			}
			sort(sizes.rbegin(), sizes.rend());
			long long total = 0, tiny = 0;
			for (long long n : sizes) { total += n; if (n < 20) tiny++; }
			printf("  cave systems: %zu, of which %lld under 20 blocks; largest holds %.1f%% of cave space\n",
				sizes.size(), tiny, sizes.empty() ? 0.0 : 100.0 * sizes[0] / total);
		}

		printf("\ncaves: block light\n");
		{
			const int n = r.chunks;
			long long lava_count = 0, glow_count = 0;
			for (ChunkData& c : r.parts)
				for (const glm::ivec3& e : c.emitters) (c.at(e.x, e.y, e.z) == lava ? lava_count : glow_count)++;

			double light_ms = 0;
			int lit_chunks = 0;
			for (int i = 0; i < n; ++i)
				for (int k = 0; k < n; ++k) {
					LightNeighbour around[9];
					for (int dx = -1; dx <= 1; ++dx)
						for (int dz = -1; dz <= 1; ++dz) {
							int ni = i + dx, nk = k + dz;
							if (ni < 0 || nk < 0 || ni >= n || nk >= n) continue;
							around[(dx + 1) * 3 + (dz + 1)] = { &r.part(ni, nk).blocks, &r.part(ni, nk).emitters };
						}
					ChunkData& c = r.part(i, k);
					double t0 = now_ms();
					c.lit = compute_block_light(around, W, H, L, c.light);
					light_ms += now_ms() - t0;
					if (c.lit) lit_chunks++;
				}
			printf("  glowing blocks: lava %lld, glowstone %lld\n", lava_count, glow_count);
			printf("  light pass: %.3f ms per chunk on average, %d of %d chunks have light\n", light_ms / (n * n), lit_chunks, n * n);

			//both chunks either side of a border must compute the same light for the cells they share.
			//the outer ring is left out: its missing neighbours read as solid, just as at the render edge
			long long compared = 0, mismatched = 0;
			for (int i = 1; i + 2 < n; ++i)
				for (int k = 1; k + 1 < n; ++k) {
					ChunkData& a = r.part(i, k);
					ChunkData& b = r.part(i + 1, k);
					for (int z = 1; z < L - 1; ++z)
						for (int y = 0; y < H; ++y) {
							compared += 2;
							if (a.light_at(W - 1, y, z) != b.light_at(1, y, z)) mismatched++;
							if (a.light_at(W - 2, y, z) != b.light_at(0, y, z)) mismatched++;
						}
					ChunkData& c = r.part(k, i);
					ChunkData& d2 = r.part(k, i + 1);
					for (int x = 1; x < W - 1; ++x)
						for (int y = 0; y < H; ++y) {
							compared += 2;
							if (c.light_at(x, y, L - 1) != d2.light_at(x, y, 1)) mismatched++;
							if (c.light_at(x, y, L - 2) != d2.light_at(x, y, 0)) mismatched++;
						}
				}
			check("light agrees across chunk borders", mismatched == 0,
				"(" + to_string(mismatched) + " of " + to_string(compared) + " differ)");

			//the open cell beside a glowing block holds one level less than the source
			long long exposed = 0, wrong = 0;
			static const int d[6][3] = { {0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
			for (ChunkData& c : r.parts)
				for (const glm::ivec3& e : c.emitters)
					for (auto& o : d) {
						int x = e.x + o[0], y = e.y + o[1], z = e.z + o[2];
						if (x < 1 || z < 1 || x > W - 2 || z > L - 2 || y < 0 || y >= H) continue;
						if (!is_see_through(c.at(x, y, z))) continue;
						exposed++;
						if (c.light_at(x, y, z) != max_block_light - 1) wrong++;
					}
			check("open cells next to glowing blocks are at level 14", wrong == 0,
				"(" + to_string(wrong) + " of " + to_string(exposed) + ")");

			long long cave_air = 0, glowing_air = 0, quads_lit = 0, quads_unlit = 0;
			for (int i = 0; i < n; ++i)
				for (int k = 0; k < n; ++k) {
					ChunkData& c = r.part(i, k);
					quads_lit += greedy_quads(c);
					bool was_lit = c.lit;
					c.lit = false;
					quads_unlit += greedy_quads(c);
					c.lit = was_lit;
					for (int x = 1; x < W - 1; ++x)
						for (int z = 1; z < L - 1; ++z)
							for (int y = 0; y < H; ++y) {
								size_t ri = r.at(i * CS + x - 1, y, k * CS + z - 1);
								if (!r.carved(ri) || c.at(x, y, z) != none) continue;
								cave_air++;
								if (c.light_at(x, y, z) > 0) glowing_air++;
							}
				}
			//mirrors ChunkManager::relight_around: a newly loaded chunk makes a built neighbour relight
			//only when a glowing block in that neighbour's 3x3 lies within reach of the new chunk
			long long relights = 0, arrivals = 0;
			for (int i = 2; i < n - 2; ++i)
				for (int k = 2; k < n - 2; ++k) {
					arrivals++;
					int lo_x = i * CS, lo_z = k * CS, hi_x = lo_x + CS - 1, hi_z = lo_z + CS - 1;
					for (int dx = -1; dx <= 1; ++dx)
						for (int dz = -1; dz <= 1; ++dz) {
							if (dx == 0 && dz == 0) continue;
							bool flag = false;
							for (int ex = -1; ex <= 1 && !flag; ++ex)
								for (int ez = -1; ez <= 1 && !flag; ++ez) {
									int ni = i + dx + ex, nk = k + dz + ez;
									for (const glm::ivec3& e : r.part(ni, nk).emitters) {
										int wx = ni * CS + e.x - 1, wz = nk * CS + e.z - 1;
										int gap = std::max(std::max(lo_x - wx, wx - hi_x), std::max(lo_z - wz, wz - hi_z));
										if (gap < max_block_light) { flag = true; break; }
									}
								}
							if (flag) relights++;
						}
				}
			printf("  a newly loaded chunk relights %.1f of its 8 neighbours on average\n", (double)relights / std::max(arrivals, 1LL));
			printf("  cave air reached by block light: %.1f%%\n", 100.0 * glowing_air / std::max(cave_air, 1LL));
			printf("  opaque quads: %.0f per chunk with block light, %.0f without\n",
				(double)quads_lit / (n * n), (double)quads_unlit / (n * n));
		}

		auto color = [&](size_t i, unsigned char* px) {
			block_type t = (block_type)r.blocks[i];
			unsigned char c[3] = { 120, 90, 60 };
			if (t == none) { c[0] = 150; c[1] = 200; c[2] = 255; }
			if (t == water) { c[0] = 40; c[1] = 80; c[2] = 200; }
			if (t == lava) { c[0] = 255; c[1] = 140; c[2] = 0; }
			if (t == glowstone) { c[0] = 255; c[1] = 255; c[2] = 200; }
			if (t == stone || t == mossy_stone) { c[0] = 110; c[1] = 110; c[2] = 110; }
			if (t == bedrock) { c[0] = 20; c[1] = 20; c[2] = 20; }
			if (t == gravel) { c[0] = 150; c[1] = 140; c[2] = 130; }
			if (t >= coal_ore && t <= diamond_ore) { c[0] = 250; c[1] = 250; c[2] = 60; }
			if (t == none && r.carved(i)) { c[0] = 235; c[1] = 60; c[2] = 40; }
			memcpy(px, c, 3);
		};
		for (int y : { 8, 18, 28, 38 }) {
			vector<unsigned char> img((size_t)S * S * 3);
			for (int x = 0; x < S; ++x)
				for (int z = 0; z < S; ++z) color(r.at(x, y, z), &img[((size_t)z * S + x) * 3]);
			write_ppm("cave_slice_y" + to_string(y) + ".ppm", S, S, img);
		}
		vector<unsigned char> cut((size_t)S * H * 3);
		for (int x = 0; x < S; ++x)
			for (int y = 0; y < H; ++y) color(r.at(x, y, S / 2), &cut[((size_t)(H - 1 - y) * S + x) * 3]);
		write_ppm("cave_cut.ppm", S, H, cut);
	}

	void run_occlusion(int region_chunks) {
		const int R = region_chunks, S = R * CS, SECTIONS = (H + section_size - 1) / section_size;
		const int cx0 = -R / 2, cz0 = -R / 2;
		printf("\nocclusion: %dx%d chunks\n", R, R);

		vector<unsigned char> world((size_t)S * H * S);
		auto wi = [&](int x, int y, int z) { return ((size_t)x * H + y) * S + z; };
		VisibilityGrid grid;
		grid.min_chunk = glm::ivec2(cx0, cz0);
		grid.span = R;
		grid.sections = SECTIONS;
		grid.links.assign((size_t)R * R * SECTIONS, 0);
		grid.present.assign((size_t)R * R, 1);
		vector<long long> section_faces((size_t)R * R * SECTIONS, 0);

		double link_ms = 0;
		ChunkData c;
		for (int i = 0; i < R; ++i)
			for (int k = 0; k < R; ++k) {
				generate(cx0 + i, cz0 + k, c);
				double t0 = now_ms();
				for (int s = 0; s < SECTIONS; ++s)
					grid.links[((size_t)i * R + k) * SECTIONS + s] = compute_section_links(c.blocks, W, H, L, s * section_size);
				link_ms += now_ms() - t0;
				for (int x = 1; x < W - 1; ++x)
					for (int z = 1; z < L - 1; ++z)
						for (int y = 0; y < H; ++y) {
							block_type t = c.at(x, y, z);
							world[wi(i * CS + x - 1, y, k * CS + z - 1)] = t;
							if (is_see_through(t)) continue;
							static const int d[6][3] = { {0,0,1},{0,0,-1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
							for (auto& o : d) {
								int ny = y + o[1];
								if (ny >= 0 && ny < H && is_see_through(c.at(x + o[0], ny, z + o[2])))
									section_faces[((size_t)i * R + k) * SECTIONS + y / section_size]++;
							}
						}
			}
		printf("  section links: %.3f ms per chunk\n", link_ms / (R * R));

		auto top_solid = [&](int x, int z) {
			int y = H - 1;
			while (y > 0 && is_see_through((block_type)world[wi(x, y, z)])) --y;
			return y;
		};

		struct cam { float x, y, z; int kind; };
		vector<cam> cams;
		WorldRandom rng(1234, 5, 6);
		const int margin = std::min(40, S / 4);
		for (int n = 0; n < 12; ++n) {
			int x = margin + rng.next_int(S - 2 * margin), z = margin + rng.next_int(S - 2 * margin);
			cams.push_back({ x + 0.5f, top_solid(x, z) + 2.62f, z + 0.5f, 0 });
		}
		for (int n = 0, tries = 0; n < 12 && tries < 200000; ++tries) {
			int x = margin + rng.next_int(S - 2 * margin), z = margin + rng.next_int(S - 2 * margin), y = 5 + rng.next_int(40);
			if (y >= top_solid(x, z) - 3) continue;
			if (world[wi(x, y, z)] != none || world[wi(x, y + 1, z)] != none || is_see_through((block_type)world[wi(x, y - 1, z)])) continue;
			cams.push_back({ x + 0.5f, y + 1.62f, z + 0.5f, 1 });
			n++;
		}
		for (int n = 0; n < 4; ++n) cams.push_back({ S * (0.25f + n * 0.15f), H + 8.0f, S * 0.4f, 2 });

		const char* kinds[3] = { "surface", "cave", "sky" };
		long long total_bad = 0;
		double walk_ms = 0;
		for (int kind = 0; kind < 3; ++kind) {
			double culled = 0;
			int count = 0;
			long long bad = 0, rays = 0;
			for (const cam& cm : cams) {
				if (cm.kind != kind) continue;
				int ccx = (int)floor(cm.x / CS) + cx0, ccz = (int)floor(cm.z / CS) + cz0;
				vector<glm::ivec3> starts = {
					glm::ivec3(ccx, (int)floor(cm.y) / section_size, ccz),
					glm::ivec3(ccx, (int)floor(cm.y - 1.62f) / section_size, ccz),
				};
				vector<uint32_t> visible;
				double t0 = now_ms();
				find_visible_sections(grid, starts, [](glm::ivec3) { return true; }, visible);
				walk_ms += now_ms() - t0;

				long long all = 0, drawn = 0;
				for (size_t col = 0; col < (size_t)R * R; ++col)
					for (int s = 0; s < SECTIONS; ++s) {
						long long f = section_faces[col * SECTIONS + s];
						all += f;
						if (visible[col] & (1u << s)) drawn += f;
					}
				culled += 100.0 * (all - drawn) / std::max(all, 1LL);
				count++;

				//every block a sight line passes through or stops at must be in a drawn section
				const int RAYS = 40000;
				for (int k = 0; k < RAYS; ++k) {
					double u = (k + 0.5) / RAYS, phi = k * 2.399963229728653;
					double dy = 1.0 - 2.0 * u, rad = sqrt(std::max(0.0, 1.0 - dy * dy));
					double dx = cos(phi) * rad, dz = sin(phi) * rad;
					int x = (int)floor(cm.x), y = (int)floor(cm.y), z = (int)floor(cm.z);
					int stx = dx > 0 ? 1 : -1, sty = dy > 0 ? 1 : -1, stz = dz > 0 ? 1 : -1;
					double tdx = dx != 0 ? fabs(1.0 / dx) : 1e30, tdy = dy != 0 ? fabs(1.0 / dy) : 1e30, tdz = dz != 0 ? fabs(1.0 / dz) : 1e30;
					double tmx = dx != 0 ? (dx > 0 ? x + 1 - cm.x : cm.x - x) * tdx : 1e30;
					double tmy = dy != 0 ? (dy > 0 ? y + 1 - cm.y : cm.y - y) * tdy : 1e30;
					double tmz = dz != 0 ? (dz > 0 ? z + 1 - cm.z : cm.z - z) * tdz : 1e30;
					for (int steps = 0; steps < 400; ++steps) {
						if (x < 0 || z < 0 || x >= S || z >= S || y < 0) break;
						if (y < H) {
							if (!(visible[(size_t)(x / CS) * R + z / CS] & (1u << (y / section_size)))) { bad++; break; }
							if (!is_see_through((block_type)world[wi(x, y, z)])) break;
						}
						if (tmx < tmy && tmx < tmz) { x += stx; tmx += tdx; }
						else if (tmy < tmz) { y += sty; tmy += tdy; }
						else { z += stz; tmz += tdz; }
					}
				}
				rays += RAYS;
			}
			total_bad += bad;
			printf("  %-8s cameras %2d: faces culled %.1f%%, rays reaching a culled section %lld of %lld\n",
				kinds[kind], count, culled / std::max(count, 1), bad, rays);
		}
		printf("  visibility walk: %.3f ms per camera (no frustum)\n", walk_ms / cams.size());
		check("no visible block in a culled section", total_bad == 0, "(" + to_string(total_bad) + " rays)");
	}
}

int main(int argc, char** argv) {
	string mode = "all";
	int region = 16;
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if (arg.rfind("region=", 0) == 0) region = std::max(4, atoi(arg.c_str() + 7));
		else mode = arg;
	}
	if (mode != "all" && mode != "caves" && mode != "occlusion") {
		printf("usage: worldgen_check [caves|occlusion|all] [region=<chunks per side>]\n");
		return 2;
	}

	if (mode == "all" || mode == "caves") run_caves(region);
	if (mode == "all" || mode == "occlusion") run_occlusion(region);

	printf("\n%s (%d failed)\n", failures ? "FAILURES" : "ALL PASS", failures);
	return failures ? 1 : 0;
}
