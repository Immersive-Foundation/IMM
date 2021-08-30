//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

// enabled this for maximum safety ---> it usually catches a good deal of logic errors!
//#define PEDANTIC

#include "piTypes.h"

namespace ImmCore
{
    class piTick
    {
    public:
        piTick() {}
        explicit piTick(int v) : mValue(v) {}
        explicit piTick(int64_t v) : mValue(v) {}
        piTick(float) = delete;
        piTick(double) = delete;

        // "Tick" is a measurement of time that is designed to work on exact arithmetic (integers, never real numbers) with a large set of framerates
        // These functions bellow help you access and work with those framerates
        static bool		 IsFramerateAllowed(uint32_t framerate);
        static int		 GetNumberOfAllowedFramerates(uint32_t max_framerate);
        static uint32_t  IndexToFramerate(int id);
        static int       FramerateToIndex(uint32_t framerate);

        // convert Frames - Ticks ----> WARNING: Do NOT use "frames" to do actual animation timing. Use ONLY for rendering and UI display
        static piTick   FromFrames(int64_t numFrames, int framerate);
        static piTick   FromOneFrame(int framerate);
        static int64_t  ToFramesFloor(piTick ticks, int framerate);
        static int64_t  ToFramesCeil(piTick ticks, int framerate);
        static int64_t  ToFramesRound(piTick ticks, int framerate);
        // convert Microseconds - Ticks ----> WARNING: Do NOT use "microseconds" to do actual animation timing. Use ONLY for UI display or UI animation
        static uint64_t ToMicroSeconds(piTick ticks);
        static piTick   FromMicroSeconds(uint64_t microseconds);
        // convert Seconds - Ticks ----> WARNING: Do NOT use "seconds" to do actual animation timing. Use ONLY for UI display or UI animation
        static double   ToSeconds(piTick ticks);
        static piTick   FromSeconds(double seconds);
        static int64_t  FromOneSecond(void);
        // cast ----> WARNING: Do NOT use this to do actual animation timing. Use ONLY for UI display or UI animation
        static double  CastDouble(piTick ticks);
        static int64_t CastInt(piTick ticks);

    public:
        piTick operator- () { return piTick(-mValue); }
        piTick & operator =(piTick  const & v) { mValue = v; return *this; }
        piTick & operator+=(piTick  const & v) { mValue += v; return *this; }
        piTick & operator-=(piTick  const & v) { mValue -= v; return *this; }
        piTick & operator%=(piTick  const & v) { mValue %= v; return *this; }
        friend inline bool   operator<(piTick const & a, piTick const & b) { return a.mValue < b.mValue; }
        friend inline bool   operator>(piTick const & a, piTick const & b) { return a.mValue > b.mValue; }
        friend inline bool   operator<=(piTick const & a, piTick const & b) { return a.mValue <= b.mValue; }
        friend inline bool   operator>=(piTick const & a, piTick const & b) { return a.mValue >= b.mValue; }
        friend inline bool   operator!=(piTick const & a, piTick const & b) { return a.mValue != b.mValue; }
        friend inline bool   operator==(piTick const & a, piTick const & b) { return a.mValue == b.mValue; }
        friend inline piTick operator+(piTick const & a, piTick const & b) { return piTick(a.mValue + b.mValue); }
        friend inline piTick operator-(piTick const & a, piTick const & b) { return piTick(a.mValue - b.mValue); }
        friend inline piTick operator%(piTick const & a, piTick const & b) { return piTick(a.mValue%b.mValue); }
        // mixed types
        #ifndef PEDANTIC
        piTick & operator+=(uint64_t const & v) { mValue += v; return *this; }
        piTick & operator-=(uint64_t const & v) { mValue -= v; return *this; }
        piTick & operator =( int64_t const & v) { mValue  = v; return *this; }
        piTick & operator+=( int64_t const & v) { mValue += v; return *this; }
        piTick & operator-=( int64_t const & v) { mValue -= v; return *this; }
        piTick & operator =(uint32_t const & v) { mValue  = v; return *this; }
        piTick & operator+=(uint32_t const & v) { mValue += v; return *this; }
        piTick & operator-=(uint32_t const & v) { mValue -= v; return *this; }
        piTick & operator =(int32_t  const & v) { mValue  = v; return *this; }
        piTick & operator+=(int32_t  const & v) { mValue += v; return *this; }
        piTick & operator-=(int32_t  const & v) { mValue -= v; return *this; }
        friend inline bool   operator<( piTick  const & a, int64_t const   b)  { return a.mValue < static_cast<int64_t>(b); }
        friend inline bool   operator<( piTick  const & a, int32_t const   b)  { return a.mValue < static_cast<int64_t>(b); }
        friend inline bool   operator>( piTick  const & a, int64_t const   b)  { return a.mValue > static_cast<int64_t>(b); }
        friend inline bool   operator>( piTick  const & a, int32_t const   b)  { return a.mValue > static_cast<int64_t>(b); }
        friend inline piTick operator+( int32_t const   a, piTick  const & b)  { return piTick(a + b.mValue); }
        friend inline piTick operator+( piTick  const & a, int64_t const   b)  { return piTick(a.mValue + b); }
        friend inline piTick operator+( piTick  const & a, int32_t const   b)  { return piTick(a.mValue + static_cast<int64_t>(b)); }
        friend inline piTick operator-( piTick  const & a, int64_t const   b)  { return piTick(a.mValue - b); }
        friend inline piTick operator-( piTick  const & a, int32_t const   b)  { return piTick(a.mValue - static_cast<int64_t>(b)); }
        friend inline piTick operator-( int32_t const   a, piTick  const & b)  { return piTick(a-b.mValue); }
        friend inline piTick operator*( piTick  const & a, int64_t const   b)  { return piTick(a.mValue*b); }
        friend inline piTick operator*( piTick  const & a, int32_t const   b)  { return piTick(a.mValue*static_cast<int64_t>(b)); }
        friend inline int64_t operator/(piTick  const & a, piTick  const & b)  { return int64_t(a.mValue/b.mValue); }
        friend inline piTick operator/( piTick  const & a, int64_t const   b)  { return piTick(a.mValue/b); }
        friend inline piTick operator/( piTick  const & a, int32_t const   b)  { return piTick(a.mValue/static_cast<int64_t>(b)); }
        friend inline bool   operator==(piTick  const & a, int64_t const   b)  { return a.mValue == b; }
        friend inline bool   operator==(piTick  const & a, int32_t const   b)  { return a.mValue == static_cast<int64_t>(b); }
        friend inline bool   operator==(int64_t const   a, piTick  const & b)  { return a == b.mValue; }
        friend inline bool   operator==(int32_t const   a, piTick  const & b)  { return static_cast<int64_t>(a) == b.mValue; }
        friend inline bool   operator!=(piTick  const & a, int64_t const   b)  { return a.mValue != b; }
        friend inline bool   operator!=(piTick  const & a, int32_t const   b)  { return a.mValue != static_cast<int64_t>(b); }
        #endif  

    private:
        int64_t mValue;
    private:
        operator int64_t() const { return mValue; }

    };



}
