//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#include <math.h>

#include "../../libBasics/piTypes.h"
#include "../../libBasics/piVecTypes.h"

namespace ImmCore
{
	namespace piTransforms
	{
		namespace piNormal
		{
			vec2 octahedral(const vec3 & v)
			{
				vec3 nor = v / (fabsf(v.x) + fabsf(v.y) + fabsf(v.z));
				nor.x = (nor.z >= 0.0f) ? nor.x : (1.0f - fabsf(nor.y))* ((nor.x >= 0.0f) ? 1.0f : -1.0f);
				nor.y = (nor.z >= 0.0f) ? nor.y : (1.0f - fabsf(nor.x))* ((nor.y >= 0.0f) ? 1.0f : -1.0f);
				return 0.5f + 0.5f*nor.xy();
			}

			vec2 polar(const vec3 & v)
			{
				return vec2(0.5f + 0.5f*atan2f(v.x, v.z) / 3.1415927f, acosf(v.y) / 3.1415927f);
			}

			vec3 ipolar(const vec2 & v)
			{
				const float a1 = 3.1415927f*(-1.0f + 2.0f*v.x);
				const float a2 = 3.1415927f*(v.y);
				return vec3(sinf(a2)*sinf(a1), cosf(a2), sinf(a2)*cosf(a1));
			}

			vec2 cube(const vec3 & v, uint8_t *face)
			{
				vec3 mor; int  id;
				mor = v.xyz(); id = 0;
				if (fabsf(v.y) > fabsf(mor.x)) { mor = v.yzx(); id = 1; }
				if (fabsf(v.z) > fabsf(mor.x)) { mor = v.zxy(); id = 2; }
				*face = id * 2 + ((mor.x < 0.0f) ? 1 : 0);
				return 0.5f + 0.5f*mor.yz() / fabsf(mor.x);
			}
		}

	}
}
