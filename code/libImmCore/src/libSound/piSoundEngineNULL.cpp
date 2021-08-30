//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include "piSoundEngineNULL.h"
#include "../libBasics/piTypes.h"

namespace ImmCore {

	struct iSoundEngineBackend
	{
		piSoundEngine *mEngine;
	};

	class piSoundEngineNULL : public piSoundEngine
	{
	public:

        int   AddSound(int numChannels, uint64_t length, const void *buffer, bool makePositional) { return 0;  }

        int   AddSound(void(*callback)(float* channelBuffer, size_t numSamples, size_t numChannels, void* userData), void * userData)
        {
            return 0;
        }

		int AddSound(const wchar_t *filename, bool makePositional)
		{
			return 0;
		}

		int AddSound(const int frequency, int precision, int numChannels, uint64_t length, const void *buffer, bool makePositional)
		{
			return 0;
		}
		void  DelSound(int id)
		{
		}
		bool Play(int id, uint64_t offset)
		{
			return true;
		}
		bool Stop(int id)
		{
			return true;
		}
		bool Pause(int id)
		{
			return true;
		}
		bool Resume(int id)
		{
			return true;
		}

		void PauseAllSounds(void)
		{
		}
		void ResumeAllSounds(void)
		{
		}

        void SetListener(const trans3d & listenerToWorld)
        {
        }

		void SetPosition(int id, const double * pos)
		{
		}

		void SetOrientation(int id, const double * dir, const double * up)
		{
		}
		void SetPositionOrientation(int id, const double * pos, const double * dir, const double * up)
		{
		}
        PlaybackState GetPlaybackState(int id)
		{
			return PlaybackState::Stopped;
		}

		float GetVolume(int id) const
		{
			return 0.0f;
		}
		bool  SetVolume(int id, float volume)
		{
			return true;
		}
        SoundType GetType(int id) const { return SoundType::Flat; }
		bool  ConvertType(int id, SoundType type) { return false; }
		bool  CanConvertType(int id, SoundType type) const { return false; }
		bool  GetLooping(int id) const
		{
			return false;
		}
		void SetLooping(int id, bool looping)
		{
		}
		bool  GetPaused(int id) const
		{
			return false;
		}

		void  SetAttenMode(int id, AttenuationType attenMode) {}
		void  SetAttenMinMax(int id, double attenMin, double) {}
        void  SetModifierType(int id, ModifierType modifType) {}
        void  SetModifierCone(int id, double angleInner, double angleBand, double attenOut) {}
        void  SetModifierFrustum(int id, double angleInnerX, double angleInnerY, double angleBand, double attenOut) {}
        float ComputeModifiers(int id) { return 1.0f; }

	public:
		piSoundEngineNULL(iSoundEngineBackend *bk) { mBk = bk; }
	private:
		iSoundEngineBackend * mBk;

	};

	piSoundEngineBackendNULL::piSoundEngineBackendNULL() :piSoundEngineBackend()
	{
	}
	piSoundEngineBackendNULL::~piSoundEngineBackendNULL()
	{
	}
	bool piSoundEngineBackendNULL::doInit(piLog *log)
	{
		iSoundEngineBackend * me = (iSoundEngineBackend*)malloc(sizeof(iSoundEngineBackend));
		if (!me) return false;
		mData = me;
		me->mEngine = new piSoundEngineNULL(me);
		return true;
	}

	void piSoundEngineBackendNULL::doDeinit(void)
	{
		iSoundEngineBackend * me = (iSoundEngineBackend*)mData;
		delete me->mEngine;
		free(me);
	}

	
	int piSoundEngineBackendNULL::GetNumDevices(void)
	{
		return 1;
	}

	const wchar_t *piSoundEngineBackendNULL::GetDeviceName(int id) const
	{
		return L"Null";
	}

    int piSoundEngineBackendNULL::GetDeviceFromName(const wchar_t *name)
    {
        return -1;
    }

	int piSoundEngineBackendNULL::GetDeviceFromGUID(void *deviceGUID)
	{
		return 0;
	}


	bool piSoundEngineBackendNULL::Init(void *hwnd, int deviceID, const Configuration* config)
	{
		iSoundEngineBackend * me = (iSoundEngineBackend*)mData;
		return true;
	}

	void piSoundEngineBackendNULL::Deinit(void)
	{
		iSoundEngineBackend * me = (iSoundEngineBackend*)mData;
	}

	void piSoundEngineBackendNULL::Tick(void)
	{
	}

	piSoundEngine * piSoundEngineBackendNULL::GetEngine(void)
	{
		iSoundEngineBackend * me = (iSoundEngineBackend*)mData;
		return me->mEngine;
	}
	bool piSoundEngineBackendNULL::ResizeMixBuffers(int const& mixSamples, int const& spatialSamples)
	{
		return true;
	}


}
