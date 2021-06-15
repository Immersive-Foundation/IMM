#pragma once

#include "libCore/src/libBasics/piArray.h"
#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libMesh/piMesh.h"
#include "libCore/src/libBasics/piLog.h"

namespace ImmImporter
{

	class LayerModel
	{
	public:

		enum class ShadingModel : int
		{
			Unlit = 0,
			Smooth = 1
		};

		LayerModel();
		~LayerModel();

		bool Init(bool renderWireFrame, const ShadingModel shadingModel);
		void Deinit(void);

		const ShadingModel GetShadingModel(void) const;
		const bool GetRenderWireframe(void) const;
		const ImmCore::bound3 & GetBBox(void) const;
		const bool HasBBox(void) const;
        ImmCore::piMesh * GetMesh(void);

		void SetGpuId(int id);
		int GetGpuId(void) const;

	private:
		ShadingModel mShadingModel;
		bool         mRenderWireFrame;
        ImmCore::piMesh	     mMesh;
		int          mGpuId;
	};
}
