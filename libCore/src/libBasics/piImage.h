//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"
#include "piTArray.h"
#include "piFile.h"
#include "../libBasics/piStreamI.h"
#include "../libBasics/piStreamO.h"

namespace ImmCore {

class piImage
{
public:
    typedef enum
    {
        TYPE_1D = 1,
        TYPE_2D = 2,
        TYPE_3D = 3,
        TYPE_CUBE = 4,
    }Type;

    typedef enum
    {
        UNUSED1 = 0,
        UNUSED2 = 1,
        FORMAT_I_GREY    = 2,
        FORMAT_I_15BIT   = 3,
        FORMAT_I_16BIT   = 4,
        FORMAT_I_RG      = 5,
        FORMAT_I_RGB     = 6,
        FORMAT_I_RGBA    = 7,
        FORMAT_F_GREY    = 8,
        FORMAT_F_RG      = 9,
        FORMAT_F_RGB     = 10,
        FORMAT_F_RGBA    = 11
    }Format;

    piImage() = default;
    piImage(piImage &&) = delete;
    piImage(const piImage &) = delete;
    piImage& operator=(piImage const&) = delete;
    piImage& operator=(piImage &&) = delete;

    void Init( void );
    bool Init(Type type, int xres, int yres, int zres, int numChannels, const Format *formats);
    bool InitCopy(const piImage *src);
    bool InitConvert(const piImage *src, Format newformat );
    void InitWrap(Type type, int xres, int yres, int zres, Format format, void *buffer);
    void InitMove(Type type, int xres, int yres, int zres, Format format, void *buffer);
    void InitMove(const piImage *img);
    void Free(void);

    bool CopyData( const piImage *b );
    int  GetBpp( int ch ) const;

    const Type   GetType(void) const;
    const int    GetXRes(void) const;
    const int    GetYRes(void) const;
    const int    GetZRes(void) const;
    const uint32_t GetNumChannels(void) const;
    const Format GetFormat(int ch) const;
    void  *      GetData(int ch);
    void  *      GetData(int ch) const;
	const uint64_t GetDataSize(int ch) const;
    const void GetFormats(Format *formats) const;

    // move this to processing
    bool Convert( int channel, Format newformat, bool swapRB); // self
    bool Convert( const piImage *src, int channel, Format newformat, bool swapRB); // from other
    void SwapRB( int ch);
    bool ExtractComponent( piImage *dst, int ch, int comp ); // comp: 2=red 1=green 0=blue
    bool Compatibles( const piImage *a ) const;

    void SampleBilinear( int ch, float x, float y, float *data ) const;

    // serializers
    bool ReadFromDisk( const wchar_t * path );
    bool ReadFromMemory(const piTArray<uint8_t> *data, const wchar_t *ext);

    bool WriteToDisk(const wchar_t *name, int channel, const wchar_t *ext) const;
    bool WriteToFile(piFile * file, int channel, const wchar_t *ext) const;
    bool WriteToMemory(piTArray<uint8_t> *data, int channel, const wchar_t *ext) const;
    bool WriteToStream(piOStream & fp, int channel, const wchar_t *ext) const;

private:
    static uint64_t iComputeDataSize(const piImage *src, int ch);
    void *   iConvertSelf(int ch, Format format, bool swapRB) const;

    bool iPNGSaveToStream(const piImage *src, int channel, piOStream & st) const;
    bool iPNGSaveToMemory(const piImage *src, int channel, piTArray<uint8_t> *data) const;
    bool iPNGSaveToDisk(const piImage * src, int channel, const wchar_t * path) const;
	bool iPNGLoadFromDisk( piImage * src, const wchar_t * path);
    bool iPNGLoadFromMemory( piImage *img, const uint8_t *ptr, uint64_t size);

    bool iJPGSaveToStream(const piImage *src, int channel, piOStream & st) const;
    bool iJPGSaveToMemory(const piImage *src, int channel, piTArray<uint8_t> *data) const;
    bool iJPGSaveToDisk(const piImage * src, int channel, const wchar_t * path) const;
    bool iJPGLoadFromDisk( piImage *image, const wchar_t *name );
    bool iJPGLoadFromMemory(piImage *image, const uint8_t *data, uint64_t size);

private:
    uint32_t    mNumChannels;
    Type        mType;
    struct
    {
        Format       mFormat;
        void	    *mData = nullptr;
    }mChannel[16];
    uint32_t	mXres, mYres, mZres;
	bool        mOwnerOfBuffers;
};

} // namespace ImmCore
