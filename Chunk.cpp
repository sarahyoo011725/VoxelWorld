#include "Chunk.h"

Chunk::Chunk() {
	blocks = new Block * *[size];
	for (int x = 0; x < size; ++x) {
		blocks[x] = new Block * [size];
		for (int y = 0; y < size; ++y) {
			blocks[x][y] = new Block[size];
		}
	}

	vao.bind();
	vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); //vertex positions
	vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); //texture coordinates

	//unbind all to prevent accidentally motifying them
	vao.unbind();
	vbo.unbind();
	ebo.unbind();
}

void Chunk::render() {
	texture.bind();
	vao.bind();
	ebo.bind();
	create_chunk();
}

void Chunk::create_cube(int x, int y, int z) {
	mat4 trans = mat4(1.0);
	trans = translate(trans, vec3(x, y, z));
	GLfloat loc = glGetUniformLocation(shader_id, "trans");
	glUniformMatrix4fv(loc, 1, false, value_ptr(trans));
	glDrawElements(GL_TRIANGLES, sizeof(cube_indices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);
}

void Chunk::create_chunk() {
	for (int x = 0; x < size; ++x) {
		for (int y = 0; y < size; ++y) {
			for (int z = 0; z < size; ++z) {
				if (blocks[x][y][z].active == false) {
					continue;
				}
				blocks[x][y][z].type = dirt;
				create_cube(x, y, z);
			}
		}
	}
}

void Chunk::destroy() {
	vao.destroy();
	vbo.destroy();
	ebo.destroy();
	texture.destroy();
}

Chunk::~Chunk() {
	for (int x = 0; x < size; ++x) {
		for (int y = 0; y < size; ++y) {
			delete[] blocks[x][y];
		}
		delete[] blocks[x];
	}
	delete[] blocks;
}