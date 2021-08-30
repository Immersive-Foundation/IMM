#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "layerSpawnArea.h"

using namespace ImmCore;

namespace ImmImporter
{
	LayerSpawnArea::LayerSpawnArea() {}

	LayerSpawnArea::~LayerSpawnArea() {}

	bool LayerSpawnArea::Init(const Volume & volume, const TrackingLevel & tracking, uint32_t version)
	{
        mVolume = volume;
        mGpuId = -1;
        mHasAsset = false;
        mTracking = tracking;
        //mScreenshot.Init();
        mVersion = version;
        return true;
	}

	void LayerSpawnArea::Deinit(void)
	{
        mScreenshot.Free();
	}

	void LayerSpawnArea::SetGpuId(int id)
	{
		mGpuId = id;
	}

	const int LayerSpawnArea::GetGpuId(void) const
    {
        return mGpuId;
    }

    const int LayerSpawnArea::GetVersion(void) const
    {
        return mVersion;
    }

    
    const LayerSpawnArea::TrackingLevel LayerSpawnArea::GetTracking(void) const
    {
        return mTracking;
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

    bool LayerSpawnArea::LoadAssetMemory(const piTArray<uint8_t>& data, piLog* log, const wchar_t* ext)
    {
        if( !mScreenshot.ReadFromMemory(&data, ext) )
            return false;
        return true;
    }

    const AssetFormat LayerSpawnArea::GetAssetFormat() const
    {
        return mFormat;
    }
}
