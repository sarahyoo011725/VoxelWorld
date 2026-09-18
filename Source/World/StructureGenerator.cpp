#include "StructureGenerator.h"
#include "Chunk/Chunk.h"

StructureGenerator::StructureGenerator() : chunk_manager(ChunkManager::get_instance()) {
	terrain_structures = {
		{ vegetation_kind::tree, [this](vec3 c) { spawn_tree(c); } },
		{ vegetation_kind::ground_cover, [this](vec3 c) { spawn_plant(c, grass); } },
		//the biome picks which flower, so one rule covers every kind
		{ vegetation_kind::flower, [this](vec3 c) { spawn_plant(c, flower_of(c)); } },
	};
	placeable_structures = {
		{ grass, [this](vec3 c) { spawn_plant(c, grass); } },
		{ flower_red, [this](vec3 c) { spawn_plant(c, flower_red); } },
		{ flower_yellow, [this](vec3 c) { spawn_plant(c, flower_yellow); } },
		{ flower_purple, [this](vec3 c) { spawn_plant(c, flower_purple); } },
		{ flower_white, [this](vec3 c) { spawn_plant(c, flower_white); } },
	};
}

/*
	looks up the structure registered for a block type and spawns it
*/
void StructureGenerator::spawn_nonblock_structure(block_type type, vec3 world_coord) {
	const auto& rule = placeable_structures.find(type);
	if (rule != placeable_structures.end()) {
		rule->second(world_coord);
	}
}

/*
	the biome at a world coordinate, or plains where that chunk isn't loaded
*/
const BiomeDefinition& StructureGenerator::biome_at(vec3 world_coord) {
	Chunk* chunk = chunk_manager.get_chunk(get_chunk_origin(world_coord));
	if (chunk == nullptr) return biome_of(biome_id::plains);
	ivec3 local = world_to_local_coord(world_coord);
	return biome_of(chunk->get_biome(local.x, local.z));
}

block_type StructureGenerator::flower_of(vec3 world_coord) {
	return biome_at(world_coord).flower;
}

void StructureGenerator::spawn_plant(vec3 world_coord, block_type type) {
	//draw grass
	ivec2 chunk_id = get_chunk_origin(world_coord);
	Chunk* chunk = chunk_manager.get_chunk(chunk_id);
	if (chunk != nullptr) {
		ivec3 local_coord = world_to_local_coord(world_coord);
		Block* block = chunk->get_block(local_coord);
		if (block == nullptr || block != nullptr && block->type != none) return; //spawn grass only if there is no structure

		//grass ships greyscale and takes the biome's colour; flowers are authored
		//in colour and pass through untinted. without a tint set here the vertex
		//would default to black and swallow the texture entirely.
		vec3 tint = vec3(1.0f);
		if (is_biome_tinted(type)) {
			tint = biome_of(chunk->get_biome(local_coord.x, local_coord.z)).foliage_tint;
		}

		vector<foliage_vertex> transformed_vertices;
		for (vector<vertex> face : grass_face_vertices) {
			vec3 edge1 = face[1].position - face[0].position;
			vec3 edge2 = face[2].position - face[0].position;
			vec3 plane_normal = normalize(cross(edge2, edge1));

			for (int i = 0; i < face.size(); ++i) {
				vec3 position = face[i].position + world_coord;
				vec2 uv_coord = convert_to_uv(i, plant_texture_coord(type));
				float sway = (face[i].position.y > 0.0f) ? 1.0f : 0.0f; //base stays pinned to the ground
				transformed_vertices.push_back({ position, uv_coord, plane_normal, sway, tint });
			}
		}
		chunk->add_nonblock_structure_vertices(local_coord, transformed_vertices);
		chunk->set_block(local_coord, type);
	}
}

/*
	spawns a tree structure at a world coordinate.
	a tree is made of blocks
*/
void StructureGenerator::spawn_tree(vec3 world_coord) {
	//checks if there are any trees 1 block away.
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			int wx = world_coord.x + dx;
			int wz = world_coord.z + dz;
			Block* block = chunk_manager.get_block_worldspace(vec3(wx, world_coord.y, wz));
			if (block == nullptr || block != nullptr && block->type == wood) {
				return;
			} 
		}
	}

	//seeded from the trunk position so a given tree is always shaped the same,
	//no matter when or in what order its chunk gets generated
	WorldRandom rng(get_terrain_generator().config.world_seed,
		(int)world_coord.x, (int)world_coord.z, tree_salt);

	//make a stem, height varies a bit so trees aren't all identical
	int trunk_height = 4 + rng.next_int(3);
	for (int h = 0; h < trunk_height; ++h) {
		chunk_manager.set_block_worldspace(vec3(world_coord.x, world_coord.y + h, world_coord.z), wood);
	}

	float layer_radius[4] = { 2.2f, 2.2f, 2.0f, 1.6f };
	for (int dx = -2; dx <= 2; ++dx) {
		for (int dz = -2; dz <= 2; ++dz) {
			for (int dy = 0; dy < 4; ++dy) {
				if (dx == 0 && dz == 0 && dy < 2) continue;

				float dist = sqrt((float)(dx * dx + dz * dz));
				if (dist > layer_radius[dy]) continue;
				if (dist > layer_radius[dy] - 0.5f && rng.next_int(5) == 0) continue;

				int wx = world_coord.x + dx;
				int wy = world_coord.y + trunk_height - 2 + dy;
				int wz = world_coord.z + dz;
				chunk_manager.set_block_worldspace(vec3(wx, wy, wz), leaf);
			}
		}
	}

	chunk_manager.set_block_worldspace(vec3(world_coord.x, world_coord.y + trunk_height - 2 + 4, world_coord.z), leaf);
}
