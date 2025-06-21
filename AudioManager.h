#pragma once
#include "SoundDevice.h"
#include "MusicBuffer.h"
#include "SoundEffectsPlayer.h"
#include "SoundEffectsLibrary.h"
#include "BlockType.h"
#include <iostream>
#include <unordered_map>>

using namespace std;

namespace audio {
	static SoundDevice* sound_device = LISTENER->get_instance();
	static SoundEffectsPlayer effect_player1 = SoundEffectsPlayer(); //used to play sound effects for block interaction
	static SoundEffectsPlayer effect_player2 = SoundEffectsPlayer(); //used to play sound effects for block walking
	static MusicBuffer *current_music = nullptr;

	void init();
	void play_block_placed(block_type type);
	void play_block_broken(block_type type); //TODO: broken sound effect
	void play_block_walked(block_type type);
	void play_random_music();
}

namespace music {
	static unordered_map<int, MusicBuffer*> musics_list = {
		{0, new MusicBuffer("minecraft.wav")},
		{1, new MusicBuffer("blind_spots.wav")},
		{2, new MusicBuffer("aria_math.wav")},
		{3, new MusicBuffer("wet_hands.wav")},
		{4, new MusicBuffer("sweden.wav")},
	};
}

namespace sound_effect {
	//make maps into function to load sound effects lazily
	static unordered_map<block_type, ALuint> block_place_sound_effects = {
		{dirt_grass, SE_LOAD("place_grass.wav")},
	};
	static unordered_map<block_type, ALuint> block_break_sound_effects = {
		{dirt_grass, SE_LOAD("break_grass.wav")},
	};
	static unordered_map<block_type, ALuint> block_walk_sound_effects = {
		{dirt_grass, SE_LOAD("walk_grass.wav")},
	};
}