#pragma once

#include "layerPaint/element.h"
#include "layerPaint/drawing.h"
#include "libImmCore/src/libBasics/piTick.h"
#include "libImmCore/src/libBasics/piVecTypes.h"
#define DEFAULT_LAYER_PAINT_VERSION 1
namespace ImmExporter
{

	class LayerPaint
	{
	public:
		LayerPaint() = default;
		~LayerPaint() = default;

        void Init(uint32_t version = DEFAULT_LAYER_PAINT_VERSION);
        void Deinit();

        // Only Alloc memory for the new drawing, user needs to call Drawing::Init() to initialize the drawing before editing it
        Drawing* CreateDrawing(void);

        void AddFrame(uint32_t drawingID);
        void SetFrame(uint32_t frameID, uint32_t drawingID);
        void SetMaxRepeatCount(uint32_t count);
        inline const uint32_t GetMaxRepeatCount() const { return mMaxRepeatCount; };
        const uint32_t GetNumDrawings() const;
        const uint32_t GetNumFrames() const;
        inline uint32_t GetVersion() const { return mVersion; };
        inline const uint32_t* GetFramesData() const { return mFrameToDrawingMap.data(); };
        const Drawing* GetDrawing(uint32_t drawingID) const;
        const Drawing* GetDrawingInFrame(uint32_t frameID) const;
        const ImmCore::bound3 GetBoundingBox(uint32_t frameID) const;

    private:
        std::vector<Drawing> mDrawings;
        std::vector<uint32_t> mFrameToDrawingMap;
        uint32_t mMaxRepeatCount; // 0 repeats forever
        uint32_t mVersion;
	};
}
