#include "AudioManager.h"

void audio::play_create_block(block_type type) {
	switch (type) {
	case dirt:
		int n = rand() % 4;
		if (n == 0) {
			effect1.play(sound_effect::grass_dig1);
		}
		else if (n == 1) {
			effect1.play(sound_effect::grass_dig2);
		}
		else if (n == 2) {
			effect1.play(sound_effect::grass_dig3);
		}
		else {
			effect1.play(sound_effect::grass_dig4);
		}

	}
}

void audio::play_walk_on_block(block_type type) {
	effect2.set_looping(true);
	switch (type) {
	case dirt:
		effect2.play(sound_effect::grass_walking);
	}
}