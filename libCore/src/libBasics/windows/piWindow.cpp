//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <malloc.h>
#include <dbt.h>

//==============================================================================================

#include "../piWindow.h"

namespace ImmCore {

typedef struct
{
    //---------------
    HDC         hDC;
    HWND        hWnd;
    //---------------
    int         full;
    //---------------
    piWindowEvents    eventinfo;

    int         mXres;
    int         mYres;

    int         exitReq;

	struct _iIqWindowMgr *mMgr;
}iIqWindow;


typedef struct _iIqWindowMgr
{
    HINSTANCE   hInstance;
    WNDCLASS    mWindowClass;
    wchar_t      lpszClassName[64];
	int			   mNumWindows;
	iIqWindow     *mWindows[256];
    piWindowEvents mEventinfo;
}iIqWindowMgr;

//==============================================================================================

static int win2pi( int k, int sk )
{
	if( sk==0 )
	{
		if( k==VK_F1 ) return KEY_F1;
		if( k==VK_F2 ) return KEY_F2;
		if( k==VK_F3 ) return KEY_F3;
		if( k==VK_F4 ) return KEY_F4;
		if( k==VK_F5 ) return KEY_F5;
		if( k==VK_F6 ) return KEY_F6;
		if( k==VK_F7 ) return KEY_F7;
		if( k==VK_F8 ) return KEY_F8;
		if( k==VK_F9 ) return KEY_F9;
		if( k==VK_F10 ) return KEY_F10;
		if( k==VK_F11 ) return KEY_F11;
		if( k==VK_F12 ) return KEY_F12;

		if( k==VK_LEFT  )    return KEY_LEFT;
		if( k==VK_RIGHT )    return KEY_RIGHT;
		if( k==VK_UP    )    return KEY_UP;
		if( k==VK_DOWN  )    return KEY_DOWN;
		if( k==VK_END   )    return KEY_END;
		if( k==VK_HOME  )    return KEY_HOME;
		if( k==VK_PRIOR )    return KEY_PGUP;
		if( k==VK_NEXT )     return KEY_PGDOWN;
//		if( k==VK_SPACE )    return KEY_SPACE;

		if( k==VK_DELETE)    return KEY_DELETE;
		if( k==VK_TAB)       return  KEY_TAB;

		if( k==VK_RSHIFT )   return KEY_RSHIFT;
		if( k==VK_RCONTROL ) return KEY_RCONTROL;
		if( k==VK_LSHIFT )   return KEY_LSHIFT;
		if( k==VK_LCONTROL ) return KEY_LCONTROL;
		if( k==VK_SHIFT )    return KEY_SHIFT;
		if( k==VK_CONTROL )  return KEY_CONTROL;
		if( k==VK_MENU )     return KEY_ALT;
		if( k==VK_LMENU )    return KEY_LALT;
		if( k==VK_RMENU )    return KEY_RALT;
	}

	if( k==VK_BACK )     return KEY_BACK;
	if( k==VK_RETURN )   return KEY_ENTER;
	if( k>31 && k<128 )
		return k;

    return k;
}

//==============================================================================================
int piWindowEvents_GetMouse_Dx( piMouseInput *me )
{
	int res = me->x - me->ox;
    me->ox = me->x;
	return res;
}
int piWindowEvents_GetMouse_Dy( piMouseInput *me )
{
	int res = me->y - me->oy;
    me->oy = me->y;
	return res;
}
int piWindowEvents_GetMouse_Dz( piMouseInput *me )
{
	int res = me->wheel/120;
    me->wheel = 0;
	return res;
}
void piWindowEvents_GetMouse_D( piMouseInput *me )
{
	me->dx = me->x - me->ox;
	me->dy = me->y - me->oy;
    me->dz = me->wheel/120;
    me->oy = me->y;
    me->ox = me->x;
    me->wheel = me->wheel;
}


static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	iIqWindow *me = (iIqWindow *)((LONG_PTR)GetWindowLongPtr( hWnd, GWLP_USERDATA ));
	if( !me )
		return( DefWindowProc(hWnd,uMsg,wParam,lParam) );

	iIqWindowMgr *mgr = me->mMgr;

	// salvapantallas
    if (uMsg == WM_SYSCOMMAND && (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER))
            return 0;

	if ( uMsg == WM_CREATE )
	{
		// This GUID is for all USB serial host PnP drivers, but you can replace it with any valid device class guid.
		GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
		ZeroMemory( &NotificationFilter, sizeof( NotificationFilter ) );
		NotificationFilter.dbcc_size = sizeof( DEV_BROADCAST_DEVICEINTERFACE );
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = WceusbshGUID;
		HDEVNOTIFY hDeviceNotify = RegisterDeviceNotification(
			hWnd,					    // events recipient
			&NotificationFilter,		// type of device
			DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
		);
		return( 0 );
	}

