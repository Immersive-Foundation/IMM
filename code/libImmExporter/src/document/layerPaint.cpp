#include "layerPaint.h"

using namespace ImmCore;
namespace ImmExporter
{

    void LayerPaint::Init(uint32_t version)
    {
        mMaxRepeatCount = 1;
        mVersion = version;
        mDrawingCapacity = 0;
        mFrameCapacity = 0;
    }

    void LayerPaint::Deinit()
    {
        for (auto drawing = mDrawings.begin(); drawing != mDrawings.end(); drawing++)
        {
            (*drawing)->Destroy();
        }
        mFrameToDrawingMap.clear();
        mDrawings.clear();
    }

    Drawing* LayerPaint::CreateDrawing()
    {
        auto drawing = new Drawing;
        mDrawings.push_back(drawing);
        return drawing;
    }

    void LayerPaint::AddFrame(uint32_t drawingID)
    {
        mFrameToDrawingMap.push_back(drawingID);
    }

    void LayerPaint::SetFrame(uint32_t frameID, uint32_t drawingID)
    {
        if (frameID >= mFrameToDrawingMap.size() || drawingID >= mDrawings.size())
            return;
        mFrameToDrawingMap[frameID] = drawingID;
    }

    void LayerPaint::SetMaxRepeatCount(uint32_t count)
    {
        mMaxRepeatCount = count;
    }

    Drawing* LayerPaint::GetDrawing(uint32_t drawingID) const
    {
        if (drawingID >= mDrawings.size())
            return nullptr;
        return mDrawings[drawingID];
    }

    Drawing* LayerPaint::GetDrawingInFrame(uint32_t frameID) const
    {
        if (frameID >= mFrameToDrawingMap.size())
            return nullptr;
        return mDrawings[mFrameToDrawingMap[frameID]];
    }

    const bound3 LayerPaint::GetBoundingBox(uint32_t frameID) const
    {
        if (frameID >= mFrameToDrawingMap.size())
            return bound3(1e20f);
        return GetDrawing(frameID)->GetBoundingBox();
    }

    void LayerPaint::SetDrawingCapacity(uint32_t num)
    {
        mDrawingCapacity = num;
    }

    void LayerPaint::SetFrameCapacity(uint32_t num)
    {
        mFrameCapacity = num;
    }

    const uint32_t LayerPaint::GetNumDrawings() const
    {
        return static_cast<uint32_t>(mDrawings.size());
    }

    const uint32_t LayerPaint::GetNumFrames() const
    {
        return static_cast<uint32_t>(mFrameToDrawingMap.size());
    }

    const uint32_t LayerPaint::GetDrawingCapacity() const
    {
        return mDrawingCapacity;
    }

    const uint32_t LayerPaint::GetFrameCapacity() const
    {
        return mFrameCapacity;
    }
}
