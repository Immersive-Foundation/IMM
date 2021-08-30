//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

namespace ImmCore
{
	#if defined(_DEBUG)
	void piAssert(bool e);
	#else
	#define piAssert(e) ((void)0)
	#endif
}
