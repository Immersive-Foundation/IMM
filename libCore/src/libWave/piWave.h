#pragma once

#include "../libBasics/piTypes.h"
#include "../libBasics/piFile.h"
#include "../libBasics/piTArray.h"

namespace ImmCore
{

	class piWav
	{
	public:
		piWav();
		~piWav();

        void  Init(void);
		bool  Init(int rate, int numChanels, int bits, uint64_t numSamples);
        bool  InitCopy(const piWav *src);
        void  InitMove(const piWav *src);
        bool  Make(int rate, int numChanels, int bits, void *data, uint64_t dataSize);
		void  Deinit(void);

        bool ReadFromDisk(const wchar_t *name);
        bool ReadFromFile(piFile * file, uint64_t size);
        bool ReadFromMemory(piTArray<uint8_t> *data);

        bool WriteToDisk(const wchar_t *name) const;
        bool WriteToFile(piFile * file) const;
        bool WriteToMemory(piTArray<uint8_t> *data) const;


		int	       mNumChannels;
		int	       mRate;
		int		   mBits;
		uint64_t   mNumSamples;
		void      *mData;
		uint64_t   mDataSize; // mNumSamples * mNumChannels * mBits / 8
	};


}
