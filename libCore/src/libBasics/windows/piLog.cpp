//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <windows.h>
#include <wincon.h>
#include <io.h>
#include <conio.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include "../../libBasics/piTypes.h"
#include "../../libBasics/piSystemInfo.h"
#include "../../libBasics/piThread.h"
#include "../../libBasics/piStr.h"

#include "../piLog.h"

namespace ImmCore {

typedef struct
{
    unsigned int  free_memory_MB;
    unsigned int  total_memory_MB;
    int     number_cpus;
    wchar_t processor[512];
    int     mhz;
    wchar_t date[512];
    wchar_t gpuVendor[64];
    wchar_t gpuModel[512];
    unsigned int  total_videomemory_MB;
    int     mScreenResolution[2];
	int     mIntegratedMultitouch;
    int     mNumMonitors;
}piLogStartInfo;


class piLogger
{
public:
	virtual bool Init( const wchar_t *path, const piLogStartInfo *info ) = 0;
    virtual void End( void ) = 0;
	virtual void Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str ) = 0;
};


//------------------------------------------------

class piTxtLogger: public piLogger
{
public:
	piTxtLogger() {};
	~piTxtLogger() {};

    bool Init( const wchar_t *path, const piLogStartInfo *info )
    {
        //path
        mFp = _wfopen( path, L"wt" );
        if( !mFp )
            return false;

        fwprintf( mFp, L"===================================================\n" );
        fwprintf( mFp, L"date  : %s\n", info->date );
        fwprintf( mFp, L"\n" );
        fwprintf( mFp, L"Memory: %d / %d Megabytes \n", info->free_memory_MB, info->total_memory_MB );
        fwprintf( mFp, L"CPU   : %s\n", info->processor );
        fwprintf( mFp, L"Units : %d\n", info->number_cpus );
        fwprintf( mFp, L"Speed : %d Mhz\n", info->mhz );
        fwprintf( mFp, L"OS    : Windows\n" );
        fwprintf( mFp, L"GPU   : %s, %s\n", info->gpuVendor, info->gpuModel );
        fwprintf( mFp, L"VRam  : %d Megabytes \n", info->total_videomemory_MB );
        fwprintf( mFp, L"Screen: %d x %d\n", info->mScreenResolution[0], info->mScreenResolution[1] );
	    fwprintf( mFp, L"Multitouch Integrated: %d\n", info->mIntegratedMultitouch );
        fwprintf( mFp, L"Monitors: %d\n", info->mNumMonitors);
        fwprintf( mFp, L"===================================================\n");
	    fflush( mFp );

	    return true;
    }

    void End( void )
    {
	    fclose( mFp );
    }

    void Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str )
    {
        if( !mFp ) return;

        switch( type )
            {
            case 1:
                fwprintf( mFp, L"[%d]  %s::%s (%d) :", threadId, file, func, line );
                fwprintf( mFp, str  );
                fwprintf( mFp, L"\n" );
                break;

            case 2:
                fwprintf( mFp, L"[%d]  %s::%s (%d) :", threadId, file, func, line );
                fwprintf( mFp, str  );
                fwprintf( mFp, L"\n" );
                break;

            case 3:
                fwprintf( mFp, str  );
                fwprintf( mFp, L"\n" );
                break;

            case 4:
                fwprintf( mFp, str  );
                fwprintf( mFp, L"\n" );
                break;
            }

        fflush( mFp );
    }

private:
    FILE *mFp;
};

//------------------------------------------------

class piCnsLogger: public piLogger
{
public:
	piCnsLogger() {};
	~piCnsLogger() {};

    bool Init( const wchar_t *path, const piLogStartInfo *info )
    {
        mIsNewConsole = (AllocConsole()!=0);
        SetConsoleTitle( L"console" );
        mCns = GetStdHandle( STD_OUTPUT_HANDLE );

        //HWND h = FindWindow(NULL, L"console"); SetWindowPos(h, HWND_NOTOPMOST, 0, 0, 100, 100, SWP_NOSIZE|SWP_NOOWNERZORDER);

        printL_con( L"===================================================\n" );
        printF_con( L"date      : %s\n", info->date );
        printF_con( L"\n" );
        printF_con( L"Memory    : %d / %d Megabytes \n", info->free_memory_MB, info->total_memory_MB );
        printF_con( L"CPU       : %s\n", info->processor );
        printF_con( L"Units     : %d\n", info->number_cpus );
        printF_con( L"Speed     : %d Mhz\n", info->mhz );
        printF_con( L"OS        : Windows\n" );
        printF_con( L"GPU       : %s, %s\n", info->gpuVendor, info->gpuModel );
        printF_con( L"VRam      : %d Megabytes \n", info->total_videomemory_MB );
        printF_con( L"Screen    : %d x %d\n", info->mScreenResolution[0], info->mScreenResolution[1] );
        printF_con( L"Monitors  : %d\n", info->mNumMonitors);
        printF_con( L"Multitouch: %d\n", info->mIntegratedMultitouch);
        printL_con( L"===================================================\n" );

	    return true;
    }

    void End( void )
    {
        if( mIsNewConsole ) FreeConsole();
    }

