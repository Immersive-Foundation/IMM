#pragma once

#include "libImmCore/src/libBasics/piStreamO.h"
#include "../document/layer.h"

namespace ImmExporter
{

	namespace tiLayerSound
	{
        enum class AudioType : int
        {
            OPUS = 0,
            OGG = 1
        };
		bool ExportData(ImmCore::piOStream *fp, LayerImplementation imp);
		bool ExportAsset(ImmCore::piOStream *fp, LayerImplementation im, const AudioType audioType, int opusBitrate);
	}

}
