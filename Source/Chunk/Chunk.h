#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glad/glad.h>
#include <vector>
#include "Block/Block.h"
#include "Buffers/VAO.h"
#include "Buffers/VBO.h"
#include "Buffers/EBO.h"
#include "Texture/Texture.h"
#include "Shader/ShaderManager.h"
#include "unordered_map"

using namespace glm;

class ChunkManager;

/*
	stores and manages a chunk of blocks
*/
class Chunk
{
private: 
	ChunkManager &cm;
	ShaderManager& sm;

	//for opaque geometry
	vector<vertex> opaque_vertices;
	vector<GLuint> opaque_indices;
	VAO opaque_vao = VAO();
	VBO opaque_vbo = VBO(nullptr, sizeof(vertex) * 0, GL_STATIC_DRAW);
	EBO opaque_ebo = EBO(nullptr, sizeof(GLuint) * 0, GL_STATIC_DRAW);

	//for transparent geometry
	unordered_map<ivec3, vector<vertex>> nonblock_structure_vertices; //all nonblock structure texture like grass has transparency
	vector<vertex> transp_vertices;
	vector<GLuint> transp_indices;
	VAO transp_vao = VAO();
	VBO transp_vbo = VBO(nullptr, sizeof(vertex) * 0, GL_STATIC_DRAW);
	EBO transp_ebo = EBO(nullptr, sizeof(GLuint) * 0, GL_STATIC_DRAW);

	vector<vertex> water_vertices;
	vector<GLuint> water_indices;
	VAO water_vao = VAO();
	VBO water_vbo = VBO(nullptr, sizeof(vertex) * 0, GL_STATIC_DRAW);
	EBO water_ebo = EBO(nullptr, sizeof(GLuint) * 0, GL_STATIC_DRAW);

	Block*** blocks;
	int** get_heightmap();
	void add_face(block_face face, block_type type, vec3 local_coord);
	void update_face_indices(bool has_transparency, bool is_water);
	void update_nonblock_structure_vertices_and_indices();
	void update_buffers_data();
public:
	bool should_rebuild = false;
	bool has_built = false;
	int** height_map;
	int width, length, height;
	vec3 world_position = vec3(0.0f);
	ivec2 id = ivec2(0);

	Chunk(ivec2 chunk_origin);
	~Chunk();
	void build_chunk();
	void rebuild_chunk();
	void draw_opaque_blocks();
	void draw_transparent_blocks();
	void draw_water();
	Block* get_block(ivec3 local_coord);
	void set_block(ivec3 local_coord, block_type type);
	void add_nonblock_structure_vertices(ivec3 local_coord, vector<vertex> vertices);
	void remove_structure(ivec3 local_coord);
};