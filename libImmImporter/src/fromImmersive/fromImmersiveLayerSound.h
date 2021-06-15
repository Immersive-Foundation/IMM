#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piFile.h"
#include "libCore/src/libBasics/piStreamArrayI.h"
#include "../document/layerSound.h"
#include "../document/layer.h"

namespace ImmImporter
{

    namespace fiLayerSound
    {			
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log);
        bool ReadAsset(LayerImplementation me, ImmCore::piIStream *fp, ImmCore::piLog* log);
    }

}
