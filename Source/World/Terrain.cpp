#include "Terrain.h"
#include "Onsen.h"
#include <algorithm>
#include <chrono>

Terrain::Terrain(vec3& cam_pos) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()), sm(ShaderManager::get_instance()) {
	player_pos = &cam_pos;
}

/*
	streams chunks around the player up to the render distance, building/rebuilding
	as needed, and populates visible_chunks. must be called once per frame, before
	draw_shadow_casters()/draw(), which both consume visible_chunks.
*/
void Terrain::update_chunks() {
	//player's pos in chunk space
	origin = {
		floor(player_pos->x / chunk_size),
		floor(player_pos->z / chunk_size),
	};

	//rebuilt from scratch every call: a chunk listed twice would be meshed by two threads at once
	visible_chunks.clear();

	//render chunks around player's pos up to the render dist
	vector<Chunk*> new_chunks;
	for (int x = origin.x - render_dist; x <= origin.x + render_dist; ++x) {
		for (int z = origin.y - render_dist; z <= origin.y + render_dist; ++z) {
			ivec2 chunk_id = { x, z };
			Chunk* chunk = cm.get_chunk(chunk_id);
			if (!chunk) {
				chunk = cm.create_chunk(chunk_id);
				new_chunks.push_back(chunk);
			}
			visible_chunks.push_back(chunk);
		}
	}

	//each chunk's blocks are a pure function of world position - including its
	//one-block border - so generation needs no neighbour and no shared state,
	//which makes this the one phase that parallelises cleanly
	auto gen_start = chrono::steady_clock::now();
	if (!new_chunks.empty()) {
		pool.parallel_for((int)new_chunks.size(), [&new_chunks](int i) {
			new_chunks[i]->generate_terrain();
		});
	}
	stats.generate_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - gen_start).count();
	stats.chunks_generated = (int)new_chunks.size();

	//block edits made while the chunk didn't exist yet. must come after
	//generation, since set_block indexes into the block array it allocates
	for (Chunk* chunk : new_chunks) {
		auto unloaded_c = cm.unloaded_blocks.find(chunk->id);
		if (unloaded_c != cm.unloaded_blocks.end()) {
			for (const block_data& b : unloaded_c->second) {
				chunk->set_block(b.local_coord, b.type);
			}
			cm.unloaded_blocks.erase(chunk->id);
		}
	}

	//a new neighbour can open or block a path for light that reaches into chunks already built
	for (Chunk* chunk : new_chunks) {
		cm.relight_around(chunk->id);
	}

	/*
		structures go in as soon as their chunks exist, all of them at once and
		serially: spawn_tree writes through ChunkManager into neighbouring chunks,
		so it is neither thread-safe nor confined to one chunk. placing them all
		before any is meshed means a tree reaching into another new chunk is there
		when that chunk is built, and an already-built neighbour is flagged once
		rather than once for every new chunk next to it.
	*/
	auto structures_start = chrono::steady_clock::now();
	for (Chunk* chunk : new_chunks) {
		spawn_structures(chunk);
		//after structures, never before: a regenerated tree would otherwise
		//overwrite a block the player had already broken
		restore_player_edits(chunk);
	}
	stats.structures_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - structures_start).count();

	//rebuilds come from the player breaking/placing a block, so they run
	//immediately - deferring them would show a stale chunk for a frame
	auto rebuild_start = chrono::steady_clock::now();
	stats.chunks_rebuilt = 0;
	for (Chunk* c : visible_chunks) {
		if (c->has_built && c->should_rebuild) {
			c->rebuild_chunk();
			stats.chunks_rebuilt++;
		}
	}
	stats.rebuild_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - rebuild_start).count();

	build_pending_chunks();
	unload_distant_chunks();
}

/*
	drops chunks that have drifted well outside the render distance. nothing ever
	removed them before, so walking in one direction grew the chunk map without
	bound. erasing runs the Chunk destructor, which frees its GL buffers too.

	safe only because keep_dist exceeds render_dist: visible_chunks holds raw
	pointers into this map and is still read by draw() later in the frame, so
	nothing within render distance may be erased here.
*/
void Terrain::unload_distant_chunks() {
	for (auto it = cm.chunks.begin(); it != cm.chunks.end(); ) {
		ivec2 id = it->first;
		if (abs(id.x - origin.x) > keep_dist || abs(id.y - origin.y) > keep_dist) {
			it = cm.chunks.erase(it);
		}
		else {
			++it;
		}
	}
	stats.chunks_loaded = (int)cm.chunks.size();
}

