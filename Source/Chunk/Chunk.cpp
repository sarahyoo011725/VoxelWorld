#include "Chunk.h"
#include <algorithm>
#include "World/ChunkManager.h"
#include "World/StructureGenerator.h"

static const uint32_t bedrock_salt = 0xBED0;

/*
	sets up the chunk's dimensions and GL buffer objects. deliberately does no
	terrain generation - that lives in generate_terrain() so it can be moved off
	the main thread, while the GL objects created here cannot be.
*/
Chunk::Chunk(ivec2 chunk_id) : cm(ChunkManager::get_instance()), sm(ShaderManager::get_instance()) {
	//add 1 to width and length to store neighbor chunks' block data in their edge
	id = chunk_id;
	world_position = vec3(chunk_id.x * chunk_size, 0, chunk_id.y * chunk_size);
	width = chunk_size + 2;
	height = 112; //sea level + tallest extreme peak, with the underground below
	length = chunk_size + 2;

	opaque_vao.bind();
	opaque_vao.link_attrib(opaque_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0); //vertex positions coords
	opaque_vao.link_attrib(opaque_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float))); //vertex texture coords
	opaque_vao.link_attrib(opaque_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float))); //vertex normal
	opaque_vao.link_attrib(opaque_vbo, 3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(8 * sizeof(float))); //atlas cell the uv repeats
	opaque_vao.link_attrib(opaque_vbo, 4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(10 * sizeof(float))); //biome tint

	transp_vao.bind();
	transp_vao.link_attrib(transp_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	transp_vao.link_attrib(transp_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
	transp_vao.link_attrib(transp_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));
	transp_vao.link_attrib(transp_vbo, 3, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(8 * sizeof(float)));
	transp_vao.link_attrib(transp_vbo, 4, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(10 * sizeof(float)));

	water_vao.bind();
	water_vao.link_attrib(water_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
	water_vao.link_attrib(water_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(3 * sizeof(float)));
	water_vao.link_attrib(water_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(5 * sizeof(float)));

	foliage_vao.bind();
	foliage_vao.link_attrib(foliage_vbo, 0, 3, GL_FLOAT, GL_FALSE, sizeof(foliage_vertex), (void*)0);
	foliage_vao.link_attrib(foliage_vbo, 1, 2, GL_FLOAT, GL_FALSE, sizeof(foliage_vertex), (void*)(3 * sizeof(float)));
	foliage_vao.link_attrib(foliage_vbo, 2, 3, GL_FLOAT, GL_FALSE, sizeof(foliage_vertex), (void*)(5 * sizeof(float)));
	foliage_vao.link_attrib(foliage_vbo, 3, 1, GL_FLOAT, GL_FALSE, sizeof(foliage_vertex), (void*)(8 * sizeof(float)));
	foliage_vao.link_attrib(foliage_vbo, 4, 3, GL_FLOAT, GL_FALSE, sizeof(foliage_vertex), (void*)(9 * sizeof(float))); //biome tint
}

/*
	releases the chunk's GL buffers. without this, unloading a distant chunk
	would free its block data but leak every VAO/VBO/EBO it owns
*/
Chunk::~Chunk() {
	opaque_vao.destroy();
	opaque_vbo.destroy();
	opaque_ebo.destroy();
	transp_vao.destroy();
	transp_vbo.destroy();
	transp_ebo.destroy();
	water_vao.destroy();
	water_vbo.destroy();
	water_ebo.destroy();
	foliage_vao.destroy();
	foliage_vbo.destroy();
	foliage_ebo.destroy();
}

/*
	fills the block array from the terrain generator. touches no GL state and
	only this chunk's own storage, so it is safe to run on a worker thread.
*/
void Chunk::generate_terrain() {
	if (has_generated) return;

	blocks.resize(static_cast<size_t>(width) * height * length);
	height_map = get_heightmap();

	//water fills to water_level even where the ground is lower, so the top of
	//the terrain alone is not the top of the solid geometry
	int highest = water_level;
	for (int h : height_map) {
		if (h > highest) highest = h;
	}
	max_occupied_y = std::min(highest, height - 1);

	const TerrainConfig& config = get_terrain_generator().config;
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			int h = get_height(x, z);
			const BiomeDefinition& biome = biome_of(get_biome(x, z));
			WorldRandom rng(config.world_seed, (int)world_position.x + x - 1, (int)world_position.z + z - 1, bedrock_salt);
			int bedrock_top = config.bedrock_height - 1 - rng.next_int(2);

			for (int y = 0; y < height; ++y) {
				block_type type = none;
				if (y > h && y <= water_level) {
					type = water;
				}
				if (y == h) {
					type = biome.surface;
				}
				if (y < h) {
					type = biome.subsurface;
					if (y < h - biome.subsurface_depth) {
						type = stone;
					}
				}
				//anything at the waterline is beach regardless of biome, so
				//shores read as shores instead of grass running into the sea
				if (y <= h && y >= h - 2 && y + 1 < height && y + 1 <= water_level) {
					type = sand;
				}
				if (y <= bedrock_top) {
					type = bedrock;
				}
				blocks[block_index(x, y, z)].type = type;
			}
		}
	}

	has_generated = true;
}

