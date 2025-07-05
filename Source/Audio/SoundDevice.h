#pragma once
#include <AL\alc.h>

/// <summary>
/// Singleton class that initializes our sound hardware device and allows
/// us to change our listener values.
/// </summary>

#define SD_INIT SoundDevice::init();
#define LISTENER SoundDevice::get_instance()

class SoundDevice
{
public:
	static SoundDevice* get_instance();
	static void init();

	void get_location(float& x, float& y, float& z);
	void get_orientation(float& ori);
	float get_gain();

	void set_attenuation(int key);
	void set_location(const float& x, const float& y, const float& z);
	void set_orientation(
		const float& atx, const float& aty, const float& atz,
		const float& upx, const float& upy, const float& upz);
	void set_gain(const float& val);

private:
	SoundDevice();
	~SoundDevice();

	ALCdevice* p_ALCDevice;
	ALCcontext* p_ALCContext;
};
