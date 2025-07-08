#include "Chunk.h"
#include "World/ChunkManager.h"
#include "World/StructureGenerator.h"

/*
	initializes blocks array, define block types based on noise height map, and links VBOs attributes to VAOs
*/
Chunk::Chunk(ivec2 chunk_id) : cm(ChunkManager::get_instance()), sm(ShaderManager::get_instance()) {
	//add 1 to width and length to store neighbor chunks' block data in their edge
	id = chunk_id;
	world_position = vec3(chunk_id.x * chunk_size, 0, chunk_id.y * chunk_size);
	width = chunk_size + 2;
	height = 50;
	length = chunk_size + 2;

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
				if (y > h && y <= water_level) {
					type = water;
				}
				if (y == h) {
					type = dirt_grass;
				}
				if (y < h) {
					type = dirt;
					if (y < h - 5) {
						type = stone;
					}
				}
				if (y <= h && y >= h - 2 && y + 1 < height && y + 1 <= water_level) {
					type = sand;
				}
				blocks[x][y][z].type = type;
				blocks[x][y][z].position = world_position + vec3(x - 1, y, z - 1); //needed for collision check
			}
		}
	}

	opaque_vao.bind();
	opaque_vao.link_attrib(opaque_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions coords
	opaque_vao.link_attrib(opaque_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float))); //vertex texture coords
	opaque_vao.link_attrib(opaque_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));

	transp_vao.bind();
	transp_vao.link_attrib(transp_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	transp_vao.link_attrib(transp_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
	transp_vao.link_attrib(transp_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));

	water_vao.bind();
	water_vao.link_attrib(water_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	water_vao.link_attrib(water_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
	water_vao.link_attrib(water_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));
}

/*
	generate a height map from a block's world coordinate
*/
int** Chunk::get_heightmap() {
	int** map = new int *[width];
	for (int x = 0; x < width; ++x) {
		map[x] = new int[length];
	}

	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			//get a block's world coords
			int x_pos = world_position.x + x - 1;
			int z_pos = world_position.z + z - 1;
			int height_val = abs(static_cast<int> (get_noise(x_pos, z_pos) * 20)) + 4;
			if (height_val > height) height_val = height;
			map[x][z] = height_val;
		}
	}
	return map;
}

/*
	updates vertices and indices of transparent and opaque geometries in VBOs and EBOs.
	This must be called after build_chunk() or rebuild_chunk()
*/
void Chunk::update_buffers_data() {
	opaque_vbo.reset_vertices(opaque_vertices.data(), sizeof(vertex) * opaque_vertices.size(), GL_STATIC_DRAW);
	opaque_ebo.reset_indices(opaque_indices.data(), sizeof(GLuint) * opaque_indices.size(), GL_STATIC_DRAW);
	transp_vbo.reset_vertices(transp_vertices.data(), sizeof(vertex) * transp_vertices.size(), GL_STATIC_DRAW);
	transp_ebo.reset_indices(transp_indices.data(), sizeof(GLuint) * transp_indices.size(), GL_STATIC_DRAW);
	water_vbo.reset_vertices(water_vertices.data(), sizeof(vertex) * water_vertices.size(), GL_STATIC_DRAW);
	water_ebo.reset_indices(water_indices.data(), sizeof(GLuint) * water_indices.size(), GL_STATIC_DRAW);
}

/*
	draws opaque objects.
	Opaque objects must be drawn before transparent objects 
*/
void Chunk::draw_opaque_blocks() {
	sm.default_shader.activate();
	opaque_vao.bind();
	opaque_ebo.bind();
	glDrawElements(GL_TRIANGLES, opaque_indices.size(), GL_UNSIGNED_INT, 0);
}

