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
	//blocks no longer carry a position, so collision candidates are gathered as
	//the world coordinates they were found at
	vector<vec3> gather_candidate_blocks(const GameObject& broadphase);
	bool is_position_clear(const GameObject& probe);
	bool has_solid_ground_below(const GameObject& probe);
	void try_auto_step(GameObject& target, float dt);

	const float step_height = 1.0f;
	const float step_speed = 6.0f; //units/sec the player rises during a step, so it's a visible motion instead of a teleport
	float pending_step = 0.0f;

	ChunkManager& cm;
	const float gravity = -30.0f;
};
