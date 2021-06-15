//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <string.h>

#include "TBE_AudioEngine.h"
#include "TBE_AudioObject.h"

#include "../../libBasics/piDebug.h"
#include "../../libBasics/piTypes.h"
#include "../../libBasics/piFile.h"

#include "piSoundEngineAudioSDKBackend.h"


namespace ImmCore {

    class MemStream : public TBE::IOStream
    {

    public:

        MemStream(uint64_t length, const void *buffer) : mBuffer(buffer), mSize(length)
        {
            mPosition = 0;
            mClosed = false;
        }

        ~MemStream() {}

        size_t read(void* data, size_t numBytes)
        {
            if (mClosed) return 0;

            uint8_t *src = ((uint8_t*)mBuffer);
            uint8_t *dst = ((uint8_t*)data);

            if (mPosition + numBytes > mSize)
                numBytes = mSize - mPosition;

            memcpy(dst, src+mPosition, numBytes);
            mPosition += numBytes;

            return numBytes;
        }
        size_t write(void* data, size_t numBytes) { return 0; }
        size_t getPosition() { return mPosition; }
        bool setPosition(int64_t pos) { mPosition = pos; return true; }
        bool setPosition(int64_t pos, int mode) { if (mode == 0) mPosition = pos; else if (mode == 1) mPosition += pos; else if (mode == 2) mPosition = mSize  - pos; return true;
        }
        int32_t pushBackByte(int c) { return c; }
        size_t getSize() { return mSize; }
        int getFD() { return -1; }
        bool canSeek() { return true; }
        bool ready() const { return !mClosed; }
        bool endOfStream() { return mPosition >= mSize; }
        void close() { mClosed = true; }
    private:
        const void *mBuffer;
    private:
        uint64_t mSize;
        uint64_t mPosition;
        bool mClosed = false;
    };


}
