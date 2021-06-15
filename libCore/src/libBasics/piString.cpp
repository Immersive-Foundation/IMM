//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <wchar.h>
#include "piString.h"
#include "piStr.h"
#include "piDebug.h"

#if defined(ANDROID)
#include "../libBasics/android/piCStr.h"
#endif

namespace ImmCore {

#ifdef _DEBUG
const uint32_t piString::UNINITED_MARK = 0x40f9247a;
const uint32_t piString::INITED_MARK = 0x39ab3022;
#endif

piString::piString()
{
    mBuffer = nullptr;
    mMax = 0;
    mNum = 0;
    #ifdef _DEBUG
    mInited = UNINITED_MARK;
    #endif
}

piString::~piString()
{
    piAssert(mInited == UNINITED_MARK); // detect memory leaks
}

piString & piString::operator=(piString && ori)
{
    if (&ori == this) return *this;

    piAssert(mInited == INITED_MARK); // prevent double initialization

    if (mBuffer != nullptr) free(mBuffer);
    mBuffer = ori.mBuffer;
    mMax = ori.mMax;
    mNum = ori.mNum;
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    ori.mBuffer = nullptr;
    ori.mMax = 0;
    ori.mNum = 0;
    #ifdef _DEBUG
    ori.mInited = UNINITED_MARK;
    #endif

    return *this;
}

bool piString::iInit(uint32_t max)
{
    mBuffer = (wchar_t*)malloc((max + 1) * sizeof(wchar_t));
    if (!mBuffer)
        return false;

    mBuffer[0] = 0;
    mMax = max;
    mNum = 0;

    return true;
}


bool piString::Init(uint32_t max )
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    return iInit( max );
}

void piString::InitMove( piString *ori)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization

    mBuffer = ori->mBuffer;
    mMax = ori->mMax;
    mNum = ori->mNum;
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    ori->mBuffer = nullptr;
    ori->mMax = 0;
    ori->mNum = 0;
    #ifdef _DEBUG
    ori->mInited = UNINITED_MARK;
    #endif
}


bool piString::InitCopyW(const wchar_t *ori)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
#ifdef _DEBUG
    mInited = INITED_MARK;
#endif

    if (!ori) { mNum = 0; mBuffer = nullptr; mMax = 0; return true; }

    const uint32_t len = piwstrlen(ori);
    if (!iInit(len))
        return false;
    mNum = len;
    if (len > 0)
        memcpy(mBuffer, ori, len * sizeof(wchar_t));
    mBuffer[len] = 0;
    return true;
}

bool piString::InitCopyS(const char *ori, uint32_t numChars)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    if (!ori) { mNum = 0; mBuffer = nullptr; mMax = 0; return 1; }

    const uint32_t len = numChars;

    if (!iInit(len))
        return false;
    mNum = len;

#if defined(WINDOWS)
    if (!MultiByteToWideChar(CP_ACP, 0, ori, len, mBuffer, len))
        return false;
#elif defined(ANDROID)
    if (len > 0)
    {
        ScopedCStr wstr(pistr2ws(ori));
        const wchar_t *wori = wstr.w_cstr();
        memcpy(mBuffer, wori, len * sizeof(wchar_t));
    }
#endif
    mBuffer[len] = 0;
    return true;
}

bool piString::InitCopy(const piString *ori)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    if (!ori) { mNum = 0; mBuffer = nullptr; mMax = 0; return 1; }
    const uint32_t len = ori->mNum;
    if (!iInit(len))
        return false;
    mNum = len;
    memcpy(mBuffer, ori->mBuffer, len * sizeof(wchar_t));
    mBuffer[len] = 0;
    return true;
}

bool piString::InitConcatW(const wchar_t *ori1, const wchar_t *ori2)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    const uint32_t len1 = piwstrlen(ori1);
    const uint32_t len2 = piwstrlen(ori2);
    if (!iInit(len1 + len2))
        return false;
    mNum = len1 + len2;
    memcpy(mBuffer, ori1, len1 * sizeof(wchar_t));
    memcpy(mBuffer + len1, ori2, len2 * sizeof(wchar_t));

    mBuffer[len1 + len2] = 0;
    return true;
}