/*
	generate a height map from a block's world coordinate
*/
vector<int> Chunk::get_heightmap() {
	vector<int> map(static_cast<size_t>(width) * length);
	biome_map.assign(static_cast<size_t>(width) * length, biome_id::plains);

	const TerrainGenerator& generator = get_terrain_generator();
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < length; ++z) {
			//get a block's world coords
			int x_pos = world_position.x + x - 1;
			int z_pos = world_position.z + z - 1;

			//one climate sample feeds both the height and the biome, so adding
			//biomes costs two extra noise lookups per column rather than a
			//second pass over the chunk
			ClimateSample climate = generator.sample_climate(x_pos, z_pos);

			int height_val = climate.elevation;
			if (height_val > height) height_val = height;
			if (height_val < 0) height_val = 0;
			map[height_index(x, z)] = height_val;
			biome_map[height_index(x, z)] = select_biome(climate, water_level);
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
	foliage_vbo.reset_vertices(foliage_vertices.data(), sizeof(foliage_vertex) * foliage_vertices.size(), GL_STATIC_DRAW);
	foliage_ebo.reset_indices(foliage_indices.data(), sizeof(GLuint) * foliage_indices.size(), GL_STATIC_DRAW);
}

/*
	draws opaque objects.
	Opaque objects must be drawn before transparent objects 
*/
void Chunk::draw_opaque_blocks() {
	if (opaque_indices.empty()) return;
	//the shader is activated once by the caller for the whole pass, not per chunk
	opaque_vao.bind();
	opaque_ebo.bind();
	glDrawElements(GL_TRIANGLES, opaque_indices.size(), GL_UNSIGNED_INT, 0);
}

