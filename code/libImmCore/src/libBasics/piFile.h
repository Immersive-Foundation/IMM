//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"
#include "../libBasics/piTArray.h"

namespace ImmCore {

class piFile
{
public:
    piFile();
    ~piFile();

    static bool  DirectoryCreate(const wchar_t *name, bool failOnExists);
    static bool  DirectoryExists(const wchar_t *dirName_in);
	static bool  DirectoryDelete(const wchar_t *name, bool evenIfNotEmpty);
    static uint64_t GetDiskSpace(const wchar_t *name);
    static bool  Exists(const wchar_t *name);
    static bool  HaveWriteAccess( const wchar_t *name );
    static bool  Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists);
	static bool  Rename(const wchar_t *dst, const wchar_t *src);
	static bool  Delete(const wchar_t *src);
	static uint64_t GetLength(const wchar_t *filename);
	static bool  ReadFromDisk(piTArray<uint8_t> *dst, const wchar_t *name);
    static bool  WriteToDisk(const piTArray<uint8_t> *dst, const wchar_t *name);

    typedef enum
    {
        CURRENT = 0,
        END = 1,
        SET = 2
    }SeekMode;

    bool         Open(const wchar_t *name, const wchar_t *mode);
    bool         Seek(uint64_t pos, SeekMode mode);
    uint64_t     Tell(void);
    void         Close( void );
    uint64_t     GetLength( void );

    //----------------------

    char *       ReadString(char *buffer, int num);
    void         Prints(const wchar_t *str);
    void         Printf(const wchar_t *format, ...);

    //-----------------------

	uint64_t     Read(void *dst, uint64_t amount);
    uint8_t      ReadUInt8( void );
    uint16_t     ReadUInt16( void );
    uint32_t     ReadUInt32( void );
    uint64_t     ReadUInt64( void );
    float        ReadFloat(void);
	double       ReadDouble(void);
    void         ReadFloatarray(float *dst, int num, int size, uint64_t amount);
    void         ReadFloatarray2(float *dst, uint64_t amount);
    void         ReadDoublearray2(double *dst, uint64_t amount);
    void         ReadUInt64array(uint64_t *dst, uint64_t amount);
    void         ReadUInt32array(uint32_t *dst, uint64_t amount);
    void         ReadUInt32array2(uint32_t *dst, uint64_t amount, int size, int stride);
    void	     ReadUInt16array(uint16_t *dst, uint64_t amount);
    void	     ReadUInt8array(uint8_t *dst, uint64_t amount);

	size_t       Write(const void *dst, uint64_t amount);
    void		 WriteUInt8(uint8_t x);
    void		 WriteUInt16(uint16_t x);
    void		 WriteUInt32(uint32_t x);
    void		 WriteUInt64(uint64_t x);
    void		 WriteFloat(float x);
    void		 WriteUInt8array(const uint8_t *dst, uint64_t amount);
    void		 WriteUInt16array(const uint16_t *dst, uint64_t amount);
    void		 WriteUInt32array(const uint32_t *dst, uint64_t amount);
    void		 WriteUInt64array(const uint64_t *dst, uint64_t amount);
    void		 WriteFloatarray(const float *ori, uint64_t amount);
    void		 WriteDoublearray(const double *ori, uint64_t amount);

private:
    void *mInternal;

};

} // namespace ImmCore
