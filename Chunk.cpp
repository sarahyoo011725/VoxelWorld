#include "Chunk.h"
#include "ChunkManager.h"

Chunk::Chunk(vec3 position) : cm(ChunkManager::get_instance()) {
	//add 1 to width and length to store neighbor chunks' block data in their edge
	width = chunk_size + 1;
	height = 100;
	length = chunk_size + 1;
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

	opaque_vbo = VBO(opaque_vertices, sizeof(vertex) * opaque_vertices.size());
	opaque_ebo = EBO(opaque_indices, sizeof(GLuint) * opaque_indices.size());
	opaque_vao.bind();
	opaque_vao.link_attrib(opaque_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions coords
	opaque_vao.link_attrib(opaque_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float))); //vertex texture coords
	
	transp_vbo = VBO(transp_vertices, sizeof(vertex) * transp_vertices.size());
	transp_ebo = EBO(transp_indices, sizeof(GLuint) * transp_indices.size());
	transp_vao.bind();
	transp_vao.link_attrib(transp_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); 
	transp_vao.link_attrib(transp_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
}

int** Chunk::get_heightmap() {
	int** map = new int *[width];
	for (int x = 0; x < width; ++x) {
		map[x] = new int[length];
	}
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			//a block's world coords
			int x_pos = position.x + x;
			int z_pos = position.z + z;
			int height_val = abs(static_cast<int> (get_noise(x_pos, z_pos) * 20)) + 4;
			if (height_val > height) height_val = height;
			map[x][z] = height_val;
		}
	}
	return map;
}

void Chunk::draw_opaque_blocks() {
	opaque_vao.bind();
	opaque_ebo.bind();
	glDrawElements(GL_TRIANGLES, opaque_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::draw_transparent_blocks() {
	transp_vao.bind();
	transp_ebo.bind();
	glDrawElements(GL_TRIANGLES, transp_indices.size(), GL_UNSIGNED_INT, 0);
}

Block* Chunk::get_block(int x, int y, int z) {
	if (x < 0 || y < 0 || z < 0 || x > width - 1 || y > height - 1 || z > length - 1) {
		cout << "Wrong block index" << endl;
		return nullptr;
	}
	return &blocks[x][y][z];
}

void Chunk::add_face(block_face face, block_type type, vec3 pos) {
	vector<vertex> verts = face_map[face];
	vec2 texture_coord = texture_map[type][face];
	bool transparency = is_transparent(type); 

	//transforms vertices
	for (int i = 0; i < verts.size(); ++i) {
		vertex v = verts[i];
		v.position += pos + position;
		v.texture = convert_to_uv(i, texture_coord);
		if (transparency) {
			transp_vertices.push_back(v);
		}
		else {
			opaque_vertices.push_back(v);
		}
	}

	//generates indices to draw faces with EBO
	if (transparency) {
		GLuint base_index = transp_vertices.size() - 4; //ensures base_index to start at 0
		transp_indices.push_back(base_index);
		transp_indices.push_back(base_index + 1);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 3);
		transp_indices.push_back(base_index);
	}
	else {
		GLuint base_index = opaque_vertices.size() - 4;
		opaque_indices.push_back(base_index);
		opaque_indices.push_back(base_index + 1);
		opaque_indices.push_back(base_index + 2);
		opaque_indices.push_back(base_index + 2);
		opaque_indices.push_back(base_index + 3);
		opaque_indices.push_back(base_index);
	}
}

void Chunk::create_chunk() {
	for (int x = 0; x < width ; ++x) {
		for (int z = 0; z < length; ++z) {
			for (int y = 0; y < height; ++y) {
				if (blocks[x][y][z].active == false || blocks[x][y][z].type == none) {
					continue;
				}
				int h = height_map[x][z];
				bool am_i_transparent = is_transparent(blocks[x][y][z].type);
				vec3 pos = vec3(x, y, z);
				block_type type = blocks[x][y][z].type;
				//TODO: I will make this look better later in some days lol
				if (x == 1 && (blocks[0][y][z].type == none || is_transparent(blocks[0][y][z].type) && !am_i_transparent) ||
					x > 0 && (blocks[x - 1][y][z].type == none ||
					is_transparent(blocks[x - 1][y][z].type) && !am_i_transparent)) {
					add_face(Left, type, pos);
				}
				if (y > 0 && (blocks[x][y - 1][z].type == none || 
					is_transparent(blocks[x][y - 1][z].type) && !am_i_transparent)) {
					add_face(Bottom, type, pos);
				}
				if (z == 1 && (blocks[x][y][0].type == none || is_transparent(blocks[x][y][0].type) && !am_i_transparent) ||
					z > 0 && (blocks[x][y][z - 1].type == none ||
					is_transparent(blocks[x][y][z - 1].type) && !am_i_transparent)) {
					add_face(Back, type, pos);
				}
				if (x == width - 2 && (blocks[width-1][y][z].type == none || is_transparent(blocks[width - 1][y][z].type) && !am_i_transparent) ||
					x < width - 1 && (blocks[x + 1][y][z].type == none ||
					is_transparent(blocks[x + 1][y][z].type) && !am_i_transparent)) {
					add_face(Right, type, pos);
				}
				if (y == height - 1 || y < height - 1 && (blocks[x][y + 1][z].type == none || 
					is_transparent(blocks[x][y + 1][z].type) && !am_i_transparent)) {
					add_face(Top, type, pos);
				}
				if (z == length - 2 && (blocks[x][y][length-1].type == none || is_transparent(blocks[x][y][length - 1].type) && !am_i_transparent) ||
					z < length - 1 && (blocks[x][y][z + 1].type == none ||
					is_transparent(blocks[x][y][z + 1].type) && !am_i_transparent)) {
					add_face(Front, type, pos);
				}
			}
		}
	}
}

void Chunk::destroy() {
	opaque_vao.destroy();
	opaque_vbo.destroy();
	opaque_ebo.destroy();
	transp_vao.destroy();
	transp_vbo.destroy();
	transp_ebo.destroy();
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