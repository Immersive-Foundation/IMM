#pragma once

#include "libImmCore/src/libBasics/piPool.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#include "libImmCore/src/libMesh/piRenderMesh.h"
#include "libImmCore/src/libSound/piSound.h"
#include "../renderLayer.h"
#include "libImmImporter/src/document/layerPaint/drawing.h"
#include "libImmImporter/src/document/layerPicture.h"
#include "../../renderMode.h"

namespace ImmPlayer
{

class LayerRendererPicture : public LayerRenderer
{
public:
    LayerRendererPicture();
    ~LayerRendererPicture();

    bool Init(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmImporter::Drawing::ColorSpace colorSpace, bool frontIsCCW) override;
    void Deinit(ImmCore::piRenderer* renderer, ImmCore::piLog* log) override;

	bool LoadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) override;
	void UnloadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) override;

    bool LoadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) override;
	bool UnloadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) override;

	void GlobalWork(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la, float masterVolume) override;
	void PrepareForDisplay(StereoMode stereoMode) override;
	void DisplayPreRender(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la, const ImmCore::frustum3& frus, const ImmCore::trans3d & layerToViewer, float opacity) override;
	void DisplayRender(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmCore::piBuffer layerStateShaderConstans, int capDelta) override;

	const DrawCallInfo & GetDrawCallInfo() override { return mDrawCallInfo; };

private:
    ImmCore::piPool      mLayerInfo;
    StereoMode  mStereoMode;
	ImmCore::piArray     mVisibleLayerInfos;
	ImmCore::piShader mShaders[12][5];
    ImmCore::piBuffer   mShaderConstants;
	ImmCore::piTexture mBlueNoise;
	ImmCore::piRenderMesh m360SphereRenderMesh;
    ImmCore::piRenderMesh m360CubemapRenderMesh;

	ImmCore::piRasterState mRasterState;
	DrawCallInfo mDrawCallInfo {};

};

}
