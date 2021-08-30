#pragma once

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piImage.h"
#include "libImmCore/src/libBasics/piStreamArrayI.h"
#include "../document/layerPicture.h"
#include "../document/layer.h"

namespace ImmImporter
{

    namespace fiLayerPicture
    {
            
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log);
            
        bool ReadAsset(LayerImplementation me, ImmCore::piIStream *fp, ImmCore::piLog* log);
    }

}
