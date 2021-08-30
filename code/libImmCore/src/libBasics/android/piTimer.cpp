#include "../piTimer.h"

#include <time.h>
#include <unistd.h>

#define ntimersub(a, b, res)                         \
    do {                                              \
        (res)->tv_sec  = (a)->tv_sec  - (b)->tv_sec;  \
        (res)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
        if ((res)->tv_nsec < 0)                       \
        {                                             \
            (res)->tv_nsec += 1000000000;            \
            (res)->tv_sec  -= 1;                      \
        }                                             \
        if ((res)->tv_sec < 0)                        \
        {                                              \
            (res)->tv_sec = 0;                         \
            (res)->tv_nsec = 0;                         \
        }                                               \
} while (0)

namespace ImmCore {

    static struct timespec to;

piTimer::piTimer()
{
    mImplementation = nullptr;
}

piTimer::~piTimer()
{
}

piTimer::Alarm piTimer::CreateAlarm( DoAlarmnCallback func, void *data, int deltamiliseconds )
{
    return nullptr;
}

void piTimer::DestroyAlarm( Alarm vme )
{
}

//----------------------------------------

bool piTimer::Init( void )
{
    if( clock_gettime(CLOCK_MONOTONIC, &to) == -1 )
        return false;

    return true;
}


void piTimer::End(void)
{
    //iIqTimerMgr *me = (iIqTimerMgr*)mImplementation;
    //free(me);
}


double piTimer::GetTime( void )
{
    double long t;
    double ret;
    struct timespec now, res;

    clock_gettime(CLOCK_MONOTONIC, &now);

    ntimersub( &now, &to, &res );

    t = (res.tv_sec*1000) + (res.tv_nsec/1000000);
    ret = .001*(double)t;

    return( ret );
}

uint64_t piTimer::GetTimeMs( void )
{
    struct timespec now, res;
    clock_gettime(CLOCK_MONOTONIC, &now);
    ntimersub( &now, &to, &res );
    uint64_t us1 = (uint64_t) 1000 * res.tv_sec;
    uint64_t us2 = (uint64_t) res.tv_nsec / 1000000;
    return us1 + us2;
}

uint64_t piTimer::GetTimeMicroseconds(void)
{
    struct timespec now, res;
    clock_gettime(CLOCK_MONOTONIC, &now);
    ntimersub( &now, &to, &res );
    uint64_t us1 = (uint64_t) 1000000 * res.tv_sec;
    uint64_t us2 = (uint64_t) res.tv_nsec / 1000;
    return us1 + us2;
}

uint64_t piTimer::GetTimeTicks(void)
{
    struct timespec now, res;
    clock_gettime(CLOCK_MONOTONIC, &now);
    ntimersub( &now, &to, &res );
    uint64_t ns1 = (uint64_t) 1000000000 * res.tv_sec;
    uint64_t ns2 = (uint64_t) res.tv_nsec;
    return ((ns1 + ns2) * (uint64_t)12600) / (uint64_t )1000000000;
}

void piTimer::Sleep( int milliseconds )
{
    struct timeval tv;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    tv.tv_sec = milliseconds / 1000;
    select(0, 0, 0, 0, &tv);
    // usleep(milliseconds * 1000);
}

}
