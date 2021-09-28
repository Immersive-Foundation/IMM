#include "libImmCore/src/libBasics/piLog.h"
#include "libImmCore/src/libBasics/piStr.h"
#include "libImmCore/src/libWave/piWave.h"
#include "libImmCore/src/libWave/formats/piWaveWAV.h"
#include "libImmCore/src/libWave/formats/piWaveOGG.h"
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
                piTArray<uint8_t> data;
                const uint64_t    size = fp->ReadUInt64();
                // TODO: use ReadWAVFromFile() instead to avoid copying data to memory
                if( !data.Init(size, true) )
                    return false;
                fp->ReadUInt8array(data.GetAddress(0), size);
                if (!ReadWAVFromMemory(me->GetSound(), &data))
                    return false;
                data.End();
            }
            else if (format == AssetFormat::OGG)
            {
                piTArray<uint8_t> data;
                const uint64_t    size = fp->ReadUInt64();
                if( !data.Init(size, true) )
                    return false;
                fp->ReadUInt8array(data.GetAddress(0), size);
                if (!ReadOGGFromMemory(me->GetSound(), &data))
                    return false;
                data.End();
            }
            else if (format == AssetFormat::OPUS)
            {
                const int numChannels = fp->ReadUInt32();                    
                const uint64_t    size = fp->ReadUInt64();
                uint8_t* opus = (uint8_t *)malloc(size);
                if (opus == nullptr)
                    return false;
                fp->ReadUInt8array(opus, size);                    

                // Hack: put the compressed opus blob into a piWav structure, even if it's not a wav
                piWav *snd = me->GetSound();
                snd->Make( 0, numChannels, 8, opus, size );
                me->SetCompressed(true); // mark this sound layer as having a blob inside piWav rather than an actual wave
            }
            else
            {
                return false;
            }

            return true;
        }


    }

}
