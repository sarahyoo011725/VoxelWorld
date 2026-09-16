#pragma once
#include "GameObject.h"
#include "PlayerPhysics.h"
#include "Camera/Camera.h"
#include "World/BlockInteractor.h"

/*
* the physical player entity - reads movement input and owns the camera,
* physics, and block interaction that make up the first-person controller
*/
class Player : public GameObject
{
public:
	Player(WindowSetting* setting, vec3 position);
	Camera camera;
	void update();
	mat4 view_matrix() const { return camera.mat; }
	Block* hovered_block() const { return block_interactor.hovered_block; }
	bool is_underwater() { return physics.is_underwater(eye_position()); }
	vec3 eye_position() const { return position + vec3(0.0f, eye_height, 0.0f); }
private:
	void update_movement(float dt);

	WindowSetting* window_setting;
	PlayerPhysics physics;
	BlockInteractor block_interactor;

	const float default_speed = 5.0f;
	const float run_speed = 10.0f;
	const float run_speed_fly = 50.0f;
	const float jump_force = 8.0f;
	const float eye_height = 0.72f;

	float last_frame = 0.0f;
	float speed = default_speed;
	bool enable_physics = false;
	bool is_running = false;
};
