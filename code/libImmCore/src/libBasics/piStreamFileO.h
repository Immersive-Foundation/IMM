//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piStreamO.h"
#include "../libBasics/piFile.h"

namespace ImmCore
{

	class piOStreamFile : public piOStream
	{
	public:
		piOStreamFile(piFile *file) :piOStream() { mFile = file; }
		~piOStreamFile() {}

		//bool         Seek(uint64_t pos, piOStream::SeekMode mode) { return mFile->Seek(pos, static_cast<piFile::SeekMode>(mode)); }
		bool         Seek(uint64_t pos, piOStream::SeekMode mode) { piFile::SeekMode m = piFile::SeekMode::SET;
		if (mode == piOStream::SeekMode::CURRENT) m = piFile::SeekMode::CURRENT;
		if (mode == piOStream::SeekMode::END) m = piFile::SeekMode::END; return mFile->Seek(pos, m);
		}

		uint64_t     Tell(void) { return mFile->Tell(); }

		void         Write(const void *dst, uint64_t amount) { mFile->Write(dst, amount); }
		void		 WriteUInt8( uint8_t  x) { mFile->WriteUInt8(x); }
		void		 WriteUInt16(uint16_t x) { mFile->WriteUInt16(x); }
		void		 WriteUInt32(uint32_t x) { mFile->WriteUInt32(x); }
		void		 WriteUInt64(uint64_t x) { mFile->WriteUInt64(x); }
		void		 WriteFloat( float    x) { mFile->WriteFloat(x); }
		void		 WriteUInt8array(const uint8_t *dst, uint64_t amount) { mFile->WriteUInt8array(dst, amount); }
		void		 WriteUInt16array(const uint16_t *dst, uint64_t amount) { mFile->WriteUInt16array(dst, amount); }
		void		 WriteUInt32array(const uint32_t *dst, uint64_t amount) { mFile->WriteUInt32array(dst, amount); }
		void		 WriteUInt64array(const uint64_t *dst, uint64_t amount) { mFile->WriteUInt64array(dst, amount); }
		void		 WriteFloatarray(const float *dst, uint64_t amount) { mFile->WriteFloatarray(dst, amount); }
		void		 WriteDoublearray(const double *dst, uint64_t amount) { mFile->WriteDoublearray(dst, amount); }

	private:
		piFile *mFile;
	};
}
