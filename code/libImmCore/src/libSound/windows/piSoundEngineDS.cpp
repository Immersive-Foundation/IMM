//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <windows.h>
#include <Dsound.h>
#include <math.h>


#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")
#include "../../libWave/piWave.h"
#include "../../libBasics/piTArray.h"
#include "../../libBasics/piStr.h"

#include "../../libWave/piWave.h"
#include "../../libWave/formats/piWaveWAV.h"
#include "../../libWave/formats/piWaveOGG.h"

#include "piSoundEngineDS.h"

namespace ImmCore {




class iSoundEngineBackendDS
{
public:
	iSoundEngineBackendDS() : mSounds(), mEngine(nullptr) {}
	~iSoundEngineBackendDS() {}


	#define kMaxDevices 4096
	int mNumDevices;
	struct DeviceInfo
	{
		wchar_t  *mName;
		GUID     mGUID;
	}mDevice[kMaxDevices];

	typedef struct
	{
		IDirectSoundBuffer8* mBuffer;
		#ifdef USE3D
		IDirectSound3DBuffer8* mBuffer3D;
		#endif
		int   mID;
		bool  mPaused;
		bool  mPlaying;
		bool  mLooped;
		float mVolume;
		int   mReadOffset;

		uint32_t mNumSamples;
	}Sound;

	IDirectSound8* mDirectSound;
	IDirectSoundBuffer* mPrimaryBuffer;
	piTArray<Sound> mSounds;
	piSoundEngine *mEngine;
};

class piSoundEngineDS : public piSoundEngine
{
public:

    int   AddSound(int numChannels, uint64_t length, const void *buffer, bool makePositional) {
        return -1;
    }

    int AddSound(void(*callback)(float* channelBuffer, size_t numSamples, size_t numChannels, void* userData), void * userData)
    {
        return -1;
    }

	int AddSound(const wchar_t *filename, bool makePositional)
	{
		piWav wav;
		if (!ReadWAVFromDisk(&wav,filename))
		{
			if (!ReadOGGFromDisk(&wav, filename))
			{
				return -1;
			}
		}

		int id = static_cast<int>(mBk->mSounds.GetLength());
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.Alloc(1, true);
		if (me == nullptr)
			return -1;

		me->mID = id;
		me->mBuffer = nullptr;
		me->mPaused = true;
		me->mPlaying = false;
		me->mLooped = false;
		me->mVolume = 1.0f;

		me->mNumSamples = static_cast<uint32_t>(wav.mNumSamples);

		WAVEFORMATEX waveFormat = {

			WAVE_FORMAT_PCM,                // wFormatTag;
			WORD(wav.mNumChannels),               // nChannels
			DWORD(wav.mRate),                      // nSamplesPerSec
			DWORD(wav.mRate*wav.mBits*wav.mNumChannels / 8), //nAvgBytesPerSec
			WORD(wav.mBits*wav.mNumChannels / 8), // nBlockAlign
			WORD(wav.mBits),                      // wBitsPerSample
			0,                              // cbSize
		};

		DSBUFFERDESC bufferDesc = { sizeof(DSBUFFERDESC), // dwSize;
			DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS, // dwFlags;
			(DWORD)wav.mDataSize,                     // dwBufferBytes;
			0,                                        // dwReserved;
			&waveFormat,                              // lpwfxFormat;
			GUID_NULL                                 // guid3DAlgorithm;
		};

#ifdef USE3D
		if (wav.mNumChannels == 1)
			bufferDesc.dwFlags |= DSBCAPS_CTRL3D;
#endif

		IDirectSoundBuffer* tempBuffer;
		// Create a temporary sound buffer with the specific buffer settings.
		HRESULT st = mBk->mDirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
		if (FAILED(st))
			return -1;

		// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
		if (FAILED(tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&me->mBuffer)))
			return -1;

#ifdef USE3D
		if (wav.mNumChannels == 1)
		{
			if (FAILED(tempBuffer->QueryInterface(IID_IDirectSound3DBuffer8, (void**)&me->mBuffer3D)))
				return -1;
		}
#endif

		// Release the temporary buffer.
		tempBuffer->Release();
		tempBuffer = 0;

