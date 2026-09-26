#include "Chunk.h"
#include <algorithm>
#include <cstddef>
#include "World/ChunkManager.h"
#include "World/StructureGenerator.h"
#include "World/ChunkGeneration.h"

namespace {
	//block properties and face geometry as flat arrays, so the meshers' inner loops
	//read an array instead of calling a switch or walking nested std::maps
	struct mesh_tables {
		bool opaque[256] = {}; //meshed by the greedy pass
		bool exposes[256] = {}; //a solid face against this block is visible
		bool textured[256] = {};
		vec2 tile[256][6] = {};
		vec3 corner[6][4] = {};

		mesh_tables() {
			for (int t = 0; t < 256; ++t) {
				block_type type = (block_type)t;
				opaque[t] = type != none && !has_transparency(type) && !is_foliage(type);
				exposes[t] = type == none || has_transparency(type);
			}
			for (const auto& entry : texture_map) {
				textured[entry.first] = true;
				for (const auto& face : entry.second) tile[entry.first][face.first] = face.second;
			}
			for (const auto& entry : cw_face_map) {
				for (int i = 0; i < 4; ++i) corner[entry.first][i] = entry.second[i].position;
			}
		}
	};

	const mesh_tables& tables() {
		static const mesh_tables instance;
		return instance;
	}
}

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
	height = chunk_height;
	length = chunk_size + 2;
	section_count = (height + section_size - 1) / section_size;
	section_connectivity.assign(section_count, all_links);

	//terrain vertices: position, one packed word (see block_vertex) and an RGBA8 tint
	VAO* terrain_vaos[3] = { &opaque_vao, &transp_vao, &water_vao };
	VBO* terrain_vbos[3] = { &opaque_vbo, &transp_vbo, &water_vbo };
	for (int i = 0; i < 3; ++i) {
		terrain_vaos[i]->bind();
		terrain_vaos[i]->link_attrib(*terrain_vbos[i], 0, 3, GL_FLOAT, GL_FALSE, sizeof(block_vertex), (void*)offsetof(block_vertex, position));
		terrain_vaos[i]->link_integer_attrib(*terrain_vbos[i], 1, 1, GL_UNSIGNED_INT, sizeof(block_vertex), (void*)offsetof(block_vertex, data));
		terrain_vaos[i]->link_attrib(*terrain_vbos[i], 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(block_vertex), (void*)offsetof(block_vertex, tint));
	}

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

	has_cave_opening = generate_chunk_blocks(id, width, height, length, blocks, height_map, biome_map).opened_surface;

	//water fills to water_level even where the ground is lower, so the top of
	//the terrain alone is not the top of the solid geometry
	int highest = water_level;
	for (int h : height_map) {
		if (h > highest) highest = h;
	}
	max_occupied_y = std::min(highest, height - 1);
	lowest_surface_y = *std::min_element(height_map.begin(), height_map.end());
	find_emitters(blocks, width, height, length, emitters);

	has_generated = true;
}

/*
	updates vertices and indices of transparent and opaque geometries in VBOs and EBOs.
	This must be called after build_chunk() or rebuild_chunk()
*/
void Chunk::update_buffers_data() {
	opaque_vbo.reset_vertices(opaque_vertices.data(), sizeof(block_vertex) * opaque_vertices.size(), GL_STATIC_DRAW);
	opaque_ebo.reset_indices(opaque_indices.data(), sizeof(GLuint) * opaque_indices.size(), GL_STATIC_DRAW);
	transp_vbo.reset_vertices(transp_vertices.data(), sizeof(block_vertex) * transp_vertices.size(), GL_STATIC_DRAW);
	transp_ebo.reset_indices(transp_indices.data(), sizeof(GLuint) * transp_indices.size(), GL_STATIC_DRAW);
	water_vbo.reset_vertices(water_vertices.data(), sizeof(block_vertex) * water_vertices.size(), GL_STATIC_DRAW);
	water_ebo.reset_indices(water_indices.data(), sizeof(GLuint) * water_indices.size(), GL_STATIC_DRAW);
	foliage_vbo.reset_vertices(foliage_vertices.data(), sizeof(foliage_vertex) * foliage_vertices.size(), GL_STATIC_DRAW);
	foliage_ebo.reset_indices(foliage_indices.data(), sizeof(GLuint) * foliage_indices.size(), GL_STATIC_DRAW);

	opaque_count = opaque_indices.size();
	transp_count = transp_indices.size();
	water_count = water_indices.size();
	foliage_count = foliage_indices.size();
	uploaded_vertices = opaque_vertices.size() + transp_vertices.size() + water_vertices.size() + foliage_vertices.size();
	uploaded_bytes = (opaque_vertices.size() + transp_vertices.size() + water_vertices.size()) * sizeof(block_vertex)
		+ foliage_vertices.size() * sizeof(foliage_vertex)
		+ (opaque_count + transp_count + water_count + foliage_count) * sizeof(GLuint);

	//the GPU has its own copy now, and the next rebuild starts from nothing, so the CPU one is dead weight
	vector<block_vertex>().swap(opaque_vertices);
	vector<GLuint>().swap(opaque_indices);
	vector<block_vertex>().swap(transp_vertices);
	vector<GLuint>().swap(transp_indices);
	vector<block_vertex>().swap(water_vertices);
	vector<GLuint>().swap(water_indices);
	vector<foliage_vertex>().swap(foliage_vertices);
	vector<GLuint>().swap(foliage_indices);
}

