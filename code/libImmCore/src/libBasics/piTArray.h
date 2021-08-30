//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <string.h>
#include <malloc.h>
#include "piTypes.h"
#include "piDebug.h"

namespace ImmCore {


template<typename piArrayType> class piTArray
{
public:
    piTArray();
    ~piTArray();

    bool         Init(uint64_t max, bool doZero );

	bool         InitCopyToMax(const piTArray *other);
	bool         InitCopyToNum(const piTArray *other);
	void         InitMove(piTArray *other);
	
	void         End(void);
    bool         IsInitialized( void ) const;

    bool         CopyToNumGrowNoShrink(const piTArray *other);
	void         CopyToNumNoGrowNoShrink(const piTArray *other);
	void         CopyToNumNoGrowNoShrink(const piArrayType *data, const uint64_t num);

	uint64_t     GetLength(void) const;
    void         SetLength(uint64_t num);
    void         Reset(void);
	uint64_t     GetMaxLength(void) const;
	bool         SetMaxLengthNoShrink(uint64_t maxLength);

    piArrayType *GetAddress(const uint64_t n) const;
    piArrayType  Get( const uint64_t id) const;
    void         Set( const piArrayType *obj, uint64_t pos);
	void         Set(const piArrayType obj, uint64_t pos);
	void         Set(const piArrayType *obj, uint64_t num, uint64_t pos);
	void         Set(piArrayType *obj, uint64_t max);
	piArrayType *Alloc(uint64_t num, bool doexpand = false);
    piArrayType *New(uint64_t num, bool doexpand = false);   // alloc AND construct

    piArrayType *Append( const piArrayType *obj, bool doexpand = false);
    bool         Append( const piArrayType v, bool doExpand = false);
	bool         Append(const piArrayType *obj, const uint64_t num, bool doexpand = false);

    piArrayType *InsertAndShift( const piArrayType *obj, uint64_t pos, bool doexpand = false);
    void         RemoveAndShift(uint64_t pos);
    void         Swap(uint64_t ida, uint64_t idb);

    piArrayType *begin() const { return mBuffer; }
    piArrayType *end() const { return mBuffer + mNum; }

    piArrayType & operator [](uint64_t i) { return mBuffer[i]; }
    const piArrayType & operator [](uint64_t i) const { return mBuffer[i]; }



private:
    piArrayType   *mBuffer;
    uint64_t       mMax;
	uint64_t       mNum;
	bool           mDoZero;
	#ifdef _DEBUG
	uint32_t       mInited;
    const uint32_t UNINITED_MARK = 0x81b0e26a;
    const uint32_t INITED_MARK = 0x12f8a4c3;

	#endif
};

//------------------------------------------------------------------------------------

template<typename piArrayType> 
piTArray<piArrayType>::piTArray()
{
    mBuffer = nullptr;
    mMax = 0;
    mNum = 0;
	mDoZero = false;
	#ifdef _DEBUG
	mInited = UNINITED_MARK;
	#endif
}

template<typename piArrayType> 
piTArray<piArrayType>::~piTArray()
{
}

template<typename piArrayType> 
bool piTArray<piArrayType>::IsInitialized(void) const
{
    return mMax > 0;
}

template<typename piArrayType> 
bool piTArray<piArrayType>::Init(uint64_t max, bool doZero )
{
	piAssert(mInited== UNINITED_MARK); // prevent double initialization
	#ifdef _DEBUG
	mInited = INITED_MARK;
	#endif

    mMax = max;
    mNum = 0;
	mDoZero = doZero;
	if (max > 0)
	{
		mBuffer = (piArrayType*)malloc(max * sizeof(piArrayType));
		if (mBuffer == nullptr)
			return false;

		if (doZero) memset(mBuffer, 0, max * sizeof(piArrayType));
	}
	else
	{
		mBuffer = nullptr;
	}


    return true;
}

template<typename piArrayType>
void piTArray<piArrayType>::InitMove(piTArray *other)
{
	piAssert(mInited== UNINITED_MARK); // prevent double initialization
	#ifdef _DEBUG
	mInited = INITED_MARK;
	#endif

	mBuffer = other->mBuffer;
	mNum = other->mNum;
	mMax = other->mMax;
	mDoZero = other->mDoZero;
}

template<typename piArrayType>
bool piTArray<piArrayType>::InitCopyToMax(const piTArray *other)
{
	piAssert(mInited == UNINITED_MARK); // prevent double initialization
	#ifdef _DEBUG
	mInited = INITED_MARK;
	#endif

	mMax = other->mMax;
	mNum = other->mNum;
	mDoZero = other->mDoZero;
	if (other->mMax > 0)
	{
		mBuffer = (piArrayType*)malloc(other->mMax * sizeof(piArrayType));
		if (mBuffer == nullptr)
			return false;
		memcpy(mBuffer, other->mBuffer, other->mMax * sizeof(piArrayType));
	}
	else
	{
		mBuffer = nullptr;
	}

	return true;
}

template<typename piArrayType>
bool piTArray<piArrayType>::InitCopyToNum(const piTArray *other)
{
	piAssert(mInited == UNINITED_MARK); // prevent double initialization
	#ifdef _DEBUG
	mInited = INITED_MARK;
	#endif

	mMax = other->mNum;
	mNum = other->mNum;
	mDoZero = other->mDoZero;
	if (other->mNum > 0)
	{
		mBuffer = (piArrayType*)malloc(other->mNum * sizeof(piArrayType));
		if (mBuffer == nullptr)
			return false;
		memcpy(mBuffer, other->mBuffer, other->mNum * sizeof(piArrayType));
	}
	else
	{
		mBuffer = nullptr;
	}

	return true;
}


template<typename piArrayType> 
void piTArray<piArrayType>::End(void)
{
	piAssert(mInited == INITED_MARK);
	#ifdef _DEBUG
	mInited = UNINITED_MARK;
	#endif

	if(mBuffer!=nullptr) free( mBuffer );
    mBuffer = nullptr;
	mMax = 0;
	mNum = 0;
}

template<typename piArrayType>
void piTArray<piArrayType>::CopyToNumNoGrowNoShrink(const piTArray *other)
{
	piAssert(mInited == INITED_MARK);
	piAssert( other->mNum <= mMax);
	mNum = other->mNum;
	memcpy(mBuffer, other->mBuffer, other->mNum * sizeof(piArrayType));
}

template<typename piArrayType>
bool piTArray<piArrayType>::CopyToNumGrowNoShrink(const piTArray *other)
{
    piAssert(mInited == INITED_MARK);

    if( other->mNum==0 )
    {
        mNum = 0;
        return true;
    }

    // fits
    if( other->mNum <= mMax )
    {
        mNum = other->mNum;
        memcpy(mBuffer, other->mBuffer, other->mNum * sizeof(piArrayType));
    }
    // doesn't fit, need to grow
    else
    {
        if (mBuffer != nullptr) free(mBuffer);

        mBuffer = (piArrayType*)malloc(other->mNum * sizeof(piArrayType));
        if (mBuffer == nullptr)
            return false;
        memcpy(mBuffer, other->mBuffer, other->mNum * sizeof(piArrayType));
        mNum = other->mNum;
        mMax = other->mNum;

    }
    return true;
}

template<typename piArrayType>
void piTArray<piArrayType>::CopyToNumNoGrowNoShrink(const piArrayType *data, const uint64_t num)
{
    piAssert(mInited == INITED_MARK);
    piAssert(num <= mMax);
	mNum = num;
	memcpy(mBuffer, data, num * sizeof(piArrayType));
}

template<typename piArrayType> 
void piTArray<piArrayType>::Reset(void)
{
    piAssert(mInited == INITED_MARK);
    if(mDoZero)
		memset( mBuffer, 0, mMax*sizeof(piArrayType) );
    mNum = 0;
}

template<typename piArrayType> 
uint64_t piTArray<piArrayType>::GetLength(void) const
{
    piAssert(mInited == INITED_MARK);
    return mNum;
}

template<typename piArrayType> 
uint64_t piTArray<piArrayType>::GetMaxLength(void) const
{
    piAssert(mInited == INITED_MARK);
    return mMax;
}

template<typename piArrayType>
bool piTArray<piArrayType>::SetMaxLengthNoShrink(uint64_t maxLength)
{
    piAssert(mInited == INITED_MARK);

	if (maxLength <= mMax) return true;

	uint64_t newmax = maxLength;
	if (newmax<4) newmax = 4;
	mBuffer = (piArrayType*)realloc(mBuffer, newmax * sizeof(piArrayType));
	if (!mBuffer)
		return false;
	mMax = newmax;
	if (mDoZero)
		memset(mBuffer + mNum, 0, (mMax - mNum) * sizeof(piArrayType));
	return true;
}

template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::GetAddress(const uint64_t n) const
{
    piAssert(mInited == INITED_MARK);
    return( mBuffer + n );
}

template<typename piArrayType> 
piArrayType piTArray<piArrayType>::Get(const uint64_t n) const
{ 
    piAssert(mInited == INITED_MARK);
    return mBuffer[n];
}


template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::Alloc(uint64_t num, bool doexpand )
{
    piAssert(mInited == INITED_MARK);

    if( (mNum+num)>mMax )
	{
		if( doexpand==false )
			return nullptr;

		if (!SetMaxLengthNoShrink(4 + 4 * (mNum+num) / 3))
			return nullptr;
	}

    piArrayType *ptr = mBuffer + mNum;

    mNum += num;

    return ptr;
}

template<typename piArrayType>
piArrayType *piTArray<piArrayType>::New(uint64_t num, bool doexpand)
{
    piAssert(mInited == INITED_MARK);

    piArrayType *ptr = this->Alloc(num, doexpand);
    if (ptr == nullptr) return nullptr;

    for (uint64_t i = 0; i < num; i++)
    {
        new (ptr+i)piArrayType();
    }

    return ptr;
}

template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::Append( const piArrayType * obj, bool doexpand )
{
    piAssert(mInited == INITED_MARK);

    if( mNum>=mMax )
    {
		if( !doexpand )
			return nullptr;

		if (!SetMaxLengthNoShrink(4 + 4 * mMax / 3))
			return nullptr;
	}

    piArrayType *ptr = mBuffer + mNum;
    memcpy( ptr, obj, sizeof(piArrayType) );
    mNum ++;
    return ptr;
}

template<typename piArrayType>
bool piTArray<piArrayType>::Append(const piArrayType * obj, const uint64_t num, bool doexpand)
{
    piAssert(mInited == INITED_MARK);

	if ((mNum+num) >= mMax)
	{
		if (!doexpand)
			return false;

		if (!SetMaxLengthNoShrink(4 + 4 * (mNum+num) / 3))
			return false;
	}

	piArrayType *ptr = mBuffer + mNum;
	memcpy(ptr, obj, num*sizeof(piArrayType));
	mNum+= num;
	return true;
}

template<typename piArrayType> 
bool piTArray<piArrayType>::Append( const piArrayType v, bool doExpand )
{
    piAssert(mInited == INITED_MARK);

    return Append( &v, doExpand ) != nullptr;
}


template<typename piArrayType> 
piArrayType *piTArray<piArrayType>::InsertAndShift(  const piArrayType *obj, uint64_t pos, bool doexpand )
{
    piAssert(mInited == INITED_MARK);

	const uint64_t num = mNum;

	if( mNum>=mMax )
    {

		if( !doexpand )
			return nullptr;

		if (!SetMaxLengthNoShrink(4 + 4 * mMax / 3))
			return nullptr;
	}

	for(uint64_t i=num; i>pos; i-- )
	{
		piArrayType *ori = mBuffer + (i-1);
		piArrayType *dst = mBuffer + (i+0);
		memcpy( dst, ori, sizeof(piArrayType) );

	}

    piArrayType *ptr = mBuffer + pos;
	if( obj ) memcpy( ptr, obj, sizeof(piArrayType) );
    mNum ++;
    return ptr;
}

template<typename piArrayType>
void piTArray<piArrayType>::Set(piArrayType *obj, uint64_t max)
{
    piAssert(mInited == INITED_MARK);
    mBuffer = obj;
	mMax = max;
}

template<typename piArrayType> 
void piTArray<piArrayType>::Set(  const piArrayType *obj, uint64_t pos )
{
    piAssert(mInited == INITED_MARK);
    piAssert(pos < mMax);
    mBuffer[pos] = *obj;
}

template<typename piArrayType>
void piTArray<piArrayType>::Set(const piArrayType *obj, uint64_t num, uint64_t pos)
{
    piAssert(mInited == INITED_MARK);
    piAssert(pos+num <= mMax);
	memcpy(mBuffer + pos, obj, num * sizeof(piArrayType));
}

template<typename piArrayType>
void piTArray<piArrayType>::Set(const piArrayType obj, uint64_t pos)
{
    piAssert(mInited == INITED_MARK);
    piAssert(pos < mMax);
	mBuffer[pos] = obj;
}

template<typename piArrayType> 
void piTArray<piArrayType>::RemoveAndShift(uint64_t pos )
{
    piAssert(mInited == INITED_MARK);

	const uint64_t num = mNum;

	const uint64_t numelems = num-1-pos;
	if( numelems>0 )
	{
        piArrayType *ptr = mBuffer + pos;
        for(uint64_t j=0; j<numelems; j++ )
        {
            //ptr[j] = ptr[j+1]; // this can invoke a copy or move constructor
            ptr[j] = static_cast<piArrayType&&>( ptr[j+1] );  // call move
            //memcpy(ptr + j, ptr + j + 1, sizeof(piArrayType));
        }
	}
    mNum --;

	if (mDoZero)
		memset(mBuffer + mNum, 0, sizeof(piArrayType));
}

template<typename piArrayType>
void piTArray<piArrayType>::Swap(uint64_t ida, uint64_t idb)
{
    piAssert(mInited == INITED_MARK);

	piArrayType tmp = mBuffer[ida];
    mBuffer[ida] = mBuffer[idb];
    mBuffer[idb] = tmp;
}

template<typename piArrayType> 
void piTArray<piArrayType>::SetLength(uint64_t num )
{
    piAssert(mInited == INITED_MARK);
    piAssert(num <= mMax);
	mNum = num;
}

} // namespace ImmCore
