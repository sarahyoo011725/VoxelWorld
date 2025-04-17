#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"

using namespace glm;

class Camera
{
private:
	GLFWwindow *window;
	float dt = 0.0;
	float last_frame = 0.0;
public:
	float speed = 1.0f;
	vec3 position = vec3(0.0); //local camera position
	vec3 target = vec3(0.0); //TODO: what is this for? 
	vec3 direction = vec3(0.0); //direction of camera (reversed?)
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = vec3(0.0);
	mat4 view = mat4(0.0);
	Camera(GLFWwindow *window, vec3 position);
	void update();
	void update_view(Shader &shader, const char* uniform_name);
};

