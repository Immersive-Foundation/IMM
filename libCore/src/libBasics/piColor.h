#pragma once

#include "../libBasics/piVecTypes.h"

namespace ImmCore
{

static vec3 linear2srgb( const vec3 & col )
{
    vec3 res;
    res.x = (col.x < 0.0031308f) ? col.x * 12.92f : 1.055f*powf(col.x, 1.0f / 2.4f) - 0.055f;
	res.y = (col.y < 0.0031308f) ? col.y * 12.92f : 1.055f*powf(col.y, 1.0f / 2.4f) - 0.055f;
	res.z = (col.z < 0.0031308f) ? col.z * 12.92f : 1.055f*powf(col.z, 1.0f / 2.4f) - 0.055f;
    return res;
}

}
