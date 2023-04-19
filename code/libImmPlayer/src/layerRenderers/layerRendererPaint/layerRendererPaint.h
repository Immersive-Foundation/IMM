#pragma once

#include "../renderLayer.h"
#include "libImmImporter/src/document/layerPaint/element.h"

namespace ImmPlayer
{
	class LayerRendererPaint : public LayerRenderer
	{
	public:
		LayerRendererPaint() = default;

		virtual ~LayerRendererPaint() = default;

		virtual bool Init(ImmCore::piRenderer *renderer, ImmCore::piLog *log, ImmImporter::Drawing::ColorSpace colorSpace, bool frontIsCCW) override = 0;

		virtual void Deinit(ImmCore::piRenderer *renderer, ImmCore::piLog *log) override = 0;

		virtual bool LoadInCPU(ImmCore::piLog *log, ImmImporter::Layer *la) override = 0;

		virtual void UnloadInCPU(ImmCore::piLog *log, ImmImporter::Layer *la) override = 0;

		virtual bool
		LoadInGPU(ImmCore::piRenderer *renderer, ImmCore::piSoundEngine *sound, ImmCore::piLog *log, ImmImporter::Layer *la) override = 0;

        virtual bool IsLoadedInGPU(ImmImporter::Layer* la) = 0;

		virtual bool
		UnloadInGPU(ImmCore::piRenderer *renderer, ImmCore::piSoundEngine *sound, ImmCore::piLog *log, ImmImporter::Layer *la) override = 0;

        virtual bool UnloadInGPU(ImmCore::piRenderer *renderer, ImmCore::piSoundEngine *sound, ImmCore::piLog *log, ImmImporter::Layer *la, unsigned int drawingID) = 0;

		virtual void PrepareForDisplay(StereoMode stereoMode) override = 0;

		virtual void
		DisplayPreRender(ImmCore::piRenderer *renderer, ImmCore::piSoundEngine *sound, ImmCore::piLog *log, ImmImporter::Layer *la,
						 const ImmCore::frustum3 &frus, const ImmCore::trans3d &layerToViewer,
						 float opacity) override = 0;

		virtual void
		DisplayRender(ImmCore::piRenderer *renderer, ImmCore::piLog *log, ImmCore::piBuffer layerStateShaderConstans,
					  int capDelta) override = 0;

		virtual const DrawCallInfo &GetDrawCallInfo () override = 0;

		// for loading data
		static constexpr int kNumChunkTypes = static_cast<int>(ImmImporter::Element::BrushSectionType::Count);
	};
}