/*
	draws objects with transparency.
	Transparent objects must be drawn after opaque objects
*/
void Chunk::draw_transparent_blocks() {
	if (transp_indices.empty()) return;
	sm.default_shader.activate();
	transp_vao.bind();
	transp_ebo.bind();
	glDrawElements(GL_TRIANGLES, transp_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::draw_water() {
	if (water_indices.empty()) return;
	sm.wave_shader.activate();
	water_vao.bind();
	water_ebo.bind();
	glDrawElements(GL_TRIANGLES, water_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::draw_foliage() {
	if (foliage_indices.empty()) return;
	sm.foliage_shader.activate();
	foliage_vao.bind();
	foliage_ebo.bind();
	glDrawElements(GL_TRIANGLES, foliage_indices.size(), GL_UNSIGNED_INT, 0);
}

//depth-only draws for the shadow map pass - the shadow shader is activated
//once by the caller, not per chunk, so these don't switch shaders themselves
void Chunk::draw_opaque_depth() {
	if (opaque_indices.empty()) return;
	opaque_vao.bind();
	opaque_ebo.bind();
	glDrawElements(GL_TRIANGLES, opaque_indices.size(), GL_UNSIGNED_INT, 0);
}

void Chunk::draw_foliage_depth() {
	if (foliage_indices.empty()) return;
	foliage_vao.bind();
	foliage_ebo.bind();
	glDrawElements(GL_TRIANGLES, foliage_indices.size(), GL_UNSIGNED_INT, 0);
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
	return &blocks[block_index(x, y, z)];
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
	blocks[block_index(x, y, z)].type = type;
	if (y > max_occupied_y) max_occupied_y = y;
}

/*
	adds vertices of a non-block structure, telling to spawn the structure at the designated local coordinate.
	non-block structures are drawn after all blocks are drawn.
	To efficiently add/remove non-block structures, use the local coordinate as the structure's unique ID within a chunk.
*/
void Chunk::add_nonblock_structure_vertices(ivec3 local_coord, vector<foliage_vertex> vertices) {
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
			foliage_vertices.push_back(e.second[i - 1]);
			if (i > 1 && i % 4 == 0) {
				add_foliage_quad_indices();
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

void Chunk::add_foliage_quad_indices() {
	GLuint base_index = foliage_vertices.size() - 4;
	foliage_indices.push_back(base_index);
	foliage_indices.push_back(base_index + 1);
	foliage_indices.push_back(base_index + 2);
	foliage_indices.push_back(base_index + 2);
	foliage_indices.push_back(base_index + 3);
	foliage_indices.push_back(base_index);
}

/*
	used to construct a chunk mesh and only for block type objects.
	pushes the new vertices to opaque or transparent vertices based on the block type's transparency
*/
void Chunk::add_face(block_face face, block_type type, vec3 local_coord) {
	if (texture_map.find(type) == texture_map.end()) return; //a type is not in texture map if it is a structure that is not cube i.e. grass
	vec2 texture_coord = texture_map[type][face];
	vec2 tile_origin = tile_uv_origin(texture_coord);
	vec3 tint = tint_for(type, face, (int)local_coord.x, (int)local_coord.z);

	vec3 normal = face_normal(face);

	if (type == water) {
		vector<vertex> cw_verts = cw_face_map[face];
		for (int i = 0; i < cw_verts.size(); ++i) {
			vertex v = cw_verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1);
			v.texture = convert_to_uv(i, texture_coord);
			v.normal = normal;
			v.tint = tint;
			water_vertices.push_back(v);
		}
		update_face_indices(true, true);

		vector<vertex> ccw_verts = ccw_face_map[face];
		for (int i = 0; i < ccw_verts.size(); ++i) {
			vertex v = ccw_verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1);
			v.texture = convert_to_uv(i, texture_coord);
			v.normal = -normal;
			v.tint = tint;
			water_vertices.push_back(v);
		}
		update_face_indices(true, true);
	}
	else if (is_foliage(type)) {
		vector<vertex> verts = cw_face_map[face];

		//a leaf block has no "root" side like a grass blade does, so it
		//sways as a rigid whole - a pinned bottom would shear it into a wobbling parallelogram
		for (int i = 0; i < verts.size(); ++i) {
			vertex v = verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1);
			v.texture = convert_to_uv(i, texture_coord);
			foliage_vertices.push_back({ v.position, v.texture, normal, 1.0f, tint });
		}
		add_foliage_quad_indices();
	}
	else {
		vector<vertex> verts = cw_face_map[face];
		bool transparency = has_transparency(type);

		//transforms vertices
		const vec2 corner_uv[4] = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };
		for (int i = 0; i < verts.size(); ++i) {
			vertex v = verts[i];
			v.position += local_coord + world_position + vec3(-1, 0, -1); //subtract 1 to adjust chunk position due to boundaries
			//the tiled form the merged quads use, since both go through default.frag
			v.texture = corner_uv[i];
			v.tile_origin = tile_origin;
			v.tint = tint;
			v.normal = normal;
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
	regenerates the mesh and re-uploads it. used when a block changed
*/
void Chunk::rebuild_chunk() {
	build_mesh();
	upload_mesh();
	should_rebuild = false;
}

/*
	generates the mesh and uploads it in one step, for callers on the GL thread
*/
void Chunk::build_chunk() {
	build_mesh();
	upload_mesh();
}

/*
	hands the built mesh to the GL buffers. the only phase that touches GL, so
	it must run on the thread owning the context
*/
void Chunk::upload_mesh() {
	update_buffers_data();
	mesh_ready = false;
	has_built = true;
}

/*
	the colour a face gets multiplied by. greyscale grass/leaf tiles take their
	biome's colour; every other tile stays as authored
*/
vec3 Chunk::tint_for(block_type type, block_face face, int x, int z) const {
	if (!is_tinted_face(type, face)) return vec3(1.0f);
	const BiomeDefinition& biome = biome_of(get_biome(x, z));
	return type == dirt_grass ? biome.grass_tint : biome.foliage_tint;
}

/*
	is this opaque block's face exposed? mirrors the neighbour test the per-block
	loop uses, minus the foliage case - foliage never reaches the opaque buffer
*/
bool Chunk::opaque_face_visible(int x, int y, int z, block_face face) const {
	vec3 n = face_normal(face);
	int nx = x + (int)n.x, ny = y + (int)n.y, nz = z + (int)n.z;
	if (ny < 0 || ny >= height) return false;
	block_type neighbour = blocks[block_index(nx, ny, nz)].type;
	return neighbour == none || has_transparency(neighbour);
}

/*
	emits one quad standing in for run_u x run_v block faces.

	the quad is built by stretching the unit face from cw_face_map rather than
	from hand-written corner tables, so winding, normal and texture orientation
	are the same as a single face by construction. base_block is the block at the
	quad's texture origin, and the uv runs 0..run across the quad, which the
	fragment shader wraps back into one atlas cell.
*/
void Chunk::add_merged_quad(block_face face, block_type type, ivec3 base_block, int run_u, int run_v) {
	if (texture_map.find(type) == texture_map.end()) return;
	vec2 texture_coord = texture_map[type][face];
	vec2 tile_origin = tile_uv_origin(texture_coord);
	vec3 tint = tint_for(type, face, base_block.x, base_block.z);

	const vector<vertex>& unit = cw_face_map[face];
	//the face's own texture axes, read off the unit quad: corner 0 -> 1 is +u,
	//corner 3 -> 0 is +v
	vec3 u_axis = unit[1].position - unit[0].position;
	vec3 v_axis = unit[0].position - unit[3].position;
	vec3 normal = face_normal(face);

	vec3 base_center = world_position + vec3(base_block.x - 1, base_block.y, base_block.z - 1);

	//corner order matches cw_face_map: left-top, right-top, right-bottom, left-bottom
	const vec2 corner_uv[4] = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };
	for (int i = 0; i < 4; ++i) {
		vertex v;
		v.position = base_center + unit[i].position
			+ u_axis * (corner_uv[i].x * (float)(run_u - 1))
			+ v_axis * (corner_uv[i].y * (float)(run_v - 1));
		v.texture = vec2(corner_uv[i].x * run_u, corner_uv[i].y * run_v);
		v.normal = normal;
		v.tile_origin = tile_origin;
		v.tint = tint;
		opaque_vertices.push_back(v);
	}
	update_face_indices(false, false);
}

/*
	greedy meshing for the opaque pass: for each of the six directions, sweep the
	chunk slice by slice, mark which faces need drawing, and merge neighbouring
	faces of the same block type into the largest rectangles that fit. cuts the
	quad count by roughly half on typical terrain compared with one quad per face.
*/
void Chunk::build_opaque_mesh() {
	int top = std::min(max_occupied_y + 1, height);
	const block_face faces[6] = { Front, Back, Left, Right, Top, Bottom };

	for (block_face face : faces) {
		//one axis is swept, the other two span the mask. a runs along the mask's
		//first axis, b along its second
		int slice_lo, slice_hi, a_lo, a_hi, b_lo, b_hi;
		if (face == Front || face == Back) {
			slice_lo = 1; slice_hi = length - 1; a_lo = 1; a_hi = width - 1; b_lo = 0; b_hi = top;
		}
		else if (face == Left || face == Right) {
			slice_lo = 1; slice_hi = width - 1; a_lo = 1; a_hi = length - 1; b_lo = 0; b_hi = top;
		}
		else {
			slice_lo = 0; slice_hi = top; a_lo = 1; a_hi = width - 1; b_lo = 1; b_hi = length - 1;
		}
		int a_count = a_hi - a_lo, b_count = b_hi - b_lo;
		if (a_count <= 0 || b_count <= 0) continue;

		//which world direction each mask axis advances in, so the quad's texture
		//origin can be placed at the right end of the rectangle
		vec3 a_dir = (face == Left || face == Right) ? vec3(0, 0, 1) : vec3(1, 0, 0);
		vec3 b_dir = (face == Top || face == Bottom) ? vec3(0, 0, 1) : vec3(0, 1, 0);
		const vector<vertex>& unit = cw_face_map[face];
		bool u_flipped = dot(unit[1].position - unit[0].position, a_dir) < 0.0f;
		bool v_flipped = dot(unit[0].position - unit[3].position, b_dir) < 0.0f;

		vector<block_type> mask(static_cast<size_t>(a_count) * b_count);
		vector<biome_id> biome_key(static_cast<size_t>(a_count) * b_count);

		for (int slice = slice_lo; slice < slice_hi; ++slice) {
			auto to_block = [&](int a, int b) {
				if (face == Front || face == Back) return ivec3(a_lo + a, b_lo + b, slice);
				if (face == Left || face == Right) return ivec3(slice, b_lo + b, a_lo + a);
				return ivec3(a_lo + a, slice, b_lo + b);
			};

			for (int a = 0; a < a_count; ++a) {
				for (int b = 0; b < b_count; ++b) {
					ivec3 p = to_block(a, b);
					block_type t = blocks[block_index(p.x, p.y, p.z)].type;
					bool opaque = t != none && !has_transparency(t) && !is_foliage(t);
					mask[static_cast<size_t>(a) * b_count + b] =
						(opaque && opaque_face_visible(p.x, p.y, p.z, face)) ? t : none;
					//a tinted face changes colour with the biome, so two columns
					//may only merge if they also share one. untinted faces keep a
					//single biome key and merge as freely as before
					biome_key[static_cast<size_t>(a) * b_count + b] =
						is_tinted_face(t, face) ? get_biome(p.x, p.z) : biome_id::plains;
				}
			}

			for (int a = 0; a < a_count; ++a) {
				for (int b = 0; b < b_count; ) {
					size_t here = static_cast<size_t>(a) * b_count + b;
					block_type t = mask[here];
					if (t == none) { ++b; continue; }
					biome_id key = biome_key[here];

					//extend along b first, then widen along a while whole rows match
					int run_b = 1;
					while (b + run_b < b_count) {
						size_t n = static_cast<size_t>(a) * b_count + b + run_b;
						if (mask[n] != t || biome_key[n] != key) break;
						++run_b;
					}

					int run_a = 1;
					bool can_widen = true;
					while (a + run_a < a_count && can_widen) {
						for (int k = 0; k < run_b; ++k) {
							size_t n = static_cast<size_t>(a + run_a) * b_count + b + k;
							if (mask[n] != t || biome_key[n] != key) { can_widen = false; break; }
						}
						if (can_widen) ++run_a;
					}

					for (int i = 0; i < run_a; ++i) {
						for (int k = 0; k < run_b; ++k) {
							mask[static_cast<size_t>(a + i) * b_count + b + k] = none;
						}
					}

					//the texture origin sits at whichever end of the rectangle the
					//face's u and v axes start from
					ivec3 base = to_block(u_flipped ? a + run_a - 1 : a, v_flipped ? b + run_b - 1 : b);
					add_merged_quad(face, t, base, run_a, run_b);

					b += run_b;
				}
			}
		}
	}
}

/*
	constructs a chunk mesh, adding only the visible faces, then appends
	non-block structure geometry. reads this chunk's blocks and writes only its
	own vertex/index buffers, so it is safe to run on a worker thread - but the
	caller must ensure no neighbour is still spawning structures into it.
*/
void Chunk::build_mesh() {
	opaque_vertices.clear();
	opaque_indices.clear();
	transp_vertices.clear();
	transp_indices.clear();
	water_vertices.clear();
	water_indices.clear();
	foliage_vertices.clear();
	foliage_indices.clear();

	build_opaque_mesh();

	//check x and z from 1 to 16 (boundaries at 0 and 17)
	int top = std::min(max_occupied_y + 1, height);
	for (int x = 1; x < width - 1; ++x) {
		for (int z = 1; z < length - 1; ++z) {
			for (int y = 0; y < top; ++y) {
				const Block &current = blocks[block_index(x, y, z)];
				if (current.type == none) {
					continue;
				}
				//already handled by the greedy pass
				if (current.type != none && !has_transparency(current.type) && !is_foliage(current.type)) continue;

				bool am_i_transparent = has_transparency(current.type);
				//foliage blocks sway independently, so a neighbor can no longer be trusted
				//to seal a culled face - always draw a full, sealed cube for them
				bool am_i_foliage = is_foliage(current.type);
				ivec3 pos = ivec3(x, y, z);
				block_type type = current.type;

				block_type left = blocks[block_index(x - 1, y, z)].type;
				block_type back = blocks[block_index(x, y, z - 1)].type;
				block_type right = blocks[block_index(x + 1, y, z)].type;
				block_type front = blocks[block_index(x, y, z + 1)].type;

				if (am_i_foliage || left == none || has_transparency(left) && !am_i_transparent) {
					add_face(Left, type, pos);
				}
				if (y > 0) {
					block_type below = blocks[block_index(x, y - 1, z)].type;
					if (am_i_foliage || below == none || has_transparency(below) && !am_i_transparent) {
						add_face(Bottom, type, pos);
					}
				}
				if (am_i_foliage || back == none || has_transparency(back) && !am_i_transparent) {
					add_face(Back, type, pos);
				}
				if (am_i_foliage || right == none || has_transparency(right) && !am_i_transparent) {
					add_face(Right, type, pos);
				}
				if (y < height - 1) {
					block_type above = blocks[block_index(x, y + 1, z)].type;
					if (am_i_foliage || above == none || has_transparency(above) && !am_i_transparent) {
						add_face(Top, type, pos);
					}
				}
				if (am_i_foliage || front == none || has_transparency(front) && !am_i_transparent) {
					add_face(Front, type, pos);
				}
			}
		}
	}

	update_nonblock_structure_vertices_and_indices(); //must be called after spawn_structures() is called in Terrain
	mesh_ready = true;
}

