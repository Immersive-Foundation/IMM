#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libBasics/piStreamFileO.h"
#include "libCore/src/libWave/piWave.h"
#include "libCore/src/libWave/formats/piWaveWAV.h"
#include "libCore/src/libWave/formats/piWaveOGG.h"
#include "libCore/src/libWave/formats/piWaveOPUS.h"

#include "../document/layer.h"
#include "../document/layerSound.h"
#include "toImmersiveUtils.h"
#include "toImmersiveLayerSound.h"
using namespace ImmCore;

namespace ImmExporter
{
	namespace tiLayerSound
	{
		bool ExportData(piOStream *fp, LayerImplementation imp)
		{
			LayerSound* lp = (LayerSound*)imp;

            LayerSound::AttenuationParameters atten;
            lp->GetAttenuation(&atten);

            LayerSound::ModifierParameters modif;
            lp->GetModifier(&modif);

            if (false)
            {
                fp->WriteUInt32(0); // version
                fp->WriteFloat(0.0f);//lp->GetDuration());
                fp->WriteFloat(lp->GetGain());
                fp->WriteFloat(atten.mMin);
                fp->WriteFloat(atten.mMax);
                fp->WriteUInt8(static_cast<uint8_t>(atten.mType));
                fp->WriteUInt8(lp->GetLooping() ? 1 : 0);
                fp->WriteUInt8(static_cast<uint8_t>(lp->GetType()));
                fp->WriteUInt8(0);  // lp->GetPlaying() ? 1 : 0
            }
            else
            {
                fp->WriteUInt32(1); // version

                fp->WriteUInt8(static_cast<uint8_t>(lp->GetType()));
                fp->WriteFloat(0.0f);
                fp->WriteFloat(lp->GetGain());
                fp->WriteUInt8(lp->GetLooping() ? 1 : 0);
                fp->WriteUInt8(0);

                fp->WriteUInt8(static_cast<uint8_t>(atten.mType));
                fp->WriteFloat(atten.mMin);
                fp->WriteFloat(atten.mMax);

                fp->WriteUInt8(static_cast<uint8_t>(modif.mType));
                if (modif.mType == LayerSound::ModifierType::DirectionalCone)
                {
                    fp->WriteFloat(modif.mParams.mDirectionalCone.mAngleIn);
                    fp->WriteFloat(modif.mParams.mDirectionalCone.mAngleBand);
                    fp->WriteFloat(modif.mParams.mDirectionalCone.mAttenOut);
                }
                else if (modif.mType == LayerSound::ModifierType::DirectionalFrus)
                {
                    fp->WriteFloat(modif.mParams.mDirectionalFrus.mAngleInX);
                    fp->WriteFloat(modif.mParams.mDirectionalFrus.mAngleInY);
                    fp->WriteFloat(modif.mParams.mDirectionalFrus.mAngleBand);
                    fp->WriteFloat(modif.mParams.mDirectionalFrus.mAttenOut);
                }
            }

			return true;
		}

		bool ExportAsset(piOStream *fp, LayerImplementation imp, const AudioType audioType, int opusBitrate)
		{
			LayerSound* lp = (LayerSound*)imp;

            const piWav *sound = lp->GetSound();

            piTArray<uint8_t> data;

            if(audioType == AudioType::OPUS)
            {
                if (!WriteOPUSToMemory(&data, sound, opusBitrate))
                {
                    data.End();
                    return false;
                }
                fp->WriteUInt32(static_cast<uint32_t>(AssetFormat::OPUS));
                fp->WriteUInt32(sound->mNumChannels);
                iWriteBlob(fp, data.GetAddress(0), data.GetLength());
                data.End();
            }
            else if (audioType == AudioType::OGG)
            {
                if (!WriteOGGToMemory(&data, sound))
                {
                    data.End();
                    return false;
                }
                fp->WriteUInt32(static_cast<uint32_t>(AssetFormat::OGG));
                iWriteBlob(fp, data.GetAddress(0), data.GetLength());
                data.End();
            }

			return true;
		}

	}

}
