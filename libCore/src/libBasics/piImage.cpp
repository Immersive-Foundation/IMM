//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <cstdio>

#include <png.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "../libBasics/piFile.h"
#include "../libBasics/piArray.h"
#include "../libBasics/piTArray.h"
#include "../libBasics/piStr.h"
#include "../libBasics/piStreamI.h"
#include "../libBasics/piStreamFileI.h"
#include "../libBasics/piStreamArrayI.h"
#include "../libBasics/piStreamO.h"
#include "../libBasics/piStreamFileO.h"
#include "../libBasics/piStreamArrayO.h"
#include "piImage.h"

#ifdef ANDROID
#include "android/piCStr.h"
#endif
namespace ImmCore {

//--------------------------------

static __inline int rgb2grey( int r, int g, int b )
{
    return (r*306 + g*601 + 117*b) >> 10;
}

static int iGetBpp(piImage::Format type)
{
	switch (type)
	{
	case piImage::FORMAT_I_GREY:    return(1);
	case piImage::FORMAT_I_15BIT:   return(2);
	case piImage::FORMAT_I_16BIT:   return(2);
	case piImage::FORMAT_I_RG:      return(2);
	case piImage::FORMAT_I_RGB:     return(3);
	case piImage::FORMAT_I_RGBA:    return(4);
	case piImage::FORMAT_F_GREY:    return(4);
	case piImage::FORMAT_F_RG:      return(8);
	case piImage::FORMAT_F_RGB:     return(12);
	case piImage::FORMAT_F_RGBA:    return(16);
    default:
	    break;
	}

	return(0);
}

//-------------------------------------------------------------------------
void piImage::Init( void )
{
    mNumChannels = 0;
    mOwnerOfBuffers = false;
    mXres = 0;
    mYres = 0;
    mZres = 0;
    for( int i=0; i<16; i++ )
    {
        mChannel[i].mData = nullptr;
        mChannel[i].mFormat = Format::UNUSED1;
    }
}

uint64_t piImage::iComputeDataSize( const piImage *me, int ch )
{
    const int bpp = iGetBpp(me->mChannel[ch].mFormat);
    const int layers = (me->GetType() == TYPE_CUBE) ? 6 : 1;
    return static_cast<uint64_t>(me->mXres)*static_cast<uint64_t>(me->mYres)*static_cast<uint64_t>(me->mZres)*static_cast<uint64_t>(bpp)*static_cast<uint64_t>(layers);
}

bool piImage::InitCopy(const piImage *src)
{
	mType = src->mType;
	mXres = src->mXres;
	mYres = src->mYres;
	mZres = src->mZres;
    mNumChannels = src->mNumChannels;
    mOwnerOfBuffers = true;

    for (uint32_t i = 0; i < mNumChannels; i++)
    {
        mChannel[i].mFormat = src->mChannel[i].mFormat;
        const size_t amount = iComputeDataSize(this, i);
        mChannel[i].mData = malloc(amount);
        if (!mChannel[i].mData)
            return false;
        memcpy(mChannel[i].mData, src->mChannel[i].mData, amount);
    }
	return true;
}

int piImage::GetBpp(int ch) const
{
    return iGetBpp(mChannel[ch].mFormat );
}


void piImage::InitMove(const piImage *img )
{
    //Free();
    mXres = img->mXres;
    mYres = img->mYres;
    mZres = img->mZres;
    mNumChannels = img->mNumChannels;
    mOwnerOfBuffers = img->mOwnerOfBuffers;
    for (uint32_t i = 0; i < mNumChannels; i++)
    {
        mChannel[i].mFormat = img->mChannel[i].mFormat;
        mChannel[i].mData = img->mChannel[i].mData;
    }
    mType = img->mType;
}

bool piImage::Init( Type type, int xres, int yres, int zres, int numChannels, const Format *formats )
{
    if (type != Type::TYPE_1D && type != Type::TYPE_2D && type != Type::TYPE_3D && type != Type::TYPE_CUBE)
        return false;
	mType = type;
    if (xres <= 0 || yres <= 0)
        return false;
	mXres = xres;
	mYres = yres;
	mZres = zres;
    mNumChannels = numChannels;
    mOwnerOfBuffers = true;

    for (int i = 0; i < numChannels; i++)
    {
        mChannel[i].mFormat = formats[i];

        const size_t amount = iComputeDataSize(this, i);

        mChannel[i].mData = malloc(amount);
        if (!mChannel[i].mData)
            return false;
    }
    return true;
}

void piImage::Free(void)
{
    if (mNumChannels == 0) return;
    if (!mOwnerOfBuffers) return;

    for(uint32_t i=0; i<mNumChannels; i++ )
	{
		if (mChannel[i].mData)
        {
            free(mChannel[i].mData);
            mChannel[i].mData = nullptr;
        }
	}
}


void piImage::InitWrap(Type type, int xres, int yres, int zres, Format format, void *buffer)
{
	mType = type;
    mXres = xres;
    mYres = yres;
	mZres = zres;
    mOwnerOfBuffers = false;
    mNumChannels = 1;
    mChannel[0].mFormat = format;
    mChannel[0].mData = buffer;
}
void piImage::InitMove(Type type, int xres, int yres, int zres, Format format, void *buffer)
{
    mType = type;
    mXres = xres;
    mYres = yres;
    mZres = zres;
    mOwnerOfBuffers = true;
    mNumChannels = 1;
    mChannel[0].mFormat = format;
    mChannel[0].mData = buffer;
}

const piImage::Format piImage::GetFormat(int ch) const
{
    return mChannel[ch].mFormat;
}

const void piImage::GetFormats(Format *formats) const
{
    for (uint32_t i = 0; i < mNumChannels; i++)
    {
        formats[i] = mChannel[i].mFormat;
    }
}

const piImage::Type piImage::GetType(void) const
{
    return mType;
}
const uint32_t piImage::GetNumChannels(void) const
{
    return mNumChannels;
}

const int piImage::GetXRes(void) const
{
    return mXres;
}
const int piImage::GetYRes(void) const
{
    return mYres;
}
const int piImage::GetZRes(void) const
{
    return mZres;
}

const uint64_t piImage::GetDataSize(int ch) const
{
    return iComputeDataSize(this, ch);
}

void * piImage::GetData(int ch)
{
    return mChannel[ch].mData;
}

void * piImage::GetData(int ch) const
{
    return mChannel[ch].mData;
}

//----------------------------------------------------------------------------
static void iConvertBytes(void *dstBuffer, piImage::Format dstFormat,
                          void *srcBuffer, piImage::Format srcFormat, uint64_t srcNumPixels, bool swapRB)
{
    if (dstFormat == piImage::FORMAT_I_RGBA && srcFormat == piImage::FORMAT_I_RGB)
    {
        const uint8_t *ori = (unsigned char*)srcBuffer;
        uint8_t *dst = (uint8_t*)dstBuffer;
        for (uint64_t i = 0; i<srcNumPixels; i++)
        {
            dst[0] = ori[0];
            dst[1] = ori[1];
            dst[2] = ori[2];
            dst[3] = 255;
            dst += 4;
            ori += 3;
        }
    }
    else if (dstFormat == piImage::FORMAT_I_RGB && srcFormat == piImage::FORMAT_F_RGB)
    {
        const uint64_t l = 3 * srcNumPixels;

        uint8_t *aux = (uint8_t*)dstBuffer;
        float *ori = (float*)srcBuffer;
        float min = ori[0];
        float max = ori[0];
        for (uint64_t i = 1; i<l; i++)
        {
            float f = ori[i];
            if (f<min) min = f; else if (f>max) max = f;
        }

        float f = max - min;
        if (f <= 0.0f) return;

        f = 255.0f / f;
        for (uint64_t i = 0; i<l; i++) aux[i] = (uint8_t)((ori[i] - min)*f);
    }
    else if (dstFormat == piImage::FORMAT_I_GREY && srcFormat == piImage::FORMAT_I_RGB)
    {
        uint8_t *ori = (unsigned char*)srcBuffer;
        uint8_t *aux = (uint8_t*)dstBuffer;
        for (uint64_t i = 0; i<srcNumPixels; i++)
        {
            int b = ori[3 * i + 0];
            int g = ori[3 * i + 1];
            int r = ori[3 * i + 2];
            aux[i] = rgb2grey(r, g, b);
        }
    }
    if (dstFormat == piImage::FORMAT_I_RGB && srcFormat == piImage::FORMAT_I_RGBA)
    {
        uint8_t *ori = (uint8_t*)srcBuffer;
        uint8_t *dst = (uint8_t*)dstBuffer;;

        if (swapRB)
        {
            for (uint64_t i = 0; i<srcNumPixels; i++)
            {
                dst[2] = ori[0];
                dst[1] = ori[1];
                dst[0] = ori[2];
                dst += 3;
                ori += 4;
            }
        }
        else
        {
            for (uint64_t i = 0; i<srcNumPixels; i++)
            {
                dst[0] = ori[0];
                dst[1] = ori[1];
                dst[2] = ori[2];
                dst += 3;
                ori += 4;
            }
        }
    }
}



void * piImage::iConvertSelf(int ch, Format format, bool swapRB) const
{
	if( format==piImage::FORMAT_I_RGBA && mChannel[ch].mFormat==piImage::FORMAT_I_RGB )
    {
		const uint64_t l = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);

        unsigned char *aux = (unsigned char*)malloc(l * 4);
        if (!aux)
            return nullptr;

        iConvertBytes(aux, format, mChannel[ch].mData, mChannel[ch].mFormat, l, swapRB);

        return aux;
    }
	else if( format==piImage::FORMAT_I_RGB && mChannel[ch].mFormat==piImage::FORMAT_F_RGB )
    {
		const uint64_t l = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);

