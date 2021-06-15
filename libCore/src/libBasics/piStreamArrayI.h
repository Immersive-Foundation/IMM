//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piStreamI.h"
#include "piTArray.h"

namespace ImmCore
{

	class piIStreamArray : public piIStream
	{
	public:
		piIStreamArray(piTArray<uint8_t> *array) :piIStream() { mArray = array; }
		~piIStreamArray() {}

        bool Seek(uint64_t pos, SeekMode mode)
        {
            const uint64_t num = mArray->GetLength();
            const uint64_t max = mArray->GetMaxLength();
                 if (mode==SeekMode::SET)       { if(pos    >=max) return false;                  }
            else if (mode == SeekMode::CURRENT) { if(pos+num>=max) return false; pos = num+pos;   }
            else                                { if(pos    >=max) return false; pos = max-1-pos; }
            
            mArray->SetLength(pos);

            return true;
        }

        uint64_t Tell(void) { return mArray->GetLength(); }

		uint64_t Read( void *dst, uint64_t amount)
		{
            const uint64_t size = mArray->GetMaxLength();
			const uint64_t pos = mArray->GetLength();
			const uint64_t bytesRead = (pos + amount > size) ? size-pos : amount;
			memcpy(dst, mArray->GetAddress(pos), bytesRead);
			mArray->SetLength(pos + bytesRead);
			return bytesRead;
			
		}
		uint8_t		 ReadUInt8(void)  { uint8_t  x; Read(&x, sizeof(uint8_t )); return x; }
		uint16_t	 ReadUInt16(void) { uint16_t x; Read(&x, sizeof(uint16_t)); return x; }
		uint32_t	 ReadUInt32(void) { uint32_t x; Read(&x, sizeof(uint32_t)); return x; }
		uint64_t	 ReadUInt64(void) { uint64_t x; Read(&x, sizeof(uint64_t)); return x; }
		float		 ReadFloat(void)  { float    x; Read(&x, sizeof(float   )); return x; }
		double		 ReadDouble(void) { double   x; Read(&x, sizeof(double  )); return x; }
		void		 ReadUInt8array(  uint8_t  *dst, uint64_t amount) { Read(dst, amount*sizeof(uint8_t)); }
		void		 ReadUInt16array( uint16_t *dst, uint64_t amount) { Read(dst, amount*sizeof(uint16_t)); }
		void		 ReadUInt32array( uint32_t *dst, uint64_t amount) { Read(dst, amount*sizeof(uint32_t)); }
		void		 ReadUInt64array( uint64_t *dst, uint64_t amount) { Read(dst, amount*sizeof(uint64_t)); }
		void		 ReadFloatarray(  float    *dst, uint64_t amount) { Read(dst, amount*sizeof(float)); }
		void		 ReadDoublearray( double   *dst, uint64_t amount) { Read(dst, amount*sizeof(double)); }

	private:
		piTArray<uint8_t> *mArray;
	};
}
