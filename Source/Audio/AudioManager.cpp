#include "AudioManager.h"

/*
	plays a placing & breaking block sound effect based on the block type
*/
void audio::play_block_sound_effect(block_type type) {
	ALuint sound = -1;
	switch (type) {
	case grass:
	case flower_red:
	case flower_yellow:
	case flower_purple:
	case flower_white:
	case leaf:
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
	case bedrock:
	case mossy_stone:
	case coal_ore:
	case iron_ore:
	case gold_ore:
	case redstone_ore:
	case diamond_ore:
	case glowstone:
		sound = sound_effect::stone;
		break;
	case sand:
	case gravel:
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

void audio::update_footsteps(block_type ground) {
	ALuint sound = 0;
	switch (ground) {
	case none:
	case water:
	case lava:
		break;
	default:
		sound = sound_effect::walk_grass; //the only footstep sound so far
		break;
	}
	if (sound == 0) {
		if (effect_player2.is_playing()) effect_player2.stop();
		return;
	}
	//play() restarts a sound that is already playing, so only start one that isn't
	if (!effect_player2.is_playing()) {
		effect_player2.set_looping(true);
		effect_player2.play(sound);
	}
}

namespace {
	//loaded on first use, once OpenAL is up, rather than at static initialisation in every file that includes the header
	struct PlayerSounds {
		ALuint attack = SE_LOAD("Resources/Sound Effects/attack_default.ogg");
		ALuint critical = SE_LOAD("Resources/Sound Effects/critical_attack.ogg");
		ALuint heavy_splash = SE_LOAD("Resources/Sound Effects/water_heavy_splash.ogg");
		ALuint water_out = SE_LOAD("Resources/Sound Effects/water_out.ogg");
		ALuint underwater = SE_LOAD("Resources/Sound Effects/underwater_ambience.ogg");
		SoundEffectsPlayer combat;
		SoundEffectsPlayer splash;
		SoundEffectsPlayer ambience;
		bool feet_were_in_water = false;
		bool eye_was_in_water = false;
	};

	PlayerSounds& player_sounds() {
		static PlayerSounds sounds;
		return sounds;
	}

	const float heavy_splash_speed = 13.0f; //blocks per second into the water, about a three-block drop
}

void audio::play_attack(bool killed) {
	PlayerSounds& s = player_sounds();
	s.combat.play(killed ? s.critical : s.attack);
}

void audio::update_water(bool feet_in_water, bool eye_in_water, float vertical_speed) {
	PlayerSounds& s = player_sounds();
	if (feet_in_water && !s.feet_were_in_water && -vertical_speed >= heavy_splash_speed) {
		s.splash.play(s.heavy_splash);
	}
	if (eye_in_water && !s.eye_was_in_water) {
		s.ambience.set_looping(true);
		s.ambience.play(s.underwater);
	}
	else if (!eye_in_water && s.eye_was_in_water) {
		s.ambience.stop();
		s.splash.play(s.water_out);
	}
	s.feet_were_in_water = feet_in_water;
	s.eye_was_in_water = eye_in_water;
}

void audio::play_random_music() {
	if (audio::current_music == nullptr || !audio::current_music->is_playing()) {
		int n = rand() % music::musics_list.size();
		audio::current_music = music::musics_list[n];
		audio::current_music->play();
	}
	audio::current_music->update_buffer_stream();
}