        unsigned char *aux = (unsigned char*)malloc(l*3);
        if (!aux)
            return nullptr;

        iConvertBytes(aux, format, mChannel[ch].mData, mChannel[ch].mFormat, l, swapRB);

        return aux;
    }
	else if( format==piImage::FORMAT_I_GREY && mChannel[ch].mFormat==piImage::FORMAT_I_RGB )
    {
        const uint64_t l = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);

        unsigned char *aux = (unsigned char*)malloc(l);
        if (!aux)
            return nullptr;

        iConvertBytes(aux, format, mChannel[ch].mData, mChannel[ch].mFormat, l, swapRB);

        return aux;
    }
	if( format==piImage::FORMAT_I_RGB && mChannel[ch].mFormat==piImage::FORMAT_I_RGBA )
    {
        const uint64_t l = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);
        unsigned char *aux = (unsigned char*)malloc(l * 3);
        if (!aux)
            return nullptr;

        iConvertBytes(aux, format, mChannel[ch].mData, mChannel[ch].mFormat, l, swapRB);

        return aux;
    }

    return nullptr;
}

bool piImage::InitConvert(const piImage *src, Format newformat)
{
    void *tmp = src->iConvertSelf(0, newformat, false);

    if (!tmp) return false;

    mType = src->mType;
    mXres = src->mXres;
    mYres = src->mYres;
    mZres = src->mZres;
    mOwnerOfBuffers = true;
    mNumChannels = 1;
    mChannel[0].mFormat = newformat;
    mChannel[0].mData = tmp;

    return true;
}