    me->eventinfo.mouse.wheel = 0;

	// boton x o pulsacion de escape, alt-f4
	if(uMsg==WM_CLOSE || uMsg==WM_DESTROY || (uMsg==WM_KEYDOWN && wParam==VK_ESCAPE) || (uMsg==WM_SYSKEYDOWN && wParam==VK_F4) )
	{
        me->exitReq = 1;
        return 0;
	}

	if( uMsg==WM_LBUTTONDOWN )
	{
		me->eventinfo.mouse.lb_isDown = 1;
		me->eventinfo.mouse.x = (int)(lParam&0xffff);
		me->eventinfo.mouse.y = (int)(int)(lParam>>16);
		me->eventinfo.mouse.ox = me->eventinfo.mouse.x;
		me->eventinfo.mouse.oy = me->eventinfo.mouse.y;
		mgr->mEventinfo.mouse.lb_isDown = 1;
		mgr->mEventinfo.mouse.x = (int)(lParam&0xffff);
		mgr->mEventinfo.mouse.y = (int)(int)(lParam>>16);
		mgr->mEventinfo.mouse.ox = mgr->mEventinfo.mouse.x;
		mgr->mEventinfo.mouse.oy = mgr->mEventinfo.mouse.y;
		return( 0 );
	}
	if( uMsg==WM_LBUTTONUP )
	{
		me->eventinfo.mouse.lb_isDown = 0;
		me->eventinfo.mouse.ox = -1;
		me->eventinfo.mouse.oy = -1;

		mgr->mEventinfo.mouse.lb_isDown = 0;
		mgr->mEventinfo.mouse.ox = -1;
		mgr->mEventinfo.mouse.oy = -1;
		return( 0 );
	}
	if( uMsg==WM_RBUTTONDOWN )
	{
		me->eventinfo.mouse.rb_isDown = 1;
		me->eventinfo.mouse.x = (int)(lParam&0xffff);
		me->eventinfo.mouse.y = (int)(int)(lParam>>16);
		me->eventinfo.mouse.ox = me->eventinfo.mouse.x;
		me->eventinfo.mouse.oy = me->eventinfo.mouse.y;

		mgr->mEventinfo.mouse.rb_isDown = 1;
		mgr->mEventinfo.mouse.x = (int)(lParam&0xffff);
		mgr->mEventinfo.mouse.y = (int)(int)(lParam>>16);
		mgr->mEventinfo.mouse.ox = mgr->mEventinfo.mouse.x;
		mgr->mEventinfo.mouse.oy = mgr->mEventinfo.mouse.y;
		return( 0 );
	}
	if( uMsg==WM_RBUTTONUP )
	{
		me->eventinfo.mouse.rb_isDown = 0;
		me->eventinfo.mouse.ox = -1;
		me->eventinfo.mouse.oy = -1;
		mgr->mEventinfo.mouse.rb_isDown = 0;
		mgr->mEventinfo.mouse.ox = -1;
		mgr->mEventinfo.mouse.oy = -1;
		return( 0 );
	}
	if( uMsg==WM_MOUSEMOVE )
	{
        me->eventinfo.mouse.x = (int)(lParam&0xffff);
        me->eventinfo.mouse.y = (int)(lParam>>16);
        mgr->mEventinfo.mouse.x = (int)(lParam&0xffff);
        mgr->mEventinfo.mouse.y = (int)(lParam>>16);
		return( 0 );
	}
	if( uMsg==WM_MOUSEWHEEL )
	{
		//int fwKeys = GET_KEYSTATE_WPARAM(wParam);
		me->eventinfo.mouse.wheel = GET_WHEEL_DELTA_WPARAM(wParam);

		return 0;
	}

    if( uMsg==WM_SYSKEYDOWN )
    {
     	const int ch = win2pi((int)(wParam&255), 0);
		me->eventinfo.keyb.state[ch] = 1;
		mgr->mEventinfo.keyb.state[ch] = 1;
        return 0;
    }
    if( uMsg==WM_SYSKEYUP )
    {
		const int ch = win2pi((int)(wParam&255), 0);
		me->eventinfo.keyb.state[ch] = 0;
		mgr->mEventinfo.keyb.state[ch] = 0;
        return 0;
    }

