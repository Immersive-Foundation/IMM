#ifndef ANDROID_HAPTICS_H
#define ANDROID_HAPTICS_H


#include <cstdint>
#include <unordered_map>
#include <string>

#include <VrApi.h>
#include <VrApi_Input.h>
#include <VrApi_Types.h>

#include "libCore/src/libBasics/piLog.h"

// For Quest:
// HapticSamplesMax: 25 HapticSampleDurationMS: 2
//
// We should actually try to develop interesting samples for these...

void HapticEffect(ImmCore::piLog * log, ovrMobile* ovr, const ovrDeviceID deviceID, const char* effectName, const int count = 0, const int total = 0);

#endif //ANDROID_HAPTICS_H
