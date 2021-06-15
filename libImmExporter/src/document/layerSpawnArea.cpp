//#include <codecvt>
//#include <string>
//#include <math.h>


#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piStr.h"

#include "layerSpawnArea.h"

using namespace ImmCore;

namespace ImmExporter
{
	LayerSpawnArea::LayerSpawnArea() {}

	LayerSpawnArea::~LayerSpawnArea() {}

	bool LayerSpawnArea::Init(uint32_t version)
	{
        mVolume.mType = Volume::Type::Sphere;
        mVolume.mShape.mSphere = vec4(0.0f, 1.0f, 0.0f, 2.0f);
        mHasAsset = false;
        //mScreenshot.Init();
        mVersion = version;
        return true;
	}

	void LayerSpawnArea::Deinit(void)
	{
        if(mHasAsset)
            mScreenshot.Free();
	}

    const int LayerSpawnArea::GetVersion(void) const
    {
        return mVersion;
    }

    void LayerSpawnArea::SetTracking(LayerSpawnArea::TrackingLevel tracking)
    {
        mTracking = tracking;
    }

    const LayerSpawnArea::TrackingLevel LayerSpawnArea::GetTracking(void) const
    {
        return mTracking;
    }

    void LayerSpawnArea::SetVolume(Volume vol)
    {
        mVolume = vol;
    }

    const LayerSpawnArea::Volume & LayerSpawnArea::GetVolume(void) const
    {
        return mVolume;
    }

    const piImage* LayerSpawnArea::GetScreenshot(void) const
    {
        if (!mHasAsset)
            return nullptr;
        return &mScreenshot;
    }

    const LayerSpawnArea::Volume::Type& LayerSpawnArea::GetVolumeType(void) const
    {
        return mVolume.mType;
    }

    const vec4& LayerSpawnArea::GetVolumeSphere(void) const
    {
        return mVolume.mShape.mSphere;
    }

    const bound3& LayerSpawnArea::GetVolumeBox(void) const
    {
        return mVolume.mShape.mBox;
    }

    bool LayerSpawnArea::AssignAsset(const piImage *asset, bool move)
    {
        if (move)
            mScreenshot.InitMove(asset);
        else
            mScreenshot.InitCopy(asset);
        if(asset->GetFormat(0) == piImage::Format::FORMAT_I_RGBA)
            mScreenshot.Convert(0, piImage::Format::FORMAT_I_RGB, false);
        mHasAsset = true;
        return true;
    }

}
