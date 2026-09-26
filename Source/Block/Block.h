#pragma once

#include <glad/glad.h>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "BlockType.h"

using namespace std;
using namespace glm;

//defines attributes of vertex
struct vertex {
	vec3 position;
	vec2 texture;
	vec3 normal;
	vec2 tile_origin;
	vec3 tint; //biome colour multiplied onto greyscale tiles; white leaves a tile as authored
};

//a vertex for wind-swayed geometry (leaves, grass). sway is 0 at a pinned base and 1 at a freely swaying tip
struct foliage_vertex {
	vec3 position;
	vec2 texture;
	vec3 normal;
	float sway;
	vec3 tint;
};

/*
	a single voxel's data.
	deliberately not a GameObject - blocks are static, so a velocity/hitbox_margin
	per voxel would just be dead weight multiplied by thousands of blocks per chunk.
	it holds no position either: that is derivable from the block's index in its
	chunk, and storing it made every voxel 16 bytes instead of 1.
*/
class Block
{
public:
	block_type type = none;
};

namespace {
	//checks if a block type has a transparency
	bool has_transparency(block_type type) {
		switch (type) {
		case water:
		case glass:
		case grass:
		case flower_red:
		case flower_yellow:
		case flower_purple:
		case flower_white:
			return true;
		}
		return false;
	}

	//checks if a block type has a transparency by passing a block pointer
	bool has_transparency(Block* block) {
		if (block == nullptr) return false;
		return has_transparency(block->type);
	}

	//checks if a block type is solid
	bool is_solid(block_type type) {
		switch (type) {
		case none:
		case water:
		case grass:
		case flower_red:
		case flower_yellow:
		case flower_purple:
		case flower_white:
			return false;
		}
		return true;
	}

	//checks if a block type is solid by passing a block pointer
	bool is_solid(Block* block) {
		if (block == nullptr) return false;
		return is_solid(block->type);
	}

	//checks if a block type is non-block geometry
	bool is_nonblock(block_type type) {
		switch (type) {
		case grass:
		case flower_red:
		case flower_yellow:
		case flower_purple:
		case flower_white:
			return true;
		}
		return false;
	}

	bool is_breakable(block_type type) {
		return type != none && type != bedrock;
	}

	//checks if a block type should wave in the wind
	bool is_foliage(block_type type) {
		switch (type) {
		case leaf:
		case grass:
		case flower_red:
		case flower_yellow:
		case flower_purple:
		case flower_white:
			return true;
		}
		return false;
	}

	//the outward-facing unit normal of a cube face, for lighting
	vec3 face_normal(block_face face) {
		switch (face) {
		case Top: return vec3(0.0f, 1.0f, 0.0f);
		case Bottom: return vec3(0.0f, -1.0f, 0.0f);
		case Left: return vec3(-1.0f, 0.0f, 0.0f);
		case Right: return vec3(1.0f, 0.0f, 0.0f);
		case Front: return vec3(0.0f, 0.0f, 1.0f);
		case Back: return vec3(0.0f, 0.0f, -1.0f);
		}
		return vec3(0.0f, 1.0f, 0.0f);
	}

	//checks if a block type can be created underwater
	bool can_be_placed_underwater(block_type type) {
		switch (type) {
		case grass:
		case flower_red:
		case flower_yellow:
		case flower_purple:
		case flower_white:
			return false;
		}
		return true;
	}

	/*
	* converts texture coord from texture_map into uv texture coord (scale it to between 0.0 and 1.0)
	* uv column & row range: 0 ~ 1.
	* index: index of a vertex from a face vertices. 
	* face left-top index: 0 / face right-top index: 1 / face right-bottom index: 2 / face left_bottom index = 3 (clockwise order)
	* *** the unit for block data modification is face, which is a 2d square.
	*/
	static vec2 convert_to_uv(int index, vec2 texture_coord) {
		//row 1 is the top of the atlas but the image is flipped on load, so a
		//tile's v range runs from (rows - row) upward
		float u_min = (texture_coord.x - 1) / textures_columns;
		float u_max = texture_coord.x / textures_columns;
		float v_min = (texture_rows - texture_coord.y) / texture_rows;
		float v_max = (texture_rows - texture_coord.y + 1) / texture_rows;
		if (index == 0) return vec2(u_min, v_max);
		if (index == 1) return vec2(u_max, v_max);
		if (index == 2) return vec2(u_max, v_min);
		if (index == 3) return vec2(u_min, v_min);
		return vec2(-1, -1); //invalid index
	}

	//the atlas-space origin of a tile, for the tiled uv form merged quads use
	static vec2 tile_uv_origin(vec2 texture_coord) {
		return vec2((texture_coord.x - 1) / textures_columns,
			(texture_rows - texture_coord.y) / texture_rows);
	}

	//a front face map of vertices and texture coordinates for a cube, winded in clock wise
	static map<block_face, vector<vertex>> cw_face_map = {
		{Front, {
			{vec3(-0.5, 0.5, 0.5),  vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(1.0, 0.0)},	
			{vec3(-0.5,-0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Back, {
			{vec3(0.5, 0.5, -0.5) ,	vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5,-0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5,-0.5,-0.5),	vec2(1.0, 0.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(0.0, 0.0)},
		}},
		{Left, {
			{vec3(-0.5, 0.5, -0.5),	vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, 0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5,-0.5),	vec2(0.0, 0.0)},
		}},
		{Right, {
			{vec3(0.5, 0.5, 0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Top, {
			{vec3(-0.5, 0.5,-0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5, 0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Bottom, {
			{vec3(0.5, -0.5, -0.5), vec2(0.0, 1.0)},
			{vec3(-0.5,-0.5,-0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
		}},
	};

	//back face map of vertices and texture coordinates for a cube, winded in counter-clock wise.
	static map<block_face, vector<vertex>> ccw_face_map = {
		{Front, {
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, 0.5, 0.5),  vec2(0.0, 1.0)},
			{vec3(-0.5,-0.5, 0.5),	vec2(0.0, 0.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
		}},
		{Back, {
			{vec3(-0.5, 0.5,-0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, 0.5, -0.5) ,	vec2(0.0, 1.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(0.0, 0.0)},
			{vec3(-0.5,-0.5,-0.5),	vec2(1.0, 0.0)},
		}},
		{Left, {
			{vec3(-0.5, 0.5, 0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, 0.5, -0.5),	vec2(0.0, 1.0)},
			{vec3(-0.5, -0.5,-0.5),	vec2(0.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
		}},
		{Right, {
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(1.0, 0.0)},
		}},
		{Top, {
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, 0.5,-0.5),	vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, 0.5),	vec2(0.0, 0.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 0.0)},
		}},
		{Bottom, {
			{vec3(-0.5,-0.5,-0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, -0.5), vec2(0.0, 1.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
		}},
	};
}