	if( uMsg==WM_KEYDOWN )
    {
		const int ch = win2pi((int)(wParam&255), 0);
		me->eventinfo.keyb.state[ch] = 1;
		mgr->mEventinfo.keyb.state[ch] = 1;

		if( ch<32 && ch!=KEY_BACK && ch!=KEY_ENTER)
		{
			if( me->eventinfo.keyb.queueLen<1024 )
				me->eventinfo.keyb.queue[me->eventinfo.keyb.queueLen++] = ch;
			if(mgr->mEventinfo.keyb.queueLen<1024 )
				mgr->mEventinfo.keyb.queue[mgr->mEventinfo.keyb.queueLen++] = ch;
		}


		return 0;
    }
    if( uMsg==WM_KEYUP )
    {
		const int ch = win2pi((int)(wParam&255), 0);
        me->eventinfo.keyb.state[ch] = 0;
        mgr->mEventinfo.keyb.state[ch] = 0;
		return 0;
    }
	if( uMsg==WM_CHAR )
    {
		const int ch = win2pi((int)(wParam&255), 1);

        me->eventinfo.keyb.key[ch] = 1;
		if( me->eventinfo.keyb.queueLen<1024 )
			me->eventinfo.keyb.queue[me->eventinfo.keyb.queueLen++] = ch;

        mgr->mEventinfo.keyb.key[ch] = 1;
		if( mgr->mEventinfo.keyb.queueLen<1024 )
			mgr->mEventinfo.keyb.queue[mgr->mEventinfo.keyb.queueLen++] = ch;

        return 0;
    }
	if( uMsg==WM_SIZE )
    {
        me->mXres = LOWORD(lParam);
        me->mYres = HIWORD(lParam);
		return 0;
    }

	return( DefWindowProc(hWnd,uMsg,wParam,lParam) );
}


piWindowMgr piWindowMgr_Init( void )
{
    iIqWindowMgr *me = (iIqWindowMgr*)malloc( sizeof(iIqWindowMgr) );
    if( !me )
        return 0;

    memset( &me->mEventinfo, 0, sizeof(piWindowEvents) );

    wcscpy_s( me->lpszClassName, 64, L"piWindow_class" );
    memset( &me->mWindowClass, 0, sizeof(WNDCLASS) );
    //me->mWindowClass.style         = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    me->mWindowClass.lpfnWndProc   = WndProc;
    me->mWindowClass.hInstance     = me->hInstance;
    me->mWindowClass.lpszClassName = me->lpszClassName;
	me->mWindowClass.hbrBackground = (HBRUSH)CreateSolidBrush(0x00000000);
	me->mWindowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    me->hInstance = 0;

    if( !RegisterClass(&me->mWindowClass) )
        return( 0 );

	me->mNumWindows = 0;

    return me;
}

void piWindowMgr_End( piWindowMgr vme )
{
    iIqWindowMgr *me = (iIqWindowMgr*)vme;

    UnregisterClass( me->lpszClassName, me->hInstance );
}

