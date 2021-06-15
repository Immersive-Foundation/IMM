//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <malloc.h>
#include <string.h>
#include "piArray.h"
#include "piTypes.h"
#include "piStr.h"
#include "piDebug.h"


namespace ImmCore {

#ifdef _DEBUG
    const uint32_t piArray::UNINITED_MARK = 0x81b0e26a;
    const uint32_t piArray::INITED_MARK = 0x12f8a4c3;
#endif

	piArray::piArray()
	{
		mBuffer = nullptr;
		mMax = 0;
		mNum = 0;
		mObjectsize = 0;
		mDoZero = false;
		#ifdef _DEBUG
		mInited = UNINITED_MARK;
		#endif
	}

	piArray::~piArray()
	{
	}

	bool piArray::IsInitialized(void) const
	{
		return (mObjectsize > 0);
	}

	bool piArray::Init(uint64_t max, unsigned int objectsize, bool doZero)
	{
		piAssert(mInited == UNINITED_MARK); // prevent double initialization

		mObjectsize = objectsize;
		mMax = max;
		mNum = 0;
		mDoZero = doZero;
		mBuffer = nullptr;
		
		if(max>0)
		{
			mBuffer = (unsigned char*)malloc(max*objectsize);
			if (!mBuffer)
				return false;

			if (mDoZero)
				memset(mBuffer, 0, max*objectsize);
		}
		#ifdef _DEBUG
		mInited = INITED_MARK;
		#endif
		return true;
	}

	void piArray::InitMove(piArray *other)
	{
		piAssert(mInited == UNINITED_MARK); // prevent double initialization

		mBuffer = other->mBuffer;
		mNum = other->mNum;
		mMax = other->mMax;
		mObjectsize = other->mObjectsize;
		mDoZero = other->mDoZero;

		#ifdef _DEBUG
		mInited = INITED_MARK;
		#endif
	}

	bool piArray::InitCopy(piArray *other)
	{
		piAssert(mInited == UNINITED_MARK); // prevent double initialization
		#ifdef _DEBUG
		mInited = INITED_MARK;
		#endif

		mNum = other->mNum;
		mMax = other->mMax;
		mObjectsize = other->mObjectsize;
		mDoZero = other->mDoZero;
		if (other->mNum > 0)
		{
			mBuffer = (unsigned char*)malloc(other->mNum * other->mObjectsize);
			if (mBuffer == nullptr)
				return false;
			memcpy(mBuffer, other->mBuffer, other->mNum * other->mObjectsize);
		}
		else
		{
			mBuffer = nullptr;
		}
		return true;

	}

	void piArray::CopyToNumNoGrowNoShrink(piArray *other)
	{
		piAssert(mInited == INITED_MARK);
		piAssert(other->mNum <= mMax);

		if (other->mNum == 0)
		{
			mNum = 0;
			return;
		}
		mNum = other->mNum;
		mMax = other->mMax;
		mObjectsize = other->mObjectsize;
		mDoZero = other->mDoZero;
		memcpy(mBuffer, other->mBuffer, mNum * mObjectsize);
	}

	void piArray::CopyToNumGrowNoShrink(piArray *other)
	{
		piAssert(mInited == INITED_MARK);

		mNum = other->mNum;
		mObjectsize = other->mObjectsize;
		mDoZero = other->mDoZero;

		if (other->mNum == 0)
		{
			return;
		}
		// fits
		if (other->mNum <= mMax)
		{
			memcpy(mBuffer, other->mBuffer, mNum * mObjectsize);
		}
		// doesn't fit, need to grow
		else
		{
			if (mBuffer != nullptr) 
				free(mBuffer);

			mBuffer = (unsigned char*)malloc(mNum * mObjectsize);
			if (mBuffer == nullptr)
				return;
			memcpy(mBuffer, other->mBuffer, mNum * mObjectsize);
			mMax = other->mMax;

		}
	}


	void piArray::End(void)
	{
		piAssert(mInited == INITED_MARK);
	    #ifdef _DEBUG
	    mInited = UNINITED_MARK;
	    #endif

		free(mBuffer);
		mBuffer = 0;
		mMax = 0;
		mNum = 0;
		mObjectsize = 0;
	}

