//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include <string.h>
#include "piPool.h"
#include "piTypes.h"
#include "piDebug.h"

namespace ImmCore
{
#ifdef _DEBUG
    const uint32_t piPool::UNINITED_MARK = 0x72e691a1;
    const uint32_t piPool::INITED_MARK = 0xb681a034;
#endif

    enum SlotState:uint8_t {
        NEW = 0,
        USED =1,
        FREED = 2
    };

    piPool::piPool()
    {
        mBuffer = nullptr;
        mUsed = nullptr;
        mMax = 0;
        mObjectsize = 0;
		#ifdef _DEBUG
		mInited = UNINITED_MARK;
		#endif
    }

    piPool::~piPool()
    {
        mBuffer = nullptr;
        mUsed = nullptr;
    }

    bool piPool::Init(uint64_t max, unsigned int objectsize)
    {
        piAssert(mInited == UNINITED_MARK); // prevent double initialization

        mObjectsize = objectsize;
        mMax = max;
        mBuffer = (unsigned char*)malloc(max*objectsize);
        mUsed = (unsigned char *)malloc(max*sizeof(uint8_t));
        if (mBuffer == nullptr || mUsed == nullptr)
            return false;

        // mark all as unused
        memset(mUsed, NEW, mMax * sizeof(uint8_t));
        
		#ifdef _DEBUG
		mInited = INITED_MARK;
		#endif

        return true;
    }

    void piPool::End(void)
    {
        piAssert(mInited == INITED_MARK);
	    #ifdef _DEBUG
	    mInited = UNINITED_MARK;
	    #endif

        free(mBuffer);
        free(mUsed);
        mBuffer = nullptr;
        mUsed = nullptr;
        mMax = 0;
        mObjectsize = 0;
    }

    void piPool::Reset(void)
    {
        piAssert(mInited == INITED_MARK);
        // mark used as freed
        for (uint64_t i = 0; i < mMax; i++)
        {
            if (mUsed[i] == USED) mUsed[i] = FREED;
        }        
    }
    /*
    uint64_t piPool::GetLength(void) const
    {
        piAssert(mInited == INITED_MARK);
        return mNum;
    }*/

    void *piPool::GetAddress(const uint64_t n) const
    {
        piAssert(mInited == INITED_MARK);
        return (void*)(mBuffer + n * mObjectsize);
    }

    uint64_t piPool::GetMaxLength(void) const
    {
        piAssert(mInited == INITED_MARK);
        return mMax;
    }

    uint64_t piPool::GetUInt64(uint64_t id) const { return *((uint64_t*)(mBuffer + id * mObjectsize)); }
    int64_t  piPool::GetSInt64(uint64_t id) const { return *((int64_t*)(mBuffer + id * mObjectsize)); }
    uint32_t piPool::GetUInt32(uint64_t id) const { return *((uint32_t*)(mBuffer + id * mObjectsize)); }
    int32_t  piPool::GetSInt32(uint64_t id) const { return *((int32_t*)(mBuffer + id * mObjectsize)); }
    uint16_t piPool::GetUInt16(uint64_t id) const { return *((uint16_t*)(mBuffer + id * mObjectsize)); }
    int16_t  piPool::GetSInt16(uint64_t id) const { return *((int16_t*)(mBuffer + id * mObjectsize)); }
    uint8_t  piPool::GetUInt8(uint64_t id) const { return *((uint8_t*)(mBuffer + id * mObjectsize)); }
    int8_t   piPool::GetSInt8(uint64_t id) const { return *((int8_t*)(mBuffer + id * mObjectsize)); }
    void   * piPool::GetPtr(uint64_t id) const { return *((void **)(mBuffer + id * mObjectsize)); }


    bool piPool::IsUsed(uint64_t id) const
    {
        piAssert(mInited == INITED_MARK);
        return mUsed[id] == USED;
    }

    void piPool::Free(uint64_t id)
    {
        piAssert(mInited == INITED_MARK);
        mUsed[id] = FREED;
    }

    void *piPool::iAllocateSlot(uint64_t id)
    {
        mUsed[id] = USED;
        return (void*)(mBuffer + id*mObjectsize);
    }


    void *piPool::Alloc(bool * isNew, uint64_t *id, bool allowGrow)
    {
        piAssert(mInited == INITED_MARK);
   
        // first, try to find an unused one
        const uint64_t num = mMax;
        for (uint64_t i = 0; i < num; i++)
        {
            if (mUsed[i] != USED)
            {
                // allocate
                *isNew = mUsed[i] == NEW;
                if (id) *id = i;
                return iAllocateSlot(i);
            }
        }

        // no empty slots found. We need to grow (by 1.33x)
        if (!allowGrow) return nullptr;

        const uint64_t oldMax = mMax;
        const uint64_t newMax = 4 + (4*mMax)/3;
        mBuffer = (unsigned char*)realloc(mBuffer, newMax * mObjectsize);
        mUsed = (unsigned char*)realloc(mUsed, newMax * sizeof(unsigned char));
        if (mBuffer==nullptr || mUsed==nullptr)
            return nullptr;
        for (uint64_t i = oldMax; i < newMax; i++) mUsed[i] = 0;
        mMax = newMax;

        // allocate
        if (id) *id = oldMax;
        *isNew = true;
        return iAllocateSlot(oldMax);
    }
}
