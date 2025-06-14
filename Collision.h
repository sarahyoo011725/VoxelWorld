#pragma once
#include <glm/glm.hpp>
#include <iostream>

using namespace std;
using namespace glm;

class GameObject;

namespace aabb {
	/*
	* calculates an area where an object potentially collides with other objects, based on its velocity
	*/
	GameObject get_broad_phase(const GameObject& obj);

	/*
	* AABB (Axis-Aligned Bounding Boxes).
	* It is the quickest algorithm to determine whether the two objects are overlapping or not.
	* fast but imprecise.
	*/
	bool check_collision(const GameObject& a, const GameObject& b);

	/*
	* Swepth AABB.
	* returns the time during an object A collided with an object B.
	* updates the normal of object A, which is the direction of A's surface colliding with B.
	* if returned time is between 0 and 1, you can get the remaining time with 1 - collision time.
	*  --> use the remaining time for performing collision responses.
	*/
	float get_collision_time(const GameObject& a, const GameObject& b, vec3& normal);
}