	void piArray::Reset(void)
	{
		piAssert(mInited == INITED_MARK);

		if (mDoZero)
			memset(mBuffer, 0, mMax*mObjectsize);
		mNum = 0;
	}

	uint64_t piArray::GetLength(void) const
	{
		piAssert(mInited == INITED_MARK);

		return(mNum);
	}

	uint64_t piArray::GetMaxLength(void) const
	{
		piAssert(mInited == INITED_MARK);

		return mMax;
	}

	void *piArray::GetAddress(const uint64_t n) const
	{
		piAssert(mInited == INITED_MARK);

		return((void*)(mBuffer + n*mObjectsize));
	}

	void  *piArray::Alloc(uint64_t num, bool doexpand)
	{
		piAssert(mInited == INITED_MARK);

		if ((mNum + num) > mMax)
		{
			if (doexpand == false)
				return nullptr;

			uint64_t newmax = 4 * mMax / 3;
			if (newmax < 4) newmax = 4;
			mBuffer = (unsigned char*)realloc(mBuffer, newmax*mObjectsize);
			if (!mBuffer)
				return nullptr;
			mMax = newmax;
			if (mDoZero)
				memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		}

		unsigned char *ptr = mBuffer + mNum*mObjectsize;

		mNum += num;

		return(ptr);
	}



	void *piArray::Append(const void *obj, bool doexpand)
	{
		piAssert(mInited == INITED_MARK);

		if (mNum >= mMax)
		{
			if (!doexpand)
				return nullptr;

			uint64_t newmax = 4 * mMax / 3;
			if (newmax < 4) newmax = 4;
			mBuffer = (unsigned char*)realloc(mBuffer, newmax*mObjectsize);
			if (!mBuffer)
				return nullptr;
			mMax = newmax;
			if (mDoZero)
				memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		}
		unsigned char *ptr = mBuffer + mNum*mObjectsize;
		memcpy(ptr, obj, mObjectsize);
		mNum++;
		return(ptr);
	}

	void *piArray::Append(const void *obj, uint64_t num, bool doexpand)
	{
		piAssert(mInited == INITED_MARK);

		if ((num + mNum) >= mMax)
		{
			if (!doexpand)
				return nullptr;

			uint64_t newmax = 4 * (num + mNum) / 3;
			if (newmax < 4) newmax = 4;
			mBuffer = (unsigned char*)realloc(mBuffer, newmax*mObjectsize);
			if (!mBuffer)
				return nullptr;
			mMax = newmax;
			if (mDoZero)
				memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		}
		unsigned char *ptr = mBuffer + mNum*mObjectsize;
		memcpy(ptr, obj, mObjectsize*num);
		mNum += num;
		return(ptr);
	}

	bool piArray::Set(const void *obj, uint64_t pos)
	{
		piAssert(mInited == INITED_MARK);

		const uint64_t num = mNum;
		const uint64_t obs = mObjectsize;
		if (pos >= mMax)
			return false;
		unsigned char *ptr = mBuffer + pos*obs;
		if (obj)
			memcpy(ptr, obj, mObjectsize);
		else
			memset(ptr, 0, mObjectsize);
		return true;
	}

	void *piArray::Insert(const void *obj, uint64_t pos, bool doexpand)
	{
		piAssert(mInited == INITED_MARK);

		const uint64_t num = mNum;
		const unsigned int obs = mObjectsize;

		if (mNum >= mMax)
		{

			if (!doexpand)
				return nullptr;

			uint64_t newmax = 4 * mMax / 3;
			if (newmax < 4) newmax = 4;
			mBuffer = (unsigned char*)realloc(mBuffer, newmax*mObjectsize);
			if (!mBuffer)
				return nullptr;
			mMax = newmax;
			if (mDoZero)
				memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		}

		for (uint64_t i = num; i > pos; i--)
		{
			unsigned char *ori = mBuffer + (i - 1)*obs;
			unsigned char *dst = mBuffer + (i + 0)*obs;
			memcpy(dst, ori, obs);
		}

		unsigned char *ptr = mBuffer + pos*obs;
		if (obj)
			memcpy(ptr, obj, mObjectsize);
		else
			memset(ptr, 0, mObjectsize);
		mNum++;
		return(ptr);

	}

