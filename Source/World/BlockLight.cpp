#include "BlockLight.h"
#include "SectionVisibility.h"
#include <algorithm>

using namespace std;

namespace {
	int floor_div(int a, int b) {
		return (a >= 0 ? a : a - b + 1) / b;
	}

	const unsigned char known_solid = 0xFF;
}

void find_emitters(const vector<Block>& blocks, int width, int height, int length, vector<glm::ivec3>& emitters) {
	emitters.clear();
	for (int x = 1; x < width - 1; ++x)
		for (int y = 0; y < height; ++y)
			for (int z = 1; z < length - 1; ++z)
				if (is_emissive(blocks[((size_t)x * height + y) * length + z].type)) emitters.push_back(glm::ivec3(x, y, z));
}

bool compute_block_light(const LightNeighbour neighbourhood[9], int width, int height, int length, vector<unsigned char>& light) {
	const int chunk_w = width - 2, chunk_l = length - 2;
	const int reach = max_block_light;

	//the window spans every cell light could travel through on its way to one of
	//the centre chunk's cells, in coordinates where the centre's interior starts at 0
	const int win_x0 = -reach, win_z0 = -reach;
	const int win_w = chunk_w + 2 * reach, win_l = chunk_l + 2 * reach;

	struct cell { short x, y, z; };
	thread_local vector<cell> seeds;
	seeds.clear();
	int y_min = height, y_max = -1;
	for (int k = 0; k < 9; ++k) {
		const LightNeighbour& n = neighbourhood[k];
		if (!n.blocks || !n.emitters) continue;
		int dx = k / 3 - 1, dz = k % 3 - 1;
		for (const glm::ivec3& e : *n.emitters) {
			int wx = dx * chunk_w + e.x - 1 - win_x0, wz = dz * chunk_l + e.z - 1 - win_z0;
			if (wx < 0 || wz < 0 || wx >= win_w || wz >= win_l) continue;
			seeds.push_back({ (short)wx, (short)e.y, (short)wz });
			y_min = std::min(y_min, e.y);
			y_max = std::max(y_max, e.y);
		}
	}
	if (seeds.empty()) return false;

	const int y0 = std::max(0, y_min - reach), y1 = std::min(height - 1, y_max + reach);
	const int win_h = y1 - y0 + 1;
	thread_local vector<unsigned char> grid;
	grid.assign((size_t)win_w * win_h * win_l, 0);
	auto at = [&](int x, int y, int z) -> unsigned char& {
		return grid[((size_t)x * win_h + (y - y0)) * win_l + z];
	};

	//reads a block from the chunk that owns it
	auto see_through = [&](int x, int y, int z) {
		int ix = x + win_x0, iz = z + win_z0;
		int cx = floor_div(ix, chunk_w), cz = floor_div(iz, chunk_l);
		const vector<Block>* blocks = neighbourhood[(cx + 1) * 3 + (cz + 1)].blocks;
		if (!blocks) return false;
		int lx = ix - cx * chunk_w + 1, lz = iz - cz * chunk_l + 1;
		return is_see_through((*blocks)[((size_t)lx * height + y) * length + lz].type);
	};

	//breadth first from every glowing block at once, so each cell is reached first by its brightest light
	thread_local vector<cell> queue;
	queue.clear();
	for (const cell& s : seeds) {
		at(s.x, s.y, s.z) = max_block_light;
		queue.push_back(s);
	}
	const int step[6][3] = { {-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1} };
	for (size_t head = 0; head < queue.size(); ++head) {
		cell c = queue[head];
		unsigned char level = at(c.x, c.y, c.z);
		if (level <= 1) continue;
		unsigned char next = level - 1;
		for (const auto& d : step) {
			int nx = c.x + d[0], ny = c.y + d[1], nz = c.z + d[2];
			if (nx < 0 || nz < 0 || nx >= win_w || nz >= win_l || ny < y0 || ny > y1) continue;
			unsigned char& v = at(nx, ny, nz);
			if (v == known_solid || v >= next) continue;
			if (!see_through(nx, ny, nz)) {
				v = known_solid;
				continue;
			}
			v = next;
			queue.push_back({ (short)nx, (short)ny, (short)nz });
		}
	}

	light.assign((size_t)width * height * length, 0);
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			int gx = x - 1 - win_x0, gz = z - 1 - win_z0;
			for (int y = y0; y <= y1; ++y) {
				unsigned char v = at(gx, y, gz);
				if (v != known_solid) light[((size_t)x * height + y) * length + z] = v;
			}
		}
	}
	return true;
}
