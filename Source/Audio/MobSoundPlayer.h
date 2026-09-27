#pragma once
#include <AL/al.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace glm;

/*
	plays mob sounds from where the mob stands. the listener never moves in this
	engine (block sounds play at it), so each source is placed relative to it in
	camera space instead: right, up and back from the ears, as OpenAL expects
*/
class MobSoundPlayer {
public:
	MobSoundPlayer();
	~MobSoundPlayer();
	//view is the camera's world to view matrix
	void play(const string& file, vec3 world_position, const mat4& view, float pitch = 1.0f);

	float gain = 0.8f;
	float full_volume_distance = 4.0f;
	float silent_distance = 32.0f;

private:
	ALuint buffer_for(const string& file);

	vector<ALuint> sources;
	unordered_map<string, ALuint> buffers;
	size_t next_source = 0;
};