/*
	draws objects with transparency.
	Transparent objects must be drawn after opaque objects
*/
void Chunk::draw_transparent_blocks() {
	sm.default_shader.activate();
	transp_vao.bind();
	transp_ebo.bind();
	glDrawElements(GL_TRIANGLES, transp_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::draw_water() {
	sm.wave_shader.activate();
	water_vao.bind();
	water_ebo.bind();
	glDrawElements(GL_TRIANGLES, water_indices.size(), GL_UNSIGNED_INT, 0);
}

/*
	gets block pointer with a local coordinate
*/
Block* Chunk::get_block(ivec3 local_coord) {
	int x = local_coord.x;
	int y = local_coord.y;
	int z = local_coord.z;
	if (x < 0 || y < 0 || z < 0 || x > width - 1 || y > height - 1 || z > length - 1) {
		//cout << "provided local coord is invalid" << endl;
		return nullptr;
	}
	return &blocks[x][y][z];
}

/*
	sets the type of block on the provided local coordinate.
	note that blocks in edges of blocks array are blocks of neighbor chunks.
	it's always from 1 to (width or length) - 1 for x and z that are 'our' blocks.
*/
void Chunk::set_block(ivec3 local_coord, block_type type) {
	int x = local_coord.x;
	int y = local_coord.y;
	int z = local_coord.z;
	if (x < 0 || x > width - 1 || y < 0 || y > height - 1 || z < 0 || z > length - 1) {
		//cout << "provided local coord is invalid" << endl;
		return;
	}
	blocks[x][y][z].type = type;
}

/*
	adds vertices of a non-block structure, telling to spawn the structure at the designated local coordinate.
	non-block structures are drawn after all blocks are drawn.
	To efficiently add/remove non-block structures, use the local coordinate as the structure's unique ID within a chunk.
*/
void Chunk::add_nonblock_structure_vertices(ivec3 local_coord, vector<vertex> vertices) {
	const auto& structure = nonblock_structure_vertices.find(local_coord);
	if (structure == nonblock_structure_vertices.end()) {
		nonblock_structure_vertices.insert({ local_coord, vertices });
	}
}

/*
	remove non-block structure at a local coordinate
*/
void Chunk::remove_structure(ivec3 local_coord) {
	nonblock_structure_vertices.erase(local_coord);
}

/*
	after adding vertices of non-block structures, update their indices.
	All objects, including non-block structures in this game, are expected to be made of 'faces'
*/
void Chunk::update_nonblock_structure_vertices_and_indices() {
	for (const auto &e : nonblock_structure_vertices) {
		for (int i = 1; i <= e.second.size(); ++i) {
			transp_vertices.push_back(e.second[i - 1]);
			if (i > 1 && i % 4 == 0) {
				update_face_indices(true, false);
			}
		}
	}
}

/*
	only used for block objects.
	updates and adds indices for either transparent or opaque types of blocks
*/
void Chunk::update_face_indices(bool has_transparency, bool is_water) {
	if (is_water) {
		GLuint base_index = water_vertices.size() - 4; 
		water_indices.push_back(base_index);
		water_indices.push_back(base_index + 1);
		water_indices.push_back(base_index + 2);
		water_indices.push_back(base_index + 2);
		water_indices.push_back(base_index + 3);
		water_indices.push_back(base_index);
		return;
	}
	if (has_transparency) {
		GLuint base_index = transp_vertices.size() - 4; //ensures base_index to start at 0
		//indices are added in: 0 -> 1 -> 2  (one triangle) --> 2 -> 3 -> 0 (another triangle)
		//a face (square) is made of 2 triangles, which is made of 6 indices in total.
		transp_indices.push_back(base_index);
		transp_indices.push_back(base_index + 1);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 2);
		transp_indices.push_back(base_index + 3);
		transp_indices.push_back(base_index);
	} else {
		GLuint base_index = opaque_vertices.size() - 4;
		opaque_indices.push_back(base_index);
		opaque_indices.push_back(base_index + 1);
		opaque_indices.push_back(base_index + 2);
		opaque_indices.push_back(base_index + 2);
		opaque_indices.push_back(base_index + 3);
		opaque_indices.push_back(base_index);
	}
}

/*
	used to construct a chunk mesh and only for block type objects.
	pushes the new vertices to opaque or transparent vertices based on the block type's transparency
*/
void Chunk::add_face(block_face face, block_type type, vec3 local_coord) {
	if (texture_map.find(type) == texture_map.end()) return; //a type is not in texture map if it is a structure that is not cube i.e. grass
	vec2 texture_coord = texture_map[type][face];

	if (type == water) {
		vector<vertex> cw_verts = cw_face_map[face];
		for (int i = 0; i < cw_verts.size(); ++i) {
			vertex v = cw_verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1); 
			v.texture = convert_to_uv(i, texture_coord);
			water_vertices.push_back(v);
		}
		update_face_indices(true, true);

		vector<vertex> ccw_verts = ccw_face_map[face];
		for (int i = 0; i < ccw_verts.size(); ++i) {
			vertex v = ccw_verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1); 
			v.texture = convert_to_uv(i, texture_coord);
			water_vertices.push_back(v);
		}
		update_face_indices(true, true);
	}
	else {
		vector<vertex> verts = cw_face_map[face];
		bool transparency = has_transparency(type);

		//transforms vertices
		for (int i = 0; i < verts.size(); ++i) {
			vertex v = verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1); //subtract 1 to adjust chunk position due to boundaries
			v.texture = convert_to_uv(i, texture_coord);
			if (transparency) {
				transp_vertices.push_back(v);
				
			}
			else {
				opaque_vertices.push_back(v);
			}
		}
		update_face_indices(transparency, false);
	}
}

