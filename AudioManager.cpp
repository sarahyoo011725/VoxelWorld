#include "AudioManager.h"

void audio::play_block_placed(block_type type) {
	auto sound_effect = sound_effect::block_place_sound_effects.find(type);
	if (sound_effect != sound_effect::block_place_sound_effects.end()) {
		audio::effect_player1.play(sound_effect->second);
	}
}

void audio::play_block_broken(block_type type) {
	auto sound_effect = sound_effect::block_break_sound_effects.find(type);
	if (sound_effect != sound_effect::block_break_sound_effects.end()) {
		audio::effect_player1.play(sound_effect->second);
	}
}

void audio::play_block_walked(block_type type) {
	auto sound_effect = sound_effect::block_walk_sound_effects.find(type);
	if (sound_effect != sound_effect::block_walk_sound_effects.end()) {
		if (!effect_player2.is_playing()) {
			audio::effect_player2.play(sound_effect->second);
		}
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