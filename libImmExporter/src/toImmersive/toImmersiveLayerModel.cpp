#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStreamO.h"

#include "../document/layer.h"

using namespace ImmCore;
namespace ImmExporter
{

	namespace tiLayerModel
	{
		bool ExportData(piOStream *fp, LayerImplementation imp)
		{
			fp->WriteUInt32(0); // version
			return false;
		}

		bool ExportAsset(piOStream *fp, LayerImplementation imp)
		{
			return true;
		}
	}

}
