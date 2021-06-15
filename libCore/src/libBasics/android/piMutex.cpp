#include <pthread.h>
#include <malloc.h>

#include "../piMutex.h"

namespace ImmCore {

bool piMutex::Init( void )
{
	if (p == nullptr)
	{
		p = malloc( sizeof(pthread_mutex_t) );
	}
	return pthread_mutex_init( (pthread_mutex_t *)p, NULL ) == 0;
}

void piMutex::End( void )
{
	pthread_mutex_destroy( (pthread_mutex_t *)p );
	free(p);
}

void piMutex::Lock(void)
{
	pthread_mutex_lock( (pthread_mutex_t *)p );
}

void piMutex::UnLock(void)
{
	pthread_mutex_unlock( (pthread_mutex_t *)p );
}

}
