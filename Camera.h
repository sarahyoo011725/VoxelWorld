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
	vec3 forward = vec3(0.0, 0.0, -1.0);
	float dt = 0.0;
	float last_frame = 0.0;
	double mouse_xpos = 0.0;
	double mouse_ypos = 0.0;
	//initial camera position: (600, 350) center
	double last_xpos = 1200 / 2;
	double  last_ypos = 700 / 2;
	float yaw = -90.0f;
	float pitch = 0.0f;
	const float mouse_sensitivity = 0.1f;
	const float speed = 1.5f;
public:
	vec3 position = vec3(0.0); //local camera position
	vec3 direction = vec3(0.0); //direction of camera (reversed?)
	vec3 up = vec3(0.0, 1.0, 0.0);
	Camera(GLFWwindow *window, vec3 position);
	void process_inputs();
	void process_mouse_inputs();
	void update_view(Shader &shader);
};