/*
	replays the player's changes over freshly generated terrain. a chunk that was
	unloaded and streamed back in is rebuilt from noise alone, so without this it
	would silently revert to its original state.
*/
void Terrain::restore_player_edits(Chunk* chunk) {
	auto edits = cm.player_edits.find(chunk->id);
	if (edits == cm.player_edits.end()) return;
	//a restored edit may add or remove something glowing that neighbours were lit without
	cm.relight_around(chunk->id);

	for (const auto& edit : edits->second) {
		ivec3 local_coord = edit.first;
		block_type type = edit.second;

		//grass and the like are geometry, not just a block type, so they have to
		//go back through the structure generator to get their vertices rebuilt
		if (is_nonblock(type)) {
			vec3 world_coord = vec3(
				chunk->id.x * chunk_size + local_coord.x - 1,
				local_coord.y,
				chunk->id.y * chunk_size + local_coord.z - 1
			);
			sg.spawn_nonblock_structure(type, world_coord);
		}
		else {
			chunk->set_block(local_coord, type);
		}
	}
}

/*
	builds newly streamed chunks under a per-frame time budget. crossing a chunk
	boundary queues a whole row of them at once (2 * render_dist + 1), which is
	far more than one frame can absorb, so they are spread over several frames
	instead. an unbuilt chunk simply draws nothing until its turn comes.
*/
void Terrain::build_pending_chunks() {
	stats.chunks_built = 0;
	stats.chunk_build_ms = 0.0f;
	stats.chunks_visible = (int)visible_chunks.size();
	stats.chunks_loaded = (int)cm.chunks.size();

	//relighting shares the budget and the worker threads with new chunks
	vector<Chunk*> pending;
	for (Chunk* c : visible_chunks) {
		if (!c->has_built || c->needs_remesh) pending.push_back(c);
	}
	stats.chunks_pending = (int)pending.size();
	if (pending.empty()) return;

	//nearest first, so the world fills in outward from the player
	auto dist_sq = [this](Chunk* c) {
		float dx = c->world_position.x - player_pos->x;
		float dz = c->world_position.z - player_pos->z;
		return dx * dx + dz * dz;
	};
	sort(pending.begin(), pending.end(), [&](Chunk* a, Chunk* b) {
		return dist_sq(a) < dist_sq(b);
	});

	auto start = chrono::steady_clock::now();

	//meshing runs one chunk per thread at a time while uploads queue up on this one,
	//so the batch grows until that predicted cost would overrun the budget.
	//at least one chunk always goes, so the queue keeps moving on a slow machine
	const int threads = (int)std::max(1u, thread::hardware_concurrency());
	auto predicted_ms = [&](int n) {
		return ((n + threads - 1) / threads) * mesh_cost_ms + n * upload_cost_ms;
	};
	vector<Chunk*> batch;
	for (Chunk* c : pending) {
		if (!batch.empty() && predicted_ms((int)batch.size() + 1) > build_budget_ms) break;
		batch.push_back(c);
	}

	//meshing only reads its own chunk's blocks and writes its own vertex
	//buffers, so it parallelises now that every structure write is done
	pool.parallel_for((int)batch.size(), [&batch](int i) {
		batch[i]->build_mesh();
	});
	auto upload_start = chrono::steady_clock::now();

	//uploads touch GL, so they stay on this thread
	for (Chunk* c : batch) {
		c->upload_mesh();
	}
	auto end = chrono::steady_clock::now();

	int rounds = ((int)batch.size() + threads - 1) / threads;
	float mesh_ms = chrono::duration<float, milli>(upload_start - start).count() / rounds;
	float upload_ms = chrono::duration<float, milli>(end - upload_start).count() / batch.size();
	mesh_cost_ms = mesh_cost_ms * 0.7f + mesh_ms * 0.3f;
	upload_cost_ms = upload_cost_ms * 0.7f + upload_ms * 0.3f;

	stats.chunks_built = (int)batch.size();
	stats.chunk_build_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - start).count();
}

