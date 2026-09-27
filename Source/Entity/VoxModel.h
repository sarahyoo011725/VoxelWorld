#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace glm;

/*
	a MagicaVoxel .vox model, turned to the engine's axes (y up, z forward), so a
	mob part can be sculpted voxel by voxel rather than built from boxes. files
	stay editable in MagicaVoxel. a sidecar "<file>.origin" holds three numbers:
	where the grid's corner sits relative to the part's pivot, in voxels
*/
struct VoxModel {
	ivec3 size = ivec3(0);
	vec3 origin = vec3(0.0f);
	std::vector<uint8_t> cells; //palette index per cell, 0 empty
	std::array<u8vec4, 256> palette{}; //palette[i] is the colour of index i
	bool loaded = false;

	uint8_t at(int x, int y, int z) const {
		if (x < 0 || y < 0 || z < 0 || x >= size.x || y >= size.y || z >= size.z) return 0;
		return cells[((size_t)x * size.y + y) * size.z + z];
	}
};

//loaded once per path and kept
const VoxModel& load_vox(const std::string& path);
