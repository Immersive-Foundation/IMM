//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore {

class piTimer
{
public:
    piTimer();
    ~piTimer();

    bool   Init( void );
    void   End(void);

	double   GetTime(void);             // in seconds      (if clock is 3.42M Hz -> 3420000 cycles per Tick)      
	uint64_t GetTimeMs(void);           // in miliseconds  (if clock of 3.42M Hz -> 3420 cycles per Tick)
	uint64_t GetTimeMicroseconds(void); // in microseconds (if clock is 3.42M Hz -> 3 cycles per Tick)
	uint64_t GetTimeTicks(void);        // in Ticks        (if clock is 3.42M Hz -> 271 cycles per Tick)

	// Notes on Time Units. Divisibility by different numbers (framerates) is important for synchronization across media events
	//
	// Miliseconds  = 2^3 x       5^3    , divisible by 1, 2,    4, 5,       8,    10,             16,     20,         25,         32,         40,                         64,         80,     100
	// Microseconds = 2^6 x       5^6    , divisible by 1, 2,    4, 5,       8,    10,             16,     20,         25,         32,         40,                         64,         80,     100
	// Ticks        = 2^3 x 3^2 x 5^2 x 7, divisible by 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 15, 16, 18, 20, 21, 24, 25, 28, 30,     35, 36, 40, 42, 45, 50, 56, 60, 63,     70, 72,     90, 100, 120, ...

    static void   Sleep( int miliseconds );

    typedef void(*DoAlarmnCallback)(void *data, long time);
    typedef void *Alarm;
    Alarm        CreateAlarm(DoAlarmnCallback func, void *data, int delta);
    void         DestroyAlarm(Alarm me);



private:
    void   *mImplementation;
};

// ---- alarms ----



} // namespace ImmCore
