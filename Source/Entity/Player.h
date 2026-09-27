#pragma once
#include "GameObject.h"
#include "PlayerPhysics.h"
#include "Camera/Camera.h"
#include "World/BlockInteractor.h"
#include "Mob.h"

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
	vec3 hovered_position() const { return block_interactor.hovered_position; }
	const Inventory& inventory() const { return block_interactor.inventory; }
	bool is_underwater() { return physics.is_underwater(eye_position()); }
	bool feet_in_water() { return physics.is_underwater(position - vec3(0.0f, size.y * 0.5f - 0.1f, 0.0f)); }
	vec3 eye_position() const { return position + vec3(0.0f, eye_height, 0.0f); }
	float reach() const { return block_interactor.reach(); }
	void limit_reach(float distance) { block_interactor.reach_limit = distance; }

	//riding: the mount moves by the player's keys, and the player sits in its saddle
	Mob* mount = nullptr;
	//F5 cycles: from the eyes, from behind, and from the front looking back at the player
	enum class view { first_person, behind, in_front };
	view view_mode = view::first_person;
	void next_view();
	void ride(Mob* mob);
	void dismount();
	//after the mobs have moved this frame: sit in the saddle and look from there
	void follow_mount();
	//the mount holds still while the player isn't steering it, e.g. with the console open
	void rest_mount();
	vec3 view_position() const { return view_eye; }
private:
	void update_movement(float dt);
	//places the camera for the current view mode around the eyes, kept out of blocks, and updates it
	void place_camera();
	void steer_mount();
	vec3 view_eye = vec3(0.0f);

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