	void piArray::RemoveAndShift(uint64_t pos)
	{
		piAssert(mInited == INITED_MARK);

		const uint64_t num = mNum;
		const unsigned int obs = mObjectsize;

		const uint64_t numelems = num - 1 - pos;
		if (numelems > 0)
		{
			unsigned char *ptr = mBuffer + pos*obs;
			for (uint64_t j = 0; j < numelems; j++)
			{
				for (uint64_t i = 0; i < obs; i++)
					ptr[i] = ptr[obs + i];
				ptr += obs;
			}
		}

		mNum--;
		if (mDoZero)
		{
			memset(mBuffer + mNum*obs, 0, obs);
		}
	}

	bool piArray::SetLength(uint64_t num, bool doexpand)
	{
		piAssert(mInited == INITED_MARK);

		if (num > mMax)
		{
			if (!doexpand) return false;
			mBuffer = (unsigned char*)realloc(mBuffer, num*mObjectsize);
			if (!mBuffer)
				return false;
			mMax = num;
			if (mDoZero)
				memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		}

		mNum = num;
		return true;
	}

	bool piArray::SetMaxLength(uint64_t num)
	{
		piAssert(mInited == INITED_MARK);

		mBuffer = (unsigned char*)realloc(mBuffer, num*mObjectsize);
		if (!mBuffer)
			return false;
		mMax = num;
		if (mDoZero && mMax > mNum)
			memset(mBuffer + mNum*mObjectsize, 0, (mMax - mNum)*mObjectsize);
		return true;
	}


	bool piArray::SetMaxLengthNoShrink(uint64_t maxLength)
	{
		piAssert(mInited == INITED_MARK);

		if (maxLength <= mMax) return true;

		uint64_t newmax = maxLength;
		if (newmax<4) newmax = 4;
		mBuffer = (unsigned char*)realloc(mBuffer, newmax * mObjectsize);
		if (!mBuffer)
			return false;
		mMax = newmax;
		if (mDoZero)
			memset(mBuffer + mNum, 0, (mMax-mNum)*mObjectsize);
		return true;
	}


	void piArray::SetPtr(uint64_t id, const void *val)
	{
		piAssert(mInited == INITED_MARK);

		((void**)mBuffer)[id] = (void*)val;
	}


	bool piArray::AppendUInt8(uint8_t  v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendSInt8(int8_t   v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendUInt16(uint16_t v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendSInt16(int16_t  v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendUInt32(uint32_t v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendSInt32(int32_t  v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendUInt64(uint64_t v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendSInt64(int64_t  v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendFloat(float  v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendDouble(double v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendPtr(void * v, bool doExpand) { return piArray::Append(&v, doExpand) != nullptr; }
	bool piArray::AppendText(const char *s, bool doExpand) { return piArray::Append(s, pistrlen(s), doExpand) != nullptr; }

	uint64_t piArray::GetUInt64(uint64_t id) const { return ((uint64_t*)mBuffer)[id]; }
	int64_t  piArray::GetSInt64(uint64_t id) const { return ((int64_t*)mBuffer)[id]; }
	uint32_t piArray::GetUInt32(uint64_t id) const { return ((uint32_t*)mBuffer)[id]; }
	int32_t  piArray::GetSInt32(uint64_t id) const { return ((int32_t*)mBuffer)[id]; }
	uint16_t piArray::GetUInt16(uint64_t id) const { return ((uint16_t*)mBuffer)[id]; }
	int16_t  piArray::GetSInt16(uint64_t id) const { return ((int16_t*)mBuffer)[id]; }
	uint8_t  piArray::GetUInt8(uint64_t id) const { return ((uint8_t *)mBuffer)[id]; }
	int8_t   piArray::GetSInt8(uint64_t id) const { return ((int8_t *)mBuffer)[id]; }
	void   * piArray::GetPtr(uint64_t id) const { return ((void **)mBuffer)[id]; }

}