		//-------------------------

		unsigned short *bufferPtr;
		unsigned long bufferSize;
		if (FAILED(me->mBuffer->Lock(0, (DWORD)wav.mDataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0)))
			return -1;

		memcpy(bufferPtr, wav.mData, wav.mDataSize);

		// Unlock the secondary buffer after the data has been written to it.
		me->mBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0);

		wav.Deinit();

		return id;
	}

	int AddSound(const int frequency, int precision, int numChannels, uint64_t length, const void *buffer, bool makePositional)
	{
		int id = static_cast<int>(mBk->mSounds.GetLength());
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.Alloc(1, true);
		if (me == nullptr)
			return -1;
		me->mID = id;
		me->mPaused = true;
		me->mPlaying = false;
		me->mLooped = false;
		me->mVolume = 1.0f;

		me->mNumSamples = static_cast<uint32_t>(length);

		const int dataSize = (44100 / 10) * 2 * sizeof(short);

		// read this ffrom SampleFormat above
#define WD_SAMPLERATE 44100
#define WD_CHANNELS 2
#define WD_BITS 16
		WAVEFORMATEX waveFormat = {

			WAVE_FORMAT_PCM,       // wFormatTag;
			WD_CHANNELS,           // nChannels
			WD_SAMPLERATE,         // nSamplesPerSec
			WD_SAMPLERATE*WD_BITS*WD_CHANNELS / 8, //nAvgBytesPerSec
			WD_BITS*WD_CHANNELS / 8, // nBlockAlign
			WD_BITS,               // wBitsPerSample
			0,                     // cbSize
		};

		DSBUFFERDESC bufferDesc = {
			sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
			DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS, // DWORD           dwFlags;
			(DWORD)dataSize,                                          // DWORD           dwBufferBytes;
			0,                                          // DWORD           dwReserved;
			&waveFormat,                                       // LPWAVEFORMATEX  lpwfxFormat;
			GUID_NULL                                   // GUID            guid3DAlgorithm;
		};

		IDirectSoundBuffer* tempBuffer;
		// Create a temporary sound buffer with the specific buffer settings.
		if (FAILED(mBk->mDirectSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL)))
			return -1;

		// Test the buffer format against the direct sound 8 interface and create the secondary buffer.
		if (FAILED(tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&me->mBuffer)))
			return -1;

		// Release the temporary buffer.
		tempBuffer->Release();
		tempBuffer = 0;

		//-------------------------

		unsigned short *bufferPtr;
		unsigned long bufferSize;
		if (FAILED(me->mBuffer->Lock(0, dataSize, (void**)&bufferPtr, (DWORD*)&bufferSize, NULL, 0, 0)))
			return -1;

		// Copy the wave data into the buffer.
		//memcpy(bufferPtr, waveData, dataSize);
		for (int i = 0; i < 44100 / 10; i++)
		{
			float t = float(i) / 44100.0f;
			float f = 440.0f * 6.2831f * t;
			float y = sinf(f);
			int   s = int(32767.0f*y);
			bufferPtr[2 * i + 0] = s;
			bufferPtr[2 * i + 1] = s;
		}

		// Unlock the secondary buffer after the data has been written to it.
		me->mBuffer->Unlock((void*)bufferPtr, bufferSize, NULL, 0);

		return id;
	}
	void  DelSound(int id)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		if (me == nullptr)
			return;
		me->mBuffer->Stop();
		me->mBuffer->Release();
	}
	bool Play(int id, uint64_t microSecondOffset)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);

		if (me->mPlaying) return false;

		//this->SetVolume(id, volume);

		//me->mLooped = doLoop;
		if (FAILED(me->mBuffer->Play(0, 0, (me->mLooped) ? DSBPLAY_LOOPING : 0)))
			return false;

		if (microSecondOffset != 0)
		{
			DWORD bytesWritten;
			WAVEFORMATEX sampleFormat;
			if (FAILED(me->mBuffer->GetFormat(&sampleFormat,sizeof(WAVEFORMATEX),&bytesWritten)))
				return false;


			DWORD samplePosition = DWORD( ( uint64_t(sampleFormat.nSamplesPerSec)*microSecondOffset) / 1000000);
			const uint32_t numSamples = me->mNumSamples;

			if (me->mLooped)
			{
				samplePosition = samplePosition % numSamples;
			}
			else if (samplePosition >= numSamples)
			{
				me->mBuffer->Stop();
			}

			DWORD byteOffset = samplePosition * sampleFormat.nBlockAlign;
			if (FAILED(me->mBuffer->SetCurrentPosition(byteOffset)))
				return false;
		}

		me->mPlaying = true;
		me->mPaused = false;
		return true;
	}
	bool Stop(int id)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);

		if (!me->mPlaying) return false;

		if (FAILED(me->mBuffer->Stop()))
			return false;

		me->mPlaying = false;

		return true;
	}

	bool Pause(int id)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);

		if (!me->mPlaying) return false;
		if (me->mPaused) return false;

		int32_t writeCursor = 0;
		me->mBuffer->GetCurrentPosition((DWORD*)&me->mReadOffset, (DWORD*)&writeCursor);

		if (FAILED(me->mBuffer->Stop()))
			return false;

		me->mPaused = true;

		return true;
	}
	bool Resume(int id)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		if (!me->mPlaying) return false;
		if (!me->mPaused) return false;

		if (FAILED(me->mBuffer->SetCurrentPosition(me->mReadOffset)))
			return false;

		if (FAILED(me->mBuffer->Play(0, 0, (me->mLooped) ? DSBPLAY_LOOPING : 0)))
			return false;

		me->mPaused = false;

		return true;
	}

	void  PauseAllSounds(void)
	{
		const int num = static_cast<int>(mBk->mSounds.GetLength());
		for (int i = 0; i < num; i++)
		{
			this->Pause(i);
		}
	}

	void  ResumeAllSounds(void)
	{
		const int num = static_cast<int>(mBk->mSounds.GetLength());
		for (int i = 0; i < num; i++)
		{
			this->Resume(i);
		}
	}

    void SetListener(const trans3d & listenerToWorld)
    {
#ifdef USE3D
        vec3 pos = d2f((lst2world * vec4d(0, 0, 0, 1)).xyz());
        vec3 dir = d2f((lst2world * vec4d(0, 0, 1, 0)).xyz());
        vec3 up = d2f((lst2world * vec4d(0, 1, 0, 0)).xyz());
        m_listener->SetPosition(pos.x, pos.z, pos.y, DS3D_DEFERRED);
        m_listener->SetOrientation(dir.x, dir.z, dir.y, up.x, up.z, up.y, DS3D_DEFERRED);
        m_listener->CommitDeferredSettings();
#endif
    }


	void SetPosition(int id, const double * pos)
	{
		#ifdef USE3D
		//DS3DBUFFER info;
		//me->mBuffer3D->GetAllParameters(&info);
		//me->mBuffer3D->SetAllParameters(&info);
		me->mBuffer3D->SetPosition(pos.x, pos.z, pos.y, DS3D_IMMEDIATE);
		#endif
	}

	void SetOrientation(int id, const double * dir, const double * up)
	{
	}

	void SetPositionOrientation(int id, const double * pos, const double * dir, const double * up)
	{
	}
    PlaybackState GetPlaybackState(int id)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		DWORD status;
		me->mBuffer->GetStatus(&status);

        if (me->mPaused) return PlaybackState::Paused;

        return ((status&DSBSTATUS_PLAYING) != 0) ? PlaybackState::Playing : PlaybackState::Stopped;
	}

	float GetVolume(int id) const
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		return me->mVolume;
	}
	bool  SetVolume(int id, float volume)
	{
		iSoundEngineBackendDS::Sound *me = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);

		me->mVolume = volume;

		int v = (int)(10.0f*100.0f * log2f(fmaxf(volume, 0.00001f)));
		if (v>DSBVOLUME_MAX) v = DSBVOLUME_MAX;
		else if (v<DSBVOLUME_MIN) v = DSBVOLUME_MIN;

		if (FAILED(me->mBuffer->SetVolume(v)))
			return false;

		return true;
	}
	//bool  GetSpatialize(int id) const { return false;  }
    SoundType GetType(int id)const { return SoundType::Flat; }
	bool  ConvertType(int id, SoundType type) { return false; }
	bool  CanConvertType(int id, SoundType type) const { return false; }
	bool  GetLooping(int id) const
	{
		iSoundEngineBackendDS::Sound *sound = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		return sound->mLooped;
	}
	void SetLooping(int id, bool looping)
	{
		iSoundEngineBackendDS::Sound *sound = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		sound->mLooped = looping;
	}
	bool  GetPaused(int id) const
	{
		iSoundEngineBackendDS::Sound *sound = (iSoundEngineBackendDS::Sound*)mBk->mSounds.GetAddress(id);
		return sound->mPaused;
	}
	void  SetAttenMode(int id, AttenuationType attenMode) {}
	void  SetAttenMinMax(int id, double attenMin, double) {}
    void  SetModifierType(int id, ModifierType modifType) {}
    void  SetModifierCone(int id, double angleInner, double angleBand, double attenOut) {}
    void  SetModifierFrustum(int id, double angleInnerX, double angleInnerY, double angleBand, double attenOut) {}
    float ComputeModifiers(int id) { return 1.0f; }

