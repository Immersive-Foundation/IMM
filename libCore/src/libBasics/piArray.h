//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore {

class piArray
{
public:
    piArray();
    ~piArray();

    bool         Init(uint64_t max, unsigned int objectsize, bool doZero = true);
	void         InitMove(piArray *other);
	bool         InitCopy(piArray *other);
    void         End(void);
	uint64_t     GetLength(void) const;
	uint64_t     GetMaxLength(void) const;
    bool         IsInitialized( void ) const;

	void         CopyToNumGrowNoShrink(piArray *other);
	void         CopyToNumNoGrowNoShrink(piArray *other);

    void   Reset(void);
    bool   SetLength(uint64_t, bool doexpand = false);
    bool   SetMaxLength(uint64_t);
	bool   SetMaxLengthNoShrink(uint64_t maxLength);
    void  *GetAddress(const uint64_t n) const;

    void  *Alloc(uint64_t num, bool doexpand = false);
    void  *Append( const void *obj, bool doexpand = false);
	void  *Append(const void *obj, uint64_t num, bool doexpand = false);
    void  *Insert( const void *obj, uint64_t pos, bool doexpand = false);
    bool   Set( const void *obj, uint64_t pos);
    void   RemoveAndShift(uint64_t pos);

    bool   AppendUInt8(uint8_t  v, bool doExpand = false);
    bool   AppendSInt8(int8_t  v, bool doExpand = false);
    bool   AppendUInt16(uint16_t v, bool doExpand = false);
    bool   AppendSInt16(int16_t v, bool doExpand = false);
    bool   AppendUInt32(uint32_t v, bool doExpand = false);
    bool   AppendSInt32(int32_t v, bool doExpand = false);
    bool   AppendUInt64(uint64_t v, bool doExpand = false);
    bool   AppendSInt64(int64_t v, bool doExpand = false);
    bool   AppendFloat(float  v, bool doExpand = false);
    bool   AppendDouble(double v, bool doExpand = false);
    bool   AppendPtr( void * v, bool doExpand=false);
	bool   AppendText(const char *str, bool doExpand=false);

    void   SetPtr(uint64_t id, const void *val);

    uint64_t GetUInt64(uint64_t id) const;
    int64_t  GetSInt64(uint64_t id) const;
    uint32_t GetUInt32(uint64_t id) const;
    int32_t  GetSInt32(uint64_t id) const;
    uint16_t GetUInt16(uint64_t id) const;
    int16_t  GetSInt16(uint64_t id) const;
    uint8_t  GetUInt8(uint64_t id) const;
    int8_t   GetSInt8(uint64_t id) const;
    void    *GetPtr(uint64_t id) const;

private:
    uint8_t  *mBuffer;
	uint64_t  mMax;
	uint64_t  mNum;
    uint32_t  mObjectsize;
	bool      mDoZero;
	#ifdef _DEBUG
    static const uint32_t UNINITED_MARK;
    static const uint32_t INITED_MARK;
	uint32_t  mInited;
	#endif
};

} // namespace ImmCore
