#pragma once
#include <AL\al.h>
#include <vector>

/// <summary>
/// SoundEffectsLibrary: Singleton class that keeps track of the shorter sounds
/// we've loaded into memory. Use the Play function from an instantiation of the
/// SoundEffectsPlayer on an ID returned from the load function.
/// 
/// load function: Use the SE_LOAD(...) function to add sounds, programmer should keep track of
/// the returned ALuint to use the sound.
/// 
/// unload function: Use SE_UNLOAD(...) remove sounds from memory. 
/// 
/// </summary>

#define SE_LOAD SoundEffectsLibrary::get_instance()->load
#define SE_UNLOAD SoundEffectsLibrary::get_instance()->unload

class SoundEffectsLibrary
{
public:
	static SoundEffectsLibrary* get_instance();

	ALuint load(const char* filename);
	bool unload(const ALuint& bufferId);

private:
	SoundEffectsLibrary();
	~SoundEffectsLibrary();

	std::vector<ALuint> p_SoundEffectBuffers;
};