/*
	draws opaque objects.
	Opaque objects must be drawn before transparent objects 
*/
void Chunk::draw_opaque_blocks() {
	if (opaque_count == 0) return;
	//the shader is activated once by the caller for the whole pass, not per chunk
	opaque_vao.bind();
	opaque_ebo.bind();
	draw_sections(opaque_ranges, visible_sections);
}

/*
	draws objects with transparency.
	Transparent objects must be drawn after opaque objects
*/
void Chunk::draw_transparent_blocks() {
	if (transp_count == 0) return;
	sm.default_shader.activate();
	transp_vao.bind();
	transp_ebo.bind();
	draw_sections(transp_ranges, visible_sections);
}

void Chunk::draw_water() {
	if (water_count == 0) return;
	sm.wave_shader.activate();
	water_vao.bind();
	water_ebo.bind();
	draw_sections(water_ranges, visible_sections);
}

void Chunk::draw_foliage() {
	if (foliage_count == 0) return;
	sm.foliage_shader.activate();
	foliage_vao.bind();
	foliage_ebo.bind();
	draw_sections(foliage_ranges, visible_sections);
}

//depth-only draws for the shadow map pass - the shadow shader is activated
//once by the caller, not per chunk, so these don't switch shaders themselves
void Chunk::draw_opaque_depth() {
	if (opaque_count == 0) return;
	opaque_vao.bind();
	opaque_ebo.bind();
	draw_sections(opaque_ranges, shadow_sections);
}

void Chunk::draw_foliage_depth() {
	if (foliage_count == 0) return;
	foliage_vao.bind();
	foliage_ebo.bind();
	draw_sections(foliage_ranges, shadow_sections);
}

