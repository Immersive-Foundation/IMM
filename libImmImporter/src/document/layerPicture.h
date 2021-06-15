#pragma once

#include "libCore/src/libBasics/piString.h"
#include "libCore/src/libRender/piRenderer.h"
#include "libCore/src/libBasics/piImage.h"
#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piVecTypes.h"

namespace ImmImporter
{

	class LayerPicture
	{
	public:
		enum ContentType
		{
			Image2D = 0,
			Image360EquirectMono = 1,
			Image360EquirectStereo = 2,
			Image360CubemapCrossMono = 3,
			Image360CubemapVstripMono = 4,
		};

		LayerPicture();
		~LayerPicture();

		bool Init(ContentType type, bool isViewerLocked, ImmCore::piLog *log);
		void Deinit(void);

		bool LoadAssetMemory(const ImmCore::piTArray<uint8_t> & data, ImmCore::piLog *log, const wchar_t *ext);

		ContentType GetType(void) const;
		ImmCore::piImage  * GetImage(void);
		const ImmCore::bound3 & GetBBox(void) const;
		const bool HasBBox(void) const;

		int GetGpuId(void) const;
		void SetGpuId(int id);

		bool GetIsViewerLocked(void) const;

	private:
		void iComputeBBox(void);

	private:
		ContentType mType;
		ImmCore::piImage  mImage;
		bool mIsViewerLocked;
        ImmCore::bound3 mBBox; // for frustum culling, mono rendering (if too small), LOD, fast selection, etc
		int mGpuId;
	};

}
