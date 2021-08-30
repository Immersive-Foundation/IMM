//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include "../../libBasics/piTypes.h"
#include "piBitInterlacer.h"

namespace ImmCore
{
	namespace piBitInterlacer
	{

		BitStream::BitStream()
		{
			mBits = 0;
			mState = 0;
		}

		bool BitStream::output(int bit, uint8_t *res)
		{
			mState |= (bit << mBits);

			mBits++;
			if (mBits == 8)
			{
				*res = mState;
				mBits = 0;
				mState = 0;
				return true;
			}
			return false;
		}

		bool BitStream::flush(uint8_t *val)
		{
			if (mBits != 0)
			{
				*val = mState;
				return true;
			}
			return false;
		}

		uint64_t interlace8(uint8_t *dst, const uint8_t *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits)
		{
			if (numPoints == 0) return 0;

			// interlace all bits, sort bits from lower to higher bits across channels
			uint64_t num = 0;
			BitStream bit;
			for (int b = 0; b < numBits; b++)
			for (int j = 0; j < numChannels; j++)
			for (int i = 0; i < numPoints; i++)
			{
				uint8_t res;
				if (bit.output((data[numChannels*i + j] >> b) & 1, &res))
					dst[num++] = res;
			}

			uint8_t res; if (bit.flush(&res)) dst[num++] = res;

			// remove trailing zeroes
			uint64_t len = 0;
			for (int64_t i = num - 1; i >= 0; i--)
			{
				if (dst[i] != 0)
				{
					len = i + 1;
					break;
				}
			}


			return len;
		}

		uint64_t interlace16(uint8_t *dst, const uint16_t *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits)
		{
			if (numPoints == 0) return 0;

			// interlace all bits, sort bits from lower to higher bits across channels
			uint64_t num = 0;
			BitStream bit;
			for (int b = 0; b < numBits; b++)
			for (int j = 0; j < numChannels; j++)
			for (int i = 0; i < numPoints; i++)
			{
				uint8_t res;
				if (bit.output((data[numChannels*i + j] >> b) & 1, &res))
					dst[num++] = res;
			}

			uint8_t res; if (bit.flush(&res)) dst[num++] = res;

			// remove trailing zeroes
			uint64_t len = 0;
			for (int64_t i = num - 1; i >= 0; i--)
			{
				if (dst[i] != 0)
				{
					len = i + 1;
					break;
				}
			}


			return len;
		}

		uint64_t interlace32(uint8_t *dst, const uint32_t *data, uint64_t numPoints, uint64_t numChannels, uint64_t numBits)
		{
			if (numPoints == 0) return 0;

			// interlace all bits, sort bits from lower to higher bits across channels
			uint64_t num = 0;
			BitStream bit;
			for (int b = 0; b < numBits; b++)
				for (int j = 0; j < numChannels; j++)
					for (int i = 0; i < numPoints; i++)
					{
						uint8_t res;
						if (bit.output((data[numChannels*i + j] >> b) & 1, &res))
							dst[num++] = res;
					}

			uint8_t res; if (bit.flush(&res)) dst[num++] = res;

			// remove trailing zeroes
			uint64_t len = 0;
			for (int64_t i = num - 1; i >= 0; i--)
			{
				if (dst[i] != 0)
				{
					len = i + 1;
					break;
				}
			}

			return len;
		}

		void deinterlace8(const uint8_t *src, uint8_t *data, uint64_t numBytes, uint64_t numPoints, unsigned int numChannels, unsigned int numBits)
		{
			for (uint64_t i = 0, num = numPoints*numChannels; i < num; i++)
			{
				data[i] = 0;
			}

			unsigned int bit = 0;
			unsigned int channel = 0;
			uint64_t point = 0;

			for (uint64_t i = 0; i < numBytes; i++)
			{
				const uint8_t byte = src[i];

				for (int j = 0; j < 8; j++)
				{
					const int b = (byte >> j) & 1;

					data[numChannels*point + channel] |= (b << bit);
					point++;
					if (point >= numPoints)
					{
						point = 0;
						channel++;
						if (channel >= numChannels)
						{
							channel = 0;
							bit++;
							if (bit >= numBits)
							{
								bit = 0;
								return;
							}
						}
					}
				}
			}
		}

		void deinterlace16(const uint8_t *src, uint16_t *data, uint64_t numBytes, uint64_t numPoints, unsigned int numChannels, unsigned int numBits)
		{
			for (uint64_t i = 0, num = numPoints*numChannels; i < num; i++)
			{
				data[i] = 0;
			}

			unsigned int bit = 0;
			unsigned int channel = 0;
			uint64_t point = 0;

			for (uint64_t i = 0; i < numBytes; i++)
			{
				const uint8_t byte = src[i];

				for (int j = 0; j < 8; j++)
				{
					const unsigned int b = (byte >> j) & 1;

					data[numChannels*point + channel] |= (b << bit);
					point++;
					if (point >= numPoints)
					{
						point = 0;
						channel++;
						if (channel >= numChannels)
						{
							channel = 0;
							bit++;
							if (bit >= numBits)
							{
								bit = 0;
								return;
							}
						}
					}
				}
			}
		}
	}
}
