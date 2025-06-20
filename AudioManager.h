#pragma once
#include "SoundDevice.h"
#include "MusicBuffer.h"
#include "SoundEffectsPlayer.h"
#include "SoundEffectsLibrary.h"
#include "BlockType.h"
#include <iostream>
namespace audio {
	static SoundDevice* sound_device = LISTENER->get_instance();
	static SoundEffectsPlayer effect1 = SoundEffectsPlayer(); //used to play sound effects for block interaction
	static SoundEffectsPlayer effect2 = SoundEffectsPlayer(); //used to play sound effects for block walking

	void play_create_block(block_type type);
	void play_walk_on_block(block_type type);
}

namespace music {
	static MusicBuffer minecraft("minecraft.wav");
}

namespace sound_effect {
	const static ALuint grass_dig1 = SE_LOAD("grass_dig1.wav");
	const static ALuint grass_dig2 = SE_LOAD("grass_dig2.wav");
	const static ALuint grass_dig3 = SE_LOAD("grass_dig3.wav");
	const static ALuint grass_dig4 = SE_LOAD("grass_dig4.wav");
	const static ALuint grass_walking = SE_LOAD("grass_walking.wav");
}