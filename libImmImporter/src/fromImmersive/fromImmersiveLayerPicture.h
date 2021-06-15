#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piImage.h"
#include "libCore/src/libBasics/piStreamArrayI.h"
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
