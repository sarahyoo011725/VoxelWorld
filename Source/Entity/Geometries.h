#pragma once
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

//collection of vertices of geometries
namespace {
	const static vector<GLfloat> crosshair_vertices = {
		//horizontal line
		-0.02f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.02f, 0.0f, 0.0f, 0.0f, 0.0f,
		//vertical line
		0.0f, -0.03f, 0.0f, 0.0f, 0.0f,
		0.0f,  0.03f, 0.0f, 0.0f, 0.0f
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