//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../../libBasics/piTypes.h"

namespace ImmCore {

	namespace piBitInterlacer
	{
		class BitStream
		{
		public:
			BitStream();
			bool output(int bit, uint8_t *val);
			bool flush(uint8_t *val);

		private:
			int mBits;
			int mState;
		};


		uint64_t  interlace8( uint8_t *dst, const uint8_t  *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits);
		uint64_t  interlace16(uint8_t *dst, const uint16_t *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits);
		uint64_t  interlace32(uint8_t *dst, const uint32_t *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits);

		void deinterlace8( const uint8_t *src, uint8_t  *data, uint64_t numBytes, uint64_t numPoints, unsigned int numChannels, unsigned int numBits);
		void deinterlace16(const uint8_t *src, uint16_t *data, uint64_t numBytes, uint64_t numPoints, unsigned int numChannels, unsigned int numBits);
	}


} // namespace ImmCore
