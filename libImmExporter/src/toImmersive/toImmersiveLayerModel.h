#pragma once

#include "libCore/src/libBasics/piStreamO.h"

#include "../document/layer.h"

namespace ImmExporter
{

	namespace tiLayerModel
	{
		bool ExportData(ImmCore::piOStream *fp, LayerImplementation imp);
		bool ExportAsset(ImmCore::piOStream *fp, LayerImplementation imp);
	}

}
