//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "../../libBasics/piTypes.h"
#include "../../libBasics/piVecTypes.h"

namespace ImmCore
{

	namespace piTransforms
	{
		namespace piNormal
		{
			vec2 octahedral(const vec3 & v);
			vec2 polar(const vec3 & v);
			vec2 cube(const vec3 & v, uint8_t *face);

			vec3 ioctahedral(const vec2 & v);
			vec3 ipolar(const vec2 & v);
			vec3 icube(const vec2 & v, uint8_t face);
		}
	}

} // namespace ImmCore
