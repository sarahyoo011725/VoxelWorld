#pragma once
#include <AL\al.h>
#include <sndfile.h>

/*
	a buffer playing music, implementing OpenAL
*/
class MusicBuffer
{
public:
	void play();
	void pause();
	void stop();
	void resume();
	void update_buffer_stream();
	ALint get_source();
	bool is_playing();
	void set_gain(const float& val);
	MusicBuffer(const char* filename);
	~MusicBuffer();
private:
	ALuint p_Source; //music source
	static const int BUFFER_SAMPLES = 8192;
	static const int NUM_BUFFERS = 4; //a music buffer can store up to 4 musics
	ALuint p_Buffers[NUM_BUFFERS];
	SNDFILE* p_SndFile;
	SF_INFO p_Sfinfo;
	short* p_Membuf;
	ALenum p_Format;

	MusicBuffer() = delete;
};
