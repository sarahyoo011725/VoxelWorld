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

	void play_block_sound_effect(block_type type);
	void play_block_walked(block_type type);
	void play_random_music();
}

namespace music {
	static MusicBuffer moog_city2 = MusicBuffer("moog_city2.wav");

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
	const static ALuint grass = SE_LOAD("place_and_break_grass.wav");
	const static ALuint dirt = SE_LOAD("place_and_break_dirt.wav");
	const static ALuint sand = SE_LOAD("place_and_break_sand.wav");
	const static ALuint stone = SE_LOAD("place_and_break_stone.wav");
	const static ALuint water = SE_LOAD("place_water.wav");
	const static ALuint wood = SE_LOAD("place_and_break_wood.wav");

	const static ALuint walk_grass = SE_LOAD("walk_grass.wav");
}