public:
	piSoundEngineDS(iSoundEngineBackendDS *bk) { mBk = bk; }
private:
	iSoundEngineBackendDS * mBk;

};

piSoundEngineBackendDS::piSoundEngineBackendDS():piSoundEngineBackend()
{
}
piSoundEngineBackendDS::~piSoundEngineBackendDS()
{
}
bool piSoundEngineBackendDS::doInit(piLog *log)
{
	iSoundEngineBackendDS * me = new iSoundEngineBackendDS();
	if (!me) return false;
	mData = me;
	me->mEngine = new piSoundEngineDS(me);
	return true;
}

void piSoundEngineBackendDS::doDeinit(void)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
	delete me->mEngine;
	delete me;
}

static BOOL CALLBACK myDSEnumCallback(LPGUID lpGuid, const wchar_t * lpcstrDescription, const wchar_t * lpcstrModule, void * lpContext)
{
	piSoundEngineBackendDS *bk = (piSoundEngineBackendDS*)lpContext;
	iSoundEngineBackendDS *me = (iSoundEngineBackendDS*)bk->mData;

	if (me->mNumDevices >= kMaxDevices) return TRUE;

	if(lpGuid==nullptr)
		memset(&me->mDevice[me->mNumDevices].mGUID, 0, sizeof(GUID));
	else
		memcpy(&me->mDevice[me->mNumDevices].mGUID, lpGuid, sizeof(GUID));
    const size_t len = wcslen(lpcstrDescription );
    me->mDevice[me->mNumDevices].mName = (wchar_t *)malloc( (len+2)*sizeof(wchar_t) );
    if( !me->mDevice[me->mNumDevices].mName )
        return FALSE;

    wcscpy_s(me->mDevice[me->mNumDevices].mName, len+2, lpcstrDescription);

    me->mNumDevices++;
    return TRUE;
}

