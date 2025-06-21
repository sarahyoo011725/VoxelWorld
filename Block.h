#pragma once

#include <glad/glad.h>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include "BlockType.h"
#include "GameObject.h"

using namespace std;
using namespace glm;

struct vertex {
	vec3 position;
	vec2 texture;
};

class Block: public GameObject
{
public:
	Block() {
		velocity = vec3(0);
		size = vec3(1, 1, 1);
	};
	block_type type = none;
};

namespace {
	bool has_transparency(block_type type) {
		switch (type) {
		case water:
		case glass:
		case leaf_transp:
		case grass:
			return true;
		}
		return false;
	}

	bool has_transparency(Block* block) {
		if (block == nullptr) return false;
		return has_transparency(block->type);
	}

	bool is_solid(block_type type) {
		switch (type) {
		case none:
		case water:
		case grass:
			return false;
		}
		return true;
	}

	bool is_solid(Block* block) {
		if (block == nullptr) return false;
		return is_solid(block->type);
	}

	bool is_nonblock(block_type type) {
		switch (type) {
		case grass:
			return true;
		}
		return false;
	}

	/*
	* converts texture coord from texture_map into uv texture coord (scale it to between 0.0 and 1.0)
	* uv column & row range: 0 ~ 1.
	* index: index of a vertex from a face vertices. 
	* face left-top index: 0 / face right-top index: 1 / face right-bottom index: 2 / face left_bottom index = 3 (clockwise order)
	* *** the unit for block data modifcation is face, which is a 2d square.
	*/
	static vec2 convert_to_uv(int index, vec2 texture_coord) {
		float x = texture_coord.x;
		float y = texture_coord.y;
		if (index == 0) {
			return vec2((x - 1)/  textures_columns, y/  texture_rows);
		}
		else if (index == 1) {
			return vec2(x/  textures_columns, y/  texture_rows);
		}
		else if (index == 2) {
			return vec2(x/  textures_columns, (y - 1)/  texture_rows);
		}
		else if (index == 3) {
			return vec2((x - 1)/  textures_columns, (y - 1)/  texture_rows);
		}
		return vec2(-1, -1); //invalid index
	}

	static map<block_face, vector<vertex>> face_map = {
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
}
