#include "libImmCore/src/libBasics/piFileName.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "layerSound.h"

using namespace ImmCore;

namespace ImmImporter
{

	LayerSound::LayerSound()
	{
	}

	LayerSound::~LayerSound()
	{
	}

	bool LayerSound::Init(bool loop, bool play, float gain, 
                          const AttenuationParameters & attenParams,
                          const ModifierParameters & modifParams,
                          Type type, float duration, piLog *log)
	{
        mType = type;

		mGpuId = -1;
		//mDuration = duration;
		mVolume = 0.0f;
        mGain = gain;
        mAttenuation = attenParams;
        mModifier = modifParams;
		mPlay = play;
		mLoop = loop;
        mPaused = false;
        mForceRestart = false;
        mSound.Init();
        mOffset = 0;
        mCompressed = false;
		return true;
	}

    void LayerSound::Deinit(void)
    {
        mSound.Deinit();
    }

    void LayerSound::SetPlaying(bool value, uint64_t startOffsetMicroseconds)
    {
        mPlay = value;
        mOffset = startOffsetMicroseconds;
    }

    void LayerSound::SetPaused(bool value)
    {
        mPaused = value;
    }
    LayerSound::Type LayerSound::GetType(void) const
    {
        return mType;
    }

    void LayerSound::SetType(Type type)
    {
        mType = type;
    }
    void LayerSound::SetForceRestart(bool force)
    {
        mForceRestart = force;
    }
    void LayerSound::GetAttenuation(AttenuationParameters *res) const
    {
        *res = mAttenuation;
    }

    void LayerSound::GetModifier(ModifierParameters *res) const
    {
        *res = mModifier;
    }

    void LayerSound::SetGpuId(int id) { mGpuId = id; }

	int LayerSound::GetGpuId(void) const { return mGpuId; }

	void LayerSound::SetVolume(float value)
	{
		mVolume = value;
	}

    float LayerSound::GetVolume(void) const
    {
        return mVolume;
    }
    
	

}
