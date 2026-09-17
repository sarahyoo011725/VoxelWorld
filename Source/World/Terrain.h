#pragma once
#include "ChunkManager.h"
#include "StructureGenerator.h"
#include "WorldRandom.h"
#include "PerfStats.h"
#include "ThreadPool.h"

using namespace std;
using namespace glm;

/*
	a class that creates and handles chunks within render distance
*/
class Terrain
{
private:
	ChunkManager& cm;
	StructureGenerator& sg;
	//one worker per spare core - the calling thread takes items too
	ThreadPool pool = ThreadPool(std::max(1u, thread::hardware_concurrency() - 1));
	void spawn_structures(Chunk* chunk);
	void build_pending_chunks();
public:
	int render_dist = 9;
	float build_budget_ms = 3.0f; //per-frame ceiling on new chunk building
	PerfStats stats;
	vec3* player_pos;
	ivec2 origin = ivec2(0); //player's pos converted into chunk coord. it is the origin of player's render range.
	vector<Chunk*> visible_chunks;
	Terrain(vec3& cam_pos);
	void update_chunks();
	void draw_shadow_casters();
	void draw();
};