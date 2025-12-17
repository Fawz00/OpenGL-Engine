#pragma once

#include <string>
#include <AL/al.h>
#include <AL/alc.h>

#include "Debug.hpp"

#define OpenAL_ErrorCheck(message)\
{\
	ALenum error = alGetError();\
	if (error != AL_NO_ERROR)\
	{\
		Debug::logError("OpenAL error: %s with call for %s", alGetString(error), message.c_str());\
	}\
}

#define alec(FUNCTION_CALL)\
FUNCTION_CALL;\
OpenAL_ErrorCheck(FUNCTION_CALL)

class AudioSystem {
	public:
		static void init();
		static void destroy();

	private:
		const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);

		static ALCdevice* device;
		static ALCcontext* context;
};