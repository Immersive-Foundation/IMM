//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <string.h>
#include "../piTimer.h"
#include "../piTypes.h"

//------------- alarms ------------

namespace ImmCore {

typedef struct
{
    
    void *                   id;
    piTimer::DoAlarmnCallback func;
    void                    *data;
    int                      used;
    long                     to;
}ALARM;

typedef struct
{
    ALARM           *pAlarms;
    int             pMaxAlarms;
    int             pNumAlarms;

    __int64			pFreq;
    __int64	        hto;
    unsigned int		lto;
}iIqTimerMgr;


piTimer::piTimer()
{
    mImplementation = nullptr;
}

piTimer::~piTimer()
{
}


static void CALLBACK timerfunc(HWND hwnd, UINT uMsg, UINT_PTR id, DWORD dwTime)
{
/*
    ALARM   *me = piTimerMgr.pAlarms;

    for( int i=0; i<piTimerMgr.pNumAlarms; i++ )
    {
        if( me->id==id )
        {
            if( me->to==0 )
                me->to = dwTime;
            me->func( me->data, dwTime-me->to );
            return;
        }
    me++;
    }
*/
}


piTimer::Alarm piTimer::CreateAlarm( DoAlarmnCallback func, void *data, int deltamiliseconds )
{
    iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;

    
    // search for an unused slot
    int id = -1;
    for (int i = 0; i<me->pNumAlarms; i++)
    {
        if (me->pAlarms[i].used == 0)
        {
            id = i;
            break;
        }
    }


    ALARM   *al;
    if( id==-1 )
    {
        if( me->pNumAlarms >= me->pMaxAlarms ) return 0;
        al = me->pAlarms + me->pNumAlarms;
        me->pNumAlarms++;

    }
    else
    {
        al = me->pAlarms + id;
    }
    
    if( !al)
        return nullptr;

    al->data = data;
    al->func = func;
    al->to = 0;
    al->id = (void *)SetTimer( 0, 0, deltamiliseconds, timerfunc );
    if( !al->id )
        return nullptr;

    return( (Alarm)me );
}

void piTimer::DestroyAlarm( Alarm vme )
{
    ALARM *me = (ALARM*)vme;

    KillTimer( 0, (UINT_PTR)me->id );
}


//----------------------------------------




bool piTimer::Init( void )
{
    iIqTimerMgr *me = (iIqTimerMgr*)malloc( sizeof(iIqTimerMgr) );
    if( !me )
        return false;


	LARGE_INTEGER li = { 0 };
	LARGE_INTEGER t;

    if( !QueryPerformanceFrequency(&li) )
		return false;
	
    me->pFreq=li.QuadPart;
	QueryPerformanceCounter(&t);
    me->hto = t.QuadPart;
	
	// init alarm manager

    me->pNumAlarms = 0;
    me->pMaxAlarms = 1024;
    me->pAlarms = (ALARM*)malloc(me->pMaxAlarms*sizeof(ALARM) );
    if( !me->pAlarms )
        return false;
    memset(me->pAlarms, 0, me->pMaxAlarms*sizeof(ALARM) );

    mImplementation = me;

    return true;
}


void piTimer::End(void)
{
	iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;

	free(me->pAlarms);
	free(me);
}


double piTimer::GetTime( void )
{
    iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;

    LARGE_INTEGER t;
    
    QueryPerformanceCounter(&t);
    return( (double)(t.QuadPart- me->hto)/(double)me->pFreq );
}

uint64_t piTimer::GetTimeMs( void )
{
    iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return ((uint64_t(1000)*(t.QuadPart- me->hto)/ me->pFreq) );
}

uint64_t piTimer::GetTimeMicroseconds(void)
{
	iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return ( ( uint64_t(1000000) * uint64_t(t.QuadPart - me->hto) ) / uint64_t(me->pFreq) );
}

uint64_t piTimer::GetTimeTicks(void)
{
	iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return ((uint64_t(12600) * uint64_t(t.QuadPart - me->hto)) / uint64_t(me->pFreq));
}


void piTimer::Sleep( int miliseconds )
{
    ::Sleep( miliseconds );
}

}
