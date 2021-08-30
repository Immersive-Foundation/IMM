//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//

#include <windows.h>
#include <winuser.h>

#include <stdio.h>

#include "../piStr.h"
#include "../piTypes.h"

namespace ImmCore {


int piSystemInfo_getIntegratedMultitouch( )
{
#if _MSC_VER < 1700 
	return 0;
#else
	// test for touch
	int value = GetSystemMetrics(SM_DIGITIZER);
	if (value & NID_READY) /* stack ready */
	{ 
		if (value  & NID_MULTI_INPUT) /* digitizer is multitouch */ 
		{
			if (value & NID_INTEGRATED_TOUCH) /* Integrated touch */
			{
				return 1;
			}
		}
	}
	return 0;
#endif
}

void piSystemInfo_getScreenResolution( int *res )
{
//SM_CXSCREEN
//SM_CXVIRTUALSCREEN
//SM_CXFULLSCREEN
    
    res[0] = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    res[1] = GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

void piSystemInfo_getFreeRAM( uint64_t *avail, uint64_t *total )
{
#if 1
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof( ms );
	GlobalMemoryStatusEx( &ms );

    *avail = ms.ullAvailPhys;
    *total = ms.ullTotalPhys;
#else
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(MEMORYSTATUSEX);

    GlobalMemoryStatusEx( &statex );

/*    
    printf ("%ld percent of memory is in use.\n",
          statex.dwMemoryLoad);
*/

    // physical memory
    *avail = statex.ullAvailPhys;
    *total = statex.ullTotalPhys;

    // paging memory
    //statex.ullTotalPageFile;
    //statex.ullAvailPageFile;
    // virtual memory
    //statex.ullTotalVirtual
    //statex.ullAvailVirtual
#endif

}

int piSystemInfo_getCPUs( void )
{
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return( si.dwNumberOfProcessors );
}

void piSystemInfo_getProcessor( wchar_t *str, int length, int *mhz )
{
	HKEY pHRoot;
	LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System", 0, KEY_READ, &pHRoot);
	if (res != ERROR_SUCCESS)
        {
        piwstrncpy( str, length, L"Unknown", 0 );
        RegCloseKey(pHRoot);
        return;
        }

	HKEY key;
	if (RegOpenKey(pHRoot, L"CentralProcessor\\0", &key) != ERROR_SUCCESS)
	    {
        piwstrncpy( str, length, L"Unknown", 0 );
        RegCloseKey(pHRoot);
        return;
        }

	unsigned char data[1024];
	unsigned long datasize = 1024;
	DWORD type;

	if( RegQueryValueEx(key, L"ProcessorNameString", 0, &type, data, &datasize) != ERROR_SUCCESS )
    {
		piwstrncpy( str, length, L"Unknown", 0 );
    }
	else if (type == REG_SZ)
	{
		data[datasize] = 0;

        //int i=0; while( data[i]==' ' ) i++;

		//piwsprintf( str, 64, L"%d", datasize-i );
		piwstrncpy( str, length, (wchar_t*)(data+0), 0 );
		str[datasize/2 - 1] = 0;

		//if( !MultiByteToWideChar(CP_ACP,0,(char*)data,datasize,str,length) ) piwstrncpy( str, length, L"Unknown", 0 );

    }


    if( RegQueryValueEx(key, L"~MHz", 0, &type, data, &datasize) != ERROR_SUCCESS )
    {
		*mhz = 13;
    }
	else if (type == REG_DWORD)
    {
        *mhz = *((int*)data);
    }

	RegCloseKey(key);
    RegCloseKey(pHRoot);

}



static wchar_t *dia[7] = { L"Mon", L"Tue", L"Wen", L"Thu", L"Fri", L"Sat", L"Sun" };

void piSystemInfo_getTime( wchar_t *str, int length )
{
    SYSTEMTIME t;

    GetSystemTime( &t );

    piwsprintf( str, length, L"%d:%d:%d of %s %d/%d/%d", t.wHour, t.wMinute, t.wSecond, dia[t.wDayOfWeek], t.wDay, t.wMonth, t.wYear );
}

static int char2num( wchar_t c )
{
    if( c>='0' && c<='9' )
        return c - '0';

    if( c>='a' && c<='f' )
        return 10 + c - 'a';

    return -1;
}


void piSystemInfo_getGfxCardIdentification( wchar_t *vendorID, int vlen, wchar_t *deviceID, int dlen)
{
    #if _MSC_VER>=1300

    DISPLAY_DEVICE dd;
    dd.cb = sizeof(DISPLAY_DEVICE);

    int i = 0;

    while( EnumDisplayDevices(NULL, i, &dd, 0) )
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            const wchar_t *tmp = dd.DeviceID;
            //-----------
            int vid = (char2num(tmp[ 8])<<12) | (char2num(tmp[ 9])<< 8) | 
                      (char2num(tmp[10])<< 4) | (char2num(tmp[11])<< 0);
            if( vid==0x10DE )
                piwstrcpy( vendorID, vlen, L"NVIDIA" );
            else if( vid==0x1002 )
                piwstrcpy( vendorID, vlen, L"ATI" );
            else if( vid==0x102B )
                piwstrcpy( vendorID, vlen, L"MATROX" );
            else if( vid==0x163C || vid==0x8086 )
                piwstrcpy( vendorID, vlen, L"INTEL" );
            else if( vid==0x104a )
                piwstrcpy( vendorID, vlen, L"POWERVR" );
            else
                piwstrcpy( vendorID, vlen, L"Unkown" );
            //------------
            piwstrcpy( deviceID, dlen, (const wchar_t*)dd.DeviceString );
            return;
        }
        i++;
    }
    #endif

    piwstrcpy( vendorID, vlen, L"Unkown" );
    piwstrcpy( deviceID, dlen, L"Unkown" );

}


uint64_t piSystemInfo_getVideoMemory( void )
{
    size_t vidMemSizeTotal = 0; 
/*
	VRRegistry registry1(L"HARDWARE\\DEVICEMAP");
	VRUniString res1 = registry1.readString(L"VIDEO", L"\\Device\\Video0", L"");


	if (res1.getLength() > 0)
	{
		/// path into registry is valid, remove header of path
		VRUniString header(L"\\Registry\\Machine\\");
		int index = res1.findSubString(header);
		if (index == 0)
		{
			/// the following line will generate a negative number, effectively
			/// taking the last N characters of res1:
			res1 = res1.sub((int)header.getLength() - (int)res1.getLength());	
			if (res1.getLength() > 0)
			{		
				VRRegistry registry2(res1);
				//VRUniString res2 = registry2.readString("", "Device Description", "");
				//VRLog::wprintf(VRWLOG_DEBUG, L"    Device descr.: '%s'", res2);
					
				vidMemSizeTotal = registry2.readUInt(L"", L"HardwareInformation.MemorySize", -1);
				//VRLog::wprintf(VRWLOG_DEBUG, L"    Video memory: '%d'", pVidMemSizeTotal);		
				
				if (vidMemSizeTotal == -1)
					vidMemSizeTotal = 0;			
			}
		}
	}
*/
    return vidMemSizeTotal;
}



static BOOL CALLBACK mMonitorEnum(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    int *Count = (int*)dwData;
    (*Count)++;
    return TRUE;
}

int piSystemInfo_getNumMonitors( void )
{
    int count = 0;
    if (!EnumDisplayMonitors(NULL, NULL, mMonitorEnum, (LPARAM)&count))
        count = 1;
    return count;
}

}
