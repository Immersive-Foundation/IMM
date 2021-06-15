#include <malloc.h>
#include "../libBasics/piTypes.h"
#include "../libBasics/piStreamI.h"
#include "../libBasics/piStreamFileI.h"
#include "../libBasics/piStreamArrayI.h"
#include "../libBasics/piStreamO.h"
#include "../libBasics/piStreamFileO.h"
#include "../libBasics/piStreamArrayO.h"

#include "piWave.h"


namespace ImmCore {


	piWav::piWav()
		:mNumChannels(0)
		, mRate(0)
		, mBits(0)
		, mNumSamples(0)
		, mData(nullptr)
		, mDataSize(0)
	{
	}

	piWav::~piWav()
	{
	}

    void piWav::Init(void)
    {
        mNumChannels = 0;
        mDataSize = 0;
        mData = nullptr;
    }
	bool piWav::Init(int rate, int numChanels, int bits, uint64_t numSamples)
	{
		const int bytesPerSample = numChanels * bits / 8;

		mNumChannels = numChanels;
		mRate = rate;
		mBits = bits;
		mNumSamples = numSamples;
		mDataSize = numSamples * bytesPerSample;

		mData = malloc(mDataSize);
		if (!mData)
			return false;

		return true;
	}

    bool piWav::InitCopy(const piWav *src)
    {
        if( !Init(src->mRate, src->mNumChannels, src->mBits, src->mNumSamples) )
            return false;

        memcpy( mData, src->mData, mDataSize);

        return true;
    }

    void piWav::InitMove(const piWav *src)
    {
        memcpy(this, src, sizeof(piWav));
    }

	bool piWav::Make(int rate, int numChanels, int bits, void *data, uint64_t dataSize)
	{
		const int bytesPerSample = numChanels * bits / 8;

		mNumChannels = numChanels;
		mRate = rate;
		mBits = bits;
		mData = data;
		mDataSize = dataSize;
		mNumSamples = dataSize / bytesPerSample;

		return true;
	}

	void piWav::Deinit(void)
	{
        if(mData!=nullptr)
		free(mData);
	}

    //------------------------------------
    static bool iWrite(const piWav *me, piOStream & fp)
    {
        fp.WriteUInt32(0); // version

        fp.WriteUInt16(0);
        fp.WriteUInt8(me->mNumChannels);
        fp.WriteUInt8(me->mBits);
        fp.WriteUInt32(me->mRate);
        fp.WriteUInt64(me->mNumSamples);
        fp.Write( me->mData, me->mDataSize);

        return true;
    }

    static bool iRead(piWav *me, piIStream &fp)
    {
        const uint32_t version = fp.ReadUInt32();
        if (version != 0) return false;

        uint16_t dummy = fp.ReadUInt16();
        uint8_t numChannels = fp.ReadUInt8();
        uint8_t bits = fp.ReadUInt8();
        uint32_t rate = fp.ReadUInt32();
        uint64_t numSamples = fp.ReadUInt64();

        if( !me->Init(rate, numChannels, bits, numSamples) )
            return false;

        fp.Read(me->mData, me->mDataSize);

        return true;
    }
    //------------------------------------

    bool piWav::ReadFromDisk(const wchar_t *name)
    {
        piFile fp;
        if (!fp.Open(name, L"rb"))
            return false;
        piIStreamFile st(&fp, piIStreamFile::kDefaultFileSize);
        bool res = iRead(this,st);
        fp.Close();
        return res;
    }

    bool piWav::ReadFromFile(piFile * fp, uint64_t size)
    {
        piIStreamFile st(fp, size);
        return iRead(this, st);
    }
    bool piWav::ReadFromMemory(piTArray<uint8_t> *data)
    {
        piIStreamArray st(data);
        return iRead(this, st);
    }

    bool piWav::WriteToDisk(const wchar_t *name) const
    {
        piFile fp;
        if (!fp.Open(name, L"wb"))
            return false;

        piOStreamFile st(&fp);

        bool res = iWrite(this, st);

        fp.Close();

        return res;
    }

    bool piWav::WriteToFile(piFile * fp) const
    {
        piOStreamFile st(fp);
        return iWrite(this, st);
    }

    bool piWav::WriteToMemory(piTArray<uint8_t> *data) const
    {
        piOStreamArray st(data);
        return iWrite(this, st);
    }


}
