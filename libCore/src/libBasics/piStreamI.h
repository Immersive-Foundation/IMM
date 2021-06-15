//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore
{

	class piIStream
	{
	public:
		piIStream() {}
		virtual ~piIStream() {}

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
		virtual uint64_t Read(void *dst, uint64_t amount) = 0;
		virtual uint8_t	 ReadUInt8(void) = 0;
		virtual uint16_t ReadUInt16(void) = 0;
		virtual uint32_t ReadUInt32(void) = 0;
		virtual uint64_t ReadUInt64(void) = 0;
		virtual float    ReadFloat(void) = 0;
		virtual double   ReadDouble(void) = 0;
		virtual void	 ReadUInt8array(uint8_t *dst, uint64_t amount) = 0;
		virtual void	 ReadUInt16array(uint16_t *dst, uint64_t amount) = 0;
		virtual void	 ReadUInt32array(uint32_t *dst, uint64_t amount) = 0;
		virtual void	 ReadUInt64array(uint64_t *dst, uint64_t amount) = 0;
		virtual void	 ReadFloatarray(float *ori, uint64_t amout) = 0;
		virtual void	 ReadDoublearray(double *ori, uint64_t amout) = 0;
	};

}
