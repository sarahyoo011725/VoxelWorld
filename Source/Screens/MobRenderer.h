#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Entity/Sheep.h"
#include "Shader/ShaderManager.h"

using namespace glm;

struct mob_vertex {
	vec3 position;
	vec3 normal;
	vec2 uv; //0..1 across the face; default.frag maps it into tile_origin's atlas cell
	vec2 tile_origin;
	float light; //daylight at the mob, so it darkens in caves like the terrain around it
};

/*
	draws every mob as boxes built in world space on the CPU each frame. a sheep
	is under 400 vertices, so rebuilding them all costs less than a draw call
	per body part would, and the whole herd goes out in one draw
*/
class MobRenderer {
public:
	MobRenderer();
	void build(const vector<unique_ptr<Sheep>>& sheep);
	void draw();
	//for the shadow pass; the caller has the shadow shader active
	void draw_depth();

private:
	void add_sheep(const Sheep& sheep);
	void add_box(const mat4& transform, vec2 tile, float light);

	ShaderManager& sm;
	VAO vao = VAO();
	VBO vbo = VBO(nullptr, 0, GL_DYNAMIC_DRAW);
	vector<mob_vertex> vertices;
};
