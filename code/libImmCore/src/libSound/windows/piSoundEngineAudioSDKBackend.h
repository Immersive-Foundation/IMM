//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../../libBasics/piTArray.h"
#include "../../libBasics/piString.h"
#include "../piSoundEngineBackend.h"

namespace ImmCore
{

    class piSoundEngineAudioSDKBackend final : public piSoundEngineBackend
    {
    public:
        piSoundEngineAudioSDKBackend();
        ~piSoundEngineAudioSDKBackend() override;

        bool            Init(void* hwnd, int deviceid, const piSoundEngineBackend::Configuration * config) override;
        bool            Init(void* hwnd, int deviceid, int maxsoundsoverride, float sampleRate = 48000.0f, int32_t bufferSize = 512);
        void            Deinit(void) override;
        int             GetNumDevices(void) override;
        const wchar_t * GetDeviceName(int id) const override;
        int             GetDeviceFromGUID(void* deviceGUID) override;
        int             GetDeviceFromName(const wchar_t *name) override;
        bool            ResizeMixBuffers(int const&, int const&) override { return true; };
        void            Tick(void) override;
        piSoundEngine*  GetEngine(void) override;

    private:
        void *mData;
        bool mDisableDevice = false;

    private:
        bool doInit(ImmCore::piLog *log) override;
        void doDeinit(void) override;

    public:
        static piSoundEngineBackend* Create(ImmCore::piLog * log);
        static void Destroy(piSoundEngineBackend*);

        static void setDisableAudioDevice(piSoundEngineBackend* vme, bool disableDevice);
        static void getAudioMix(piSoundEngineBackend* vme, float* buffer, int sampleNumPerChannel);
    };

}
