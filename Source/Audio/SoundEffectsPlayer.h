#pragma once
#include <AL\al.h>
/*
	a buffer that is designed to play a sound effect. only one sound effect can be loaded once.
*/
class SoundEffectsPlayer
{
public:
	SoundEffectsPlayer();
	~SoundEffectsPlayer();

	void play(const ALuint& buffer_to_play);
	void stop();
	void pause();
	void resume();
	void set_buffer_to_play(const ALuint& buffer_to_play);
	void set_looping(const bool& loop);
	void set_position(const float& x, const float& y, const float& z);

	bool is_playing();

private:
	ALuint p_Source;
	ALuint p_Buffer = 0;
};
