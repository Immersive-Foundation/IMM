//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piStreamO.h"
#include "../libBasics/piTArray.h"

namespace ImmCore
{

	class piOStreamArray : public piOStream
	{
	public:
		piOStreamArray(piTArray<uint8_t> *array) :piOStream() { mArray = array; }
		~piOStreamArray() {}

		bool Seek(uint64_t pos, piOStream::SeekMode mode)
		{
			const uint64_t current = mArray->GetLength();
			const uint64_t maxlen = mArray->GetMaxLength();

			uint64_t desired = 0;
			     if (mode == piOStream::SeekMode::SET)     desired = pos;
			else if (mode == piOStream::SeekMode::CURRENT) desired = current + pos;
			else                                           desired = maxlen - 1 - pos;

			if (desired >= maxlen )
			{
				if (!mArray->SetMaxLengthNoShrink(4* desired /3+32)) return false;
			}
			mArray->SetLength(desired);
			return true;
		}

		uint64_t     Tell(void) { return mArray->GetLength(); }

		void         Write(const void *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount, true); }
		void		 WriteUInt8( uint8_t  x) { mArray->Append(x,true); }
		void		 WriteUInt16(uint16_t x) { mArray->Append((uint8_t*)&x, 2, true); }
		void		 WriteUInt32(uint32_t x) { mArray->Append((uint8_t*)&x, 4, true); }
		void		 WriteUInt64(uint64_t x) { mArray->Append((uint8_t*)&x, 8, true); }
		void		 WriteFloat( float    x) { mArray->Append((uint8_t*)&x, 4, true); }
		void		 WriteUInt8array(  const uint8_t  *dst, uint64_t amount) { mArray->Append(dst,amount,true); }
		void		 WriteUInt16array( const uint16_t *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount*2, true); }
		void		 WriteUInt32array( const uint32_t *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount*4, true); }
		void		 WriteUInt64array( const uint64_t *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount*8, true); }
		void		 WriteFloatarray(  const float    *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount*4, true); }
		void		 WriteDoublearray( const double   *dst, uint64_t amount) { mArray->Append((uint8_t*)dst, amount*8, true); }

	private:
		piTArray<uint8_t> *mArray;
	};
}
