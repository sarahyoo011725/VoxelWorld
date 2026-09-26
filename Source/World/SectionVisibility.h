#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include "Block/Block.h"

/*
	cave culling in the style of Tommaso Checchi's "advanced cave culling".
	a chunk column is split into cubic sections; for each one we record which of
	its six faces are joined by see-through blocks. each frame a breadth-first
	walk starts at the camera's section and only crosses into a neighbour through
	a face its entry face is joined to, never heading back towards the camera.
	sections the walk cannot reach are hidden behind solid rock and are skipped.
*/

const int section_size = 16;

//ordered so that the opposite of face f is f ^ 1
enum section_face {
	face_neg_x,
	face_pos_x,
	face_neg_y,
	face_pos_y,
	face_neg_z,
	face_pos_z,
};

//bit a * 6 + b is set when faces a and b are joined through see-through blocks
using section_links = uint64_t;
const section_links all_links = (1ull << 36) - 1;

inline bool faces_linked(section_links links, int a, int b) {
	return (links >> (a * 6 + b)) & 1ull;
}

//water, glass and plants are looked through; leaves are opaque tiles, so they occlude
inline bool is_see_through(block_type type) {
	return type == none || has_transparency(type);
}

/*
	links for the section starting at y0 of a chunk's block array, indexed
	(x * height + y) * length + z. the one-block border is not part of it.
*/
section_links compute_section_links(const std::vector<Block>& blocks, int width, int height, int length, int y0);

//the loaded area as a grid of columns; links are indexed (x * span + z) * sections + section
struct VisibilityGrid {
	glm::ivec2 min_chunk = glm::ivec2(0);
	int span = 0;
	int sections = 0;
	std::vector<section_links> links;
	std::vector<unsigned char> present;
};

/*
	fills visible with one bitmask of reachable sections per column. starts are in
	(chunk x, section, chunk z); in_view rejects sections outside the frustum, which
	is safe because every sight line stays inside it.
*/
void find_visible_sections(const VisibilityGrid& grid, const std::vector<glm::ivec3>& starts,
	const std::function<bool(glm::ivec3)>& in_view, std::vector<uint32_t>& visible);
