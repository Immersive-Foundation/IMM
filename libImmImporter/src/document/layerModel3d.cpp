#include "libCore/src/libBasics/piFile.h"

#include "layerModel3d.h"

using namespace ImmCore;


namespace ImmImporter
{

	LayerModel::LayerModel() {}

	LayerModel::~LayerModel() {}

	bool LayerModel::Init(bool renderWireFrame, const LayerModel::ShadingModel shadingModel)
	{
		mMesh.Init();

		/*
		if (!piMeshObj_Read(&mMesh, file, true))
			return false;

		mMesh.CalcBBox(0, 0);

		LayerModel::SurfaceType surfaceType = LayerModel::SurfaceType::SolidColor;
		if (mMesh.mVertexData.mVertexArray[0].mFormat.mNumElems == 3)
			surfaceType = LayerModel::SurfaceType::VertexColor;

		mSurfaceType = surfaceType;
		*/
		mShadingModel = shadingModel;
		mRenderWireFrame = renderWireFrame;

		return true;
	}

	void LayerModel::Deinit(void)
	{
		//mMesh.DeInit();
	}
    
	const LayerModel::ShadingModel LayerModel::GetShadingModel(void) const
	{
		return mShadingModel;
	}

	const bool LayerModel::GetRenderWireframe(void) const
	{
		return mRenderWireFrame;
	}

	void LayerModel::SetGpuId(int id) { mGpuId = id; }

	int LayerModel::GetGpuId(void) const { return mGpuId; }

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
		return &mMesh;
	}
}
