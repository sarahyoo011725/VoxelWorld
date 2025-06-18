#pragma once
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

namespace {
	const static vector<vec2> crosshair_vertices = {
		//horizontal line
		vec2(-0.02f, 0.0f),
		vec2(0.02f, 0.0f),
		//vertical line
		vec2(0.0f, -0.03f),
		vec2(0.0f,  0.03f),
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