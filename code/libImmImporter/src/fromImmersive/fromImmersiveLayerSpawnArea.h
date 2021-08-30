#pragma once

#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "../document/layerSpawnArea.h"
#include "libImmCore/src/libBasics/piStreamArrayI.h"

namespace ImmImporter
{
    namespace fiLayerSpawnArea
    {
            
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log);
            
        bool ReadAsset(LayerImplementation vme, ImmCore::piIStream *fp, ImmCore::piLog* log);
    }

}
