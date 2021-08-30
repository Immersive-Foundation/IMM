#pragma once

#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piStreamArrayI.h"
#include "../document/layerModel3d.h"
#include "../document/layer.h"

namespace ImmImporter
{

    namespace fiLayerModel
    {
            
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log);
            
        bool ReadAsset(LayerImplementation me, ImmCore::piIStream *fp, ImmCore::piLog* log);
    }

}