bool piImage::Convert(const piImage *src, int ch, Format format, bool swapRB)
{
    if (ch == -1)
    {
        for (uint32_t i = 0; i < mNumChannels; i++)
        {
            if (!this->Convert(src, i, format, swapRB))
                return false;
        }
    }
    else
    {
        const void *dstBuffer = mChannel[ch].mData;
        const uint64_t dstSize = iComputeDataSize(this, ch);

        const int srcNumLayers = (src->GetType() == TYPE_CUBE) ? 6 : 1;
        const uint64_t srcNumPixels = static_cast<uint64_t>(src->mXres)*static_cast<uint64_t>(src->mYres)*static_cast<uint64_t>(src->mZres)*static_cast<uint64_t>(srcNumLayers);
        const int bpp = iGetBpp(format);
        const uint64_t neededSize = srcNumPixels * static_cast<uint64_t>(bpp);

        if (neededSize > dstSize)
            return false;

        iConvertBytes(mChannel[ch].mData, format, src->mChannel[ch].mData, src->mChannel[ch].mFormat, srcNumPixels, swapRB);
    }

    return true;
}

bool piImage::Convert(int ch, Format format, bool swapRB)
{
    if (ch == -1)
    {
        for (uint32_t i = 0; i < mNumChannels; i++)
        {
            if (!this->Convert(i, format, swapRB))
                return false;
        }
    }
    else
    {
        void *tmp = iConvertSelf(ch, format, swapRB);
        if (!tmp) return false;
        if (mOwnerOfBuffers) free(mChannel[ch].mData);
        mChannel[ch].mData = (void*)tmp;
        mChannel[ch].mFormat = format;
        mOwnerOfBuffers = true;
    }

    return true;
}

void piImage::SwapRB(int ch)
{
	if(mChannel[ch].mFormat==piImage::FORMAT_I_RGBA  )
    {
		unsigned char *ptr = (unsigned char*)mChannel[ch].mData;
		const uint64_t num = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);
		for(uint64_t i=0; i<num; i++ )
		{
			int j = ptr[0];
			ptr[0] = ptr[2];
			ptr[2] = j;
			ptr += 4;
		}
	}
	else if(mChannel[ch].mFormat==piImage::FORMAT_I_RGB  )
    {
        unsigned char *ptr = (unsigned char*)mChannel[ch].mData;
        const uint64_t num = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);
        for (uint64_t i = 0; i<num; i++)
		{
			int j = ptr[0];
			ptr[0] = ptr[2];
			ptr[2] = j;
			ptr += 3;
		}
	}
}

bool piImage::ExtractComponent(piImage *img, int ch, int compo)
{
    if(mChannel[ch].mFormat==piImage::FORMAT_I_RGB )
    {
        piImage::Format format = piImage::FORMAT_I_GREY;
        if (!img->Init(mType, mXres, mYres, mZres, 1, &format))
            return false;

        const unsigned char *src = (unsigned char*)mChannel[ch].mData;
        unsigned char *dst = (unsigned char*)img->mChannel[ch].mData;
        const uint64_t l = static_cast<uint64_t>(mXres)*static_cast<uint64_t>(mYres)*static_cast<uint64_t>(mZres);
        for (uint64_t i = 0; i<l; i++)
        {
            dst[i] = src[3*i+compo];
        }
        return true;
    }

    return false;
}

