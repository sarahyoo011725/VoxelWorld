#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include "Chunk.h"

using namespace std;
using namespace glm;

class Terrain
{
public:
	int render_dist = 9;
	vec3 *player_pos;
	unordered_map<ivec2, Chunk> chunks;
	Terrain(vec3 &cam_pos);
	void update();
	void create_chunk(ivec2 chunk_coord);
};

//TODO: remove chunk walls