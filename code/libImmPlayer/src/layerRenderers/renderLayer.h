#pragma once

#include "libImmCore/src/libRender/piRenderer.h"
#include "libImmImporter/src/document/layer.h"
#include "libImmImporter/src/document/layerPaint/drawing.h"
#include "libImmCore/src/libSound/piSound.h"
#include "../renderMode.h"


namespace ImmPlayer
{

	class LayerRenderer
	{
	public:
		LayerRenderer() {};
		virtual ~LayerRenderer() {};

		// Holds the latest info after DisplayRender call.
		struct DrawCallInfo
		{
			int numDrawCalls = 0;
			int numDrawCallsCulled = 0;
			int numIndices = 0;
			int numTriangles = 0;
			int numTrianglesCulled = 0;
		};

		virtual const DrawCallInfo & GetDrawCallInfo() = 0;

		virtual bool Init(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmImporter::Drawing::ColorSpace colorSpace, bool frontIsCCW) = 0;
		virtual void Deinit(ImmCore::piRenderer* renderer, ImmCore::piLog* log) = 0;

		virtual bool LoadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) = 0;
		virtual void UnloadInCPU(ImmCore::piLog* log, ImmImporter::Layer* la) = 0;

		virtual bool LoadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) = 0;
		virtual bool UnloadInGPU(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la) = 0;

		virtual void PrepareForDisplay(StereoMode stereoMode) = 0;
		virtual void DisplayPreRender(ImmCore::piRenderer* renderer, ImmCore::piSoundEngine* sound, ImmCore::piLog* log, ImmImporter::Layer* la, const ImmCore::frustum3& frus, const ImmCore::trans3d & layerToViewer, float opacity) = 0;
		virtual void DisplayRender(ImmCore::piRenderer* renderer, ImmCore::piLog* log, ImmCore::piBuffer layerStateShaderConstans, int capDelta) = 0;
	};

}
