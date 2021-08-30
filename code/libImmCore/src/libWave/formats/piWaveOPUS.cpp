#include <malloc.h>
#include <string.h>


#include "../piWave.h"
#include "../../libBasics/piTArray.h"
#include "../../libBasics/piStreamI.h"
#include "../../libBasics/piStreamFileI.h"
#include "../../libBasics/piStreamArrayI.h"
#include "../../libBasics/piStreamO.h"
#include "../../libBasics/piStreamFileO.h"
#include "../../libBasics/piStreamArrayO.h"

#include "opusenc.h"


namespace ImmCore
{
    static int write(void *user_data, const unsigned char *ptr, opus_int32 len) {
        piOStream *fp = (piOStream*)user_data;
        fp->WriteUInt8array(ptr, len);
        return 0;
    }

    static int close(void *user_data) {
        piOStream *fp = (piOStream*)user_data;        
        return 0;
    }

    static const OpusEncCallbacks callbacks = {
      write,
      close
    };

    static bool iConvert_WAVToOPUS(const piWav * wav, piOStream * fp, int bitRate)
    {
        OggOpusEnc *enc;
        OggOpusComments *comments;
        
        // conversion buffer
        opus_int16* buffer16 = nullptr;        
        if (wav->mBits!=16)
        {
            buffer16 = (opus_int16 *)malloc(wav->mNumChannels * wav->mNumSamples * 2);
            if (!buffer16)
                return false;
        }

        int error;
        
        comments = ope_comments_create();
        //ope_comments_add(comments, "ARTIST", "Someone");
        //ope_comments_add(comments, "TITLE", "Some track");
        
        enc = ope_encoder_create_callbacks(&callbacks, fp, comments, wav->mRate, wav->mNumChannels, wav->mNumChannels<3 ? 0 : 2, &error);

        if (!enc) 
        {
            ope_comments_destroy(comments);
            return false;
        }

        if(bitRate!=0)
            ope_encoder_ctl(enc, OPUS_SET_BITRATE(bitRate * wav->mNumChannels)); 
        
        // convert to 16 bits
        if (wav->mBits != 16)
        {
            const size_t totalSamples = wav->mNumSamples * wav->mNumChannels;
            const size_t srcStride = wav->mBits / 8;
            
            size_t srcOffset = 0;
            size_t dstOffset = 0;
            uint8_t* data = (uint8_t*)wav->mData;

            for (int i = 0; i < totalSamples; ++i)
            {                
                switch (wav->mBits)
                {
                case 8:
                {
                    uint16_t sample = (data[srcOffset] << 8);
                    buffer16[dstOffset] = *((opus_int16*)&sample);
                    break;
                }
                case 24:
                {
                    // shift to 16 bits
                    uint16_t sample = (data[srcOffset + 2] << 8) | data[srcOffset + 1];
                    buffer16[dstOffset] = *((opus_int16*)&sample);
                    break;
                }
                case 32:
                {
                    // 32 float
                    float sample32 = ((float*)wav->mData)[dstOffset];
                    buffer16[dstOffset] = opus_int16(sample32 * ((1 << 15) - 1));
                    break;
                }
                }
                dstOffset++;
                srcOffset += srcStride;
            }
            
        }
        error = ope_encoder_write(enc, wav->mBits != 16 ? buffer16 : (opus_int16*)wav->mData,  (int)wav->mNumSamples);        
        
        ope_encoder_drain(enc);
        ope_encoder_destroy(enc);
        ope_comments_destroy(comments);
        
        if (buffer16)
            free(buffer16);

        return error == 0;
        }


    bool WriteOPUSToMemory(piTArray<uint8_t> *data, const piWav *wav, int bitRate)
    {
        if (!data->Init(1024 * 1024, false))
            return false;

        piOStreamArray st(data);

        return iConvert_WAVToOPUS(wav, &st, bitRate);
    }

    bool WriteOPUSToFile(const wchar_t * name, const piWav *wav, int bitRate)
    {
        piFile fp;
        if (!fp.Open(name, L"wb"))
            return false;

        piOStreamFile st(&fp);

        bool res = iConvert_WAVToOPUS(wav, &st, bitRate);

        fp.Close();

        return res;
    }

}
