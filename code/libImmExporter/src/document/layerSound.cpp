#include "libImmCore/src/libBasics/piFile.h"
#include "layerSound.h"

using namespace ImmCore;

namespace ImmExporter
{

	LayerSound::LayerSound()
	{
	}

	LayerSound::~LayerSound()
	{
	}

	bool LayerSound::Init(uint32_t version)
	{
        mType = SpatialType::Flat;
		mVolume = 1.0f;
        mGain = 1.0f;
        mAttenuation.mType = AttenuationType::None;
        mModifier.mType = ModifierType::None;
		mLoop = false;
        mVersion = version;
        mHasAsset = false;
        //mSound.Init();
		return true;
	}

    void LayerSound::Deinit(void)
    {
        if(mHasAsset)
            mSound.Deinit();
    }

    LayerSound::SpatialType LayerSound::GetType(void) const
    {
        return mType;
    }

    void LayerSound::SetSpatialType(SpatialType type)
    {
        mType = type;
    }

    void LayerSound::SetGain(float gain)
    {
        mGain = gain;
    }

    void LayerSound::SetAttenuation(AttenuationParameters attenParams)
    {
        mAttenuation = attenParams;
    }

    void LayerSound::SetModifier(ModifierParameters modifParams)
    {
        mModifier = modifParams;
    }

    void LayerSound::SetLooping(bool loop)
    {
        mLoop = loop;
    }

    const void LayerSound::GetAttenuation(AttenuationParameters *res) const
    {
        *res = mAttenuation;
    }

    const void LayerSound::GetModifier(ModifierParameters *res) const
    {
        *res = mModifier;
    }

	void LayerSound::SetVolume(float value)
	{
		mVolume = value;
	}

    const float LayerSound::GetVolume(void) const
    {
        return mVolume;
    }

    void LayerSound::AssignAsset(const ImmCore::piWav* wav, bool move)
    {
        if (move)
            mSound.InitMove(wav);
        else
            mSound.InitCopy(wav);
        mHasAsset = true;
    }

}
