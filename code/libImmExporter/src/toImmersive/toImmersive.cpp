//#include "libDataUtils/piUOIntMap.h"
#include "libImmCore/src/libBasics/piStreamO.h"
#include "libImmCore/src/libBasics/piStreamFileO.h"
#include "libImmCore/src/libBasics/piStreamArrayO.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "../document/layerPaint/element.h"
#include "../document/layer.h"
#include "../document/layerPaint.h"
#include "../document/layerModel3d.h"
#include "../document/layerPicture.h"
#include "../document/layerSpawnArea.h"
#include "../document/sequence.h"

#include "toImmersiveLayer.h"
#include "toImmersiveLayerPaint.h"
#include "toImmersiveLayerPicture.h"
#include "toImmersiveLayerModel.h"
#include "toImmersiveLayerSpawnArea.h"
#include "toImmersiveUtils.h"
#include "toImmersive.h"
using namespace ImmCore;
namespace ImmExporter
{

	bool iExportSequence(piOStream *fp, Sequence* sq, int iOpusBitRate, tiLayerSound::AudioType iAudioType, std::function<void(float)> onProgress = nullptr)
	{

		piArray delayed;

		if (!delayed.Init(1024, sizeof(tiLayer::DelayedSerialization), false))
			return false;


		fp->WriteUInt64(0x76697372656d6d49); // Signature "Immersiv"
		fp->WriteUInt64(4);                  // size: 4 bytes
		{
			fp->WriteUInt32(0x00010001);         // file format version 16.16
		}

		fp->WriteUInt64(0x79726f6765746143); // "Category"
		fp->WriteUInt64(4);                  // size: 4 bytes
		{
			Sequence::Type sqType;
			bool           sqHasSound;
			int			   numSpawnAreas;
			sq->GetInfo(&sqType, &sqHasSound, &numSpawnAreas);

			uint8_t t = 0;
			if (sqType == Sequence::Type::Still) t = 0;
			if (sqType == Sequence::Type::Animated) t = 1;
			if (sqType == Sequence::Type::Comic) t = 2;

			uint8_t caps = 0;

            //caps |= 1; // mIMM.canGrab
			if (sqHasSound) caps |= 2;

			fp->WriteUInt8(t);
			fp->WriteUInt8(caps);                   // Caps: bit 0:can be grabbed
			fp->WriteUInt16(0);                  // dummy
		}

		fp->WriteUInt64(0x73795364726f6f43); // "CoordSys"
		fp->WriteUInt64(2);                  // size: 2 bytes
		{
			fp->WriteUInt8(0);                   // Units:  0=meters
			fp->WriteUInt8(0);                   // Axes:   0={X=right, Y=up, Z=
			//fp.WriteUInt8(0);                  // Origin: 0=floor, 1:viewer
		}

		/*
		fp.WriteUInt64(0x202020726f6c6f43); // "Color   "
		fp.WriteUInt64(0);                  // size: 0 bytes
		*/

		/*
		fp.WriteUInt64(0x656e696c656d6954); // "Timeline"
		fp.WriteUInt64(0);                  // size: 0 bytes
		*/

		// tabla de sequence offsets


		//-------------------------------------
		{
			fp->WriteUInt64(0x65636e6575716553); // "Sequence"
			const uint64_t seqSizeOffset = iWriteForward64(fp);

			iWriteString(fp, sq->GetInitialSpawnArea()->GetFullName());
			iWriteVec3f(fp, sq->GetBackgroundColor());

			Layer* root = sq->GetRoot();

			if (!tiLayer::ExportLayer(fp, root, &delayed, sq->GetFrameRate()))
				return false;

			iWriteBack64(fp, seqSizeOffset, fp->Tell() - seqSizeOffset - 8);
		}
		//-------------------------------------
		{
			const uint64_t num = delayed.GetLength();
			fp->WriteUInt64(0x656c626174736552); // "Restable"
			fp->WriteUInt64(num*(4 + 8 + 8) + 4);
			fp->WriteUInt32(static_cast<uint32_t>(num));
			for (uint64_t i = 0; i < num; i++)
			{
				tiLayer::DelayedSerialization *ds = (tiLayer::DelayedSerialization*)delayed.GetAddress(i);
				fp->WriteUInt32( static_cast<uint32_t>(ds->mLayer->GetID())); // layer ID
				ds->mTableOffset = fp->Tell();
				fp->WriteUInt64(0); // offset
				fp->WriteUInt64(0); // size
			}
		}
		//-------------------------------------
		{
			fp->WriteUInt64(0x656372756f736552); // "Resource"
			const uint64_t resSizeOffset = iWriteForward64(fp);

			const uint64_t num = delayed.GetLength();
			for (uint64_t i = 0; i < num; i++)
			{
				const tiLayer::DelayedSerialization* ds = (tiLayer::DelayedSerialization*)delayed.GetAddress(i);
				const Layer::Type lt = ds->mLayer->GetType();
				LayerImplementation imp = ds->mLayer->GetImplementation();

				const uint64_t assetOffset = fp->Tell();

				bool res = true;
					    if (lt == Layer::Type::Model)     res = tiLayerModel::ExportAsset(fp, imp);
				else if (lt == Layer::Type::Paint)     res = tiLayerPaint::ExportAsset(fp, imp);
				else if (lt == Layer::Type::Picture)   res = tiLayerPicture::ExportAsset(fp, imp);
				else if (lt == Layer::Type::Sound)     res = tiLayerSound::ExportAsset(fp, imp, iAudioType, iOpusBitRate);
				else if (lt == Layer::Type::SpawnArea) res = tiLayerSpawnArea::ExportAsset(fp, imp);

                if (!res)
                    return false;

				const uint64_t curr = fp->Tell();
				const uint64_t assetSize = curr - assetOffset;
				fp->Seek(ds->mTableOffset, piOStream::SeekMode::SET);
				fp->WriteUInt64(assetOffset);
				fp->WriteUInt64(assetSize);
				fp->Seek(curr, piOStream::SeekMode::SET);

				if(onProgress)
					onProgress(float(i)*100.0f / float(num-1));
			}
			iWriteBack64(fp, resSizeOffset, fp->Tell() - resSizeOffset - 8);
		}
		//------------------------------

		delayed.End();
		return true;
	}

    bool ExportToFile(const char* iFileName, ImmExporter::Sequence* Sequence, int iOpusBitRate, ImmExporter::tiLayerSound::AudioType iAudioType, std::function<void(float)> onProgress)
    {
        piFile file;
        if (!file.Open(pistr2ws(iFileName), L"wb"))
            return false;
        piOStreamFile fileStream(&file);

        return iExportSequence(&fileStream, Sequence, iOpusBitRate, iAudioType, onProgress);
    }

    bool ExportToMemory(piTArray<uint8_t>* iBuffer, Sequence* Sequence, int iOpusBitRate, tiLayerSound::AudioType iAudioType, piLog* log, std::function<void(float)> onProgress)
    {
        piOStreamArray mOutStream(iBuffer);
        return iExportSequence(&mOutStream, Sequence, iOpusBitRate, iAudioType, onProgress);
    }

}
