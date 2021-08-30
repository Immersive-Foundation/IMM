#pragma once

#include "libImmCore/src/libBasics/piArray.h"
#include "libImmCore/src/libBasics/piString.h"
#include "libImmCore/src/libMesh/piMesh.h"
#include "libImmCore/src/libBasics/piLog.h"
#define DEFAULT_LAYER_MODEL_VERSION 1

namespace ImmExporter
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

		bool Init(uint32_t version = DEFAULT_LAYER_MODEL_VERSION);
		void Deinit();

        bool AssignAsset(const ImmCore::piMesh *asset, bool move);

		const ShadingModel GetShadingModel(void) const;
        void SetShadingModel(const ShadingModel shadingModel);

		const bool GetRenderWireframe(void) const;
        void SetRenderWireframe(bool renderWireFrame);

		const ImmCore::bound3 & GetBBox(void) const;
		const bool HasBBox(void) const;
        ImmCore::piMesh * GetMesh(void);


	private:
		ShadingModel mShadingModel;
		bool         mRenderWireFrame;
        ImmCore::piMesh		 mMesh;
        uint32_t mVersion;
        bool mHasAsset;

	};
}
