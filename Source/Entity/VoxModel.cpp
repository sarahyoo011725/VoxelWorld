#include "VoxModel.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

namespace {
	int32_t read_int(const std::vector<char>& data, size_t at) {
		int32_t v;
		memcpy(&v, data.data() + at, 4);
		return v;
	}

	VoxModel parse(const std::string& path) {
		VoxModel model;
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			std::cerr << "Could not open voxel model " << path << std::endl;
			return model;
		}
		std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (data.size() < 20 || memcmp(data.data(), "VOX ", 4) != 0) {
			std::cerr << "Not a .vox file: " << path << std::endl;
			return model;
		}

		//chunks follow the MAIN chunk's header: id, content size, children size, content
		ivec3 vox_size = ivec3(0);
		std::vector<u8vec4> voxels;
		for (size_t at = 20; at + 12 <= data.size();) {
			std::string id(data.data() + at, 4);
			int32_t content = read_int(data, at + 4);
			size_t body = at + 12;
			if (id == "SIZE") {
				vox_size = ivec3(read_int(data, body), read_int(data, body + 4), read_int(data, body + 8));
			}
			else if (id == "XYZI") {
				int32_t count = read_int(data, body);
				for (int32_t i = 0; i < count; ++i) {
					const unsigned char* v = (const unsigned char*)data.data() + body + 4 + i * 4;
					voxels.push_back(u8vec4(v[0], v[1], v[2], v[3]));
				}
			}
			else if (id == "RGBA") {
				//the file's entry i is palette index i + 1
				for (int i = 0; i < 255; ++i) {
					const unsigned char* c = (const unsigned char*)data.data() + body + i * 4;
					model.palette[i + 1] = u8vec4(c[0], c[1], c[2], c[3]);
				}
			}
			at = body + content;
		}

		//MagicaVoxel stands z up and y forward; the engine stands y up and z forward
		model.size = ivec3(vox_size.x, vox_size.z, vox_size.y);
		model.cells.assign((size_t)model.size.x * model.size.y * model.size.z, 0);
		for (const u8vec4& v : voxels) {
			if (v.x >= model.size.x || v.z >= model.size.y || v.y >= model.size.z) continue;
			model.cells[((size_t)v.x * model.size.y + v.z) * model.size.z + v.y] = v.w;
		}

		std::ifstream origin(path + ".origin");
		if (origin) origin >> model.origin.x >> model.origin.y >> model.origin.z;
		model.loaded = true;
		return model;
	}
}

const VoxModel& load_vox(const std::string& path) {
	static std::map<std::string, VoxModel> cache;
	auto found = cache.find(path);
	if (found != cache.end()) return found->second;
	return cache.emplace(path, parse(path)).first->second;
}
