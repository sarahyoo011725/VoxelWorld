#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <map>
#include "Chunk.h"

using namespace std;
using namespace glm;

class Terrain
{
private:
	double render_dist = 16;
public:
	vec3 *player_pos;
	map<vec3, Chunk> chunks; //idk
	Terrain(vec3 *cam_pos);
	~Terrain();
	void render();
};

//TODO: procedural terrain / chunk generation
