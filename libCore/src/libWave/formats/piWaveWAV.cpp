#include <malloc.h>
#include "../../libBasics/piTypes.h"
#include "../../libBasics/piFile.h"
#include "../../libBasics/piStreamI.h"
#include "../../libBasics/piStreamFileI.h"
#include "../../libBasics/piStreamArrayI.h"
#include "../../libBasics/piStreamO.h"
#include "../../libBasics/piStreamFileO.h"
#include "../../libBasics/piStreamArrayO.h"

#include "../piWave.h"


namespace ImmCore {

#define  WAV_FORMAT_PCM        1 // PCM (uncompressed) wave file
#define  WAV_FORMAT_IEEE_FLOAT 3
#define  WAV_FORMAT_ALAW	   6 // 8 - bit ITU - T G.711 A - law
#define  WAV_FORMAT_MULAW	   7 // 8 - bit ITU - T G.711 µ - law
#define  WAV_FORMAT_EXT        0x0000fffe         // WAVE_FORMAT_EXTENSIBLE: either 24bit or > 2 channels

#define  WAV_RIFF     0x46464952         // "RIFF"
#define  WAV_WAVE     0x45564157         // "WAVE"
#define  WAV_FMT      0x20746D66         // " fmt"
#define  WAV_DATA     0x61746164         // "data"
#define  WAV_JUNK     0x4b4e554a         // "junk"
#define  WAV_BEXT     0x74786562         // "bext"
#define  WAV_FACT     0x74636166         // "fact"
#define  WAV_PEAK     0x4b414550         // "peak"

	typedef struct                   // wave chunk header
	{
		uint32_t ID;
		uint32_t SIZE;
	}CHUNKSTRUCT;

	typedef struct
	{
		uint16_t FORMAT;
		uint16_t CHANNEL;
		uint32_t RATE;
		uint32_t AVGBYTES;
		uint16_t BLOCKALIGN;
		uint16_t BIT;
	}FMTSTRUCT;

	typedef struct
	{
		CHUNKSTRUCT  riff;
		uint32_t     wave;
		CHUNKSTRUCT  fmt;
	}HEADERSTRUCT;


    static bool iOpenWAV(piWav *dst, piIStream *fp)
    {
		HEADERSTRUCT header;
        fp->Read(&header, sizeof(HEADERSTRUCT));
		if (header.riff.ID != WAV_RIFF || header.wave != WAV_WAVE)
			return false;

		bool readOK = true;

		// skip any non-format chunks
		while (header.fmt.ID != WAV_FMT && readOK)
		{
			fp->Seek(header.fmt.SIZE, piIStream::SeekMode::CURRENT);
			readOK = (fp->Read(&header.fmt, sizeof(CHUNKSTRUCT))== sizeof(CHUNKSTRUCT));
		}

		if (!readOK)
		{
			return false;
		}

		// get the format structure, skipping any included extensions
		FMTSTRUCT format;
		fp->Read(&format, sizeof(FMTSTRUCT));
		if (format.FORMAT != WAV_FORMAT_PCM && format.FORMAT != WAV_FORMAT_EXT && format.FORMAT != WAV_FORMAT_IEEE_FLOAT)
		{
			return false;
		}
		if (header.fmt.SIZE != sizeof(FMTSTRUCT))
		{
			if (header.fmt.SIZE != 18 && header.fmt.SIZE != 40)
			{
				return false;
			}
			fp->Seek(header.fmt.SIZE - sizeof(FMTSTRUCT), piIStream::SeekMode::CURRENT);
		}

		// skip any non-data chunks
		CHUNKSTRUCT data;
		fp->Read(&data, sizeof(CHUNKSTRUCT));
		while (data.ID != WAV_DATA && readOK)
		{
			//0x74786562 // bext
			//0x6b6e756a // junk

            if (data.ID == WAV_FACT)      // 0x74636166 "fact"
            {
                uint32_t numSamples = fp->ReadUInt32();
            }
            else if (data.ID == WAV_PEAK) // 0x4b414550 = "peak"
            {

            }

			fp->Seek(data.SIZE, piIStream::SeekMode::CURRENT);
			readOK = (fp->Read(&data, sizeof(CHUNKSTRUCT))== sizeof(CHUNKSTRUCT));
		}

		if (!readOK)
		{
			return false;
		}

		const uint32_t datalength = data.SIZE;
		const int bytesPerSample = (format.CHANNEL*format.BIT / 8);
		const uint64_t numSamples = datalength / bytesPerSample;

		if( !dst->Init(format.RATE, format.CHANNEL, format.BIT, numSamples ) )
			return false;

		fp->Read(dst->mData, datalength);

		return true;
	}

    static bool iWriteWAV(const piWav *me, piOStream * fp)
    {
        const uint32_t ds = static_cast<uint32_t>(me->mDataSize);

        fp->WriteUInt32(WAV_RIFF);
        fp->WriteUInt32(ds + 36);
        {
            fp->WriteUInt32(WAV_WAVE);

            fp->WriteUInt32(WAV_FMT);
            {
                fp->WriteUInt32(16);
                fp->WriteUInt16( (me->mNumChannels==32)? WAV_FORMAT_IEEE_FLOAT : WAV_FORMAT_PCM );
                fp->WriteUInt16(me->mNumChannels);
                fp->WriteUInt32(me->mRate);
                fp->WriteUInt32(me->mRate*me->mNumChannels*me->mBits / 8);
                fp->WriteUInt16(me->mNumChannels*me->mBits / 8);
                fp->WriteUInt16(me->mBits);
            }

            fp->WriteUInt32(WAV_DATA);
            {
                fp->WriteUInt32(ds);
                fp->Write(me->mData, me->mDataSize);
            }
        }

        return true;
    }

    //----------------------------------------------------------

	bool WriteWAVToDisk(const wchar_t *name, const piWav *me )
	{
		piFile fp;
		if (!fp.Open(name, L"wb"))
			return false;

        piOStreamFile st(&fp);

        bool res = iWriteWAV(me, &st);

		fp.Close();

		return res;
	}

    bool WriteWAVToMemory(piTArray<uint8_t> *data, const piWav *me)
    {
        if( !data->Init(me->mDataSize+128, false))
            return false;
        piOStreamArray st(data);
        return iWriteWAV(me, &st);

    }

    bool ReadWAVFromMemory(piWav *dst, piTArray<uint8_t> *data)
    {
        piIStreamArray st(data);
        return iOpenWAV(dst, &st);
    }

    bool ReadWAVFromFile(piWav *me, piFile *fp)
    {
        piIStreamFile st(fp, piIStreamFile::kDefaultFileSize);

        return iOpenWAV(me, &st);
    }

    bool ReadWAVFromDisk(piWav *dst, const wchar_t *name)
    {
        piFile fp;
        if (!fp.Open(name, L"rb"))
            return false;

        piIStreamFile st(&fp, piIStreamFile::kDefaultFileSize);

        bool res = iOpenWAV(dst, &st);

        fp.Close();

        return res;
    }

}
