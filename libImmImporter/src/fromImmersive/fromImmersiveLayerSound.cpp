#include "libCore/src/libBasics/piLog.h"
#include "libCore/src/libBasics/piStr.h"
#include "libCore/src/libWave/piWave.h"
#include "libCore/src/libWave/formats/piWaveWAV.h"
#include "libCore/src/libWave/formats/piWaveOGG.h"
#include "../document/layer.h"
#include "../document/layerSound.h"
#include "fromImmersiveUtils.h"

using namespace ImmCore;

namespace ImmImporter
{

    namespace fiLayerSound
    {

        LayerImplementation ReadData(piIStream *fp, piLog* log)
        {
            LayerSound *me = new LayerSound();
            if (!me) return nullptr;

            LayerSound::ModifierParameters modifier;
            LayerSound::AttenuationParameters attenuation;
            float duration;
            float volume;
            bool isLooping;
            LayerSound::Type type;
            bool isPlaying;

            const uint32_t version = fp->ReadUInt32(); // version
            if (version == 0)
            {
                duration = fp->ReadFloat();
                volume = fp->ReadFloat();
                attenuation.mMin = fp->ReadFloat();
                attenuation.mMax = fp->ReadFloat();
                attenuation.mType = static_cast<LayerSound::AttenuationType>(fp->ReadUInt8());
                isLooping = (fp->ReadUInt8() == 1);
                type = static_cast<LayerSound::Type>(fp->ReadUInt8());
                isPlaying = (fp->ReadUInt8() == 1);
                modifier.mType = LayerSound::ModifierType::None;
            }
            else if (version == 1)
            {
                type = static_cast<LayerSound::Type>(fp->ReadUInt8());
                duration = fp->ReadFloat();
                volume = fp->ReadFloat();
                isLooping = (fp->ReadUInt8() == 1);
                isPlaying = (fp->ReadUInt8() == 1);

                attenuation.mType = static_cast<LayerSound::AttenuationType>(fp->ReadUInt8());
                attenuation.mMin = fp->ReadFloat();
                attenuation.mMax = fp->ReadFloat();

                modifier.mType = static_cast<LayerSound::ModifierType>(fp->ReadUInt8());
                if (modifier.mType == LayerSound::ModifierType::DirectionalCone)
                {
                    modifier.mParams.mDirectionalCone.mAngleIn = fp->ReadFloat();
                    modifier.mParams.mDirectionalCone.mAngleBand = fp->ReadFloat();
                    modifier.mParams.mDirectionalCone.mAttenOut = fp->ReadFloat();
                }
                else if (modifier.mType == LayerSound::ModifierType::DirectionalFrus)
                {
                    modifier.mParams.mDirectionalFrus.mAngleInX = fp->ReadFloat();
                    modifier.mParams.mDirectionalFrus.mAngleInY = fp->ReadFloat();
                    modifier.mParams.mDirectionalFrus.mAngleBand = fp->ReadFloat();
                    modifier.mParams.mDirectionalFrus.mAttenOut = fp->ReadFloat();
                }
            }
            else
            {
                return nullptr;
            }

            if (!me->Init(isLooping, isPlaying, volume, attenuation, modifier, type, duration, log))
                return nullptr;

            return me;
        }



        bool ReadAsset(LayerImplementation vme, piIStream *fp, piLog* log)
        {
            LayerSound *me = (LayerSound*)vme;

            const AssetFormat format = static_cast<AssetFormat>(fp->ReadUInt32());                

            if (format == AssetFormat::WAV)
            {
                piTArray<uint8_t> wav;
                const uint64_t    size = fp->ReadUInt64();
                wav.Init(size, true);
                fp->ReadUInt8array(wav.GetAddress(0), size);
                if (!ReadWAVFromMemory(me->GetSound(), &wav))
                    return false;
            }
            else if (format == AssetFormat::OGG)
            {
                piTArray<uint8_t> ogg;
                const uint64_t    size = fp->ReadUInt64();
                ogg.Init(size, true);
                fp->ReadUInt8array(ogg.GetAddress(0), size);
                if (!ReadOGGFromMemory(me->GetSound(), &ogg))
                    return false;
            }
            else if (format == AssetFormat::OPUS)
            {
                me->GetSound()->mNumChannels = fp->ReadUInt32();                    
                const uint64_t    size = fp->ReadUInt64();
                uint8_t* opus = (uint8_t *)malloc(size);
                if (opus == nullptr)
                    return false;
                me->GetSound()->mData = opus;
                me->GetSound()->mDataSize = size;
                fp->ReadUInt8array(opus, size);                    
                me->SetCompressed(true);
            }
            else
            {
                return false;
            }

            return true;
        }


    }

}
