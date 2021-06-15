#pragma once

#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piFile.h"
#include "../document/layerSpawnArea.h"
#include "libCore/src/libBasics/piStreamArrayI.h"

namespace ImmImporter
{
    namespace fiLayerSpawnArea
    {
            
        LayerImplementation ReadData(ImmCore::piIStream *fp, ImmCore::piLog* log);
            
        bool ReadAsset(LayerImplementation vme, ImmCore::piIStream *fp, ImmCore::piLog* log);
    }

}
