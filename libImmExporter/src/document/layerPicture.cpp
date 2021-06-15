#undef BYTE

#include "libCore/src/libBasics/piVecTypes.h"
#include "libCore/src/libBasics/piImage.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStr.h"

#include "layerPicture.h"

using namespace ImmCore;

namespace ImmExporter
{
	LayerPicture::LayerPicture() {}

	LayerPicture::~LayerPicture() {}

	bool LayerPicture::Init(uint32_t version)
	{
		//mImage.Init();
        mIsViewerLocked = false;
        mVersion = version;
        mHasAsset = false;
		return true;
	}

    bool LayerPicture::AssignAsset(const piImage *asset, bool move)
    {
        if (move)
            mImage.InitMove(asset);
        else
            mImage.InitCopy(asset);
        mImage.Convert(0, piImage::Format::FORMAT_I_RGBA, false);
        iComputeBBox();
        mHasAsset = true;
        return true;
    }

	void LayerPicture::Deinit(void)
	{
        if(mHasAsset)
		    mImage.Free();
	}

	bool LayerPicture::GetIsViewerLocked(void) const
	{
		return mIsViewerLocked;
	}

    void LayerPicture::SetIsViewerLocked(bool viewerLock)
    {
        mIsViewerLocked = viewerLock;
    }

	piImage * LayerPicture::GetImage(void)
	{
        if (!mHasAsset)
            return nullptr;
		return &mImage;
	}

	void LayerPicture::iComputeBBox(void)
	{
		if (mType == PictureType::Image2D)
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

	const LayerPicture::PictureType LayerPicture::GetType(void) const
	{
		return mType;
	}

    void LayerPicture::SetType(PictureType type)
    {
        mType = type;
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
