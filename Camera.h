#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "ChunkManager.h"

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
	float fov_degrees = 45.0f;
	float near_plane = 1.0f;
	float far_plane = 100.0f;
	const float mouse_sensitivity = 0.1f;
	float default_speed = 5.0f;
	float run_speed = 10.0f;
	float run_speed_fly = 50.0f;
	float speed = default_speed;
	float jump_force = 5; 
	float gravity = -10;

	bool wasSpacePressedLastFrame = false; // global/static or per-frame variable
	bool is_running = false;
	bool is_jumping = false;
	void update_movement(float dt);
	void update_mouse();
	ChunkManager& cm;
public:
	vec3 position = vec3(0.0); //local camera position
	vec3 direction = vec3(0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 velocity = vec3(0.0);
	const vec3 player_size = vec3(1, 2, 1); //width x length: 1 block, height: 2 blocks
	bool enable_physics = false;
	//bool window_refocused = false; //TODO: prevent cam view jump on window re-focus
	Camera(GLFWwindow *window, int window_width, int window_height, vec3 position);
	void update();
	void update_matrix(int shader_id);
	bool is_ground();
};