#pragma once
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

struct vertex_2d {
	vec2 position;
	vec2 texture;
};

//collection of vertices of geometries
namespace {
	const static vector<vertex_2d> crosshair_vertices = {
		//horizontal line
		{vec2(-0.02f, 0.0f), vec2(0.0f, 0.0f)},
		{vec2(0.02f, 0.0f), vec2(0.0f, 0.0f)},
		//vertical line
		{vec2(0.0f, -0.03f), vec2(0.0f, 0.0f)},
		{vec2(0.0f,  0.03f), vec2(0.0f, 0.0f)}
	};

	//quad (rectangle) vertices winded in clock-wise
	const static vector<vertex_2d> quad_vertices = {
		{vec2(-1.0f, 1.0f), vec2(0.0f, 1.0f)},
		{vec2(1.0f, 1.0f), vec2(1.0f, 1.0f)},
		{vec2(1.0f, -1.0f), vec2(1.0f, 0.0f)},

		{vec2(1.0f, -1.0f), vec2(1.0f, 0.0f)},
		{vec2(-1.0f, -1.0f), vec2(0.0f, 0.0f)},
		{vec2(-1.0f,  1.0f), vec2(0.0f, 1.0f)},
	};

	/*
	* vertices for a cube that is used for outlining a hovered block in game
	*/
	const static vector<vec3> cube_edges = {
		//bottom edges
		{-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5},
		{0.5, -0.5, -0.5}, {0.5, -0.5, 0.5},
		{0.5, -0.5, 0.5}, {-0.5, -0.5, 0.5},
		{-0.5, -0.5, 0.5}, {-0.5, -0.5, -0.5},
		//top edges
		{-0.5, 0.5, -0.5}, {0.5, 0.5, -0.5},
		{0.5, 0.5, -0.5}, {0.5, 0.5, 0.5},
		{0.5, 0.5, 0.5}, {-0.5, 0.5, 0.5},
		{-0.5, 0.5, 0.5}, {-0.5, 0.5, -0.5},
		//side edges
		{-0.5, 0.5, -0.5}, {-0.5, -0.5, -0.5}, //connects top and bottom
		{-0.5, 0.5, 0.5}, {-0.5, -0.5, 0.5},
		{0.5, 0.5, 0.5}, {0.5, -0.5, 0.5},
		{0.5, 0.5, -0.5}, {0.5, -0.5, -0.5}
	};
}