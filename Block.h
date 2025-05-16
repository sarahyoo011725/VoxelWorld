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