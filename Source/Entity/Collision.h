#pragma once
#include <glm/glm.hpp>

using namespace glm;

class GameObject;

namespace aabb {
	GameObject get_broad_phase(const GameObject& obj, float dt);
	bool check_collision(const GameObject& a, const GameObject& b);
	float get_collision_time(const GameObject& a, const GameObject& b, vec3& normal, float dt);
}
