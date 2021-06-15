//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include "piTypes.h"

namespace ImmCore {

typedef union
{
    unsigned short v;
    struct
    {
        unsigned short m : 10;
        unsigned short e : 5;
        unsigned short s : 1;
    } bits;
}float16_s;

typedef union
{
    float v;
    struct
    {
        unsigned int m : 23;
        unsigned int e : 8;
        unsigned int s : 1;
    } bits;
}float32_s;


half float2half(float x)
{
    float32_s f32 = { x };
    int e = (int)(f32.bits.e - 127);
    if (e>16) e = 16; else if (e<-15) e = -15;
    float16_s f16;
    f16.bits.s = f32.bits.s;
    f16.bits.e = 15 + e;
    f16.bits.m = f32.bits.m >> 13;
    return f16.v;
}


float half2float(half x)
{
    float16_s f16 = { x };
    float32_s f32;
    f32.bits.s = f16.bits.s;
    f32.bits.e = (f16.bits.e - 15) + 127; // safe in this direction
    f32.bits.m = ((unsigned int)f16.bits.m) << 13;
    return f32.v;
}

}
