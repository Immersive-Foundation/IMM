//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <string.h>
#include "../../libBasics/piFile.h"

#include "TBE_AudioEngine.h"
#include "TBE_AudioObject.h"

#include "../../libBasics/piDebug.h"
#include "../../libBasics/piTypes.h"
#include "piSoundEngineAudioSDKBackend.h"


namespace ImmCore {

    class FakeWav : public TBE::IOStream
    {
    private:
        #pragma pack(1)
        struct WavWrapper1
        {
            uint32_t TagRiff;
            uint32_t SizeRiff;
              uint32_t TagWav;
                uint32_t TagFormat;
                uint32_t SizeFormat;
                  uint16_t Format;
                  uint16_t NumChannels;
                  uint32_t Rate;
                  uint32_t BPSec;
                  uint16_t BPSam;
                  uint16_t Bits;
                uint32_t TagData;
                uint32_t SizeData;
        };
        struct WavWrapper2
        {
            uint32_t TagRiff;
            uint32_t SizeRiff;
              uint32_t TagWav;
                uint32_t TagFormat;
                uint32_t SizeFormat;
                  uint16_t Format;
                  uint16_t NumChannels;
                  uint32_t Rate;
                  uint32_t BPSec;
                  uint16_t BPSam;
                  uint16_t Bits;
                uint32_t TagFact;
                uint32_t SizeFact;
                  uint32_t NumSamples;
                uint32_t TagData;
                uint32_t SizeData;
        };
        #pragma pack()
    public:

        FakeWav(const int rate, int bits, int numChannels, uint64_t length, const void *buffer) : mBuffer(buffer)
        {
#ifdef _DEBUG
            ImmCore::piAssert( sizeof(WavWrapper1)==44 );
            ImmCore::piAssert( sizeof(WavWrapper2)==56 );
#endif // _DEBUG



            mPosition = 0;
            mClosed = false;

            if (bits == 32)
            {
                mHeaderSize = sizeof(WavWrapper2);
                mSize = length + mHeaderSize;

                WavWrapper2 *h = (WavWrapper2*)mHeader;

                h->TagRiff = 0x46464952;
                h->SizeRiff = static_cast<uint32_t>(length + 48);
                h->TagWav = 0x45564157;
                h->TagFormat = 0x20746D66;
                h->SizeFormat = 16;
                h->Format = (bits == 32) ? 3 : 1;
                h->NumChannels = numChannels;
                h->Rate = rate;
                h->BPSec = rate * numChannels*bits / 8;
                h->BPSam = bits / 8;                 // note NO numchannels
                h->Bits = bits;
                h->TagFact = 0x74636166;
                h->SizeFact = 4;
                h->NumSamples = static_cast<uint32_t>(length / (bits / 8)); // note NO numchannels
                h->TagData = 0x61746164;
                h->SizeData = static_cast<uint32_t>(length);
            }
            else
            {
                mHeaderSize = sizeof(WavWrapper1);
                mSize = length + mHeaderSize;

                WavWrapper1 *h = (WavWrapper1*)mHeader;

                h->TagRiff = 0x46464952;
                h->SizeRiff = static_cast<uint32_t>(length + 36);
                h->TagWav = 0x45564157;
                h->TagFormat = 0x20746D66;
                h->SizeFormat = 16;
                h->Format = (bits == 32) ? 3 : 1;
                h->NumChannels = numChannels;
                h->Rate = rate;
                h->BPSec = rate * numChannels*bits / 8;
                h->BPSam = numChannels * bits / 8;
                h->Bits = bits;
                h->TagData = 0x61746164;
                h->SizeData = static_cast<uint32_t>(length);
            }

        }
        ~FakeWav() {}
        size_t read(void* data, size_t numBytes)
        { 
            if (mClosed) return 0;
            const uint8_t *src1 = mHeader;
            const uint8_t *src2 = ((uint8_t*)mBuffer);
                    uint8_t *dst = ((uint8_t*)data);

            if(mPosition<mHeaderSize)
            {
                if( (mPosition + numBytes) <= mHeaderSize)
                {
                    memcpy(dst, src1 + mPosition, numBytes);
                    mPosition += numBytes;
                }
                else
                {
                    memcpy(dst, src1 + mPosition, mHeaderSize - mPosition);
                    memcpy(dst + mHeaderSize - mPosition, src2, mPosition + numBytes - mHeaderSize);
                    mPosition += numBytes;
                }
            }
            else
            {
                if (mPosition + numBytes > mSize)
                    numBytes = mSize - mPosition;

                memcpy(dst, src2 + (mPosition- mHeaderSize), numBytes );
                mPosition += numBytes;
            }
            return numBytes;
        }
        size_t write(void* data, size_t numBytes) { return 0; }
        size_t getPosition() { return mPosition; }
        bool setPosition(int64_t pos) { mPosition = pos; return true; }
        bool setPosition(int64_t pos, int mode) { if( mode==0) mPosition = pos; else if( mode==1) mPosition += pos; else if( mode==2 ) mPosition = mSize-pos; return true; }
        int32_t pushBackByte(int c) { return c; }
        size_t getSize() { return mSize; }
        int getFD() { return -1; }
        bool canSeek() { return true; }
        bool ready() const { return true; }
        bool endOfStream() { return (mPosition>=mSize); }
        void close() { mClosed = true; }
    private:
        const void *mBuffer;
    private:        
        uint64_t mSize;
        uint64_t mPosition;
        uint32_t mHeaderSize;
        uint8_t  mHeader[128]; // can hold both PCM and FLOAT headers
        bool mClosed = false;
    };


}
