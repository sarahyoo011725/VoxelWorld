#include "Chunk.h"

Chunk::Chunk(vec3 position) {
	width = chunk_size;
	height = 100;
	length = chunk_size;
	this->position = position;
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
			for (int y = 0; y < height; ++y) {
				block_type type = none;
				int h = height_map[x][z];
				if (y == h) {
					type = grass;
				}
				if (y < h) {
					type = dirt;
					if (y < h - 3)
						type = sand;
					if (y < h - 5) {
						type = stone;
					}
				}
				if (y > h && y <= water_level) {
					type = water;
				}
				blocks[x][y][z].type = type;
			}
		}
	}

	create_chunk();

	vbo = VBO(vertices, sizeof(vertex) * vertices.size());
	ebo = EBO(indices, sizeof(GLuint) * indices.size());
	vao.bind();
	vao.link_attrib(vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions
	vao.link_attrib(vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float))); //texture coordinates
	
	transp_vbo = VBO(transp_vertices, sizeof(vertex) * transp_vertices.size());
	transp_ebo = EBO(transp_indices, sizeof(GLuint) * transp_indices.size());
	transp_vao.bind();
	transp_vao.link_attrib(transp_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions
	transp_vao.link_attrib(transp_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));

	//unbind all to prevent accidentally motifying them
	vao.unbind();
	vbo.unbind();
	ebo.unbind();
	transp_vao.unbind();
	transp_vbo.unbind();
	transp_ebo.unbind();
}

int** Chunk::get_heightmap() {
	int** map = new int *[width];
	for (int x = 0; x < width; ++x) {
		map[x] = new int[length];
	}

	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			int x_pos = position.x + x;
			int z_pos = position.z + z;
			int height_val = abs(static_cast<int> (get_noise(x_pos, z_pos) * 20)) + 4;
			if (height_val > height) height_val = height;
			map[x][z] = height_val;
		}
	}
	return map;
}

void Chunk::render() {
	vao.bind();
	ebo.bind();
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	transp_vao.bind();
	transp_ebo.bind();
	glDrawElements(GL_TRIANGLES, transp_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::add_face(block_face face, block_type type, vec3 pos) {
	vector<vertex> verts = face_map[face];
	vec2 texture_coord = texture_map[type][face];
	bool transparency = is_transparent(type); //checks if the texture is translucent or transparent.
	//transforms vertices
	for (int i = 0; i < verts.size(); ++i) {
		vertex v = verts[i];
		v.position += pos + position;
		v.texture = convert_to_uv(i, texture_coord);
		if (transparency) {
			transp_vertices.push_back(v);
		}
		else {
			vertices.push_back(v);
		}
	}

	//generates indices to draw faces with EBO
	if (transparency) {
		GLuint base_index = transp_vertices.size() - 4;
		transp_indices.push_back(base_index);
		transp_indices.push_back(base_index + 1);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 3);
		transp_indices.push_back(base_index);
	}
	else {
		GLuint base_index = vertices.size() - 4; //forces base index start at 0
		indices.push_back(base_index);
		indices.push_back(base_index + 1);
		indices.push_back(base_index + 2);
		indices.push_back(base_index + 2);
		indices.push_back(base_index + 3);
		indices.push_back(base_index);
	}
}

void Chunk::create_chunk() {
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			for (int y = 0; y < height; ++y) {
				if (blocks[x][y][z].active == false || blocks[x][y][z].type == none) {
					continue;
				}
				vec3 pos = vec3(x, y, z);
				block_type type = blocks[x][y][z].type;
				bool am_i_transparent = is_transparent(blocks[x][y][z].type);
				if (x == 0 || x > 0 && blocks[x - 1][y][z].type == none || 
					x > 0 && is_transparent(blocks[x - 1][y][z].type) && !am_i_transparent) {
					add_face(Left, type, pos);
				}
				if (y == 0 || y > 0 && blocks[x][y - 1][z].type == none || 
					y > 0 && is_transparent(blocks[x][y - 1][z].type) && !am_i_transparent) {
					add_face(Bottom, type, pos);
				}
				if (z == 0 || z > 0 && blocks[x][y][z - 1].type == none ||
					z > 0 && is_transparent(blocks[x][y][z - 1].type) && !am_i_transparent) {
					add_face(Back, type, pos);
				}
				if (x == width - 1 || x < width - 1 && blocks[x + 1][y][z].type == none || 
					x < width - 1 && is_transparent(blocks[x + 1][y][z].type) && !am_i_transparent) {
					add_face(Right, type, pos);
				}
				if (y == height - 1 || y < height - 1 && blocks[x][y + 1][z].type == none || 
					y < height-1 && is_transparent(blocks[x][y + 1][z].type) && !am_i_transparent) {
					add_face(Top, type, pos);
				}
				if (z == length - 1 || z < length - 1 && blocks[x][y][z + 1].type == none || 
					z < length - 1 && is_transparent(blocks[x][y][z + 1].type) && !am_i_transparent) {
					add_face(Front, type, pos);
				}
			}
		}
	}
}

void Chunk::destroy() {
	vao.destroy();
	vbo.destroy();
	ebo.destroy();
}

Chunk::~Chunk() {
	//TODO: deleting blocks array causes an issue.
	/* 
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			delete[] blocks[x][y];
		}
		delete[] blocks[x];
	}
	delete[] blocks;
	
	*/
}