int piWindowMgr_MessageLoop( piWindowMgr vme )
{
    iIqWindowMgr *me = (iIqWindowMgr*)vme;

    MSG         msg;
    int         done  = 0;
#if 0
	const int num = me->mNumWindows;
	for( int i=0; i<num; i++ )
	{
		const iIqWindow *win = me->mWindows[ i ];

		while( PeekMessage(&msg,win->hWnd,0,0,PM_REMOVE) )
		{
			if( msg.message==WM_QUIT ) { done=1; break; }
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
#else
	while( PeekMessage(&msg,0,0,0,PM_REMOVE) )
	{
		if( msg.message==WM_QUIT ) { done=1; break; }
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

#endif

    return( done );
}

piWindowEvents *piWindowMgr_getEvents( piWindowMgr vme )
{
    iIqWindowMgr *me = (iIqWindowMgr*)vme;
    return &me->mEventinfo;
}

//========================================================================
//========================================================================
//========================================================================

void piWindow_end( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;

    if( me->hDC  ) ReleaseDC( me->hWnd, me->hDC );
    if( me->hWnd ) DestroyWindow( me->hWnd );

    if( me->full )
    {
        ChangeDisplaySettings( 0, 0 );
		while( ShowCursor( 1 )<0 ); // show cursor
    }
}


static BOOL CALLBACK MyInfoEnumProc(HMONITOR h, HDC hdv, LPRECT r, LPARAM p)
{
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo( h, &info );


	return true;
}



piWindow piWindow_init( piWindowMgr vmgr, const wchar_t *title, int xo, int yo, int xres, int yres, bool full, bool decoration, bool resizable, bool hideCursor )
{
    iIqWindowMgr *mgr = (iIqWindowMgr*)vmgr;

    iIqWindow *me = (iIqWindow*)malloc( sizeof(iIqWindow) );
    if( !me )
        return 0;

    memset( &me->eventinfo, 0, sizeof(piWindowEvents) );
	me->mMgr = mgr;


    DWORD			dwExStyle, dwStyle;
    DEVMODEW			dmScreenSettings;
    RECT			rec;

    me->full = full;

    if( me->full )
    {
        dmScreenSettings.dmSize       = sizeof(DEVMODEW);
        dmScreenSettings.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmPelsWidth  = xres;
        dmScreenSettings.dmPelsHeight = yres;

        if( ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return 0;

		//EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, 0);
/*

		wchar_t name[256];
		for(int de=0; ; de++ )
		{
			info.cb = sizeof(DISPLAY_DEVICEW);
			if( !EnumDisplayDevices( 0, de, &info, 0 ) )
				break;
			if( de==0 )
			{
				wcscpy(name,info.DeviceName);
				break;
			}
		}
		wcscpy(dmScreenSettings.dmDeviceName,name);
		int res = ChangeDisplaySettingsEx(name,&dmScreenSettings,0,CDS_FULLSCREEN,0);
		if( res!=DISP_CHANGE_SUCCESSFUL)
            return( 0 );
*/
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_VISIBLE | WS_POPUP | WS_MAXIMIZE | WS_CLIPSIBLINGS|WS_CLIPCHILDREN;

		if( hideCursor )
			while( ShowCursor( 0 )>=0 );	// hide cursor

        xo = 0;
        yo = 0;
    }
    else
    {
        if( decoration )
        {
			dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
			if( resizable )
                 dwStyle = WS_VISIBLE | WS_CLIPSIBLINGS|WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW ;
			else			     
                dwStyle = WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_MINIMIZEBOX | WS_SYSMENU;
        }
        else
        {
			dwExStyle = WS_EX_APPWINDOW;
			dwStyle   = WS_VISIBLE | WS_CLIPSIBLINGS|WS_CLIPCHILDREN | WS_POPUP;
        }
    }

    rec.left   = 0;
    rec.top    = 0;
    rec.right  = xres;
    rec.bottom = yres;

    AdjustWindowRect( &rec, dwStyle, 0 );
	
	//int numMonitors = GetSystemMetrics(SM_CMONITORS);
	const int left = xo;//GetSystemMetrics(SM_XVIRTUALSCREEN);
	const int top =  yo;//GetSystemMetrics(SM_YVIRTUALSCREEN);
	//int virXres = GetSystemMetrics(SM_CXVIRTUALSCREEN);

    me->hWnd = CreateWindowEx( dwExStyle, mgr->lpszClassName, title, dwStyle,
                               left,//(GetSystemMetrics(SM_CXSCREEN)-rec.right+rec.left)>>1,
                               top,//(GetSystemMetrics(SM_CYSCREEN)-rec.bottom+rec.top)>>1,
                               rec.right-rec.left, rec.bottom-rec.top, 0, 0, mgr->hInstance, 0 );
    if( !me->hWnd )
        return( 0 );

    me->hDC = GetDC( me->hWnd );
    if( !me->hDC )
        return( 0 );

    me->mXres = xres;
    me->mYres = yres;
    me->exitReq = 0;

    #pragma warning(push)
    #pragma warning (disable: 4244)
	SetWindowLongPtr( me->hWnd, GWLP_USERDATA, (LONG_PTR)me );
    #pragma warning(pop)

    //-----------------------

    SetForegroundWindow( me->hWnd );    // slightly higher priority
    SetFocus( me->hWnd );               // sets keyboard focus to the window
    
//	ShowWindow( me->hWnd, SW_SHOWNORMAL );
   // UpdateWindow( me->hWnd );

	mgr->mWindows[ mgr->mNumWindows++ ] = me;

    return me;
}


void piWindow_setText( piWindow vme, const wchar_t *str )
{
    iIqWindow *me = (iIqWindow*)vme;

    SetWindowText( me->hWnd, str );
}



void *piWindow_getHandle( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;

    return (void*)me->hWnd;
}


piWindowEvents *piWindow_getEvents( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;
    return &me->eventinfo;
}

int piWindow_getExitReq( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;

    return me->exitReq;
}

void piWIndow_getSize( piWindow vme, int *res )
{
    iIqWindow *me = (iIqWindow*)vme;
    res[0] = me->mXres;
    res[1] = me->mYres;
}

void piWindow_hide( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;
    ShowWindow( me->hWnd, SW_HIDE );
}

void piWindow_show( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;
    ShowWindow( me->hWnd, SW_SHOW );
}

void piWindowEvents_Erase( piWindow vme )
{
    iIqWindow *me = (iIqWindow*)vme;

    //memset( me->eventinfo.keyb.state, 0, 256*sizeof(int) );
    memset( me->eventinfo.keyb.key,   0, 256*sizeof(int) );
}

void piWindowEvents_EraseFull( piKeyboardInput *me )
{
    me->queueLen = 0;
    memset( me->state, 0, 256*sizeof(int) );
    memset( me->key,   0, 256*sizeof(int) );
}

//=========================================


}

