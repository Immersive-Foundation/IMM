#pragma once

#include "libCore/src/libBasics/piStreamO.h"

#include "../document/layer.h"

namespace ImmExporter
{

	namespace tiLayerPaint
	{
		bool ExportData(uint32_t fps, ImmCore::piOStream *fp, LayerImplementation imp);
		bool ExportAsset(ImmCore::piOStream *fp, LayerImplementation imp);
	}

}
