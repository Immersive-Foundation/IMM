#pragma once

#include "libCore/src/libBasics/piFile.h"
#include "../document/layer.h"

namespace ImmExporter
{

	namespace tiLayerSpawnArea
	{
		bool ExportData(ImmCore::piOStream *fp, LayerImplementation imp);
		bool ExportAsset(ImmCore::piOStream *fp, LayerImplementation imp);
	}

}
