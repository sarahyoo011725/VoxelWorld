#pragma once
#include "ChunkManager.h"

class Chunk;

class StructureGenerator {
private:
	ChunkManager& cm;
	StructureGenerator() : cm(ChunkManager::get_instance()) {};
	StructureGenerator(const StructureGenerator&) = delete;
	StructureGenerator& operator=(const StructureGenerator&) = delete;
public:
	static StructureGenerator& get_instance() {
		static StructureGenerator instance;
		return instance;
	}
	void spawn_tree(vec3 world_vec3);
};

