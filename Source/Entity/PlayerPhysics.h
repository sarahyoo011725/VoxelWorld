#pragma once
#include "GameObject.h"
#include "World/ChunkManager.h"

/*
* gravity, AABB collision, and world queries (ground/underwater) for a GameObject
*/
class PlayerPhysics
{
public:
	PlayerPhysics();
	bool on_ground = false;
	void integrate(GameObject& target, float dt, bool enabled);
	bool is_underwater(vec3 position);
private:
	void collision(GameObject& target, float vx, float vy, float vz, bool enabled);

	ChunkManager& cm;
	const float gravity = -30.0f;
};
