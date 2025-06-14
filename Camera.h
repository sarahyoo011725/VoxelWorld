#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "ChunkManager.h"
#include "GameObject.h"

using namespace glm;

class Camera: public GameObject
{
private:
	ChunkManager& cm;

	const float mouse_sensitivity = 0.1f;
	const float default_speed = 5.0f;
	const float run_speed = 10.0f;
	const float run_speed_fly = 50.0f;
	const float jump_force = 8.0f; 
	const float gravity = -30.0f;
	
	GLFWwindow *window;
	vec3 direction = vec3(0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);
	int window_width, window_height;
	double mouse_xpos, mouse_ypos;
	double last_xpos, last_ypos;
	float last_frame = 0.0;
	float dt = 0.0;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov_degrees = 45.0f;
	float near_plane = 1.0f;
	float far_plane = 100.0f;
	float speed = default_speed;
	bool enable_physics = false;
	bool is_running = false;
	bool is_jumping = false;
	bool on_ground = false;

	void update_movement(float dt);
	void update_mouse();
	bool is_ground();
	void collision(float dx, float dy, float dz);
public:
	//bool window_refocused = false; //TODO: prevent cam view jump on window re-focus
	Camera(GLFWwindow *window, int window_width, int window_height, vec3 position);
	void update();
	void update_matrix(int shader_id);
};