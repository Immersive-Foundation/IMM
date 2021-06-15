#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"

#include "../document/layer.h"
#include "../document/layerModel3d.h"
#include "fromImmersiveLayerModel.h"
using namespace ImmCore;

namespace ImmImporter
{

    namespace fiLayerModel
    {

        LayerImplementation ReadData(piIStream *fp, piLog* log)
        {
            LayerModel *me = new LayerModel();
            if (!me) return nullptr;

            const uint32_t version = fp->ReadUInt32(); // version
            if (version != 0) return nullptr;

            if (!me->Init(false, LayerModel::ShadingModel::Unlit))
                return nullptr;

            return me;
        }



        bool ReadAsset(LayerImplementation me, piIStream *fp, piLog* log)
        {
            return true;
        }


    }
}
