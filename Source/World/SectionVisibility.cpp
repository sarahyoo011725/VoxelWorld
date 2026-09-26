#include "SectionVisibility.h"
#include <algorithm>

using namespace std;

section_links compute_section_links(const vector<Block>& blocks, int width, int height, int length, int y0) {
	const int sx = width - 2, sz = length - 2;
	const int sy = std::min(section_size, height - y0);
	if (sy <= 0) return all_links;

	/*
		the section plus a one-cell shell. shell cells hold the face they lie on,
		so the fill learns which faces a region reaches without any bounds checks
	*/
	enum : unsigned char { solid = 0, open = 1, filled = 2, shell = 0x10 };
	const int px = sx + 2, py = sy + 2, pz = sz + 2;
	const int step_x = py * pz, step_y = pz;
	thread_local vector<unsigned char> cells;
	thread_local vector<int> stack;
	cells.assign((size_t)px * py * pz, solid);

	int open_count = 0;
	for (int x = 0; x < px; ++x) {
		for (int y = 0; y < py; ++y) {
			for (int z = 0; z < pz; ++z) {
				unsigned char& c = cells[(size_t)x * step_x + y * step_y + z];
				if (x == 0) c = shell | face_neg_x;
				else if (x == px - 1) c = shell | face_pos_x;
				else if (y == 0) c = shell | face_neg_y;
				else if (y == py - 1) c = shell | face_pos_y;
				else if (z == 0) c = shell | face_neg_z;
				else if (z == pz - 1) c = shell | face_pos_z;
				else if (is_see_through(blocks[((size_t)x * height + (y0 + y - 1)) * length + z].type)) {
					c = open;
					++open_count;
				}
			}
		}
	}
	if (open_count == 0) return 0;
	if (open_count == sx * sy * sz) return all_links;

	const int offsets[6] = { -step_x, step_x, -step_y, step_y, -1, 1 };
	section_links links = 0;
	for (int start = 0; start < px * py * pz; ++start) {
		if (cells[start] != open) continue;

		unsigned touched = 0;
		cells[start] = filled;
		stack.clear();
		stack.push_back(start);
		while (!stack.empty()) {
			int c = stack.back();
			stack.pop_back();
			for (int offset : offsets) {
				int n = c + offset;
				unsigned char v = cells[n];
				if (v == open) {
					cells[n] = filled;
					stack.push_back(n);
				}
				else if (v & shell) {
					touched |= 1u << (v & 7);
				}
			}
		}

		for (int f = 0; f < 6; ++f) {
			if (touched & (1u << f)) links |= (section_links)touched << (f * 6);
		}
		if (links == all_links) return links;
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
