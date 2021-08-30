//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piSoundEngine.h"
#include "../libBasics/piLog.h"

namespace ImmCore {

class piSoundEngineBackend
{
public:
	enum class API : int
	{
		Null = 0,
		DirectSound = 1,
		DirectSoundOVR = 2
	};

    struct Configuration
    {
        int mMaxSoundVoices = 0;
        int mSampleRate = 48000;
        int mBufferSize = 512;
        bool mLowLatency = false;
    };

    virtual bool           Init(void *hwnd, int deviceID, const Configuration* config) = 0;
    virtual void           Deinit(void) = 0;
    virtual int            GetNumDevices( void ) = 0;
    virtual const wchar_t *GetDeviceName(int id) const = 0;
	virtual int            GetDeviceFromGUID(void *deviceGUID) = 0;
    virtual int            GetDeviceFromName(const wchar_t *name) = 0;
    virtual bool           ResizeMixBuffers(int const& mixSamples, int const& spatialSamples) = 0;
    virtual void           Tick(void) = 0;
	virtual piSoundEngine *GetEngine( void ) = 0;

protected:
	piSoundEngineBackend() {}
	friend piSoundEngineBackend * piCreateSoundEngineBackend(piSoundEngineBackend::API api, piLog *log);
	friend void                   piDestroySoundEngineBackend(piSoundEngineBackend *me);
	virtual ~piSoundEngineBackend() {}
	virtual bool doInit(piLog *log) = 0;
	virtual void doDeinit(void) = 0;

};

} // namespace ImmCore