/*
	rebuilds chunk, emptying all vertices and indices of chunk
*/
void Chunk::rebuild_chunk() {
	opaque_vertices.clear();
	opaque_indices.clear();
	transp_vertices.clear();
	transp_indices.clear();
	water_vertices.clear();
	water_indices.clear();

	build_chunk();
	should_rebuild = false;
}

/*
	constructs a chunk mesh, adding only the visible faces.
	once the chunk meshing is done, adds non-block structures, updating VAOs and EBOs.
	lastly sets has_built to be true.
*/
void Chunk::build_chunk() {
	//check x and z from 1 to 16 (boundaries at 0 and 17)
	for (int x = 1; x < width - 1; ++x) {
		for (int z = 1; z < length - 1; ++z) {
			for (int y = 0; y < height; ++y) {
				const Block &current = blocks[x][y][z];
				if (current.type == none) {
					continue;
				}
				int h = height_map[x][z];
				bool am_i_transparent = has_transparency(current.type);
				ivec3 pos = ivec3(x, y, z);
				block_type type = current.type;

				if (blocks[x - 1][y][z].type == none || has_transparency(blocks[x - 1][y][z].type) && !am_i_transparent) {
					add_face(Left, type, pos);
				}
				if (y > 0 && (blocks[x][y - 1][z].type == none || has_transparency(blocks[x][y - 1][z].type) && !am_i_transparent)) {
					add_face(Bottom, type, pos);
				}
				if (blocks[x][y][z - 1].type == none || has_transparency(blocks[x][y][z - 1].type) && !am_i_transparent) {
					add_face(Back, type, pos);
				}
				if (blocks[x + 1][y][z].type == none || has_transparency(blocks[x + 1][y][z].type) && !am_i_transparent) {
					add_face(Right, type, pos);
				}
				if (y < height - 1 && (blocks[x][y + 1][z].type == none || has_transparency(blocks[x][y + 1][z].type) && !am_i_transparent)) {
					add_face(Top, type, pos);
				}
				if (blocks[x][y][z + 1].type == none || has_transparency(blocks[x][y][z + 1].type) && !am_i_transparent) {
					add_face(Front, type, pos);
				}
			}
		}
	}

	update_nonblock_structure_vertices_and_indices(); //must be called after spawn_structures() is called in Terrain
	update_buffers_data();
	has_built = true;
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