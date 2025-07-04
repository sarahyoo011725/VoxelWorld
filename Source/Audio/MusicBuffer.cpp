#include "MusicBuffer.h"
#include <cstddef>
#include <AL\alext.h>
#include <malloc.h>
#include "OpenALErrorCheck.h"

/*
	plays a music buffer
*/
void MusicBuffer::play()
{
	ALsizei i;

	// clear any al errors
	alGetError();

	/* Rewind the source position and clear the buffer queue */
	alSourceRewind(p_Source);
	alSourcei(p_Source, AL_BUFFER, 0);

	/* Fill the buffer queue */
	for (i = 0; i < NUM_BUFFERS; i++)
	{
		/* Get some data to give it to the buffer */
		sf_count_t slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
		if (slen < 1) break;

		slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
		alBufferData(p_Buffers[i], p_Format, p_Membuf, (ALsizei)slen, p_Sfinfo.samplerate);
	}
	if (alGetError() != AL_NO_ERROR)
	{
		throw("Error buffering for playback");
	}

	/* Now queue and start playback! */
	alSourceQueueBuffers(p_Source, i, p_Buffers);
	alSourcePlay(p_Source);
	if (alGetError() != AL_NO_ERROR)
	{
		throw("Error starting playback");
	}

}

/*
	pause a music being played
*/
void MusicBuffer::pause()
{
	alSourcePause(p_Source);
	AL_CheckAndThrow();
}

/*
	stops playing a music
*/
void MusicBuffer::stop()
{
	alSourceStop(p_Source);
	AL_CheckAndThrow();
}

/*
	resumes a music that has been paused
*/
void MusicBuffer::resume()
{
	alSourcePlay(p_Source);
	AL_CheckAndThrow();
}

/*
	updates the buffer of the currently playing music.
	this must be called every frame to play the music
*/
void MusicBuffer::update_buffer_stream()
{
	ALint processed, state;

	// clear error 
	alGetError();
	/* Get relevant source info */
	alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
	alGetSourcei(p_Source, AL_BUFFERS_PROCESSED, &processed);
	AL_CheckAndThrow();

	/* Unqueue and handle each processed buffer */
	while (processed > 0)
	{
		ALuint bufid;
		sf_count_t slen;

		alSourceUnqueueBuffers(p_Source, 1, &bufid);
		processed--;

		/* Read the next chunk of data, refill the buffer, and queue it
		 * back on the source */
		slen = sf_readf_short(p_SndFile, p_Membuf, BUFFER_SAMPLES);
		if (slen > 0)
		{
			slen *= p_Sfinfo.channels * (sf_count_t)sizeof(short);
			alBufferData(bufid, p_Format, p_Membuf, (ALsizei)slen,
				p_Sfinfo.samplerate);
			alSourceQueueBuffers(p_Source, 1, &bufid);
		}
		if (alGetError() != AL_NO_ERROR)
		{
			throw("error buffering music data");
		}
	}

	/* Make sure the source hasn't underrun */
	if (state != AL_PLAYING && state != AL_PAUSED)
	{
		ALint queued;

		/* If no buffers are queued, playback is finished */
		alGetSourcei(p_Source, AL_BUFFERS_QUEUED, &queued);
		AL_CheckAndThrow();
		if (queued == 0)
			return;

		alSourcePlay(p_Source);
		AL_CheckAndThrow();
	}
}

/*
	returns the current music buffer source
*/
ALint MusicBuffer::get_source()
{
	return p_Source;
}

/*
	checks if a music is being played
*/
bool MusicBuffer::is_playing()
{
	ALint state;
	alGetSourcei(p_Source, AL_SOURCE_STATE, &state);
	AL_CheckAndThrow();
	return (state == AL_PLAYING);
}

/*
	sets the gain of a music buffer
*/
void MusicBuffer::set_gain(const float& val)
{
	float newval = val;
	if (newval < 0)
		newval = 0;
	alSourcef(p_Source, AL_GAIN, val);
	AL_CheckAndThrow();
}

/*
	generates a music buffer with a music file (only in wav format)
*/
MusicBuffer::MusicBuffer(const char* filename)
{
	alGenSources(1, &p_Source);
	alGenBuffers(NUM_BUFFERS, p_Buffers);

	std::size_t frame_size;

	p_SndFile = sf_open(filename, SFM_READ, &p_Sfinfo);
	if (!p_SndFile)
	{
		throw("could not open provided music file -- check path");
	}

	/* Get the sound format, and figure out the OpenAL format */
	if (p_Sfinfo.channels == 1)
		p_Format = AL_FORMAT_MONO16;
	else if (p_Sfinfo.channels == 2)
		p_Format = AL_FORMAT_STEREO16;
	else if (p_Sfinfo.channels == 3)
	{
		if (sf_command(p_SndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			p_Format = AL_FORMAT_BFORMAT2D_16;
	}
	else if (p_Sfinfo.channels == 4)
	{
		if (sf_command(p_SndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
			p_Format = AL_FORMAT_BFORMAT3D_16;
	}
	if (!p_Format)
	{
		sf_close(p_SndFile);
		p_SndFile = NULL;
		throw("Unsupported channel count from file");
	}

	frame_size = ((size_t)BUFFER_SAMPLES * (size_t)p_Sfinfo.channels) * sizeof(short);
	p_Membuf = static_cast<short*>(malloc(frame_size));
}

/*
	destroys music buffer
*/
MusicBuffer::~MusicBuffer()
{
	alDeleteSources(1, &p_Source);
	if (p_SndFile)
		sf_close(p_SndFile);
	p_SndFile = nullptr;
	free(p_Membuf);
	alDeleteBuffers(NUM_BUFFERS, p_Buffers);
}