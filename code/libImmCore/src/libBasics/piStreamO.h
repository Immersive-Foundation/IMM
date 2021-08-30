//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore
{

	class piOStream
	{
	public:
		piOStream() {}
		virtual ~piOStream() {}

		enum class SeekMode : int
		{
			CURRENT = 0,
			END = 1,
			SET = 2
		};

		virtual bool     Seek(uint64_t pos, SeekMode mode) = 0;
		virtual uint64_t Tell(void) = 0;

		//void         Prints(const wchar_t *str);
		//void         Printf(const wchar_t *format, ...);

		virtual void     Write(const void *dst, uint64_t amount) = 0;
		virtual void	 WriteUInt8(uint8_t x) = 0;
		virtual void     WriteUInt16(uint16_t x) = 0;
		virtual void     WriteUInt32(uint32_t x) = 0;
		virtual void     WriteUInt64(uint64_t x) = 0;
		virtual void     WriteFloat(float x) = 0;
		virtual void	 WriteUInt8array(const uint8_t *dst, uint64_t amount) = 0;
		virtual void	 WriteUInt16array(const uint16_t *dst, uint64_t amount) = 0;
		virtual void	 WriteUInt32array(const uint32_t *dst, uint64_t amount) = 0;
		virtual void	 WriteUInt64array(const uint64_t *dst, uint64_t amount) = 0;
		virtual void	 WriteFloatarray(const float *ori, uint64_t amout) = 0;
		virtual void	 WriteDoublearray(const double *ori, uint64_t amout) = 0;
	};

}
