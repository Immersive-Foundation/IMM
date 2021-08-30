//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include <stdint.h>

namespace ImmCore {

#define piOpaqueType(N) struct N##S; typedef N##S * N;

#ifdef WINDOWS
 #define PIAPICALL __stdcall
#endif


#ifdef MACOS
#endif

#ifdef LINUX
 #define PIAPICALL
#endif

typedef uint16_t          half;


half  float2half(float x);
float half2float(half x);

} // namespace ImmCore
