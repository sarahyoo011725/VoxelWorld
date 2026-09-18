#include "Terrain.h"
#include <algorithm>
#include <chrono>

Terrain::Terrain(vec3& cam_pos) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()), sm(ShaderManager::get_instance()) {
	player_pos = &cam_pos;
}

/*
	streams chunks around the player up to the render distance, building/rebuilding
	as needed, and populates visible_chunks. must be called once per frame, before
	draw_shadow_casters()/draw() - both consume visible_chunks and draw() clears it.
*/
void Terrain::update_chunks() {
	//player's pos in chunk space
	origin = {
		floor(player_pos->x / chunk_size),
		floor(player_pos->z / chunk_size),
	};

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

	//rebuilds come from the player breaking/placing a block, so they run
	//immediately - deferring them would show a stale chunk for a frame
	for (Chunk* c : visible_chunks) {
		if (c->has_built && c->should_rebuild) {
			c->rebuild_chunk();
		}
	}

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

	vector<Chunk*> pending;
	for (Chunk* c : visible_chunks) {
		if (!c->has_built) pending.push_back(c);
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

	/*
		structures run first for the whole batch, and serially: spawn_tree writes
		through ChunkManager into neighbouring chunks, so it is neither
		thread-safe nor confined to the chunk being built. doing the whole batch
		up front also means a tree crossing into another chunk of the same batch
		lands before that chunk is meshed, instead of forcing it to rebuild.
		the budget is checked after each one, so at least one always progresses.
	*/
	vector<Chunk*> batch;
	for (Chunk* c : pending) {
		spawn_structures(c);
		//after structures, never before: a regenerated tree would otherwise
		//overwrite a block the player had already broken
		restore_player_edits(c);
		batch.push_back(c);

		float elapsed_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - start).count();
		if (elapsed_ms >= build_budget_ms) break;
	}

	//meshing only reads its own chunk's blocks and writes its own vertex
	//buffers, so it parallelises now that every structure write is done
	pool.parallel_for((int)batch.size(), [&batch](int i) {
		batch[i]->build_mesh();
	});

	//uploads touch GL, so they stay on this thread
	for (Chunk* c : batch) {
		c->upload_mesh();
	}

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

	for (Chunk* c : visible_chunks) {
		if (!is_chunk_visible(c, light_frustum)) continue;
		c->draw_opaque_depth();
		c->draw_foliage_depth();
	}
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
	draws the chunks streamed by update_chunks() and clears visible_chunks.
	chunks outside the camera frustum are skipped, which also keeps them out of
	the transparency sort below
*/
void Terrain::draw(const mat4& view_projection) {
	Frustum camera_frustum;
	camera_frustum.from_matrix(view_projection);

	vector<Chunk*> drawn;
	drawn.reserve(visible_chunks.size());
	for (Chunk* c : visible_chunks) {
		if (is_chunk_visible(c, camera_frustum)) drawn.push_back(c);
	}
	stats.chunks_drawn = (int)drawn.size();

	//one shader bind for the whole opaque pass instead of one per chunk
	sm.default_shader.activate();
	for (Chunk* c : drawn) {
		c->draw_opaque_blocks();
	}

	//alpha blending needs back-to-front order, or a nearer chunk's transparent
	//faces can wrongly show through a farther chunk's water/leaves
	sort(drawn.begin(), drawn.end(), [this](Chunk* a, Chunk* b) {
		vec3 a_center = a->world_position + vec3(chunk_size / 2.0f, 0.0f, chunk_size / 2.0f);
		vec3 b_center = b->world_position + vec3(chunk_size / 2.0f, 0.0f, chunk_size / 2.0f);
		float a_dist = distance(vec2(a_center.x, a_center.z), vec2(player_pos->x, player_pos->z));
		float b_dist = distance(vec2(b_center.x, b_center.z), vec2(player_pos->x, player_pos->z));
		return a_dist > b_dist;
	});

	for (Chunk* c : drawn) {
		c->draw_transparent_blocks();
		c->draw_foliage();
		c->draw_water();
	}

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
}