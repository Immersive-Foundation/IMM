//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <malloc.h>

#include "../piThread.h"

namespace ImmCore {

typedef struct
{
    HANDLE		th;
    void	    *data;
    piThreadDoworFunc    doworkcb;
}iTHREAD;


static DWORD __stdcall th_iteration( void *vme )
{
    iTHREAD *me = (iTHREAD*)vme;

	return me->doworkcb( me->data );
}


void *piThread_Init( piThreadDoworFunc func, void *data )
{
	unsigned long	thid;

    iTHREAD *me = (iTHREAD *)malloc( sizeof(iTHREAD) );
    if( !me )
        return( 0 );

    me->data = data;
    me->doworkcb = func;
	me->th = CreateThread( NULL, 0, th_iteration, me, 0, &thid );
    if( !me->th )
    	return( 0 );

	//SetThreadPriority( sth->th, THREAD_PRIORITY_HIGHEST );

    return( (piThread)me );
}


void piThread_End( piThread vme )
{
    iTHREAD *me = (iTHREAD*)vme;

 	CloseHandle( me->th );

    free( vme );
}


void *piThread_GetOSID( void )
{
    return (void*)GetCurrentThread();
}

}
