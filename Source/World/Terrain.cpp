#include "Terrain.h"
#include <algorithm>
#include <chrono>

Terrain::Terrain(vec3& cam_pos) : cm(ChunkManager::get_instance()), sg(StructureGenerator::get_instance()) {
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
	for (int x = origin.x - render_dist; x <= origin.x + render_dist; ++x) {
		for (int z = origin.y - render_dist; z <= origin.y + render_dist; ++z) {
			ivec2 chunk_id = { x, z };
			Chunk* chunk = cm.get_chunk(chunk_id);
			if (!chunk) {
				chunk = cm.create_chunk(chunk_id);

				auto unloaded_c = cm.unloaded_blocks.find(chunk_id);
				if (unloaded_c != cm.unloaded_blocks.end()) {
					for (const block_data& b : unloaded_c->second) {
						chunk->set_block(b.local_coord, b.type);
					}
					cm.unloaded_blocks.erase(chunk->id);
				}
			}
			visible_chunks.push_back(chunk);
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

	//budget is checked after each chunk, so at least one always makes progress
	auto start = chrono::steady_clock::now();
	for (Chunk* c : pending) {
		spawn_structures(c);
		c->build_chunk();
		++stats.chunks_built;

		float elapsed_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - start).count();
		if (elapsed_ms >= build_budget_ms) {
			stats.chunk_build_ms = elapsed_ms;
			return;
		}
	}
	stats.chunk_build_ms = chrono::duration<float, milli>(chrono::steady_clock::now() - start).count();
}

/*
	depth-only draw of shadow-casting geometry (opaque terrain + foliage) for the
	shadow map pass. the shadow shader is activated once by the caller beforehand.
*/
void Terrain::draw_shadow_casters() {
	for (Chunk* c : visible_chunks) {
		c->draw_opaque_depth();
		c->draw_foliage_depth();
	}
}

/*
	draws the chunks streamed by update_chunks() and clears visible_chunks
*/
void Terrain::draw() {
	for (Chunk* c : visible_chunks) {
		c->draw_opaque_blocks();
	}

	//alpha blending needs back-to-front order, or a nearer chunk's transparent
	//faces can wrongly show through a farther chunk's water/leaves
	sort(visible_chunks.begin(), visible_chunks.end(), [this](Chunk* a, Chunk* b) {
		vec3 a_center = a->world_position + vec3(chunk_size / 2.0f, 0.0f, chunk_size / 2.0f);
		vec3 b_center = b->world_position + vec3(chunk_size / 2.0f, 0.0f, chunk_size / 2.0f);
		float a_dist = distance(vec2(a_center.x, a_center.z), vec2(player_pos->x, player_pos->z));
		float b_dist = distance(vec2(b_center.x, b_center.z), vec2(player_pos->x, player_pos->z));
		return a_dist > b_dist;
	});

	for (Chunk* c : visible_chunks) {
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

			WorldRandom rng(get_terrain_generator().config.world_seed, wx, wz);
			for (const structure_rule& rule : sg.terrain_structures) {
				if (rng.next_int(rule.spawn_chance) == 0) {
					rule.spawn(vec3(wx, h + 1, wz));
				}
			}
		}
	}
}