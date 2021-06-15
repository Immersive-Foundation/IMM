#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"

#include "../document/layer.h"
#include "../document/layerSpawnArea.h"
#include "fromImmersiveLayerSpawnArea.h"
#include "fromImmersiveUtils.h"

using namespace ImmCore;

namespace ImmImporter
{

    namespace fiLayerSpawnArea
    {
            

        LayerImplementation ReadData(piIStream *fp, piLog* log)
        {
            LayerSpawnArea *me = new LayerSpawnArea();
            if (!me) return nullptr;

            const uint32_t version = fp->ReadUInt32();

            piAssert((0 <= version) || (2 >= version));

            LayerSpawnArea::Volume volume;

            bool isFloorLevel = false;

            if (0 == version)
            {
                volume.mType = LayerSpawnArea::Volume::Type::Sphere;
                isFloorLevel = false; // Older IMM files are eye level
            }
            else //if (version <=2)
            {
                (void)fp->ReadUInt8();
                (void)fp->ReadUInt8();
                (void)fp->ReadUInt8();

                vec4 sph;
                bound3 box;

                const uint8_t type = fp->ReadUInt8();
                switch (type)
                {
                case (uint8_t)LayerSpawnArea::Volume::Type::Sphere:
                    volume.mType = LayerSpawnArea::Volume::Type::Sphere;
                    volume.mShape.mSphere = iReadVec4f(fp);
                    break;

                case (uint8_t)LayerSpawnArea::Volume::Type::Box:
                    volume.mType = LayerSpawnArea::Volume::Type::Box;
                    volume.mShape.mBox = iReadBound3f(fp);
                    break;

                default:
                    piAssert(false);
                    break;
                }

                const uint32_t flags = fp->ReadUInt32();
                volume.mAllowTranslationX = (0x1 == (flags & 0x1));
                volume.mAllowTranslationY = (0x2 == (flags & 0x2));
                volume.mAllowTranslationZ = (0x4 == (flags & 0x4));

                (void)fp->ReadUInt8();
                (void)fp->ReadUInt8();

                isFloorLevel = (0 != fp->ReadUInt8());
            }

            if (!me->Init(volume, isFloorLevel ? LayerSpawnArea::TrackingLevel::Floor : LayerSpawnArea::TrackingLevel::Eye, version))
                return nullptr;
         
            return me;
        }


        bool ReadAsset(LayerImplementation vme, piIStream *fp, piLog* log)
        {
            LayerSpawnArea* me = (LayerSpawnArea*)vme;
            const int version = me->GetVersion();

            if (version == 0)
            {
                return true;
            }
            else if (version <= 2)
            {                    
                const AssetFormat format = static_cast<AssetFormat>(fp->ReadUInt32());

                // read directly from memory
                const wchar_t *ext = nullptr;
                if (format == AssetFormat::PNG) ext = L"png";
                else if (format == AssetFormat::JPG) ext = L"jpg";
                else return true; // legacy file with no thumbnail

                piTArray<uint8_t> data;
                if (!iReadBlob(fp, &data))
                    return false;

                bool loadedScreenshot = me->LoadAssetMemory(data, log, ext);

                // Seems IMM spawn area screenshot is exported in BGR and qbin is RGB.
                if (version==1 && loadedScreenshot)
                    const_cast<piImage *>(me->GetScreenshot())->SwapRB(0);

                return loadedScreenshot;
            }

            // version > 2
            return false;
        }


    }

}