bool piImage::Compatibles( const piImage *b ) const
{
    if( mXres   !=b->mXres    ) return false;
    if( mYres   != b->mYres   ) return false;
    if( mZres   != b->mZres   ) return false;
    if (mNumChannels != b->mNumChannels) return false;
    for (uint32_t i = 0; i < mNumChannels; i++)
    {
        if (mChannel[i].mFormat != b->mChannel[i].mFormat) return false;
    }
    if( mType   != b->mType   ) return false;

    return true;
}

bool piImage::CopyData( const piImage *b )
{
    if( !Compatibles( b ) )
        return false;

    for (uint32_t i = 0; i < mNumChannels; i++)
    {
        const uint64_t amount = iComputeDataSize(this,i);
        memcpy(mChannel[i].mData, b->mChannel[i].mData, amount);
    }

    return true;
}

void piImage::SampleBilinear(int ch, float fx, float fy, float *data) const
{
	const int xres = mXres;
	const int yres = mYres;

	const float ifx = floorf( fx );
	const float ify = floorf( fy );

	const float ffx = fx - ifx;
	const float ffy = fy - ify;

	const int tuA = ((int)ifx) % xres;
	const int tvA = ((int)ify) % yres;

	const int tuB = (1+tuA) % xres;
	const int tvB = (1+tvA) % yres;

	float a[4];
	float b[4];
	float c[4];
	float d[4];

	if(mChannel[ch].mFormat == piImage::FORMAT_F_RGBA )
	{
		const float *ptr = (float*)mChannel[ch].mData;
		a[0] = ptr[ 4*(tvA*xres+tuA) + 0 ];
        a[1] = ptr[ 4*(tvA*xres+tuA) + 1 ];
        a[2] = ptr[ 4*(tvA*xres+tuA) + 2 ];
        a[3] = ptr[ 4*(tvA*xres+tuA) + 3 ];
		b[0] = ptr[ 4*(tvA*xres+tuB) + 0 ];
        b[1] = ptr[ 4*(tvA*xres+tuB) + 1 ];
        b[2] = ptr[ 4*(tvA*xres+tuB) + 2 ];
        b[3] = ptr[ 4*(tvA*xres+tuB) + 3 ];
		c[0] = ptr[ 4*(tvB*xres+tuA) + 0 ];
        c[1] = ptr[ 4*(tvB*xres+tuA) + 1 ];
        c[2] = ptr[ 4*(tvB*xres+tuA) + 2 ];
        c[3] = ptr[ 4*(tvB*xres+tuA) + 3 ];
		d[0] = ptr[ 4*(tvB*xres+tuB) + 0 ];
        d[1] = ptr[ 4*(tvB*xres+tuB) + 1 ];
        d[2] = ptr[ 4*(tvB*xres+tuB) + 2 ];
        d[3] = ptr[ 4*(tvB*xres+tuB) + 3 ];

        float ab[4] = { a[0] + (b[0] - a[0])*ffx,
            a[1] + (b[1] - a[1])*ffx,
            a[2] + (b[2] - a[2])*ffx,
            a[3] + (b[3] - a[3])*ffx };
        float cd[4] = { c[0] + (d[0] - c[0])*ffx,
            c[1] + (d[1] - c[1])*ffx,
            c[2] + (d[2] - c[2])*ffx,
            c[3] + (d[3] - c[3])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
        data[3] = ab[3] + (cd[3] - ab[3])*ffy;

	}
	else if(mChannel[ch].mFormat == piImage::FORMAT_I_RGBA )
	{
		const unsigned char *ptr = (unsigned char*)mChannel[ch].mData;
		a[0] = ptr[ 4*(tvA*xres+tuA) + 0 ] * (1.0f/255.0f);
        a[1] = ptr[ 4*(tvA*xres+tuA) + 1 ] * (1.0f/255.0f);
        a[2] = ptr[ 4*(tvA*xres+tuA) + 2 ] * (1.0f/255.0f);
        a[3] = ptr[ 4*(tvA*xres+tuA) + 3 ] * (1.0f/255.0f);
		b[0] = ptr[ 4*(tvA*xres+tuB) + 0 ] * (1.0f/255.0f);
        b[1] = ptr[ 4*(tvA*xres+tuB) + 1 ] * (1.0f/255.0f);
        b[2] = ptr[ 4*(tvA*xres+tuB) + 2 ] * (1.0f/255.0f);
        b[3] = ptr[ 4*(tvA*xres+tuB) + 3 ] * (1.0f/255.0f);
		c[0] = ptr[ 4*(tvB*xres+tuA) + 0 ] * (1.0f/255.0f);
        c[1] = ptr[ 4*(tvB*xres+tuA) + 1 ] * (1.0f/255.0f);
        c[2] = ptr[ 4*(tvB*xres+tuA) + 2 ] * (1.0f/255.0f);
        c[3] = ptr[ 4*(tvB*xres+tuA) + 3 ] * (1.0f/255.0f);
		d[0] = ptr[ 4*(tvB*xres+tuB) + 0 ] * (1.0f/255.0f);
        d[1] = ptr[ 4*(tvB*xres+tuB) + 1 ] * (1.0f/255.0f);
        d[2] = ptr[ 4*(tvB*xres+tuB) + 2 ] * (1.0f/255.0f);
        d[3] = ptr[ 4*(tvB*xres+tuB) + 3 ] * (1.0f/255.0f);

        float ab[4] = { a[0] + (b[0] - a[0])*ffx,
            a[1] + (b[1] - a[1])*ffx,
            a[2] + (b[2] - a[2])*ffx,
            a[3] + (b[3] - a[3])*ffx };
        float cd[4] = { c[0] + (d[0] - c[0])*ffx,
            c[1] + (d[1] - c[1])*ffx,
            c[2] + (d[2] - c[2])*ffx,
            c[3] + (d[3] - c[3])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
        data[3] = ab[3] + (cd[3] - ab[3])*ffy;

	}
    else if (mChannel[ch].mFormat == piImage::FORMAT_I_RGB)
    {
        const unsigned char *ptr = (unsigned char*)mChannel[ch].mData;
        a[0] = ptr[3 * (tvA*xres + tuA) + 0] * (1.0f / 255.0f);
        a[1] = ptr[3 * (tvA*xres + tuA) + 1] * (1.0f / 255.0f);
        a[2] = ptr[3 * (tvA*xres + tuA) + 2] * (1.0f / 255.0f);
        b[0] = ptr[3 * (tvA*xres + tuB) + 0] * (1.0f / 255.0f);
        b[1] = ptr[3 * (tvA*xres + tuB) + 1] * (1.0f / 255.0f);
        b[2] = ptr[3 * (tvA*xres + tuB) + 2] * (1.0f / 255.0f);
        c[0] = ptr[3 * (tvB*xres + tuA) + 0] * (1.0f / 255.0f);
        c[1] = ptr[3 * (tvB*xres + tuA) + 1] * (1.0f / 255.0f);
        c[2] = ptr[3 * (tvB*xres + tuA) + 2] * (1.0f / 255.0f);
        d[0] = ptr[3 * (tvB*xres + tuB) + 0] * (1.0f / 255.0f);
        d[1] = ptr[3 * (tvB*xres + tuB) + 1] * (1.0f / 255.0f);
        d[2] = ptr[3 * (tvB*xres + tuB) + 2] * (1.0f / 255.0f);

        float ab[3] = { a[0] + (b[0] - a[0])*ffx,
                        a[1] + (b[1] - a[1])*ffx,
                        a[2] + (b[2] - a[2])*ffx };
        float cd[3] = { c[0] + (d[0] - c[0])*ffx,
                        c[1] + (d[1] - c[1])*ffx,
                        c[2] + (d[2] - c[2])*ffx };

        data[0] = ab[0] + (cd[0] - ab[0])*ffy;
        data[1] = ab[1] + (cd[1] - ab[1])*ffy;
        data[2] = ab[2] + (cd[2] - ab[2])*ffy;
    }
    else
    {
        data[0] = 0.0f;
        data[1] = 0.0f;
        data[2] = 0.0f;
        data[3] = 0.0f;
    }

}

bool piImage::WriteToStream(piOStream & fp, int channel, const wchar_t *ext) const
{
    bool res = false;
    if( ext[0]=='j' && ext[1]=='p' && ext[2]=='g' )
    {
        res = iJPGSaveToStream(this, channel, fp );
    }
    else if( ext[0]=='p' && ext[1]=='n' && ext[2]=='g' )
    {
        res = iPNGSaveToStream(this, channel, fp );
    }
    return res;
}


bool piImage::WriteToFile(piFile * fp, int channel, const wchar_t *ext) const
{
    piOStreamFile st(fp);
    return WriteToStream(st,channel,ext);
}

bool piImage::WriteToDisk(const wchar_t *name,int channel, const wchar_t *ext) const
{
    piFile fp;
    if (!fp.Open(name, L"wb"))
        return false;

    piOStreamFile st(&fp);

    bool res = WriteToStream(st,channel,ext);

    fp.Close();

    return res;
}

bool piImage::WriteToMemory(piTArray<uint8_t> *data, int channel, const wchar_t *ext) const
{
    piOStreamArray st(data);
    return WriteToStream(st, channel, ext);
}

bool piImage::ReadFromDisk( const wchar_t * path )
{
    if( !iJPGLoadFromDisk(this, path ))
    if( !iPNGLoadFromDisk(this, path ))
        return false;
    return true;
}

bool piImage::ReadFromMemory( const piTArray<uint8_t> * data, const wchar_t *ext)
{
    bool res = false;
    if( ext[0]=='j' && ext[1]=='p' && ext[2]=='g' )
    {
        res = iJPGLoadFromMemory(this, data->GetAddress(0), data->GetLength() );
    }
    else if( ext[0]=='p' && ext[1]=='n' && ext[2]=='g' )
    {
        res = iPNGLoadFromMemory(this, data->GetAddress(0), data->GetLength() );
    }
    return res;
}


    static void iWrite(png_structp _png, png_bytep _data, png_size_t _sz)
    {
        piOStreamArray *data = (piOStreamArray *)png_get_io_ptr(_png);
        data->Write(_data, _sz);
    }

    static void iFlush(png_structp _png)
    {
        piOStreamArray *data = (piOStreamArray *)png_get_io_ptr(_png);
    }

    bool piImage::iPNGSaveToStream(const piImage *src, int channel, piOStream & st) const
    {
        const int xres = src->GetXRes();
        const int yres = src->GetYRes();

        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
            return false;

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
            return false;

        if (setjmp(png_jmpbuf(png_ptr)))
            return false;

        png_set_write_fn(png_ptr, &st, iWrite, iFlush);

        const piImage::Format format = src->GetFormat(channel);
        int bit_depth = 8;
        int color_type = PNG_COLOR_TYPE_GRAY;
        if (format == piImage::Format::FORMAT_I_GREY) color_type = PNG_COLOR_TYPE_GRAY;
        if (format == piImage::Format::FORMAT_I_RGB) color_type = PNG_COLOR_TYPE_RGB;
        if (format == piImage::Format::FORMAT_I_RGBA) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        png_set_IHDR(png_ptr, info_ptr, xres, yres, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        if (setjmp(png_jmpbuf(png_ptr)))
            return false;

        const int bpp = src->GetBpp(channel);
        uint8_t **rows = (uint8_t**)malloc( yres*sizeof(uint8_t*) );
        if( !rows )
            return false;
        for (int i = 0; i < yres; i++)
            rows[i] = (uint8_t*)src->GetData(channel) + i * bpp*xres;
        png_set_rows(png_ptr, info_ptr, &rows[0]);
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
        free(rows);

        if (setjmp(png_jmpbuf(png_ptr)))
            return false;

        png_write_end(png_ptr, NULL);

        png_destroy_write_struct(&png_ptr, &info_ptr);

        return true;
    }

    bool piImage::iPNGSaveToMemory(const piImage *src, int channel, piTArray<uint8_t> *data) const
    {
        piOStreamArray st(data);

        return iPNGSaveToStream(src, channel, st);
    }

    bool piImage::iPNGSaveToDisk(const piImage * src, int channel, const wchar_t * path) const
    {
        piFile fp;
        if (!fp.Open(path, L"wb") )
            return false;

        piOStreamFile st(&fp);

        const bool res = iPNGSaveToStream(src, channel, st);
        fp.Close();

        return res;
    }

	bool piImage::iPNGLoadFromDisk( piImage * src, const wchar_t * path)
	{
		FILE *fp = nullptr;

        char output[256];
        sprintf(output, "%ls", path );

        fp = fopen(output, "rb");
		if (!fp)
			return false;

		unsigned char header[8];
		fread(header, 1, 8, fp);
		if (png_sig_cmp(header, 0, 8))
		{
			fclose(fp);
			return false;
		}


		/* initialize stuff */
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			fclose(fp);
			return false;
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			fclose(fp);
			return false;
		}

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			fclose(fp);
			return false;
		}

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		const int xres = png_get_image_width(png_ptr, info_ptr);
		const int yres = png_get_image_height(png_ptr, info_ptr);
		const int color_type = png_get_color_type(png_ptr, info_ptr);
		const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (bit_depth > 8) // currently do not support higher bit depth.
        {
            fclose(fp);
            return false;
        }

		piImage::Format format = piImage::Format::FORMAT_I_GREY;
		if (color_type == PNG_COLOR_TYPE_GRAY) format = piImage::Format::FORMAT_I_GREY;
		else if(color_type== PNG_COLOR_TYPE_RGB ) format = piImage::Format::FORMAT_I_RGB;
		else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) format = piImage::Format::FORMAT_I_RGBA;
		else
		{
			fclose(fp);
			return false;
		}

		if (!src->Init(piImage::TYPE_2D, xres, yres, 1, 1, &format))
		{
			fclose(fp);
			return false;
		}

		const int number_of_passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);


		/* read file */
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			fclose(fp);
			return false;
		}

		png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * yres);
		const png_bytep buffer = (png_bytep)src->GetData(0);
		const int bpp = src->GetBpp(0);
		for (int y = 0; y < yres; y++)
		{
			row_pointers[y] = buffer + y*xres*bpp;
		}

		png_read_image(png_ptr, row_pointers);

		free(row_pointers);

        //png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);
		fclose(fp);

		return true;
	}

    struct iMyStream
    {
        const uint8_t *ptr;
        uint64_t pos;
    };

    static void iReadStream(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
    {
       png_voidp io_ptr = png_get_io_ptr(png_ptr);
       if(io_ptr == NULL) return;

       // using pulsar::InputStream
       // -> replace with your own data source interface
       iMyStream *inputStream = (iMyStream*)io_ptr;
       //const size_t bytesRead = inputStream.Read( (byte*)outBytes, (size_t)byteCountToRead);
       memcpy( outBytes, inputStream->ptr + inputStream->pos, byteCountToRead);
       inputStream->pos += byteCountToRead;
    }

    bool piImage::iPNGLoadFromMemory( piImage *img, const uint8_t *ptr, uint64_t size)
    {
		const unsigned char *header = (const unsigned char*)ptr;
		if (png_sig_cmp(header, 0, 8))
			return false;
        ptr += 8;

		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
			return false;

        iMyStream myStream = { ptr, 0 };
        png_set_read_fn( png_ptr, &myStream, iReadStream );


		png_infop info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
			return false;

		if (setjmp(png_jmpbuf(png_ptr)))
			return false;

		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		const int xres = png_get_image_width(png_ptr, info_ptr);
		const int yres = png_get_image_height(png_ptr, info_ptr);
		const int color_type = png_get_color_type(png_ptr, info_ptr);
		const int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (bit_depth > 8) // currently do not support higher bit depth.
            return false;

		piImage::Format format = piImage::Format::FORMAT_I_GREY;
		if (color_type == PNG_COLOR_TYPE_GRAY) format = piImage::Format::FORMAT_I_GREY;
		else if(color_type== PNG_COLOR_TYPE_RGB ) format = piImage::Format::FORMAT_I_RGB;
		else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) format = piImage::Format::FORMAT_I_RGBA;
		else
			return false;

		if (!img->Init(piImage::TYPE_2D, xres, yres, 1, 1, &format))
			return false;

		const int number_of_passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);

		if (setjmp(png_jmpbuf(png_ptr)))
			return false;

		png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * yres);
		const png_bytep buffer = (png_bytep)img->GetData(0);
		const int bpp = img->GetBpp(0);
		for (int y = 0; y < yres; y++)
		{
			row_pointers[y] = buffer + y*xres*bpp;
		}

		png_read_image(png_ptr, row_pointers);

		free(row_pointers);

        //png_destroy_write_struct(&png_ptr, &info_ptr);
        png_destroy_info_struct(png_ptr, &info_ptr);

		return true;
    }

    // --- jpg ------------------------------------------------------------------------

    typedef struct
    {
        struct jpeg_destination_mgr pub;
        piOStream * outfile;
        JOCTET * buffer;
    } MyDestinationManager;

    #define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

    static void init_destination(j_compress_ptr cinfo)
    {
        MyDestinationManager * dest = (MyDestinationManager *)cinfo->dest;
        /* Allocate the output buffer --- it will be released when done with image */
        dest->buffer = (JOCTET *) (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));
        dest->pub.next_output_byte = dest->buffer;
        dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
    }

    static boolean empty_output_buffer(j_compress_ptr cinfo)
    {
        MyDestinationManager * dest = (MyDestinationManager *)cinfo->dest;
        dest->outfile->Write(dest->buffer, OUTPUT_BUF_SIZE);
        dest->pub.next_output_byte = dest->buffer;
        dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
        return TRUE;
    }

    static void term_destination(j_compress_ptr cinfo)
    {
        MyDestinationManager * dest = (MyDestinationManager *)cinfo->dest;
        size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

        if (datacount > 0)
        {
            dest->outfile->Write(dest->buffer, datacount);
        }
    }

    bool piImage::iJPGSaveToStream(const piImage *src, int channel, piOStream & st) const
    {
        const int quality = 85;

        const int xres = src->GetXRes();
        const int yres = src->GetYRes();
        uint8_t *buffer = (uint8_t*)src->GetData(channel);
        const piImage::Format format = src->GetFormat(channel);

        if (format == piImage::Format::FORMAT_I_RGBA) return false;

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;


        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);


        //jpeg_stdio_dest(&cinfo, outfile);
        //jpeg_mem_dest(&cinfo,
        //custom output manager
        MyDestinationManager * dest;
        cinfo.dest = (struct jpeg_destination_mgr *)(*cinfo.mem->alloc_small) ((j_common_ptr)&cinfo, JPOOL_PERMANENT, sizeof(MyDestinationManager));
        dest = (MyDestinationManager *)cinfo.dest;
        dest->pub.init_destination = init_destination;
        dest->pub.empty_output_buffer = empty_output_buffer;
        dest->pub.term_destination = term_destination;
        dest->outfile = &st;

        cinfo.image_width = xres;
        cinfo.image_height = yres;
        cinfo.input_components = (format == piImage::Format::FORMAT_I_GREY) ? 1 : 3;
        cinfo.in_color_space = (format == piImage::Format::FORMAT_I_GREY) ? JCS_GRAYSCALE : JCS_RGB;

        jpeg_set_defaults(&cinfo);

        jpeg_set_quality(&cinfo, quality, TRUE);



        jpeg_start_compress(&cinfo, TRUE);

        const int row_stride = xres * cinfo.input_components;
        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] = buffer + cinfo.next_scanline * row_stride;
            (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);

        jpeg_destroy_compress(&cinfo);

        return true;
    }

    bool piImage::iJPGSaveToMemory(const piImage *src, int channel, piTArray<uint8_t> *data) const
    {
        piOStreamArray st(data);

        return iJPGSaveToStream(src, channel, st);
    }

    bool piImage::iJPGSaveToDisk(const piImage * src, int channel, const wchar_t * path) const
    {
        piFile fp;
        if (fp.Open(path, L"wb") )
            return false;

        piOStreamFile st(&fp);

        const bool res = iJPGSaveToStream(src, channel, st);
        fp.Close();

        return res;
    }

    //==========================================================

    struct my_error_mgr
    {
      struct jpeg_error_mgr pub;	/* "public" fields */

      jmp_buf setjmp_buffer;	/* for return to caller */
    };

    typedef struct my_error_mgr * my_error_ptr;

    METHODDEF(void)
    my_error_exit (j_common_ptr cinfo)
    {
      /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
      my_error_ptr myerr = (my_error_ptr) cinfo->err;

      /* Always display the message. */
      /* We could postpone this until after returning, if we chose. */
      (*cinfo->err->output_message) (cinfo);

      /* Return control to the setjmp point */
      longjmp(myerr->setjmp_buffer, 1);
    }

    bool piImage::iJPGLoadFromDisk( piImage *image, const wchar_t *name )
    {

        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;

	    FILE *infile;

    #if defined(ANDROID)
        ScopedCStr cName(piws2str(name));
        infile = fopen( cName.c_str(), "rb" );
    #else
        infile = _wfopen(name, L"rb");
    #endif
        if( !infile  )
        {
            return false;
        }

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            fclose(infile);
            return false;
        }

        jpeg_create_decompress( &cinfo );
        jpeg_stdio_src(&cinfo, infile);

        jpeg_read_header(&cinfo, TRUE);

        jpeg_start_decompress(&cinfo);

        int row_stride = cinfo.output_width * cinfo.output_components;

        piImage::Format format = piImage::FORMAT_I_RGB;
        if (!image->Init(piImage::TYPE_2D, cinfo.output_width, cinfo.output_height, 1, 1, &format))
            return false;

        // Make a one-row-high sample array that will go away when done with image
        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        unsigned char *dst = (unsigned char *)image->GetData(0);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            unsigned char *bufferTemp[1] = { dst };
            jpeg_read_scanlines(&cinfo, bufferTemp, 1);
            dst += row_stride;
        }

        jpeg_finish_decompress(&cinfo);

        jpeg_destroy_decompress(&cinfo);

        fclose(infile);

        return true;
    }

    bool piImage::iJPGLoadFromMemory(ImmCore::piImage *image, const uint8_t *data, uint64_t size)
    {
        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;
        if (setjmp(jerr.setjmp_buffer))
        {
            jpeg_destroy_decompress(&cinfo);
            return false;
        }

        jpeg_create_decompress( &cinfo );
        //jpeg_stdio_src(&cinfo, infile);
        jpeg_mem_src(&cinfo, (unsigned char*)data, static_cast<unsigned long>(size));

        jpeg_read_header(&cinfo, TRUE);

        jpeg_start_decompress(&cinfo);

        int row_stride = cinfo.output_width * cinfo.output_components;

        piImage::Format format = piImage::FORMAT_I_RGB;
        if (!image->Init(piImage::TYPE_2D, cinfo.output_width, cinfo.output_height, 1, 1, &format))
            return 0;

        // Make a one-row-high sample array that will go away when done with image
        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        unsigned char *dst = (unsigned char *)image->GetData(0);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            unsigned char *bufferTemp[1] = { dst };
            jpeg_read_scanlines(&cinfo, bufferTemp, 1);
            dst += row_stride;
        }

        jpeg_finish_decompress(&cinfo);

        jpeg_destroy_decompress(&cinfo);

        return true;
    }


}
