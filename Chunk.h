#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <FastNoise/FastNoiseLite.h>

#include "Block.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Shader.h"

using namespace glm;

class Chunk
{
private: 
	vector<vertex> vertices;
	vector<GLuint> indices;
	VAO vao = VAO();
	VBO vbo = VBO(vertices, sizeof(vertex) * vertices.size());
	EBO ebo = EBO(indices, sizeof(GLuint) * indices.size());
	Texture texture = Texture("dirt.jpg", 1, 1, 1);
	int** height_map;
	void create_chunk();
	void add_face(block_face face, vec3 pos);
	int** get_heightmap();
public:
	const int size = 16;
	int width = 16, length = 16, height = 16;
	Block*** blocks;
	GLuint shader_id = 0;
	vec3 spawn_coord = vec3(0.0f);
	Chunk(vec3 spawn_coord);
	~Chunk();
	void render();
	void destroy();
};

namespace {
	static FastNoiseLite m_noise;
	float get_noise(int x, int z) {
		return m_noise.GetNoise((float)x, (float)z);
	}
}