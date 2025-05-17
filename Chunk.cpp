#include "Chunk.h"

Chunk::Chunk(int width, int height, int length) {
	this->width = width;
	this->height = height;
	this->length = length;
	//TODO: optimize blocks initialization
	blocks = new Block * *[width];
	for (int x = 0; x < width; ++x) {
		blocks[x] = new Block * [height];
		for (int y = 0; y < height; ++y) {
			blocks[x][y] = new Block[length];
		}
	}

	//define block types
	height_map = get_heightmap();
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			for (int y = 0; y < height_map[x][z]; ++y) {
				blocks[x][y][z].type = dirt;
			}
		}
	}

	create_chunk();

	vbo = VBO(vertices, sizeof(vertex) * vertices.size());
	ebo = EBO(indices, sizeof(GLuint) * indices.size());
	vao.bind();
	vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions
	vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float))); //texture coordinates

	//unbind all to prevent accidentally motifying them
	vao.unbind();
	vbo.unbind();
	ebo.unbind();
}

int** Chunk::get_heightmap() {
	int** map = new int *[width];
	for (int x = 0; x < width; ++x) {
		map[x] = new int[length];
	}

	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			map[x][z] = abs(static_cast<int> (get_noise(x, z) * 20));
		}
	}
	return map;
}

void Chunk::render() {
	texture.bind();
	vao.bind();
	ebo.bind();
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::add_face(block_face face, vec3 pos) {
	vector<vertex> verts = face_map[face];
	//transforms vertices
	for (vertex v : verts) {
		v.position.x += pos.x;
		v.position.y += pos.y;
		v.position.z += pos.z;
		vertices.push_back(v);
	}

	//generates indices to draw faces with EBO
	GLuint base_index = vertices.size() - 4; //forces base index start at 0
	indices.push_back(base_index);
	indices.push_back(base_index + 1);
	indices.push_back(base_index + 2);
	indices.push_back(base_index + 2);
	indices.push_back(base_index + 3);
	indices.push_back(base_index);
}

void Chunk::create_chunk() {
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			for (int y = 0; y < height; ++y) {
				if (blocks[x][y][z].active == false || blocks[x][y][z].type == none) {
					continue;
				}
				vec3 pos = vec3(x, y, z);
				if (x == 0 || x > 0 && blocks[x - 1][y][z].type == none) {
					add_face(Left, pos);
				}
				if (y == 0 || y > 0 && blocks[x][y - 1][z].type == none) {
					add_face(Bottom, pos);
				}
				if (z == 0 || z > 0 && blocks[x][y][z - 1].type == none) {
					add_face(Back, pos);
				}
				if (x == width - 1 || x < width - 1 && blocks[x + 1][y][z].type == none) {
					add_face(Right, pos);
				}
				if (y == height - 1 || y < height - 1 && blocks[x][y + 1][z].type == none) {
					add_face(Top, pos);
				}
				if (z == length - 1 || z < length - 1 && blocks[x][y][z + 1].type == none) {
					add_face(Front, pos);
				}
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
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			delete[] blocks[x][y];
		}
		delete[] blocks[x];
	}
	delete[] blocks;
}