    void Printf( int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str )
    {
	    int strLen = (int)wcslen(str);
	    int maxLen = strLen; if (maxLen < 256) maxLen = 256;
	    int bufLen = maxLen + 1;
	    wchar_t *aux = (wchar_t*)_malloca(bufLen*sizeof(wchar_t));
	    if( !aux ) return;

        unsigned long res;

        switch( type )
        {
            case 1:
		    {
                SetConsoleTextAttribute(mCns, FOREGROUND_RED | FOREGROUND_INTENSITY);
			    int len = swprintf(aux, bufLen, L"[%d]  %s::%s (%d) :", threadId, file, func, line);
			    WriteConsole(mCns, aux, len, &res, 0);
			    SetConsoleTextAttribute(mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			    WriteConsole(mCns, str, strLen, &res, 0);
			    WriteConsole(mCns, L"\n", 1, &res, 0);
			    break;
		    }

		    case 2:
		    {
                SetConsoleTextAttribute(mCns, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			    int len = swprintf(aux, bufLen, L"[%d]  %s::%s (%d) :", threadId, file, func, line);
			    WriteConsole(mCns, aux, len, &res, 0);
			    SetConsoleTextAttribute(mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			    WriteConsole(mCns, str, strLen, &res, 0);
			    WriteConsole(mCns, L"\n", 1, &res, 0);
			    break;
		    }

		    case 3:
		    {
                SetConsoleTextAttribute(mCns, FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN);
			    WriteConsole(mCns, str, strLen, &res, 0);
			    WriteConsole(mCns, L"\n", 1, &res, 0);
			    break;
		    }

		    case 4:
		    {
                SetConsoleTextAttribute(mCns, FOREGROUND_BLUE | FOREGROUND_GREEN);
			    WriteConsole(mCns, str, strLen, &res, 0);
			    WriteConsole(mCns, L"\n", 1, &res, 0);
			    break;
		    }
        }

	    _freea( aux );
    }
private:
    void printF_con( const wchar_t *format, ... )
    {
        wchar_t    tmpstr[1024];
        unsigned long res;

        va_list arglist;
        va_start( arglist, format );
        int len = vswprintf_s( tmpstr, 1023, format, arglist );
	    va_end( arglist );

        WriteConsole( mCns, tmpstr, len, &res, 0  );
    }

    void printL_con( const wchar_t *str )
    {
        unsigned long res;
        WriteConsole( mCns, str, (int)wcslen(str), &res, 0  );
    }


private:
    bool    mIsNewConsole;
    HANDLE  mCns;

};

//------------------------------------------------


static void piLogStartInfo_Get( piLogStartInfo *info )
{
    memset( info, 0, sizeof(piLogStartInfo) );

	uint64_t fm, tm, vm;
    piSystemInfo_getFreeRAM( &fm, &tm );
    vm = piSystemInfo_getVideoMemory();

    info->number_cpus = piSystemInfo_getCPUs();
    info->free_memory_MB = (unsigned int)(fm >> 20L);
	info->total_memory_MB = (unsigned int)(tm >> 20L);
    piSystemInfo_getTime( info->date, 511 );
    piSystemInfo_getProcessor( info->processor, 511, &info->mhz );
    piSystemInfo_getGfxCardIdentification( info->gpuVendor, 64, info->gpuModel, 512 );
    info->total_videomemory_MB = (unsigned int)(vm >> 20L);
    piSystemInfo_getScreenResolution( info->mScreenResolution );
	info->mIntegratedMultitouch = piSystemInfo_getIntegratedMultitouch();
    info->mNumMonitors = piSystemInfo_getNumMonitors();
}

//-------------------------------------------------------------
struct iLog
{
	int       mNumLoggers;
	piLogger *mLoggers[8];
	int       mMessageCounter;
};

piLog::piLog()
{
}

piLog::~piLog()
{
}

bool piLog::Init( const wchar_t *path, int loggers)
{
    iLog *me = (iLog*)malloc( sizeof(iLog));
    if( !me ) return false;
    mImp = me;

	piLogStartInfo info;
    piLogStartInfo_Get( &info );

	me->mNumLoggers = 0;
	me->mMessageCounter = 0;


	for( int i=0; i<2; i++ )
	{
		if( (loggers & (1<<i))==0 ) continue;

		piLogger *lo = 0;
		if( i==0 ) lo = new piTxtLogger();
		if( i==1 ) lo = new piCnsLogger();
		if( !lo )
			continue;

		if( !lo->Init(path,&info) )
			return false;

		me->mLoggers[me->mNumLoggers++] = lo;
	}

    return true;
}

void piLog::End( void )
{
    iLog *me = (iLog*)mImp;
	for( int i=0; i<me->mNumLoggers; i++ )
	{
		piLogger *lo = me->mLoggers[i];
		lo->End();
		delete lo;
	}
    free( me );
}

void piLog::Printf( const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *format, ... )
{
	#ifndef _DEBUG
	if (type == 5) return;
	#endif

    iLog *me = (iLog*)mImp;
    if( me->mNumLoggers<1 ) return;

    va_list arglist;


    va_start( arglist, format );

	int maxLen = pivscwprintf( format, arglist ) + 1;
	wchar_t *tmpstr = (wchar_t*)_malloca( maxLen*sizeof(wchar_t) );
	if (!tmpstr) return;


	pivwsprintf(tmpstr, maxLen, format, arglist);

	va_end( arglist );

    for( int i=0; i<me->mNumLoggers; i++ )
    {
		piLogger *lo = me->mLoggers[i];
        lo->Printf( me->mMessageCounter, (int)(uint64_t)piThread_GetOSID(), file, func, line, type, tmpstr );
    }

    me->mMessageCounter++;

	_freea( tmpstr );
}


}
