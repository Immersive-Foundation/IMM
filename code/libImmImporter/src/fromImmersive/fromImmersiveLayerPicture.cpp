#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piStr.h"

#include "../document/layer.h"
#include "../document/layerPicture.h"
#include "fromImmersiveUtils.h"

using namespace ImmCore;

namespace ImmImporter
{

    namespace fiLayerPicture
    {
            
        LayerImplementation ReadData(piIStream *fp, piLog* log)
        {
            LayerPicture *me = new LayerPicture();
            if (!me) return nullptr;

            const uint32_t version = fp->ReadUInt32(); // version
            if (version != 0) return nullptr;

            const LayerPicture::ContentType type = static_cast<LayerPicture::ContentType>(fp->ReadUInt16());
            const bool isViewerLocked = (fp->ReadUInt8() == 1);
            fp->ReadUInt8();

            if (!me->Init(type, isViewerLocked, log))
                return nullptr;

            return me;
        }


        bool ReadAsset(LayerImplementation vme, piIStream *fp, piLog* log)
        {
            LayerPicture *me = (LayerPicture*)vme;

            const AssetFormat format = static_cast<AssetFormat>(fp->ReadUInt32());

            // read directly from memory
            const wchar_t *ext = nullptr;
            if (format == AssetFormat::PNG) ext = L"png";
            else if (format == AssetFormat::JPG) ext = L"jpg";
            else return false;

            piTArray<uint8_t> data;
            if (!iReadBlob(fp, &data))
                return false;

            return me->LoadAssetMemory(data, log, ext);
        }

    }

}