void Chunk::draw_sections(const vector<index_range>& ranges, uint32_t mask) const {
	size_t s = 0;
	while (s < ranges.size()) {
		if (!(mask & (1u << s))) { ++s; continue; }
		GLuint first = ranges[s].first;
		GLsizei count = 0;
		while (s < ranges.size() && (mask & (1u << s))) {
			count += ranges[s].count;
			++s;
		}
		if (count > 0) {
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(first * sizeof(GLuint)));
		}
	}
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
	block_type previous = blocks[block_index(x, y, z)].type;
	blocks[block_index(x, y, z)].type = type;
	if (y > max_occupied_y) max_occupied_y = y;

	bool owned = x >= 1 && x <= width - 2 && z >= 1 && z <= length - 2;
	if (owned && is_emissive(previous) != is_emissive(type)) {
		if (is_emissive(type)) emitters.push_back(local_coord);
		else emitters.erase(std::remove(emitters.begin(), emitters.end(), local_coord), emitters.end());
	}
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
void Chunk::update_nonblock_structure_vertices_and_indices(int y_lo, int y_hi) {
	for (const auto &e : nonblock_structure_vertices) {
		if (e.first.y < y_lo || e.first.y >= y_hi) continue;
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
	const mesh_tables& tab = tables();
	if (!tab.textured[type]) return; //a type is not in texture map if it is a structure that is not cube i.e. grass
	vec2 texture_coord = tab.tile[type][face];
	vec3 tint = tint_for(type, face, (int)local_coord.x, (int)local_coord.z);
	uint16_t light = light_key(type, face, (int)local_coord.x, (int)local_coord.y, (int)local_coord.z);
	uint32_t packed_tint = pack_tint(tint);

	vec3 normal = face_normal(face);
	//corner order matches cw_face_map and ccw_face_map: left-top, right-top, right-bottom, left-bottom
	const ivec2 corner_uv[4] = { ivec2(0, 1), ivec2(1, 1), ivec2(1, 0), ivec2(0, 0) };
	vec3 offset = local_coord + world_position + vec3(-1, 0, -1); //subtract 1 to adjust chunk position due to boundaries

	if (type == water) {
		//both windings, so the surface shows from above and from under the water
		for (int i = 0; i < 4; ++i) {
			water_vertices.push_back({ tab.corner[face][i] + offset,
				pack_block_vertex(normal, light, corner_uv[i].x, corner_uv[i].y, texture_coord), packed_tint });
		}
		update_face_indices(true, true);

		const vector<vertex>& ccw_verts = ccw_face_map[face];
		for (int i = 0; i < 4; ++i) {
			water_vertices.push_back({ ccw_verts[i].position + offset,
				pack_block_vertex(-normal, light, corner_uv[i].x, corner_uv[i].y, texture_coord), packed_tint });
		}
		update_face_indices(true, true);
	}
	else if (is_foliage(type)) {
		const vec3* verts = tab.corner[face];

		//a leaf block has no "root" side like a grass blade does, so it
		//sways as a rigid whole - a pinned bottom would shear it into a wobbling parallelogram
		for (int i = 0; i < 4; ++i) {
			foliage_vertices.push_back({ verts[i] + offset, convert_to_uv(i, texture_coord), normal, 1.0f, tint });
		}
		add_foliage_quad_indices();
	}
	else {
		const vec3* verts = tab.corner[face];
		bool transparency = has_transparency(type);
		vector<block_vertex>& target = transparency ? transp_vertices : opaque_vertices;
		for (int i = 0; i < 4; ++i) {
			target.push_back({ verts[i] + offset,
				pack_block_vertex(normal, light, corner_uv[i].x, corner_uv[i].y, texture_coord), packed_tint });
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
	a face is lit by the cell it looks into. packed as daylight in bits 0-3 and
	block light in bits 4-7, with bit 8 for a block that glows itself, so one
	number decides both the vertex values and whether two faces may merge
*/
uint16_t Chunk::light_key(block_type type, block_face face, int x, int y, int z) const {
	if (is_emissive(type)) return 0x100;
	vec3 n = face_normal(face);
	int fx = x + (int)n.x, fy = y + (int)n.y, fz = z + (int)n.z;
	uint16_t glow = 0;
	if (has_block_light && fy >= 0 && fy < height) glow = block_light[block_index(fx, fy, fz)];
	return (uint16_t)(daylight_level(get_height(fx, fz), fy) | (glow << 4));
}



/*
	emits one quad standing in for run_u x run_v block faces.

	the quad is built by stretching the unit face from cw_face_map rather than
	from hand-written corner tables, so winding, normal and texture orientation
	are the same as a single face by construction. base_block is the block at the
	quad's texture origin, and the uv runs 0..run across the quad, which the
	fragment shader wraps back into one atlas cell.
*/
void Chunk::add_merged_quad(block_face face, block_type type, ivec3 base_block, int run_u, int run_v, uint16_t light) {
	const mesh_tables& tab = tables();
	if (!tab.textured[type]) return;
	vec2 texture_coord = tab.tile[type][face];
	uint32_t packed_tint = pack_tint(tint_for(type, face, base_block.x, base_block.z));

	const vec3* unit = tab.corner[face];
	//the face's own texture axes, read off the unit quad: corner 0 -> 1 is +u,
	//corner 3 -> 0 is +v
	vec3 u_axis = unit[1] - unit[0];
	vec3 v_axis = unit[0] - unit[3];
	vec3 normal = face_normal(face);

	vec3 base_center = world_position + vec3(base_block.x - 1, base_block.y, base_block.z - 1);

	//corner order matches cw_face_map: left-top, right-top, right-bottom, left-bottom
	const ivec2 corner_uv[4] = { ivec2(0, 1), ivec2(1, 1), ivec2(1, 0), ivec2(0, 0) };
	for (int i = 0; i < 4; ++i) {
		vec3 position = base_center + unit[i]
			+ u_axis * (corner_uv[i].x * (float)(run_u - 1))
			+ v_axis * (corner_uv[i].y * (float)(run_v - 1));
		opaque_vertices.push_back({ position,
			pack_block_vertex(normal, light, corner_uv[i].x * run_u, corner_uv[i].y * run_v, texture_coord), packed_tint });
	}
	update_face_indices(false, false);
}

/*
	greedy meshing for the opaque pass: for each of the six directions, sweep the
	chunk slice by slice, mark which faces need drawing, and merge neighbouring
	faces of the same block type into the largest rectangles that fit. cuts the
	quad count by roughly half on typical terrain compared with one quad per face.
*/
void Chunk::build_opaque_mesh(int y_lo, int y_hi) {
	const mesh_tables& tab = tables();
	const block_face faces[6] = { Front, Back, Left, Right, Top, Bottom };
	const int step_x = height * length, step_y = length;

	//reused across calls: a chunk build runs this 42 times
	thread_local vector<block_type> mask;
	thread_local vector<biome_id> biome_key;
	thread_local vector<uint16_t> light_keys;

	for (block_face face : faces) {
		//one axis is swept, the other two span the mask. a runs along the mask's
		//first axis, b along its second; the steps are how far each moves an index into blocks
		int slice_lo, slice_hi, a_lo, a_hi, b_lo, b_hi;
		int slice_step, a_step, b_step;
		if (face == Front || face == Back) {
			slice_lo = 1; slice_hi = length - 1; a_lo = 1; a_hi = width - 1; b_lo = y_lo; b_hi = y_hi;
			slice_step = 1; a_step = step_x; b_step = step_y;
		}
		else if (face == Left || face == Right) {
			slice_lo = 1; slice_hi = width - 1; a_lo = 1; a_hi = length - 1; b_lo = y_lo; b_hi = y_hi;
			slice_step = step_x; a_step = 1; b_step = step_y;
		}
		else {
			slice_lo = y_lo; slice_hi = y_hi; a_lo = 1; a_hi = width - 1; b_lo = 1; b_hi = length - 1;
			slice_step = step_y; a_step = step_x; b_step = 1;
		}
		int a_count = a_hi - a_lo, b_count = b_hi - b_lo;
		if (a_count <= 0 || b_count <= 0) continue;

		//which world direction each mask axis advances in, so the quad's texture
		//origin can be placed at the right end of the rectangle
		vec3 a_dir = (face == Left || face == Right) ? vec3(0, 0, 1) : vec3(1, 0, 0);
		vec3 b_dir = (face == Top || face == Bottom) ? vec3(0, 0, 1) : vec3(0, 1, 0);
		const vec3* unit = tab.corner[face];
		bool u_flipped = dot(unit[1] - unit[0], a_dir) < 0.0f;
		bool v_flipped = dot(unit[0] - unit[3], b_dir) < 0.0f;

		vec3 normal = face_normal(face);
		int neighbour_step = (int)normal.x * step_x + (int)normal.y * step_y + (int)normal.z;
		bool vertical = face == Top || face == Bottom;

		size_t cells = static_cast<size_t>(a_count) * b_count;
		mask.resize(cells);
		biome_key.resize(cells);
		light_keys.resize(cells);

		for (int slice = slice_lo; slice < slice_hi; ++slice) {
			auto to_block = [&](int a, int b) {
				if (face == Front || face == Back) return ivec3(a_lo + a, b_lo + b, slice);
				if (face == Left || face == Right) return ivec3(slice, b_lo + b, a_lo + a);
				return ivec3(a_lo + a, slice, b_lo + b);
			};

			//only vertical faces can look out of the column
			bool neighbour_inside = !vertical || (slice + (int)normal.y >= 0 && slice + (int)normal.y < height);
			for (int a = 0; a < a_count; ++a) {
				size_t row = static_cast<size_t>(slice) * slice_step + static_cast<size_t>(a_lo + a) * a_step;
				for (int b = 0; b < b_count; ++b) {
					size_t index = row + static_cast<size_t>(b_lo + b) * b_step;
					block_type t = blocks[index].type;
					size_t here = static_cast<size_t>(a) * b_count + b;
					bool visible = tab.opaque[t] && neighbour_inside && tab.exposes[blocks[index + neighbour_step].type];
					mask[here] = visible ? t : none;
					if (!visible) continue;

					//a tinted face changes colour with the biome and a face's light with its depth,
					//so two faces may only merge where both agree
					ivec3 p = to_block(a, b);
					biome_key[here] = is_tinted_face(t, face) ? get_biome(p.x, p.z) : biome_id::plains;
					light_keys[here] = light_key(t, face, p.x, p.y, p.z);
				}
			}

			for (int a = 0; a < a_count; ++a) {
				for (int b = 0; b < b_count; ) {
					size_t here = static_cast<size_t>(a) * b_count + b;
					block_type t = mask[here];
					if (t == none) { ++b; continue; }
					biome_id key = biome_key[here];
					uint16_t light = light_keys[here];

					//extend along b first, then widen along a while whole rows match
					int run_b = 1;
					while (b + run_b < b_count) {
						size_t n = static_cast<size_t>(a) * b_count + b + run_b;
						if (mask[n] != t || biome_key[n] != key || light_keys[n] != light) break;
						++run_b;
					}

					int run_a = 1;
					bool can_widen = true;
					while (a + run_a < a_count && can_widen) {
						for (int k = 0; k < run_b; ++k) {
							size_t n = static_cast<size_t>(a + run_a) * b_count + b + k;
							if (mask[n] != t || biome_key[n] != key || light_keys[n] != light) { can_widen = false; break; }
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
					add_merged_quad(face, t, base, run_a, run_b, light);

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

	//neighbours are only read here, never written: every structure is placed before meshing starts
	LightNeighbour around[9];
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dz = -1; dz <= 1; ++dz) {
			Chunk* c = (dx == 0 && dz == 0) ? this : cm.get_chunk(id + ivec2(dx, dz));
			if (c != nullptr && c->has_generated) around[(dx + 1) * 3 + (dz + 1)] = { &c->blocks, &c->emitters };
		}
	}
	has_block_light = compute_block_light(around, width, height, length, block_light);
	needs_remesh = false;

	opaque_ranges.assign(section_count, {});
	transp_ranges.assign(section_count, {});
	water_ranges.assign(section_count, {});
	foliage_ranges.assign(section_count, {});

	//built one section at a time so each section's geometry is a contiguous
	//slice of every buffer, which is what lets hidden sections be skipped
	int top = std::min(max_occupied_y + 1, height);
	for (int section = 0; section < section_count; ++section) {
		int y_lo = section * section_size;
		int y_hi = std::min(y_lo + section_size, top);
		GLuint opaque_start = (GLuint)opaque_indices.size();
		GLuint transp_start = (GLuint)transp_indices.size();
		GLuint water_start = (GLuint)water_indices.size();
		GLuint foliage_start = (GLuint)foliage_indices.size();

		if (y_lo < y_hi) {
			build_opaque_mesh(y_lo, y_hi);
			build_block_faces(y_lo, y_hi);
		}
		update_nonblock_structure_vertices_and_indices(y_lo, y_lo + section_size);

		opaque_ranges[section] = { opaque_start, (GLsizei)(opaque_indices.size() - opaque_start) };
		transp_ranges[section] = { transp_start, (GLsizei)(transp_indices.size() - transp_start) };
		water_ranges[section] = { water_start, (GLsizei)(water_indices.size() - water_start) };
		foliage_ranges[section] = { foliage_start, (GLsizei)(foliage_indices.size() - foliage_start) };

		section_connectivity[section] = y_lo > max_occupied_y
			? all_links
			: compute_section_links(blocks, width, height, length, y_lo);
	}
	vector<unsigned char>().swap(block_light);
	has_block_light = false;
	mesh_ready = true;
}

/*
	faces of the blocks the greedy pass leaves out - transparent blocks, water and
	foliage - for the rows y_lo..y_hi
*/
void Chunk::build_block_faces(int y_lo, int y_hi) {
	const mesh_tables& tab = tables();
	//check x and z from 1 to 16 (boundaries at 0 and 17)
	for (int x = 1; x < width - 1; ++x) {
		for (int z = 1; z < length - 1; ++z) {
			for (int y = y_lo; y < y_hi; ++y) {
				const Block &current = blocks[block_index(x, y, z)];
				//air, or solid and already handled by the greedy pass
				if (current.type == none || tab.opaque[current.type]) continue;

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
}

Chunk::MeshStats Chunk::mesh_stats() const {
	MeshStats stats;
	stats.vertices = uploaded_vertices;
	stats.indices = opaque_count + transp_count + water_count + foliage_count;
	stats.gpu_bytes = uploaded_bytes;
	stats.cpu_bytes = (opaque_vertices.capacity() + transp_vertices.capacity() + water_vertices.capacity()) * sizeof(block_vertex)
		+ foliage_vertices.capacity() * sizeof(foliage_vertex)
		+ (opaque_indices.capacity() + transp_indices.capacity() + water_indices.capacity() + foliage_indices.capacity()) * sizeof(GLuint)
		+ blocks.capacity() * sizeof(Block) + block_light.capacity();
	for (const auto& e : nonblock_structure_vertices) stats.cpu_bytes += e.second.capacity() * sizeof(foliage_vertex);
	return stats;
}
