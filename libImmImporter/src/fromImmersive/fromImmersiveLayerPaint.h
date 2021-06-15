#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStreamArrayI.h"
#include "../document/layerPaint.h"
#include "../document/layer.h"

namespace ImmImporter
{

    namespace fiLayerPaint
    {
        
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log, Drawing::PaintRenderingTechnique renderingTechnique);
        
        bool ReadAsset(LayerImplementation me, ImmCore::piIStream *fp, ImmCore::piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique, bool flipped);

        bool ReadDrawing(LayerImplementation vme, uint32_t drawingId, ImmCore::piIStream *fp, ImmCore::piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique, bool flipped);
    }

}
