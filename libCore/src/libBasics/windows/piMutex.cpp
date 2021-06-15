//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <malloc.h>
#include <process.h>
#include "../piMutex.h" 

namespace ImmCore {

bool piMutex::Init( void )
{
	HANDLE h = CreateMutex( NULL, false, NULL );
	if (h == NULL)
		return false;
	p = (void*)h;
	return true;
}

void piMutex::End( void )
{
    CloseHandle( (HANDLE)p );
}

void piMutex::Lock(void)
{
	WaitForSingleObject( (HANDLE)p, INFINITE );
}

void piMutex::UnLock(void)
{
	ReleaseMutex( (HANDLE)p );
}

}
