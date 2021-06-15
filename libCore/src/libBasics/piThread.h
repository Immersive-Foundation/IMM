//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

namespace ImmCore {

typedef void *piThread;

typedef int (*piThreadDoworFunc)( void *data );

piThread piThread_Init( piThreadDoworFunc func, void *data );
void     piThread_End( piThread me );
void    *piThread_GetOSID( void );

} // namespace ImmCore
