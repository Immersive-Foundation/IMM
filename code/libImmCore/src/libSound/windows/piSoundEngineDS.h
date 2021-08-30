//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once
#include "../piSoundEngineBackend.h"

namespace ImmCore {

class piSoundEngineBackendDS : public piSoundEngineBackend
{
public:
    bool            Init(void *hwnd, int deviceID, const Configuration* config);
    void            Deinit(void);
    int             GetNumDevices(void);
    const wchar_t * GetDeviceName(int id) const;
	int             GetDeviceFromGUID(void *deviceGUID);
    int             GetDeviceFromName(const wchar_t *name);
	bool            ResizeMixBuffers(int const& mixSamples, int const& spatialSamples);
	void            Tick(void);
	piSoundEngine * GetEngine(void);

public:
	void *mData;
private:
	friend piSoundEngineBackend * piCreateSoundEngineBackend(piSoundEngineBackend::API api, piLog *log);
	friend void                   piDestroySoundEngineBackend(piSoundEngineBackend *me);
	piSoundEngineBackendDS();
	~piSoundEngineBackendDS();
	bool doInit(piLog *log);
	void doDeinit(void);
};

} // namespace ImmCore
