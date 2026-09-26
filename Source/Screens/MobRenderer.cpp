#include "MobRenderer.h"
#include "Chunk/Chunk.h"
#include "World/ChunkGeneration.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	struct cube_face {
		vec3 normal, u, v;
	};

	//each face spans u and v from its centre at normal * 0.5
	const cube_face cube_faces[6] = {
		{ vec3(1, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0) },
		{ vec3(-1, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0) },
		{ vec3(0, 1, 0), vec3(1, 0, 0), vec3(0, 0, -1) },
		{ vec3(0, -1, 0), vec3(1, 0, 0), vec3(0, 0, 1) },
		{ vec3(0, 0, 1), vec3(1, 0, 0), vec3(0, 1, 0) },
		{ vec3(0, 0, -1), vec3(-1, 0, 0), vec3(0, 1, 0) },
	};

	const vec2 white_wool = vec2(1, 5);
	const vec2 skin = vec2(15, 18); //white terracotta, a pale beige
	const vec2 black_wool = vec2(2, 8);

	//a box of the given size whose origin sits at `at` in the parent's space
	mat4 box(const mat4& parent, vec3 at, vec3 size) {
		return scale(translate(parent, at), size);
	}
}

MobRenderer::MobRenderer() : sm(ShaderManager::get_instance()) {
	vao.bind();
	vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, position));
	vao.link_attrib(vbo, 1, 3, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, normal));
	vao.link_attrib(vbo, 2, 2, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, uv));
	vao.link_attrib(vbo, 3, 2, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, tile_origin));
	vao.link_attrib(vbo, 4, 1, GL_FLOAT, GL_FALSE, sizeof(mob_vertex), (void*)offsetof(mob_vertex, light));
}

void MobRenderer::add_box(const mat4& transform, vec2 tile, float light) {
	vec2 origin = tile_uv_origin(tile);
	mat3 rotation = mat3(transform);
	const vec2 corner_uv[4] = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };
	const int order[6] = { 0, 1, 2, 2, 3, 0 };
	for (const cube_face& face : cube_faces) {
		vec3 corners[4] = {
			face.normal * 0.5f + (-face.u + face.v) * 0.5f,
			face.normal * 0.5f + (face.u + face.v) * 0.5f,
			face.normal * 0.5f + (face.u - face.v) * 0.5f,
			face.normal * 0.5f + (-face.u - face.v) * 0.5f,
		};
		//scaling a box leaves its face normals pointing the same way, so the
		//rotation part of the transform is enough once renormalised
		vec3 normal = normalize(rotation * face.normal);
		for (int i : order) {
			vertices.push_back({ vec3(transform * vec4(corners[i], 1.0f)), normal, corner_uv[i], origin, light });
		}
	}
}

/*
	a sheep facing +z with its feet at the origin: a wool body on four legs that
	swing in diagonal pairs, and a beige face that dips down to graze
*/
void MobRenderer::add_sheep(const Sheep& sheep) {
	float light = 1.0f;
	Chunk* chunk = ChunkManager::get_instance().get_chunk(sheep.position);
	if (chunk != nullptr) {
		ivec3 local = world_to_local_coord(sheep.position);
		light = daylight_level(chunk->get_height(local.x, local.z), (int)std::round(sheep.position.y)) / 15.0f;
	}

	mat4 root = rotate(translate(mat4(1.0f), sheep.feet()), sheep.yaw, vec3(0, 1, 0));

	float swing = sin(sheep.walk_phase) * 0.4f * sheep.leg_swing;
	const float leg_height = 0.7f;
	const vec3 hips[4] = { vec3(0.18f, leg_height, 0.32f), vec3(-0.18f, leg_height, -0.32f),
		vec3(-0.18f, leg_height, 0.32f), vec3(0.18f, leg_height, -0.32f) };
	for (int i = 0; i < 4; ++i) {
		float angle = i < 2 ? swing : -swing;
		mat4 hip = rotate(translate(root, hips[i]), angle, vec3(1, 0, 0));
		add_box(box(hip, vec3(0, -leg_height * 0.5f, 0), vec3(0.24f, leg_height, 0.24f)), skin, light);
	}

	add_box(box(root, vec3(0, leg_height + 0.3f, 0), vec3(0.62f, 0.6f, 1.0f)), white_wool, light);

	mat4 neck = rotate(translate(root, vec3(0, leg_height + 0.45f, 0.45f)), sheep.head_pitch, vec3(1, 0, 0));
	add_box(box(neck, vec3(0, 0.08f, 0.2f), vec3(0.4f, 0.4f, 0.42f)), skin, light);
	for (float side : { -1.0f, 1.0f }) {
		add_box(box(neck, vec3(side * 0.11f, 0.14f, 0.415f), vec3(0.1f, 0.08f, 0.02f)), white_wool, light);
		add_box(box(neck, vec3(side * 0.13f, 0.14f, 0.425f), vec3(0.05f, 0.08f, 0.02f)), black_wool, light);
	}
}

void MobRenderer::build(const vector<unique_ptr<Sheep>>& sheep) {
	vertices.clear();
	for (const auto& s : sheep) add_sheep(*s);
	vbo.reset_vertices(vertices.data(), sizeof(mob_vertex) * vertices.size(), GL_DYNAMIC_DRAW);
}

//both passes draw with culling off: the boxes are closed, so the depth test already hides their far sides
void MobRenderer::draw() {
	if (vertices.empty()) return;
	sm.mob_shader.activate();
	glDisable(GL_CULL_FACE);
	vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
	glEnable(GL_CULL_FACE);
}

void MobRenderer::draw_depth() {
	if (vertices.empty()) return;
	//attribute 3 is the tile origin here, not a sway weight
	sm.shadow_shader.set_uniform_1f("sway_scale", 0.0f);
	glDisable(GL_CULL_FACE);
	vao.bind();
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
	glEnable(GL_CULL_FACE);
}
