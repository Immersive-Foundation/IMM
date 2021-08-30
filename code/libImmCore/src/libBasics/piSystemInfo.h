//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

#include "piTypes.h"

namespace ImmCore {


void     piSystemInfo_getFreeRAM( uint64_t *avail, uint64_t *total );
int      piSystemInfo_getCPUs( void );
void     piSystemInfo_getProcessor( wchar_t *str, int length, int *mhz );
void     piSystemInfo_getTime( wchar_t *str, int length );
void     piSystemInfo_getGfxCardIdentification( wchar_t *vendorID, int vlen, wchar_t *deviceID, int dlen);
uint64_t piSystemInfo_getVideoMemory( void );
void     piSystemInfo_getScreenResolution( int *res );
int      piSystemInfo_getIntegratedMultitouch( );
int      piSystemInfo_getNumMonitors( void );

} // namespace ImmCore
