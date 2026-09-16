#pragma once
#include "GameObject.h"
#include "Collision.h"
#include "World/ChunkManager.h"
#include <vector>
#include <algorithm>

/*
* gravity, swept AABB collision, and world queries (ground/underwater) for a GameObject
*/
class PlayerPhysics
{
public:
	PlayerPhysics();
	bool on_ground = false;
	void integrate(GameObject& target, float dt, bool enabled);
	bool is_underwater(vec3 position);
private:
	vector<Block*> gather_candidate_blocks(const GameObject& broadphase);

	ChunkManager& cm;
	const float gravity = -30.0f;
};
