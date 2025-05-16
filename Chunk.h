#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Block.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Shader.h"
#include <vector>

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
	void create_chunk();
	void add_face(block_face face, vec3 pos);
public:
	const int size = 16;
	int width = 16, length = 16, height = 16;
	Block*** blocks;
	GLuint shader_id = 0;
	Chunk(int width, int length, int height);
	~Chunk();
	void render();
	void destroy();
};

