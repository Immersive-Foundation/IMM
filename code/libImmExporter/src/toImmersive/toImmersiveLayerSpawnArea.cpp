#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piFile.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libBasics/piStreamO.h"

#include "../document/layer.h"
#include "../document/layerSpawnArea.h"

#include "toImmersiveUtils.h"
#include "toImmersiveLayerSpawnArea.h"

using namespace ImmCore;

namespace ImmExporter
{

	namespace tiLayerSpawnArea
	{
		bool ExportData(piOStream *fp, LayerImplementation imp)
		{
            LayerSpawnArea* lv = (LayerSpawnArea*)imp;
            LayerSpawnArea::Volume::Type type = lv->GetVolumeType();
            vec4 sph;
            bound3 bnd;
            constexpr uint32_t kVersion = 2;
            // version 1 adds all data and thumbnail (spaces)
            // version 2 changes swap of RB on thumbnail asset

            fp->WriteUInt32(kVersion);

            constexpr uint8_t kReserved = 0;
            fp->WriteUInt8(kReserved);
            fp->WriteUInt8(kReserved);
            fp->WriteUInt8(kReserved);

            fp->WriteUInt8((uint8_t)type);
            switch (type)
            {
            case LayerSpawnArea::Volume::Type::Sphere:
                sph = lv->GetVolumeSphere();
                fp->WriteFloat(sph.x);
                fp->WriteFloat(sph.y);
                fp->WriteFloat(sph.z);
                fp->WriteFloat(sph.w);
                break;

            case LayerSpawnArea::Volume::Type::Box:
                bnd = lv->GetVolumeBox();
                fp->WriteFloat(bnd.mMinX);
                fp->WriteFloat(bnd.mMaxX);
                fp->WriteFloat(bnd.mMinY);
                fp->WriteFloat(bnd.mMaxY);
                fp->WriteFloat(bnd.mMinZ);
                fp->WriteFloat(bnd.mMaxZ);
                break;

            default:
                piAssert(false);
                break;
            }

            fp->WriteUInt32(
                ((lv->GetTranslationX() ? 0x1 : 0)) |
                ((lv->GetTranslationY() ? 0x2 : 0)) |
                ((lv->GetTranslationZ() ? 0x4 : 0))
            );

            // NOTE: These are important so current version of QuillPlayer will run in Spaces
            fp->WriteUInt8(lv->GetScreenshot()!=nullptr); // Has screenshot
            fp->WriteUInt8(1); // Is exported (always true)
            fp->WriteUInt8(lv->GetTracking() == LayerSpawnArea::TrackingLevel::Floor ? 1 : 0); // Is floor level spawn area (Not used in Spaces)

            return true;
		}

        bool ExportAsset(piOStream *fp, LayerImplementation imp)
        {
            LayerSpawnArea* lv = (LayerSpawnArea*)imp;

            const int channel = 0;
            const piImage *image = lv->GetScreenshot();
            piImage tmp;
            bool wasTmpCreated = false;
            if (image == nullptr)
            {
                // if there is no thumbnail, create a blank one
                piImage::Format formats[1] = { piImage::FORMAT_I_RGB };
                if (!tmp.Init(piImage::TYPE_2D, 512, 512, 1, 1, formats))
                    return false;
                memset(tmp.GetData(0), 0, tmp.GetDataSize(0));
                image = &tmp;
                wasTmpCreated = true;
            }

            const int xres = image->GetXRes();
            const int yres = image->GetYRes();
            const piImage::Format format = image->GetFormat(channel);
            bool success = false;
            uint64_t preOffset;
            // NOTE: 512x512 RGB - Always use JPG. Leaving the PNG export code for possible future support.
                if (format == piImage::Format::FORMAT_I_RGB)
                {
                fp->WriteUInt32(static_cast<uint32_t>(AssetFormat::JPG));
                preOffset = fp->Tell();
                fp->WriteUInt64(0); // size
                success = image->WriteToStream(*fp,0,L"jpg");
                }
                else
                {
                    fp->WriteUInt32(static_cast<uint32_t>(AssetFormat::PNG));
                    preOffset = fp->Tell();
                    fp->WriteUInt64(0); // size
                    success = image->WriteToStream(*fp,0,L"png");
                }

            if (wasTmpCreated)
                tmp.Free();

            if (!success)
                return false;

            uint64_t postOffset = fp->Tell();

            fp->Seek(preOffset, piOStream::SeekMode::SET);
            fp->WriteUInt64(postOffset - preOffset);
            fp->Seek(postOffset, piOStream::SeekMode::SET);


            return true;
        }
	}
}
