#pragma once
#include <cstdint>

/*
	deterministic RNG seeded from a world position. structure spawning used
	rand(), which made results depend on the order chunks happened to be
	generated in - the same world seed produced different trees run to run, and
	it could never be called off the main thread. seeding from (world seed, x, z)
	instead lets each position decide its own contents independently of when, in
	what order, or on which thread it is generated.
*/
class WorldRandom {
public:
	WorldRandom(int world_seed, int x, int z, uint32_t salt = 0) {
		uint32_t h = (uint32_t)world_seed;
		h = mix(h ^ ((uint32_t)x * 0x9E3779B9u));
		h = mix(h ^ ((uint32_t)z * 0x85EBCA6Bu));
		h = mix(h ^ salt);
		state = h | 1u; //xorshift stalls at zero
	}

	uint32_t next() {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return state;
	}

	int next_int(int bound) { return (int)(next() % (uint32_t)bound); }

private:
	static uint32_t mix(uint32_t v) {
		v ^= v >> 16; v *= 0x7FEB352Du;
		v ^= v >> 15; v *= 0x846CA68Bu;
		v ^= v >> 16;
		return v;
	}

	uint32_t state;
};
