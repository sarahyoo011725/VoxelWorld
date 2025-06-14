#pragma once
#include <glm/glm.hpp>
#include "Collision.h"
#include "vector"

using namespace std;
using namespace glm;

class GameObject
{
private:
public:
	vec3 position = vec3(0.0);
	vec3 velocity = vec3(0.0);
	vec3 size = vec3(0.0); //size of hitbox
	vec3 hitbox_margin = vec3(0.0);
};