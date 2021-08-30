//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <string.h>
#include <malloc.h>
#include "../libBasics/piTypes.h"

namespace ImmCore {


    template<typename piPoolType> class piTPool
    {
    public:
        piTPool();
        ~piTPool();

        bool         Init(const uint64_t max);
        void         End(void);

        piPoolType * Allocate(bool * isNew, uint64_t *id);
        void         Free(uint64_t id);
        void         Reset(void);
        piPoolType * Get(const uint64_t n) const;
        uint64_t     GetLength(void) const;

        piPoolType & operator [](uint64_t i) { return mBuffer[i]; }
        const piPoolType & operator [](uint64_t i) const { return mBuffer[i]; }


    private:
        piPoolType *mBuffer;
        bool       *mUsed;
        uint64_t    mMax;
        uint64_t    mNum;
    	#ifdef _DEBUG
	    uint32_t    mInited;
        const uint32_t UNINITED_MARK = 0x81b0e26a;
        const uint32_t INITED_MARK = 0x12f8a4c3;
	    #endif
};

    //------------------------------------------------------------------------------------

    template<typename piPoolType>
    piTPool<piPoolType>::piTPool()
    {
        mBuffer = nullptr;
        mUsed = nullptr;
        mMax = 0;
        mNum = 0;
	    #ifdef _DEBUG
	    mInited = UNINITED_MARK;
	    #endif
    }

    template<typename piPoolType>
    piTPool<piPoolType>::~piTPool()
    {
    }

    template<typename piPoolType>
    bool piTPool<piPoolType>::Init(const uint64_t max)
    {
        #ifdef _DEBUG
	    piAssert(mInited== UNINITED_MARK); // prevent double initialization
	    mInited = INITED_MARK;
	    #endif

        mMax = max;
        mNum = 0;

        mBuffer = (piPoolType*)malloc(max*sizeof(piPoolType));
        mUsed = (bool*)malloc(max * sizeof(bool));
        if (mBuffer == nullptr || mUsed == nullptr)
            return false;

        this->Reset();
        return true;
    }

    template<typename piPoolType>
    void piTPool<piPoolType>::End(void)
    {
        #ifdef _DEBUG
	    piAssert(mInited== INITED_MARK); // prevent double initialization
	    mInited = UNINITED_MARK;
	    #endif

        if (mBuffer != nullptr)
        {
            free(mBuffer);
            free(mUsed);
        }
        mBuffer = 0;
        mMax = 0;
        mNum = 0;
    }

    template<typename piPoolType>
    void piTPool<piPoolType>::Free(uint64_t id)
    {
        #ifdef _DEBUG
        piAssert(mInited == INITED_MARK);
        piAssert(mUsed[id] == true);
        #endif
        mUsed[id] = false;
    }

    template<typename piPoolType>
    piPoolType * piTPool<piPoolType>::Allocate(bool * isNew, uint64_t *id)
    {
        #ifdef _DEBUG
        piAssert(mInited == INITED_MARK);
        #endif
        // first, try to find an unused one
        const uint64_t num = mNum;
        for (uint64_t i = 0; i<num; i++)
        {
            if (mUsed[i] == false)
            {
                *isNew = false;
                mUsed[i] = true; // mark it as used
                if( id ) *id = i;
                return mBuffer+i;
            }
        }

        // allocate a new one

        // grow if needed
        {
            if ((mNum + 1)>mMax)
            {
                uint64_t newmax = 4 * mMax / 3; if (newmax<4) newmax = 4;
                mBuffer = (piPoolType*)realloc(mBuffer, newmax*sizeof(piPoolType));
                mUsed = (bool*)realloc(mUsed, newmax * sizeof(bool));
                if (!mBuffer || !mUsed)
                    return nullptr;
                mMax = newmax;
                for (uint64_t i = mNum; i<mMax; i++)
                {
                    mUsed[i] = false;
                }
            }
        }

        // allocate
        if (id) *id = mNum;
        mUsed[mNum] = true;
        piPoolType *res = mBuffer + mNum;
        mNum += 1;
        *isNew = true;
        return res;
    }

    template<typename piPoolType>
    void piTPool<piPoolType>::Reset(void)
    {
        #ifdef _DEBUG
        piAssert(mInited == INITED_MARK);
        #endif
        // mark all as unused
        const uint64_t num = mNum;
        for (uint64_t i = 0; i<num; i++)
        {
            mUsed[i] = false;
        }
    }

    template<typename piPoolType>
    piPoolType * piTPool<piPoolType>::Get(const uint64_t id) const
    {
        #ifdef _DEBUG
        piAssert(mInited == INITED_MARK);
        piAssert(mUsed[id] == true);
        #endif
        return mBuffer + id;
    }

    template<typename piPoolType>
    uint64_t piTPool<piPoolType>::GetLength(void) const
    {
        #ifdef _DEBUG
        piAssert(mInited == INITED_MARK);
        #endif
        return mNum;
    }





} // namespace ImmCore
