#include "../document/layer.h"
#include "../document/layerPicture.h"
#include "toImmersiveUtils.h"
#include "toImmersiveLayerPicture.h"

using namespace ImmCore;
namespace ImmExporter
{

	namespace tiLayerPicture
	{
		bool ExportData(piOStream *fp, LayerImplementation imp)
		{
			LayerPicture* lp = (LayerPicture*)imp;

			fp->WriteUInt32(0); // version
			fp->WriteUInt16(static_cast<int>(lp->GetType()));
			fp->WriteUInt8(lp->GetIsViewerLocked() ? 1 : 0);
			fp->WriteUInt8(0);
			return true;
		}

		bool ExportAsset(piOStream *fp, LayerImplementation imp)
		{
			LayerPicture* lp = (LayerPicture*)imp;

            const int channel = 0;
            const piImage *image = lp->GetImage();
            const int xres = image->GetXRes();
            const int yres = image->GetYRes();
            const piImage::Format format = image->GetFormat(channel);


            // do JPG. Unless image is very small or contains alpha, in which case we go with PNG
            AssetFormat fmt = AssetFormat::JPG;
            if ((xres < 32 && yres < 32) || format == piImage::Format::FORMAT_I_RGBA)
            {
                fmt = AssetFormat::PNG;
            }

			fp->WriteUInt32(static_cast<uint32_t>(fmt));

            const uint64_t preOffset = fp->Tell();
            fp->WriteUInt64(0);

            if (fmt == AssetFormat::PNG)
            {
                if (!image->WriteToStream(*fp,0,L"png"))
                    return false;
            }
            else if (fmt == AssetFormat::JPG)
            {
                if (!image->WriteToStream(*fp,0,L"jpg"))
                    return false;
            }
            else
            {
                return false;
            }
            uint64_t postOffset = fp->Tell();

            fp->Seek(preOffset, piOStream::SeekMode::SET);
            fp->WriteUInt64(postOffset - preOffset);
            fp->Seek(postOffset, piOStream::SeekMode::SET);

			return true;
		}
	}

}
