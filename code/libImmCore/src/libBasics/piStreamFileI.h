//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piStreamI.h"
#include "piFile.h"

namespace ImmCore
{

	class piIStreamFile : public piIStream
	{
	public:
        static constexpr uint64_t kDefaultFileSize = 0xffffffffffffffff;

        piIStreamFile(piFile *file, uint64_t maxSize) :piIStream()
        { 
            mFile = file; 
            mPos = 0; 
            mFileOrigin = file->Tell();
            mMaxSize = (maxSize!=kDefaultFileSize) ? maxSize : file->GetLength() - mFileOrigin;
        }
		~piIStreamFile() {}

        bool Seek(uint64_t pos, SeekMode mode)
        { 
                 if (mode == SeekMode::SET)     { mPos = pos; }
            else if (mode == SeekMode::CURRENT) { mPos = mPos + pos; }
            else                                { mPos = mMaxSize - pos; }
            return mFile->Seek(mFileOrigin + mPos, piFile::SeekMode::SET);

        }
        uint64_t     Tell(void) { return mPos; }
		uint64_t     Read(void *dst, uint64_t amount)
        {
            const uint64_t sReadAmount = (mPos + amount < mMaxSize) ? amount : mMaxSize - mPos;
            const uint64_t fReadAmount = mFile->Read(dst, sReadAmount);
            mPos += fReadAmount;
            return fReadAmount;
        }
        uint8_t		 ReadUInt8(void) { uint8_t res = mFile->ReadUInt8(); mPos += 1U; return res; }
        uint16_t     ReadUInt16(void) { uint16_t res = mFile->ReadUInt16(); mPos += 2U; return res; }
        uint32_t	 ReadUInt32(void) { uint32_t res = mFile->ReadUInt32(); mPos += 4U; return res; }
        uint64_t	 ReadUInt64(void) { uint64_t res = mFile->ReadUInt64(); mPos += 8U; return res; }
        float		 ReadFloat(void) { float res = mFile->ReadFloat(); mPos += 4U; return res; }
        double		 ReadDouble(void) { double res = mFile->ReadDouble(); mPos += 8U; return res; }
        void		 ReadUInt8array(uint8_t *dst, uint64_t amount) { mFile->ReadUInt8array(dst, amount); mPos += amount*1U; }
        void		 ReadUInt16array(uint16_t *dst, uint64_t amount) { mFile->ReadUInt16array(dst, amount); mPos += amount * 2U; }
        void		 ReadUInt32array(uint32_t *dst, uint64_t amount) { mFile->ReadUInt32array(dst, amount); mPos += amount * 4U; }
        void		 ReadUInt64array(uint64_t *dst, uint64_t amount) { mFile->ReadUInt64array(dst, amount); mPos += amount * 8U; }
        void		 ReadFloatarray(float *dst, uint64_t amount) { mFile->ReadFloatarray2(dst, amount); mPos += amount * 4U; }
        void		 ReadDoublearray(double *dst, uint64_t amount) { mFile->ReadDoublearray2(dst, amount); mPos += amount * 8U; }

	private:
		piFile  *mFile;
        uint64_t mFileOrigin;
        uint64_t mPos;
        uint64_t mMaxSize;
	};
}
