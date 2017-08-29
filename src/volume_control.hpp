#pragma once
#ifndef __VOLUME_CONTROL_HPP__
#define __VOLUME_CONTROL_HPP__

#include "unicode.h"

#include <cmath>
#include <vector>
#include <memory>

class MediaPlayerVolumeControlProvider
{
public:
	enum VOLUME_CHANGE_STATUS { STATUS_ERROR = -1, STATUS_FOUND = 0, STATUS_NOT_FOUND = 1 };

	virtual ~MediaPlayerVolumeControlProvider() { }
	VOLUME_CHANGE_STATUS volume_change(float delta)
	{
		return change_volume(delta);
	}
	VOLUME_CHANGE_STATUS volume_up(float amount)
	{
#if NDEBUG
		return change_volume(amount);
#else
		return change_volume(std::abs(amount));
#endif
	}
	VOLUME_CHANGE_STATUS volume_down(float amount)
	{
#if NDEBUG
		return change_volume(-amount);
#else
		return change_volume(-std::abs(amount));
#endif
	}
private:
	virtual VOLUME_CHANGE_STATUS change_volume(float delta) = 0;
	virtual VOLUME_CHANGE_STATUS set_volume(float volume)
	{
		VOLUME_CHANGE_STATUS ret;
		if ((ret = change_volume(-1.f)) != STATUS_FOUND)
			return ret;
		return change_volume(volume);
	}
	virtual VOLUME_CHANGE_STATUS set_mute(bool mute)
	{
		if (mute)
			return change_volume(-1.f);
		return change_volume(1.f);
	}
};

bool add_volume_control(MediaPlayerVolumeControlProvider*);
bool remove_volume_control(MediaPlayerVolumeControlProvider*);
void delete_volume_controls();

MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS volume_change(float amount);

inline MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS volume_up(float amount)
{
#if NDEBUG
	return volume_change(amount);
#else
	assert(amount >= 0);
	return volume_change(std::abs(amount));
#endif
}
inline MediaPlayerVolumeControlProvider::VOLUME_CHANGE_STATUS volume_down(float amount)
{
#if NDEBUG
	return volume_change(-amount);
#else
	assert(amount >= 0);
	return volume_change(-std::abs(amount));
#endif
}

#endif // __VOLUME_CONTROL_HPP__
