#include "SoundEffectsPlayer.h"
#include <iostream>
#include "OpenALErrorCheck.h"

/*
	creates a sound player by generating a source
*/
SoundEffectsPlayer::SoundEffectsPlayer()
{
	alGenSources(1, &p_Source);
	alSourcei(p_Source, AL_BUFFER, p_Buffer);
	AL_CheckAndThrow();
}

//destroys a sound effect player instance
SoundEffectsPlayer::~SoundEffectsPlayer()
{
	alDeleteSources(1, &p_Source);
}

//plays a sound effect (only one allowed). if more than one buffer provided simultaneously, it will crash
void SoundEffectsPlayer::play(const ALuint& buffer_to_play)
{
	ALint state;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
	if (state == AL_PLAYING)
	{
		alSourceStop(p_Source);
		AL_CheckAndThrow();
	}

	if (buffer_to_play != p_Buffer)
	{
		p_Buffer = buffer_to_play;
		alSourcei(p_Source, AL_BUFFER, (ALint)p_Buffer);
		AL_CheckAndThrow();
	}

	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

//stops playing a sound effect
void SoundEffectsPlayer::stop()
{
	alSourceStop(p_Source);
	AL_CheckAndThrow();
}

//pauses a sound effect
void SoundEffectsPlayer::pause()
{
	alSourcePause(p_Source);
	AL_CheckAndThrow();
}

//resumes a sound effect
void SoundEffectsPlayer::resume()
{
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

//sets or chages the sound effect for a sound player
void SoundEffectsPlayer::set_buffer_to_play(const ALuint& buffer_to_play)
{
	if (buffer_to_play != p_Buffer)
	{
		p_Buffer = buffer_to_play;
		alSourcei(p_Source, AL_BUFFER, (ALint)p_Buffer);
		AL_CheckAndThrow();
	}
}

//tells whether to loop a sound effect or not
void SoundEffectsPlayer::set_looping(const bool& loop)
{
	alSourcei(p_Source, AL_LOOPING, (ALint)loop);
	AL_CheckAndThrow();
}

//sets the position of sound source in 3D space
void SoundEffectsPlayer::set_position(const float& x, const float& y, const float& z)
{
	alSource3f(p_Source, AL_POSITION, x, y, z);
	AL_CheckAndThrow();
}

//checks if a sound effect is playing
bool SoundEffectsPlayer::is_playing()
{
	ALint playState;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &playState);
	return (playState == AL_PLAYING);
}