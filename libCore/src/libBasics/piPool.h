//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore
{
    class piPool
    {
    public:
        piPool();
        ~piPool();

        bool         Init(uint64_t max, unsigned int objectsize);
        void         End(void);
        void         Reset(void);
        uint64_t     GetMaxLength(void) const;
        void        *Alloc(bool * isNew, uint64_t *id, bool allowGrow = true);
        void         Free(uint64_t id);
        bool         IsUsed(uint64_t id) const;

        void        *GetAddress(const uint64_t n) const;
        uint64_t     GetUInt64(uint64_t id) const;
        int64_t      GetSInt64(uint64_t id) const;
        uint32_t     GetUInt32(uint64_t id) const;
        int32_t      GetSInt32(uint64_t id) const;
        uint16_t     GetUInt16(uint64_t id) const;
        int16_t      GetSInt16(uint64_t id) const;
        uint8_t      GetUInt8(uint64_t id) const;
        int8_t       GetSInt8(uint64_t id) const;
        void        *GetPtr(uint64_t id) const;


    private:
        uint8_t  *mBuffer;
        uint8_t  *mUsed;
        uint64_t  mMax;
        uint32_t  mObjectsize;
	    #ifdef _DEBUG
        static const uint32_t UNINITED_MARK;
        static const uint32_t INITED_MARK;
        uint32_t  mInited;
	    #endif

        void *iAllocateSlot(uint64_t id);
    };

} // namespace ImmCore
