#include "libImmCore/src/libBasics/piFile.h"

#include "layerModel3d.h"

using namespace ImmCore;


namespace ImmExporter
{

    LayerModel::LayerModel() {}

    LayerModel::~LayerModel() {}

    bool LayerModel::Init(uint32_t version)
    {
        //mMesh.Init();
        mVersion = version;
        mRenderWireFrame = false;
        mShadingModel = ShadingModel::Unlit;
        mHasAsset = false;
        return true;
    }

    void LayerModel::Deinit()
    {
        if(mHasAsset)
            mMesh.DeInit();
    }

    bool LayerModel::AssignAsset(const piMesh *asset, bool move)
    {
        if (move)
            mMesh.InitMove(asset);
        else
            asset->Clone(&mMesh);
        mHasAsset = true;
        return true;
    }


    const LayerModel::ShadingModel LayerModel::GetShadingModel(void) const
    {
        return mShadingModel;
    }

    void LayerModel::SetShadingModel(const ShadingModel shadingModel)
    {
        mShadingModel = shadingModel;
    }

	const bool LayerModel::GetRenderWireframe(void) const
	{
		return mRenderWireFrame;
	}

    void LayerModel::SetRenderWireframe(bool renderWireFrame)
    {
        mRenderWireFrame = renderWireFrame;
    }

	const bool LayerModel::HasBBox(void) const
	{
		return true;
	}
	const bound3 & LayerModel::GetBBox(void) const
	{
		return mMesh.mBBox;
	}

	piMesh *LayerModel::GetMesh(void)
	{
        if (!mHasAsset)
            return nullptr;
		return &mMesh;
	}
}
