#pragma once

#include "ChunkManager.h"

using namespace std;
using namespace glm;

class Terrain
{
private:
	ChunkManager &cm;
public:
	int render_dist = 9;
	vec3 *player_pos;
	vector<Chunk*> visible_chunks;
	Terrain(vec3 &cam_pos);
	void update();
};