int piSoundEngineBackendDS::GetNumDevices(void)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;

    me->mNumDevices = 0;
    DirectSoundEnumerate(myDSEnumCallback, this);
    return me->mNumDevices;
}

const wchar_t *piSoundEngineBackendDS::GetDeviceName(int id) const
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
	return me->mDevice[id].mName;
}

int piSoundEngineBackendDS::GetDeviceFromGUID( void *deviceGUID)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
	GUID *guid = (GUID*)deviceGUID;
	for (int i = 0; i <me->mNumDevices; i++)
	{
		if (me->mDevice[i].mGUID.Data1 == guid->Data1 &&
			me->mDevice[i].mGUID.Data2 == guid->Data2 &&
			me->mDevice[i].mGUID.Data3 == guid->Data3 &&
			memcmp(me->mDevice[i].mGUID.Data4, guid->Data4, 8) == 0)
		{
			return i;
		}
	}

	return -1;
}


int piSoundEngineBackendDS::GetDeviceFromName(const wchar_t *name)
{
    iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
    for (int i = 0; i <me->mNumDevices; i++)
    {
        if( piwstrcmp(name,me->mDevice[i].mName) == 0)
        {
            return i;
        }
    }

    return -1;
}

bool piSoundEngineBackendDS::Init(void *hwnd, int deviceID, const Configuration* config)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;

	if (!me->mSounds.Init(1024, true ) )
        return false;
