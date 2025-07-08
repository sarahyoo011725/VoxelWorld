#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "World/ChunkManager.h"
#include "World/StructureGenerator.h"
#include "Entity/GameObject.h"
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/EBO.h"
#include "Buffers/FBO.h"
#include "Buffers/RBO.h"
#include "Shader/ShaderManager.h"
#include "Texture/Texture.h"
#include "Entity/Geometries.h"
#include "Audio/AudioManager.h"

using namespace glm;

//a struct to save window informations
struct WindowSetting {
	GLFWwindow* window;
	int width;
	int height;
	bool window_active;
};

namespace {
	static bool window_refocused = false;
}

/*
* Camera or Player class that handles physics, collision, block interaction, and updates view
*/
class Camera: public GameObject
{
public:
	Camera(WindowSetting *setting, vec3 position);
	void update();
	void draw_outlines();
	void draw_HUDs();
	void post_process();
	void send_matrix(GLuint shader_id);
	bool is_ground();
	bool is_underwater();
	void bind_fbo();
	void unbind_fbo();
private:
	void update_matrix();
	void update_movement(float dt);
	void update_mouse();
	void update_other_inputs();
	void collision(float dx, float dy, float dz);
	void raycast();
	void block_interaction();
	void outline_hovered_cube();

	//constant variables setup
	const float mouse_sensitivity = 0.1f;
	const float default_speed = 5.0f;
	const float run_speed = 10.0f;
	const float run_speed_fly = 50.0f;
	const float jump_force = 8.0f;
	const float gravity = -30.0f;
	const float outline_thickness = 2.0f;
	const float max_ray_length = 3.0f;
	
	//saves the information of block interacting with
	Chunk* current_chunk = nullptr;
	Block* hovered_block = nullptr;
	block_type holding_block_type = none;

	//non-constant variables that are updated during game
	WindowSetting *window_setting;
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

	//non-constant boolean variables that are updated on user inputs
	bool enable_physics = false;
	bool enable_outline = true;
	bool is_running = false;
	bool is_jumping = false;
	bool on_ground = false;
	bool should_outline = false;

	//non-constant variables that are used to update view
	vec3 ray = vec3(0.0);
	vec3 direction = vec3(0.0); //this is a forward vector
	vec3 up = vec3(0.0, 1.0, 0.0);
	mat4 view = mat4(0.0);
	mat4 projection = mat4(0.0);
	mat4 mat = mat4(0.0);

	//singleton helpers
	ChunkManager& cm;
	StructureGenerator& sg;
	ShaderManager& sm;

	//setups for outlining interacted block and 2D HUDs
	vec4 hovered_block_outline_color = vec4(0.0, 1.0, 1.0, 1.0); //cyan
	vec4 crosshair_color = vec4(1.0); //white
	VAO outline_vao = VAO(); //draws objects with lines, such as hitbox
	VBO outline_vbo = VBO(cube_edges.data(), sizeof(vec3) * cube_edges.size(), GL_STATIC_DRAW);
	VAO HUD_vao = VAO();  //draws 2d components, like crosshair and inventory box
	VBO HUD_vbo = VBO(crosshair_vertices.data(), sizeof(vertex_2d) * crosshair_vertices.size(), GL_STATIC_DRAW);
	
	//post-processing framebuffer. used for effects such as water submerging
	VAO quad_vao = VAO();
	VBO quad_vbo = VBO(quad_vertices.data(), sizeof(vertex_2d) * quad_vertices.size(), GL_STATIC_DRAW);
	FBO fbo = FBO();
	RBO rbo = RBO(window_setting->width, window_setting->height, GL_DEPTH24_STENCIL8);
	Texture texture_color_buffer = Texture(window_setting->width, window_setting->height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
		GL_CLAMP_TO_EDGE, GL_LINEAR);
	/*
	Texture depth_texture = Texture(window_setting->width, window_setting->height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT,
		GL_FLOAT, GL_CLAMP_TO_BORDER, GL_LINEAR);
	*/
};