/*
	depth-only draw of shadow-casting geometry (opaque terrain + foliage) for the
	shadow map pass. the shadow shader is activated once by the caller beforehand.
	culled against the light's frustum rather than the camera's - geometry behind
	the player can still cast a shadow into view, so culling it by what the camera
	sees would make shadows pop in and out.
*/
void Terrain::draw_shadow_casters(const mat4& light_space_matrix) {
	Frustum light_frustum;
	light_frustum.from_matrix(light_space_matrix);

	//sections, not whole columns: the light's view is a long slanted box, and a 112-block column
	//clips it far more often than the 16-block sections that actually hold geometry inside it
	vector<Chunk*> casters;
	for (Chunk* c : visible_chunks) {
		if (!is_chunk_visible(c, light_frustum)) continue;
		c->shadow_sections = 0;
		for (int s = 0; s < c->section_count; ++s) {
			bool buried = occlusion_culling && !c->has_cave_opening
				&& (s + 1) * section_size <= c->lowest_surface_y - shadow_bury_depth;
			vec3 lo = c->world_position + vec3(-1.0f, s * section_size - 1.0f, -1.0f);
			vec3 hi = lo + vec3(chunk_size + 2.0f, section_size + 2.0f, chunk_size + 2.0f);
			if (!buried && light_frustum.intersects_aabb(lo, hi)) c->shadow_sections |= 1u << s;
		}
		if (c->shadow_sections) casters.push_back(c);
	}

	sm.shadow_shader.set_uniform_1f("sway_scale", 0.0f);
	for (Chunk* c : casters) c->draw_opaque_depth();
	sm.shadow_shader.set_uniform_1f("sway_scale", 1.0f);
	for (Chunk* c : casters) c->draw_foliage_depth();
}

/*
	tests a chunk's bounding box against a frustum. the box is deliberately a
	little larger than the chunk so that block faces and swaying foliage on the
	boundary cannot be clipped away early
*/
bool Terrain::is_chunk_visible(Chunk* chunk, const Frustum& frustum) const {
	vec3 min_corner = chunk->world_position + vec3(-1.0f, -1.0f, -1.0f);
	vec3 max_corner = chunk->world_position + vec3(chunk_size + 1.0f, chunk->height + 1.0f, chunk_size + 1.0f);
	return frustum.intersects_aabb(min_corner, max_corner);
}

/*
	marks which sections of each chunk the camera could possibly see. a chunk that
	has not been meshed yet is treated as open so it cannot hide what lies behind it.
*/
void Terrain::update_section_visibility(const Frustum& frustum) {
	auto start = chrono::steady_clock::now();
	stats.occlusion_culling = occlusion_culling;
	stats.sections_drawn = 0;
	stats.sections_total = 0;

	if (!occlusion_culling || visible_chunks.empty()) {
		for (Chunk* c : visible_chunks) {
			c->visible_sections = is_chunk_visible(c, frustum) ? ~0u : 0u;
			stats.sections_total += c->section_count;
			if (c->visible_sections) stats.sections_drawn += c->section_count;
		}
		stats.visibility_ms = 0.0f;
		return;
	}

	VisibilityGrid& grid = visibility_grid;
	grid.span = render_dist * 2 + 1;
	grid.sections = visible_chunks.front()->section_count;
	grid.min_chunk = origin - ivec2(render_dist);
	grid.links.assign((size_t)grid.span * grid.span * grid.sections, all_links);
	grid.present.assign((size_t)grid.span * grid.span, 0);

	for (Chunk* c : visible_chunks) {
		ivec2 g = c->id - grid.min_chunk;
		if (g.x < 0 || g.y < 0 || g.x >= grid.span || g.y >= grid.span) continue;
		size_t column = (size_t)g.x * grid.span + g.y;
		grid.present[column] = 1;
		if (c->has_built) {
			std::copy(c->section_connectivity.begin(), c->section_connectivity.end(), grid.links.begin() + column * grid.sections);
		}
	}

	//the hitbox is centred on player_pos, so both the feet and the eyes get a starting section
	vector<ivec3> starts = {
		ivec3(origin.x, (int)floor((player_pos->y + 0.72f) / section_size), origin.y),
		ivec3(origin.x, (int)floor((player_pos->y - 0.9f) / section_size), origin.y),
	};

	auto in_view = [&frustum](ivec3 s) {
		vec3 lo = vec3(s.x * chunk_size - 1.0f, s.y * section_size - 1.0f, s.z * chunk_size - 1.0f);
		vec3 hi = lo + vec3(chunk_size + 2.0f, section_size + 2.0f, chunk_size + 2.0f);
		return frustum.intersects_aabb(lo, hi);
	};
	find_visible_sections(grid, starts, in_view, visible_masks);

	for (Chunk* c : visible_chunks) {
		ivec2 g = c->id - grid.min_chunk;
		bool inside = g.x >= 0 && g.y >= 0 && g.x < grid.span && g.y < grid.span;
		c->visible_sections = inside ? visible_masks[(size_t)g.x * grid.span + g.y] : 0u;
		stats.sections_total += c->section_count;
		for (uint32_t m = c->visible_sections; m; m &= m - 1) stats.sections_drawn++;
	}
	stats.visibility_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - start).count();
}

