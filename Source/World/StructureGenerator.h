#pragma once
#include "World/ChunkManager.h"
#include "Entity/Geometries.h"
#include "Block/BlockType.h"

class Chunk;

/*
	a singleton class that creates structures across chunks
*/
class StructureGenerator {
private:
	ChunkManager& chunk_manager;
	StructureGenerator() : chunk_manager(ChunkManager::get_instance()) {};
	StructureGenerator(const StructureGenerator&) = delete;
	StructureGenerator& operator=(const StructureGenerator&) = delete;
public:
	static StructureGenerator& get_instance() {
		static StructureGenerator instance;
		return instance;
	}
	void spawn_nonblock_structure(block_type type, vec3 world_coord);
	void spawn_tree(vec3 world_vec3);
	void spawn_grass(vec3 world_vec3);
};

namespace {
	/*
		vertices for a grass geometry.
		adds front and back faces to make grass visible in either side, preventing it from disappearing due to back-face culling
	*/
	static vector<vector<vertex>> grass_face_vertices = {
		//face 1 front
		{
			{vec3(-0.5, 0.5, -0.5),		vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, 0.5), 		vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, 0.5), 		vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5, -0.5),	vec2(0.0, 0.0)},
		},
		//face 1 back
		{
			{vec3(0.5, 0.5, 0.5),		vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, -0.5),		vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5, -0.5),	vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, 0.5),		vec2(0.0, 0.0)},
		},
		//face 2 front
		{
			{vec3(-0.5, 0.5, 0.5),		vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5),		vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, -0.5),		vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),		vec2(0.0, 0.0)},
		},
		//face 2 back
		{
			{vec3(0.5, 0.5, -0.5),		vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, 0.5),		vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5, 0.5),		vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, -0.5),		vec2(0.0, 0.0)},
		},
	};
}