#if 1
    LPCGUID dv = NULL;
    if (deviceID>0) dv = &me->mDevice[deviceID].mGUID;

    if (FAILED(DirectSoundCreate8(dv, &me->mDirectSound, NULL)))
    {
        return false;
    }
#else

    static DSBUFFERDESC bufferDesc = {
        sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
        DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME, // DWORD           dwFlags;
        0,                                          // DWORD           dwBufferBytes;
        0,                                          // DWORD           dwReserved;
        NULL,                                       // LPWAVEFORMATEX  lpwfxFormat;
        GUID_NULL                                   // GUID            guid3DAlgorithm;
    };
    if (FAILED(CoInitializeEx(NULL, 0)))
        return false;

    if (FAILED(CoCreateInstance(CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (LPVOID*)&m_DirectSound)))
        return false;

    if (FAILED(m_DirectSound->Initialize(NULL)))
        return false;
#endif

    // Set the cooperative level to priority so the format of the primary sound buffer can be modified.
    if (FAILED(me->mDirectSound->SetCooperativeLevel((HWND)hwnd, DSSCL_PRIORITY)))
        return false;

    #define WD_SAMPLERATE 44100
    #define WD_CHANNELS 2
    #define WD_BITS 16

    static WAVEFORMATEX waveFormat = {

        WAVE_FORMAT_PCM,       // wFormatTag;
        WD_CHANNELS,           // nChannels
        WD_SAMPLERATE,         // nSamplesPerSec
        WD_SAMPLERATE*WD_BITS*WD_CHANNELS / 8, //nAvgBytesPerSec
        WD_BITS*WD_CHANNELS / 8, // nBlockAlign
        WD_BITS,               // wBitsPerSample
        0,                     // cbSize
    };

    DSBUFFERDESC bufferDesc = {
        sizeof(DSBUFFERDESC),                       // DWORD           dwSize;
        DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME, // DWORD           dwFlags;
        0,                                          // DWORD           dwBufferBytes;
        0,                                          // DWORD           dwReserved;
        NULL,                                       // LPWAVEFORMATEX  lpwfxFormat;
        GUID_NULL                                   // GUID            guid3DAlgorithm;
    };

#ifdef USE3D
    bufferDesc.dwFlags |= DSBCAPS_CTRL3D;
#endif

    // Get control of the primary sound buffer on the default sound device.
    if (FAILED(me->mDirectSound->CreateSoundBuffer(&bufferDesc, &me->mPrimaryBuffer, NULL)))
        return false;

    // Setup the format of the primary sound bufffer.
    // In this case it is a .WAV file recorded at 44,100 samples per second in 16-bit stereo (cd audio format).
    if (FAILED(me->mPrimaryBuffer->SetFormat(&waveFormat)))
        return false;

    // Obtain a listener interface.
#ifdef USE3D
    if (FAILED(m_primaryBuffer->QueryInterface(IID_IDirectSound3DListener8, (void**)&m_listener)))
        return false;

    m_listener->SetPosition(0.0f, 0.0f, 0.0f, DS3D_IMMEDIATE);
    m_listener->SetDistanceFactor(1.0f, DS3D_IMMEDIATE);
    m_listener->SetRolloffFactor(1.0f, DS3D_IMMEDIATE);
#endif

    return true;
}

void piSoundEngineBackendDS::Deinit(void)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;

	me->mSounds.End();

    // Release the primary sound buffer pointer.
    if (me->mPrimaryBuffer)
		me->mPrimaryBuffer->Release();
    if (me->mDirectSound)
		me->mDirectSound->Release();
}

void piSoundEngineBackendDS::Tick(void)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
}

piSoundEngine * piSoundEngineBackendDS::GetEngine(void)
{
	iSoundEngineBackendDS * me = (iSoundEngineBackendDS*)mData;
	return me->mEngine;
}
bool piSoundEngineBackendDS::ResizeMixBuffers(int const& mixSamples, int const& spatialSamples)
{
	return true;
}


}
