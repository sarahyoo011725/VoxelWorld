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

using namespace glm;

class Chunk
{
private: 
	VAO vao = VAO();
	Texture texture = Texture("dirt.jpg", 1, 1, 1);
	VBO vbo = VBO(cube_vertices, sizeof(cube_vertices));
	EBO ebo = EBO(cube_indices, sizeof(cube_indices));
	void create_cube(int x, int y, int z);
	void create_chunk();
public:
	const int size = 16;
	Block*** blocks;
	GLuint shader_id = 0;
	Chunk();
	~Chunk();
	void render();
	void destroy();
};

