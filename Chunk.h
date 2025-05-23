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
	//TODO: shared VAO, VBO, Texture
	vector<vertex> vertices;
	vector<GLuint> indices;
	VAO vao = VAO();
	VBO vbo = VBO(vertices, sizeof(vertex) * vertices.size());
	EBO ebo = EBO(indices, sizeof(GLuint) * indices.size());
	Texture texture = Texture("texture_atlas.png");
	int** height_map;
	int** get_heightmap();
	void add_face(block_face face, block_type type, vec3 pos);
	void create_chunk();
public:
	bool active = true;
	int width, length, height;
	Block*** blocks;
	vec3 position = vec3(0.0f);
	Chunk(vec3 position);
	~Chunk();
	void render();
	void destroy();
};

namespace {
	static FastNoiseLite m_noise;
	static int chunk_size = 16;
	float get_noise(int x, int z) {
		return m_noise.GetNoise((float)x, (float)z);
	}
}