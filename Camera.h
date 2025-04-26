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
	int window_width, window_height;
	float dt = 0.0;
	float last_frame = 0.0;
	double mouse_xpos, mouse_ypos;
	double last_xpos, last_ypos;
	float yaw = -90.0f;
	float pitch = 0.0f;
	const float mouse_sensitivity = 0.1f;
	const float speed = 5.0f;
public:
	vec3 position = vec3(0.0); //local camera position
	vec3 direction = vec3(0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);
	//bool window_refocused = false; //TODO: prevent cam view jump on window re-focus
	Camera(GLFWwindow *window, int window_width, int window_height, vec3 position);
	void process_inputs();
	void process_mouse_inputs();
	void update_matrix(int shader_id, float fov_degrees, float near_plane, float far_plane);
};