bool piString::InitConcatW(const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    const unsigned int len1 = piwstrlen(ori1);
    const unsigned int len2 = piwstrlen(ori2);
    const unsigned int len3 = piwstrlen(ori3);
    const unsigned int lent = len1 + len2 + len3;

    if (!iInit(lent))
        return false;
    mNum = lent;
    memcpy(mBuffer, ori1, len1 * sizeof(wchar_t));
    memcpy(mBuffer + len1, ori2, len2 * sizeof(wchar_t));
    memcpy(mBuffer + len1 + len2, ori3, len3 * sizeof(wchar_t));

    mBuffer[lent] = 0;
    return true;
}

bool piString::InitCopyS(const char *ori)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    if (!ori) { mNum = 0; mBuffer = 0; mMax = 0; return true; }

    const uint32_t len = (uint32_t)strlen(ori);

    if (!iInit(len))
        return false;
    mNum = len;

    if (len > 0)
    {
#ifdef WINDOWS
        if (!MultiByteToWideChar(CP_ACP, 0, ori, len, mBuffer, len))
            return false;
#elif defined(ANDROID)
        if (len > 0)
        {
            ScopedCStr wstr(pistr2ws(ori));
            const wchar_t *wori = wstr.w_cstr();
            memcpy(mBuffer, wori, len * sizeof(wchar_t));
        }
#endif
    }

    mBuffer[len] = 0;

    return true;
}

bool piString::InitWrapW(const wchar_t *ori)
{
    piAssert(mInited == UNINITED_MARK); // prevent double initialization
    #ifdef _DEBUG
    mInited = INITED_MARK;
    #endif

    if (ori == 0)
    {
        mMax = 0;
        mNum = 0;
        mBuffer = 0;
    }
    else
    {
        const uint32_t len = piwstrlen(ori);
        mMax = len;
        mNum = len;
        mBuffer = (wchar_t*)ori;
    }
    return true;
}
void piString::End( void )
{
	piAssert(mInited == INITED_MARK);
	#ifdef _DEBUG
	mInited = UNINITED_MARK;
	#endif

    if( mBuffer != nullptr ) free( mBuffer );
    mBuffer = nullptr;
	mMax = 0;
	mNum = 0;
}

bool piString::IsNull(void) const
{
    piAssert(mInited == INITED_MARK);
    return mNum == 0;
}

void piString::SetNull(void)
{
    piAssert(mInited == INITED_MARK);
    mNum = 0;
}
bool piString::Copy( const piString *ori )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = ori->mNum;
	if (len>mMax)
	{
		const uint32_t newmax = (4*len)/3;
 		mBuffer = (wchar_t*)realloc(mBuffer, (1 + newmax) * sizeof(wchar_t));
		if (!mBuffer)
			return false;
		mMax = newmax;
	}
	mNum = ori->mNum;
	memcpy( mBuffer, ori->mBuffer, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return true;
}

bool piString::CopyS( const char *ori )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = static_cast<uint32_t>(strlen( ori ));
    if (len > mMax)
    {
        const uint32_t newLen = len + 8;
        mBuffer = (wchar_t*)malloc((newLen + 1) * sizeof(wchar_t));
        if (!mBuffer)
            return false;
        mMax = newLen;
    }

	mNum = len;

    #ifdef WINDOWS
	if (!MultiByteToWideChar(CP_ACP, 0, ori, len, mBuffer, len))
		return false;
    #elif defined(ANDROID)
    ScopedCStr wstr(pistr2ws(ori));
    int ret = CopyW(wstr.w_cstr());
    return ret;
    #endif

    mBuffer[len] = 0;

	return true;
}

bool piString::CopyW( const wchar_t *ori )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = piwstrlen( ori );
    if (len > mMax)
    {
        const uint32_t newLen = len + 8;
        mBuffer = (wchar_t*)malloc((newLen+1) * sizeof(wchar_t));
        if (!mBuffer)
            return false;
        mMax = newLen;
    }

	mNum = len;
	memcpy( mBuffer, ori, len*sizeof(wchar_t) );
	mBuffer[len] = 0;
	return true;
}


bool piString::ConcatW(const wchar_t *ori1, const wchar_t *ori2)
{
    piAssert(mInited == INITED_MARK);

    const uint32_t len1 = piwstrlen(ori1);
    const uint32_t len2 = piwstrlen(ori2);
    const uint32_t lent = len1 + len2;

    if (lent > mMax)
    {
        const uint32_t newLen = lent + 8;
        mBuffer = (wchar_t*)malloc((newLen + 1) * sizeof(wchar_t));
        if (!mBuffer)
            return false;
        mMax = newLen;
    }


    mNum = lent;
    memcpy(mBuffer, ori1, len1 * sizeof(wchar_t));
    memcpy(mBuffer + len1, ori2, len2 * sizeof(wchar_t));

    mBuffer[len1 + len2] = 0;
    return true;
}

