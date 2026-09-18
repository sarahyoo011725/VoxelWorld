#pragma once
#include "World/ChunkManager.h"
#include "World/WorldRandom.h"
#include "World/Biome.h"
#include "Entity/Geometries.h"
#include "Block/BlockType.h"
#include <functional>
#include <map>

class Chunk;

//which of a biome's two vegetation budgets a rule draws from
enum class vegetation_kind {
	tree,         //uses BiomeDefinition::tree_chance
	ground_cover, //uses BiomeDefinition::ground_cover_chance
};

/*
	a rule for procedurally spawning a structure while generating a chunk.
	the density comes from the biome rather than the rule, so a new biome sets
	its own vegetation without any rule changing, and a new rule only has to say
	which budget it belongs to.
*/
struct structure_rule {
	vegetation_kind kind;
	function<void(vec3)> spawn;
};

//1 in N for this rule in this biome, or 0 where it should never spawn
inline int spawn_chance_in(const structure_rule& rule, const BiomeDefinition& biome) {
	return rule.kind == vegetation_kind::tree ? biome.tree_chance : biome.ground_cover_chance;
}

//keeps a structure's own randomness on a different stream from the per-column
//roll that decided to spawn it
const static uint32_t tree_salt = 0x7EE5;

/*
	a singleton class that creates structures across chunks.
	to add a new structure: write a spawn_X method, then register it in
	terrain_structures (world generation) and/or placeable_structures (player placement)
	in the constructor - no other class needs to change.
*/
class StructureGenerator {
private:
	ChunkManager& chunk_manager;
	StructureGenerator();
	StructureGenerator(const StructureGenerator&) = delete;
	StructureGenerator& operator=(const StructureGenerator&) = delete;

	void spawn_tree(vec3 world_coord);
	void spawn_grass(vec3 world_coord);
public:
	static StructureGenerator& get_instance() {
		static StructureGenerator instance;
		return instance;
	}
	vector<structure_rule> terrain_structures;
	map<block_type, function<void(vec3)>> placeable_structures;
	void spawn_nonblock_structure(block_type type, vec3 world_coord);
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