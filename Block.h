#pragma once

#include <glad/glad.h>
#include <map>
#include <glm/glm.hpp>
#include <vector>

using namespace std;
using namespace glm;

enum block_type {
	none,
	dirt,
	grass,
	stone
};

enum block_face {
	Front,
	Back,
	Left,
	Right,
	Top,
	Bottom,
};

struct vertex {
	vec3 position;
	vec2 texture;
};

class Block
{
public:
	Block();
	~Block();
	bool active = true;
	block_type type = none;
};

namespace {
	const static int texture_rows = 16;
	const static int textures_columns = 16;

	static vec2 convert_to_uv(int index, vec2 texture_coord) {
		float x = texture_coord.x;
		float y = texture_coord.y;
		if (index == 0) {
			return vec2((x - 1) / textures_columns, y / texture_rows);
		}
		else if (index == 1) {
			return vec2(x / textures_columns, y / texture_rows);
		}
		else if (index == 2) {
			return vec2(x / textures_columns, (y - 1) / texture_rows);
		}
		else if (index == 3) {
			return vec2((x - 1) / textures_columns, (y - 1) / texture_rows);
		}
		return vec2(-1, -1); //invalid index
	}

	static map <block_type, map<block_face, vec2>> texture_map = {
		{dirt, {
			{Front,  vec2(3, 16)}, 
			{Back,   vec2(3, 16)}, 
			{Left,   vec2(3, 16)}, 
			{Right,	 vec2(3, 16)}, 
			{Top,	 vec2(3, 16)},
			{Bottom, vec2(3, 16)},
		}},
		{grass, {
			{Front,  vec2(4, 16)},
			{Back,   vec2(4, 16)},
			{Left,   vec2(4, 16)},
			{Right,	 vec2(4, 16)},
			{Top,	 vec2(1, 16)},
			{Bottom, vec2(3, 16)},
		}},
		{stone, {
			{Front,  vec2(2, 16)},
			{Back,   vec2(2, 16)},
			{Left,   vec2(2, 16)},
			{Right,	 vec2(2, 16)},
			{Top,	 vec2(2, 16)},
			{Bottom, vec2(2, 16)},
		}},
	};

	static map<block_face, vector<vertex>> face_map = {
		{Front, {
			{vec3(-0.5, 0.5, 0.5),  vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(1.0, 0.0)},	
			{vec3(-0.5,-0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Back, {
			{vec3(-0.5, 0.5,-0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5) ,	vec2(1.0, 1.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5,-0.5,-0.5),	vec2(0.0, 0.0)},
		}},
		{Left, {
			{vec3(-0.5, 0.5, 0.5),	vec2(0.0, 1.0)},
			{vec3(-0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(-0.5, -0.5,-0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Right, {
			{vec3(0.5, 0.5, 0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(0.5,-0.5, -0.5),	vec2(1.0, 0.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Top, {
			{vec3(-0.5,0.5,-0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, 0.5, -0.5),	vec2(1.0, 1.0)},
			{vec3(0.5, 0.5, 0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5, 0.5, 0.5),	vec2(0.0, 0.0)},
		}},
		{Bottom, {
			{vec3(-0.5,-0.5,-0.5),	vec2(0.0, 1.0)},
			{vec3(0.5, -0.5, -0.5), vec2(1.0, 1.0)},
			{vec3(0.5, -0.5, 0.5),	vec2(1.0, 0.0)},
			{vec3(-0.5, -0.5, 0.5),	vec2(0.0, 0.0)},
		}},
	};
}
