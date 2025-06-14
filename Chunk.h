#pragma once
#include <glad/glad.h>
#include <vector>
#include "Block.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Shader.h"

using namespace glm;

class ChunkManager;
class Chunk
{
private: 
	ChunkManager &cm;

	//for opaque geometry
	vector<vertex> opaque_vertices;
	vector<GLuint> opaque_indices;
	VAO opaque_vao = VAO();
	VBO opaque_vbo = VBO(opaque_vertices, sizeof(vertex) * opaque_vertices.size());
	EBO opaque_ebo = EBO(opaque_indices, sizeof(GLuint) * opaque_indices.size());

	//for transparent geometry
	vector<vertex> transp_vertices;
	vector<GLuint> transp_indices; 
	VAO transp_vao = VAO();
	VBO transp_vbo = VBO(transp_vertices, sizeof(vertex) * transp_vertices.size());
	EBO transp_ebo = EBO(transp_indices, sizeof(GLuint) * transp_indices.size());

	Block*** blocks;
	int** get_heightmap();
	void add_face(block_face face, block_type type, vec3 pos);
	void add_face_indices(bool has_transparency);
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
	void bind_buffers();
	void draw_opaque_blocks();
	void draw_transparent_blocks();
	Block* get_block(ivec3 local_coord);
	void set_block(ivec3 local_coord, block_type type);
	void add_structure_vertices(vector<vertex> vertices_to_add, bool has_transparency);
	void destroy();
};