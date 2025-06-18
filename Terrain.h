#pragma once
#include "ChunkManager.h"
#include "StructureGenerator.h"

using namespace std;
using namespace glm;

class Terrain
{
private:
	ChunkManager& cm;
	StructureGenerator& sg;
	void spawn_structures(Chunk* chunk);
public:
	int render_dist = 9;
	vec3* player_pos;
	ivec2 origin = ivec2(0); //player's pos converted into chunk coord. it is the origin of player's render range.
	vector<Chunk*> visible_chunks;
	Terrain(vec3& cam_pos);
	void update();
};