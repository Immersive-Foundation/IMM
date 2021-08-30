#undef BYTE

#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libBasics/piImage.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "layerPicture.h"

using namespace ImmCore;

namespace ImmImporter
{
	LayerPicture::LayerPicture() {}

	LayerPicture::~LayerPicture() {}

	bool LayerPicture::Init(ContentType type, bool isViewerLocked, piLog *log)
	{
		mGpuId = -1;
		mType = type;
		mIsViewerLocked = isViewerLocked;
		mImage.Init();

		return true;
	}
    
    bool LayerPicture::LoadAssetMemory(const piTArray<uint8_t> & data, piLog *log, const wchar_t *ext)
    {
        if( !mImage.ReadFromMemory(&data, ext) )
            return false;

        mImage.Convert( 0, piImage::Format::FORMAT_I_RGBA, false );
        
        iComputeBBox();
        return true;
    }

	void LayerPicture::Deinit(void)
	{
		mImage.Free();
	}

	void LayerPicture::SetGpuId(int id)
	{
		mGpuId = id;
	}

	bool LayerPicture::GetIsViewerLocked(void) const
	{
		return mIsViewerLocked;
	}

	piImage * LayerPicture::GetImage(void)
	{
		return &mImage;
	}


	int LayerPicture::GetGpuId(void) const { return mGpuId; }


	void LayerPicture::iComputeBBox(void)
	{
		if (mType == Image2D)
		{
			const float ar = float(mImage.GetXRes()) / float(mImage.GetYRes());
			const vec3 rad = vec3(ar, 1.0f, 0.05f);
			mBBox = bound3(-rad.x, rad.x, -rad.y, rad.y, -rad.z, rad.z);
		}
		else // Cubemaps and Equirect spheres
		{
			mBBox = bound3(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		}
	}

	LayerPicture::ContentType LayerPicture::GetType(void) const
	{
		return mType;
	}

	const bool LayerPicture::HasBBox(void) const
	{
		return true;
	}
	const bound3 & LayerPicture::GetBBox(void) const
	{
		return mBBox;
	}

}