bool piString::ConcatW(const wchar_t *ori1, const wchar_t *ori2, const wchar_t *ori3)
{
    piAssert(mInited == INITED_MARK);

    const unsigned int len1 = piwstrlen(ori1);
    const unsigned int len2 = piwstrlen(ori2);
    const unsigned int len3 = piwstrlen(ori3);
    const unsigned int lent = len1 + len2 + len3;

    if (lent > mMax)
    {
        const uint32_t newLen = lent + 8;
        mBuffer = (wchar_t*)malloc((newLen + 1) * sizeof(wchar_t));
        if (!mBuffer)
            return false;
        mMax = newLen;
    }

    mNum = lent;
    memcpy(mBuffer, ori1, len1 * sizeof(wchar_t));
    memcpy(mBuffer + len1, ori2, len2 * sizeof(wchar_t));
    memcpy(mBuffer + len1 + len2, ori3, len3 * sizeof(wchar_t));

    mBuffer[lent] = 0;
    return true;
}

int piString::GetLength( void ) const
{
    piAssert(mInited == INITED_MARK);
    return( mNum );
}

void piString::SetLength(uint32_t len )
{
    piAssert(mInited == INITED_MARK);
    mNum = len;
	mBuffer[len] = 0;
}


bool piString::iReallocate(void)
{
	long newmax = 4*mMax/3;
	if( newmax<4 ) newmax = 4;

	mBuffer = (wchar_t*)realloc( mBuffer, (1+newmax)*sizeof(wchar_t) );
	if( !mBuffer )
		return false;

	memset( mBuffer+mMax, 0, (newmax-mMax) );
	mMax = newmax;

	return true;
}

bool piString::AppendWC( wchar_t obj )
{
    piAssert(mInited == INITED_MARK);
    if( mNum>=(mMax-1) )
    {
        if (!iReallocate())
			return false;
    }

    mBuffer[ mNum++ ] = obj;
    mBuffer[ mNum ] = 0;

    return true;
}

bool piString::AppendAC( char cobj )
{
    piAssert(mInited == INITED_MARK);
    wchar_t obj = cobj;
    if( mNum>=(mMax-1) )
    {
        if (!iReallocate())
			return false;
    }

    mBuffer[ mNum++ ] = obj;
    mBuffer[ mNum ] = 0;

    return true;
}

bool piString::AppendCAt( wchar_t obj, uint32_t pos )
{
    piAssert(mInited == INITED_MARK);
    if( pos>=mNum )
		return AppendWC(obj);

	const uint32_t len = mNum;
    if( len>=(mMax-1) )
	{
        if (!iReallocate())
			return false;
	}

	for(uint32_t i=len; i>pos; i-- )
	{
		mBuffer[i] = mBuffer[i-1];
	}
	mBuffer[pos] = obj;
	mBuffer[len+1] = 0;
	mNum++;

	return true;
}

bool piString::RemoveCAt(uint32_t pos )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = mNum;
	if( len<1 ) return true;
	for(uint32_t i=pos; i<len; i++ )
	{
		mBuffer[i] = mBuffer[i+1];
	}
	mBuffer[len-1] = 0;
	mNum--;

	return true;
}

bool piString::AppendS( const piString *str )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = str->mNum;
    for(uint32_t i=0; i<len; i++ )
        if( !AppendWC(str->mBuffer[i]) )
            return false;
    return true;
}

bool piString::AppendSW( const wchar_t *str )
{
    piAssert(mInited == INITED_MARK);
    const uint32_t l = piwstrlen( str );
    for(uint32_t i=0; i<l; i++ )
        if( !AppendWC(str[i]) )
            return false;
    return true;
}



bool piString::AppendSA(const char *str)
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = (int)strlen(str);

#ifdef WINDOWS
    uint32_t n = MultiByteToWideChar(CP_ACP, 0, str, len, 0, 0);
    if (n<1) return false;

    wchar_t *tmp = (wchar_t*)malloc((n + 1)*sizeof(wchar_t));
    if (!tmp) return false;

    MultiByteToWideChar(CP_ACP, 0, str, len, tmp, n);

    for( uint32_t i = 0; i<n; i++ )
        if (!AppendWC(tmp[i]))
            return false;

    free( tmp );

    return true;

