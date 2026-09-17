#pragma once

/*
	per-frame counters for the F3 performance overlay. chunk_build_ms and
	chunks_built cover only work done this frame, so they show whether the
	per-frame build budget is actually being hit; chunks_pending is what is
	still queued behind it.
*/
struct PerfStats {
	float frame_ms = 0.0f;
	float fps = 0.0f;
	float chunk_build_ms = 0.0f;
	int chunks_built = 0;
	int chunks_pending = 0;
	int chunks_visible = 0;
	int chunks_loaded = 0;
	int player_x = 0;
	int player_y = 0;
	int player_z = 0;
};
