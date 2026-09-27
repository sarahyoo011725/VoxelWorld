#include "MobSoundPlayer.h"
#include "SoundEffectsLibrary.h"

MobSoundPlayer::MobSoundPlayer() {
	sources.resize(6);
	alGenSources((ALsizei)sources.size(), sources.data());
	for (ALuint source : sources) {
		alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcef(source, AL_REFERENCE_DISTANCE, full_volume_distance);
		alSourcef(source, AL_MAX_DISTANCE, silent_distance);
	}
}

MobSoundPlayer::~MobSoundPlayer() {
	alDeleteSources((ALsizei)sources.size(), sources.data());
}

ALuint MobSoundPlayer::buffer_for(const string& file) {
	auto found = buffers.find(file);
	if (found != buffers.end()) return found->second;
	ALuint buffer = SE_LOAD(file.c_str());
	buffers[file] = buffer;
	return buffer;
}

void MobSoundPlayer::play(const string& file, vec3 world_position, const mat4& view, float pitch) {
	vec3 relative = vec3(view * vec4(world_position, 1.0f));
	if (length(relative) > silent_distance) return;
	ALuint buffer = buffer_for(file);
	if (buffer == 0) return;

	//round robin: a new call cuts off the oldest one still playing
	ALuint source = sources[next_source];
	next_source = (next_source + 1) % sources.size();
	alSourceStop(source);
	alSourcei(source, AL_BUFFER, (ALint)buffer);
	alSource3f(source, AL_POSITION, relative.x, relative.y, relative.z);
	alSourcef(source, AL_GAIN, gain);
	alSourcef(source, AL_PITCH, pitch);
	alSourcePlay(source);
}
