#pragma once

#include "libCore/src/libBasics/piStreamO.h"

#include "../document/layerPicture.h"

namespace ImmExporter
{

	namespace tiLayerPicture
	{
		bool ExportData(ImmCore::piOStream *fp, LayerImplementation imp);
		bool ExportAsset(ImmCore::piOStream *fp, LayerImplementation imp);
	}

}
