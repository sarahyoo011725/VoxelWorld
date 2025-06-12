#pragma once
#include "ChunkManager.h"
#include "BlockType.h"

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
	void spawn_grass(vec3 world_vec3);
};

namespace {
	static vec2 grass_text_coord = vec2(8, 14);
	//add front and back of faces to make grass visible in either side (prevent disappearing due to back-face culling)
	//TODO: fix grass mesh issue when back face cull mode is on
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
			{vec3(0.5, 0.5, -0.5),		vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),		vec2(0.0, 0.0)},
		},
		//face 2 back
		{
			{vec3(0.5, 0.5, -0.5),		vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, 0.5),		vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5, 0.5),		vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, -0.5),		vec2(0.0, 0.0)},
		}
	};
}