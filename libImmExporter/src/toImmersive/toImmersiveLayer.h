#pragma once

#include "libCore/src/libBasics/piStreamO.h"
#include "libCore/src/libBasics/piArray.h"
#include "../document/layer.h"

namespace ImmExporter
{

	namespace tiLayer
	{
		struct DelayedSerialization
		{
			Layer*   mLayer;
			//uint64_t mFileOffset;
			uint64_t mTableOffset;
		};

		bool ExportLayer(ImmCore::piOStream *fp, Layer* la, ImmCore::piArray *delayed, uint32_t fps);
	}

}
