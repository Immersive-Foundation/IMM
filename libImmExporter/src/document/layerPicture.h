#pragma once

#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libBasics/piImage.h"
#include "layer.h"

#define DEFAULT_LAYER_PICTURE_VERSION 1

namespace ImmExporter
{
	class LayerPicture
	{
	public:
		enum class PictureType :uint32_t
		{
			Image2D = 0,
			Image360EquirectMono = 1,
			Image360EquirectStereo = 2,
			Image360CubemapCrossMono = 3,
			Image360CubemapVstripMono = 4,
		};

		LayerPicture();
		~LayerPicture();

		bool Init(uint32_t version = DEFAULT_LAYER_PICTURE_VERSION);
		void Deinit();

        bool AssignAsset(const ImmCore::piImage *asset, bool move);

		const PictureType GetType() const;
        void SetType(PictureType type);
		ImmCore::piImage  * GetImage();
		const ImmCore::bound3 & GetBBox() const;
		const bool HasBBox() const;

		bool GetIsViewerLocked() const;
        void SetIsViewerLocked(bool viewerLock);

	private:
		void iComputeBBox(void);

	private:
		PictureType mType;
		ImmCore::piImage  mImage;
		bool mIsViewerLocked;
        ImmCore::bound3 mBBox; // for frustum culling, mono rendering (if too small), LOD, fast selection, etc
        uint32_t mVersion;
        bool mHasAsset;

	};

}
