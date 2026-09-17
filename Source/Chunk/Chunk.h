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

	//for wind-swayed geometry (leaves, grass)
	unordered_map<ivec3, vector<foliage_vertex>> nonblock_structure_vertices; //all nonblock structure geometry like grass is foliage
	vector<foliage_vertex> foliage_vertices;
	vector<GLuint> foliage_indices;
	VAO foliage_vao = VAO();
	VBO foliage_vbo = VBO(nullptr, sizeof(foliage_vertex) * 0, GL_STATIC_DRAW);
	EBO foliage_ebo = EBO(nullptr, sizeof(GLuint) * 0, GL_STATIC_DRAW);

	vector<Block> blocks;
	vector<int> height_map;
	inline size_t block_index(int x, int y, int z) const { return (static_cast<size_t>(x) * height + y) * length + z; }
	inline size_t height_index(int x, int z) const { return static_cast<size_t>(x) * length + z; }
	vector<int> get_heightmap();
	void add_face(block_face face, block_type type, vec3 local_coord);
	void build_opaque_mesh();
	void add_merged_quad(block_face face, block_type type, ivec3 base_block, int run_u, int run_v);
	bool opaque_face_visible(int x, int y, int z, block_face face) const;
	void update_face_indices(bool has_transparency, bool is_water);
	void add_foliage_quad_indices();
	void update_nonblock_structure_vertices_and_indices();
	void update_buffers_data();
public:
	bool should_rebuild = false;
	bool has_built = false;
	bool has_generated = false;
	bool mesh_ready = false;
	//highest y that can hold a block. everything above is air, so meshing stops
	//there instead of scanning the ~65% of each chunk that is empty sky
	int max_occupied_y = 0;
	int width, length, height;
	vec3 world_position = vec3(0.0f);
	ivec2 id = ivec2(0);

	Chunk(ivec2 chunk_origin);
	//never copied or moved: it owns GL handles that a shallow copy would alias
	//and the destructor would then free twice
	Chunk(const Chunk&) = delete;
	Chunk& operator=(const Chunk&) = delete;
	~Chunk();
	int get_height(int x, int z) const { return height_map[height_index(x, z)]; }

	/*
		the chunk pipeline is split into three phases so the expensive ones can
		move onto a worker thread later: generate_terrain() and build_mesh()
		touch no GL state and only this chunk's own storage, while upload_mesh()
		is the only phase that must run on the thread owning the GL context.
	*/
	void generate_terrain();
	void build_mesh();
	void upload_mesh();

	void build_chunk();
	void rebuild_chunk();
	void draw_opaque_blocks();
	void draw_transparent_blocks();
	void draw_water();
	void draw_foliage();
	void draw_opaque_depth();
	void draw_foliage_depth();
	Block* get_block(ivec3 local_coord);
	void set_block(ivec3 local_coord, block_type type);
	void add_nonblock_structure_vertices(ivec3 local_coord, vector<foliage_vertex> vertices);
	void remove_structure(ivec3 local_coord);
};