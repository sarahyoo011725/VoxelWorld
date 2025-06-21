#include "AudioManager.h"

void audio::play_block_sound_effect(block_type type) {
	if (type == none) return;
	ALuint sound = -1;
	switch (type) {
	case grass:
	case leaf_transp:
	case leaf_yellow:
	case leaf_red:
	case dirt_grass:
		sound = sound_effect::grass;
		break;
	case dirt:
		sound = sound_effect::dirt;
		break;
	case wood:
		sound = sound_effect::wood;
		break;
	case stone:
		sound = sound_effect::stone;
		break;
	case sand:
		sound = sound_effect::sand;
		break;
	case water:
		sound = sound_effect::water;
		break;
	}

	if (sound != -1) {
		audio::effect_player1.play(sound);
	}
}

void audio::play_block_walked(block_type type) {
	if (type == none) return;
	ALuint sound = -1;
	switch (type) {
	case dirt_grass:
		sound = sound_effect::walk_grass;
		break;
	}
	if (sound != -1) {
		audio::effect_player2.play(sound);
	}
}

void audio::play_random_music() {
	if (audio::current_music == nullptr || !audio::current_music->is_playing()) {
		int n = rand() % music::musics_list.size();
		audio::current_music = music::musics_list[n];
		audio::current_music->play();
	}
	audio::current_music->update_buffer_stream();
}