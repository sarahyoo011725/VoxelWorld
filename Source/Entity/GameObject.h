#pragma once
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

/*
	a class made to be inherited by other classes, such as Player (Camera) and Block
	primarily used for collision check
*/
class GameObject
{
private:
public:
	vec3 position = vec3(0.0);
	vec3 velocity = vec3(0.0);
	vec3 size = vec3(0.0); //size of hitbox
	vec3 hitbox_margin = vec3(0.0);
};