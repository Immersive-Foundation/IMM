#pragma once

#include "libImmCore/src/libBasics/piTArray.h"
#include "libImmCore/src/libBasics/piPool.h"
#include "libImmCore/src/libSound/piSound.h"
#include "../../renderLayer.h"
#include "../layerRendererPaint.h"
#include "../../../renderMode.h"

namespace ImmPlayer
{

	class LayerRendererPaintPretessellated : public LayerRendererPaint
	{
	public:
		LayerRendererPaintPretessellated();
		~LayerRendererPaintPretessellated();

		bool Init(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmImporter::Drawing::ColorSpace colorSpace, bool frontIsCCW) override;
		void Deinit(ImmCore::piRenderer* renderer, ImmCore::piLog* log) override;

		bool LoadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) override;
		void UnloadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) override;
        bool IsLoadedInGPU(ImmImporter::Layer* la) override;
		bool LoadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) override;
		bool UnloadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) override;
        bool UnloadInGPU(ImmCore::piRenderer *renderer, ImmCore::piSoundEngine *sound, ImmCore::piLog *log, ImmImporter::Layer *la, unsigned int drawingID) override;

		void GlobalWork(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la, float masterVolume) override;
		void PrepareForDisplay(StereoMode stereoMode) override;
		void DisplayPreRender(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la, const ImmCore::frustum3& frus, const ImmCore::trans3d & layerToViewer, float opacity) override;
		void DisplayRender(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmCore::piBuffer layerStateShaderConstans, int capDelta) override;

		const DrawCallInfo & GetDrawCallInfo() override { return mDrawCallInfo; };

		// for loading data
		static constexpr int kNumChunkTypes = 5;

	private:
        ImmCore::piPool      mLayerInfo;   // per camera pass, partially
		StereoMode  mStereoMode;  // per camera pass
		ImmCore::piArray     mVisibleLayerInfos;

        ImmImporter::Drawing::ColorSpace mColorSpace;
#if defined(ANDROID)
		static const int kNumShaders = 3 * 2; // 3 stereo modes, 2 wiggle = 6
#else
		static const int kNumShaders = 3 * 2 * 2; // 3 stereo modes, 2 wiggle, 2 drawin = 12
#endif
		ImmCore::piShader mShader[kNumShaders];
		ImmCore::piBuffer mChunkData;
		ImmCore::piTexture mBlueNoise;
		ImmCore::piRasterState mRasterState[4];
		DrawCallInfo mDrawCallInfo {};
	};

}