#else
    return false;
#endif
}


bool piString::EqualW( const wchar_t *str ) const
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = piwstrlen( str );
	if( len != mNum ) return 0;
    for(uint32_t i=0; i<len; i++ )
        if( mBuffer[i] != str[i] )
            return false;
    return true;
}

bool piString::Equal( const piString *str ) const
{
    piAssert(mInited == INITED_MARK);
    const uint32_t len = str->mNum;
	if( len != mNum ) return 0;
    for(uint32_t i=0; i<len; i++ )
        if( mBuffer[i] != str->mBuffer[i] )
            return false;
    return true;

}

void piString::Reset( void )
{
    piAssert(mInited == INITED_MARK);
    mNum = 0;
}


const wchar_t piString::GetC(const uint32_t n) const
{
    piAssert(mInited == INITED_MARK);
    if( n>=mNum ) return( 0 );
    return mBuffer[n];
}


const wchar_t *piString::GetS(void) const
{
    piAssert(mInited == INITED_MARK);
    if( mBuffer != nullptr )
	    mBuffer[ mNum ] = 0;
    return mBuffer;
}


bool piString::AppendPrintf( const wchar_t *format, ... )
{
    piAssert(mInited == INITED_MARK);

    wchar_t tmpstr[4096];
    va_list arglist;
    va_start( arglist, format );
    const int res = pivwsprintf( tmpstr, 4095, format, arglist );
	va_end( arglist );
    return AppendSW( tmpstr );
}

bool piString::Printf( const wchar_t *format,  ...  )
{
    piAssert(mInited == INITED_MARK);

    va_list args;
    va_start( args, format );
#if !defined(ANDROID)
    const int res = vswprintf_s( mBuffer, mMax, format, args );
#else
    const int res = vswprintf( mBuffer, mMax, format, args );
#endif
    va_end( args );
	mNum = res;
    return 1;
}

bool piString::ToSInt( int *res ) const
{
    piAssert(mInited == INITED_MARK);
    wchar_t *ec;
    res[0] = wcstoul( mBuffer, &ec, 10 );
    if( ec[0]!=0 )
        return false;
    return true;
}

bool piString::ToUInt( uint32_t *res ) const
{
    piAssert(mInited == INITED_MARK);
    wchar_t *ec;
    res[0] = wcstoul( mBuffer, &ec, 10 );
    if( ec[0]!=0 )
        return false;
    return true;
}


bool piString::ToFloat( float *res ) const
{
    piAssert(mInited == INITED_MARK);
    wchar_t *ec;
    res[0] = (float)wcstod( mBuffer, &ec );
    if( ec[0]!=0 )
        return false;

    return true;
}

bool piString::ContainsW( const wchar_t *str ) const
{
    piAssert(mInited == INITED_MARK);
    return (wcsstr(mBuffer,str)!=NULL );
}

void piString::ToUpper(void)
{
    piAssert(mInited == INITED_MARK);
    const int num = mNum;
	for (int i = 0; i<num; i++)
	{
		const wchar_t c = mBuffer[i];
		if (c >= L'a' && c <= L'z')
			mBuffer[i] = c + L'A' - L'a';
	}
}

bool piString::RemoveOccurrences(const wchar_t what)
{
    piAssert(mInited == INITED_MARK);
    const int num = mNum;
    int wptr = 0;

    for (int i = 0; i<num; i++)
    {
        const wchar_t c = mBuffer[i];
        if (c == what)
        {
        }
        else
        {
            mBuffer[wptr++] = c;
        }
    }
    mBuffer[wptr] = 0;
    mNum = wptr;

    return true;
}

bool piString::ReplaceOccurrences(const wchar_t what, const wchar_t bywhat)
{
    piAssert(mInited == INITED_MARK);
    const int num = mNum;
    int wptr = 0;
    for( int i=0; i<num; i++ )
    {
        const wchar_t c = mBuffer[i];
        if( c==what )
        {
            mBuffer[wptr++] = bywhat;
        }
        else
        {
            mBuffer[wptr++] = c;
        }
    }
    mBuffer[wptr] = 0;
    mNum = wptr;

    return true;
}


}
