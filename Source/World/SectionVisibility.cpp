#include "SectionVisibility.h"
#include <algorithm>

using namespace std;

section_links compute_section_links(const vector<Block>& blocks, int width, int height, int length, int y0) {
	const int sx = width - 2, sz = length - 2;
	const int sy = std::min(section_size, height - y0);
	if (sy <= 0) return all_links;

	auto block_at = [&](int x, int y, int z) {
		return blocks[((size_t)(x + 1) * height + (y0 + y)) * length + (z + 1)].type;
	};

	int open = 0;
	for (int x = 0; x < sx; ++x)
		for (int y = 0; y < sy; ++y)
			for (int z = 0; z < sz; ++z)
				if (is_see_through(block_at(x, y, z))) ++open;
	if (open == 0) return 0;
	if (open == sx * sy * sz) return all_links;

	thread_local vector<unsigned char> seen;
	thread_local vector<int> stack;
	seen.assign((size_t)sx * sy * sz, 0);
	auto cell = [&](int x, int y, int z) { return (x * sy + y) * sz + z; };

	section_links links = 0;
	for (int ix = 0; ix < sx; ++ix) {
		for (int iy = 0; iy < sy; ++iy) {
			for (int iz = 0; iz < sz; ++iz) {
				int start = cell(ix, iy, iz);
				if (seen[start] || !is_see_through(block_at(ix, iy, iz))) continue;

				unsigned touched = 0;
				seen[start] = 1;
				stack.clear();
				stack.push_back(start);
				while (!stack.empty()) {
					int c = stack.back();
					stack.pop_back();
					int x = c / (sy * sz), y = (c / sz) % sy, z = c % sz;
					if (x == 0) touched |= 1u << face_neg_x;
					if (x == sx - 1) touched |= 1u << face_pos_x;
					if (y == 0) touched |= 1u << face_neg_y;
					if (y == sy - 1) touched |= 1u << face_pos_y;
					if (z == 0) touched |= 1u << face_neg_z;
					if (z == sz - 1) touched |= 1u << face_pos_z;

					const int step[6][3] = { {-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1} };
					for (const auto& d : step) {
						int nx = x + d[0], ny = y + d[1], nz = z + d[2];
						if (nx < 0 || ny < 0 || nz < 0 || nx >= sx || ny >= sy || nz >= sz) continue;
						int n = cell(nx, ny, nz);
						if (seen[n] || !is_see_through(block_at(nx, ny, nz))) continue;
						seen[n] = 1;
						stack.push_back(n);
					}
				}

				for (int a = 0; a < 6; ++a) {
					if (!(touched & (1u << a))) continue;
					for (int b = 0; b < 6; ++b) {
						if (touched & (1u << b)) links |= 1ull << (a * 6 + b);
					}
				}
				if (links == all_links) return links;
			}
		}
	}
	return links;
}

void find_visible_sections(const VisibilityGrid& grid, const vector<glm::ivec3>& starts,
	const function<bool(glm::ivec3)>& in_view, vector<uint32_t>& visible) {
	const int span = grid.span, sections = grid.sections;
	visible.assign((size_t)span * span, 0);
	if (span <= 0 || sections <= 0) return;

	//per section: bits 0-5 are the faces already entered through, bit 6 marks a start
	thread_local vector<unsigned char> entered;
	//per section: 0 untested, 1 in view, 2 outside the frustum
	thread_local vector<unsigned char> view_state;
	entered.assign((size_t)span * span * sections, 0);
	view_state.assign((size_t)span * span * sections, 0);

	struct step {
		int x, y, z;
		int entry;
		unsigned char travelled;
	};
	thread_local vector<step> queue;
	queue.clear();

	auto section_index = [&](int x, int y, int z) { return ((size_t)x * span + z) * sections + y; };

	for (glm::ivec3 s : starts) {
		int x = s.x - grid.min_chunk.x, z = s.z - grid.min_chunk.y;
		if (x < 0 || z < 0 || x >= span || z >= span) continue;
		if (!grid.present[(size_t)x * span + z]) continue;
		int y = glm::clamp(s.y, 0, sections - 1);
		unsigned char& e = entered[section_index(x, y, z)];
		if (e & 64) continue;
		e |= 64;
		visible[(size_t)x * span + z] |= 1u << y;
		queue.push_back({ x, y, z, -1, 0 });
	}

	const int step_of[6][3] = { {-1,0,0},{1,0,0},{0,-1,0},{0,1,0},{0,0,-1},{0,0,1} };
	for (size_t head = 0; head < queue.size(); ++head) {
		step cur = queue[head];
		section_links links = grid.links[section_index(cur.x, cur.y, cur.z)];

		for (int f = 0; f < 6; ++f) {
			if (cur.travelled & (1u << (f ^ 1))) continue;
			if (cur.entry >= 0 && !faces_linked(links, cur.entry, f)) continue;

			int nx = cur.x + step_of[f][0], ny = cur.y + step_of[f][1], nz = cur.z + step_of[f][2];
			if (nx < 0 || nz < 0 || nx >= span || nz >= span || ny < 0 || ny >= sections) continue;
			if (!grid.present[(size_t)nx * span + nz]) continue;

			size_t n = section_index(nx, ny, nz);
			int entry = f ^ 1;
			if (entered[n] & (1u << entry)) continue;

			if (view_state[n] == 0) {
				view_state[n] = in_view(glm::ivec3(nx + grid.min_chunk.x, ny, nz + grid.min_chunk.y)) ? 1 : 2;
			}
			if (view_state[n] == 2) continue;

			entered[n] |= (unsigned char)(1u << entry);
			visible[(size_t)nx * span + nz] |= 1u << ny;
			queue.push_back({ nx, ny, nz, entry, (unsigned char)(cur.travelled | (1u << f)) });
		}
	}
}