/*
	draws the chunks streamed by update_chunks() and clears visible_chunks.
	chunks outside the camera frustum are skipped, which also keeps them out of
	the transparency sort below
*/
void Terrain::draw(const mat4& view_projection) {
	draw_opaque(view_projection);
	draw_translucent();
}

void Terrain::draw_opaque(const mat4& view_projection) {
	Frustum camera_frustum;
	camera_frustum.from_matrix(view_projection);

	update_section_visibility(camera_frustum);

	vector<Chunk*>& drawn = drawn_chunks;
	drawn.clear();
	drawn.reserve(visible_chunks.size());
	for (Chunk* c : visible_chunks) {
		if (c->visible_sections != 0) drawn.push_back(c);
	}
	stats.chunks_drawn = (int)drawn.size();

	//one distance sort serves both passes: nearest first for solid geometry,
	//then walked backwards for the transparent pass
	auto dist_sq = [this](Chunk* c) {
		float dx = c->world_position.x + chunk_size / 2.0f - player_pos->x;
		float dz = c->world_position.z + chunk_size / 2.0f - player_pos->z;
		return dx * dx + dz * dz;
	};
	//one shader bind for the whole opaque pass instead of one per chunk
	sm.default_shader.activate();
	if (!sort_opaque_front_to_back) {
		for (Chunk* c : drawn) c->draw_opaque_blocks();
	}
	sort(drawn.begin(), drawn.end(), [&](Chunk* a, Chunk* b) {
		return dist_sq(a) < dist_sq(b);
	});
	if (sort_opaque_front_to_back) {
		for (Chunk* c : drawn) c->draw_opaque_blocks();
	}
}

void Terrain::draw_translucent() {
	vector<Chunk*>& drawn = drawn_chunks;
	//alpha blending needs back-to-front order, or a nearer chunk's transparent
	//faces can wrongly show through a farther chunk's water/leaves
	for (auto it = drawn.rbegin(); it != drawn.rend(); ++it) {
		(*it)->draw_transparent_blocks();
		(*it)->draw_foliage();
		(*it)->draw_water();
	}

	drawn.clear();
	visible_chunks.clear();
}

/*
	randomly spawns structure on a chunk
*/
void Terrain::spawn_structures(Chunk* chunk) {
	for (int x = 1; x < chunk_size + 1; ++x) {
		for (int z = 1; z < chunk_size + 1; ++z) {
			//convert local coords into world coord
			int wx = chunk->world_position.x + x - 1;
			int wz = chunk->world_position.z + z - 1;
			int h = chunk->get_height(x, z);

			//do not spawn anything in water
			if (h <= water_level) continue;
			//a cave entrance can carve the surface block away
			if (!is_solid(chunk->get_block(ivec3(x, h, z)))) continue;

			//the biome decides how dense each kind of vegetation is here, so a
			//forest fills in and a desert stays bare without the rules knowing
			//anything about biomes
			const BiomeDefinition& biome = biome_of(chunk->get_biome(x, z));
			WorldRandom rng(get_terrain_generator().config.world_seed, wx, wz);
			for (const structure_rule& rule : sg.terrain_structures) {
				int chance = spawn_chance_in(rule, biome);
				if (chance > 0 && rng.next_int(chance) == 0) {
					rule.spawn(vec3(wx, h + 1, wz));
				}
			}
		}
	}
	//after the vegetation, so the terrace is clear of anything that grew there
	onsen::build_in_chunk(chunk);
}