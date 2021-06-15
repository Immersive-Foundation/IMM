//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "../libBasics/piTypes.h"
#include "../libBasics/piDebug.h"
#include "piTick.h"

namespace ImmCore
{
	static constexpr int64_t kTicksPerSecond = 12600; // 12600 = 2^3 x 3^2 x 5^2 x 7, divisible by 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 15, 18, 20, 21, 24, 25, 28, 30, 35, 36, 40, 42, 45, 50, 56, 60, 63, 70, 72, 90 and others

	static int iNumFramerates = -1;
	static uint32_t iFrameRatesLUT[512];


    // compute all allowed framerates. Only once. Not multithreaded in principle
    static void iInitialize(void)
    {
        int num = 0;
        for (int i = 1; i < 512; i++)
        {
            if ((kTicksPerSecond%i) == 0)
            {
                iFrameRatesLUT[num++] = i;
            }
        }
        iNumFramerates = num;
    }


	int piTick::GetNumberOfAllowedFramerates(uint32_t max_framerate)
	{
		if (iNumFramerates == -1) iInitialize();

		int num = 0;
		for (int i = 0; i < iNumFramerates; i++)
		{
			if (iFrameRatesLUT[i] > max_framerate)
				break;
			num++;
		}
		return num;
	}

	int piTick::FramerateToIndex(uint32_t framerate)
	{
        if (iNumFramerates == -1) iInitialize();

		for (int i = 0; i < iNumFramerates; i++)
			if (iFrameRatesLUT[i] == framerate)
				return i;
		piAssert(false);
		return -1;
	}

	uint32_t piTick::IndexToFramerate(int id)
	{
        if (iNumFramerates == -1) iInitialize();
        piAssert(id >= 0 && id < iNumFramerates);
		return iFrameRatesLUT[id];
	}

	bool piTick::IsFramerateAllowed(uint32_t framerate)
	{
        if (iNumFramerates == -1) iInitialize();
        piAssert(framerate >= iFrameRatesLUT[0] && framerate <= iFrameRatesLUT[iNumFramerates-1]);
		return ((kTicksPerSecond % framerate) == 0);
	}

	//===================

    // convert Frames - Ticks ----> WARNING: Do NOT use "frames" to do actual animation timing. Use ONLY for rendering and UI display
    piTick   piTick::FromFrames(int64_t numFrames, int framerate) { piAssert(IsFramerateAllowed(framerate)); return piTick{ numFrames*int64_t(kTicksPerSecond /framerate) }; }
	piTick   piTick::FromOneFrame(int framerate) { piAssert(IsFramerateAllowed(framerate)); return piTick{ int64_t(kTicksPerSecond /framerate) }; }
	int64_t  piTick::ToFramesFloor(piTick ticks, int framerate) { piAssert(IsFramerateAllowed(framerate)); return (static_cast<uint64_t>(framerate) * ticks) / int64_t(kTicksPerSecond); }
    int64_t  piTick::ToFramesCeil(piTick ticks, int framerate)
	{
		piAssert(IsFramerateAllowed(framerate));
		const int64_t num = (static_cast<uint64_t>(framerate) * ticks);
		const int64_t remain = num % kTicksPerSecond;
		return  num / kTicksPerSecond + (remain > 0);
	}
    int64_t piTick::ToFramesRound(piTick ticks, int framerate)
    {
        piAssert(IsFramerateAllowed(framerate));
        const int64_t num = (static_cast<uint64_t>(framerate) * ticks);
        const int64_t remain = num % kTicksPerSecond;
        const int64_t half = kTicksPerSecond / 2;
        return  num / kTicksPerSecond + (remain > half);
    }
	// convert from seconds to Ticks ----> WARNING: Do NOT use to do actual animation timing. Use ONLY for UI display or UI animation
	double piTick::ToSeconds(piTick ticks) { return double(ticks.mValue) / double(kTicksPerSecond); }
	piTick piTick::FromSeconds(double seconds) { return piTick{ int64_t(seconds*double(kTicksPerSecond)) }; }
    int64_t piTick::FromOneSecond(void) { return kTicksPerSecond; }
    // convert Microseconds - Ticks ----> WARNING: Do NOT use "microseconds" to do actual animation timing. Use ONLY for UI display or UI animation
    uint64_t piTick::ToMicroSeconds(piTick ticks) { return uint64_t(1000000.0*(double(ticks.mValue)/(double(kTicksPerSecond)))); }
    piTick   piTick::FromMicroSeconds(uint64_t microseconds) { return piTick{ int64_t(double(microseconds)*double(kTicksPerSecond)/1000000.0) }; }
    // cast ----> WARNING: Do NOT use this to do actual animation timing. Use ONLY for UI display or UI animation
    double   piTick::CastDouble(piTick ticks) { return double(ticks.mValue); }
    int64_t  piTick::CastInt(piTick ticks) { return ticks.mValue; }

}
