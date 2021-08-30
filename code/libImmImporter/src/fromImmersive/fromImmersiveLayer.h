#pragma once

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piStreamArrayI.h"
#include "../document/layer.h"

namespace ImmImporter
{
    namespace fiLayer
    {
            
        bool Read(Layer *me, ImmCore::piIStream *fp, Sequence *sq, ImmCore::piLog* log, Drawing::PaintRenderingTechnique renderingTechnique, volatile bool * doLoad);
            
        bool LoadAsset(Layer *me, ImmCore::piIStream *fp, Sequence *sq, ImmCore::piLog* log, Drawing::ColorSpace colorSpace, Drawing::PaintRenderingTechnique renderingTechnique);

    }

}
