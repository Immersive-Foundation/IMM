//
// Branched off piLibs (Copyright © 2015 Inigo Quilez, The MIT License), in 2015. See THIRD_PARTY_LICENSES.txt
//
#pragma once

namespace ImmCore {

#define KEY_LEFT	0
#define KEY_RIGHT	1
#define KEY_UP		2
#define KEY_DOWN	3
#define KEY_SPACE   4
#define KEY_PGUP	5
#define KEY_PGDOWN	6
#define KEY_ENTER   7
#define KEY_BACK    8
#define KEY_END     9
#define KEY_HOME   10


#define KEY_F1      11
#define KEY_F2      12
#define KEY_F3      13
#define KEY_F4      14
#define KEY_F5      15
#define KEY_F6      16
#define KEY_F7      17
#define KEY_F8      18
#define KEY_F9      19
#define KEY_F10     20
#define KEY_F11     21
#define KEY_F12     22

#define KEY_LSHIFT	    23
#define KEY_RSHIFT	    24
#define KEY_LCONTROL	25
#define KEY_RCONTROL	26
#define KEY_LALT        27
#define KEY_RALT        28
#define KEY_SHIFT	    29
#define KEY_CONTROL	    30
#define KEY_ALT         31

#define KEY_DELETE      129
#define KEY_TAB         130

#define KEY_0		48
#define KEY_1		49
#define KEY_2		50
#define KEY_3		51
#define KEY_4		52
#define KEY_5		53
#define KEY_6		54
#define KEY_7		55
#define KEY_8		56
#define KEY_9		57

#define TOUCH_NONE				0
#define TOUCH_BEGIN				1
#define TOUCH_PAN				2
#define TOUCH_ZOOM				3
#define TOUCH_ROTATE			4
#define TOUCH_TWOFINGERTAP		5
#define TOUCH_PRESSANDTAP		6
#define TOUCH_END				7

typedef struct
{
    int     state[256];
    int     key[256];
	int     queue[1024];
	int		queueLen;
}piKeyboardInput;

typedef struct
{
    int	    x, y, ox, oy, dx, dy, dz;
    int     lb_isDown;
    int     rb_isDown;
	int     wheel;
}piMouseInput;


typedef struct
{
	piKeyboardInput	keyb;
	piMouseInput	mouse;
}piWindowEvents;

typedef void * piWindowMgr;
typedef void * piWindow;


piWindowMgr piWindowMgr_Init( void );
int         piWindowMgr_MessageLoop( piWindowMgr vme );
void        piWindowMgr_End( piWindowMgr vme );
piWindowEvents *piWindowMgr_getEvents( piWindowMgr vme );

piWindow        piWindow_init( piWindowMgr mgr, const wchar_t *title, int xo, int yo, int xres, int yres, bool full, bool decoration, bool resizable, bool hideCursor );
void            piWindow_end( piWindow me );
void            piWindow_setText( piWindow me, const wchar_t *str );
void           *piWindow_getHandle( piWindow me );
piWindowEvents *piWindow_getEvents( piWindow me );
int             piWindow_getExitReq( piWindow vme );
void            piWIndow_getSize( piWindow vme, int *res );
void            piWindow_hide( piWindow vme );
void            piWindow_show( piWindow vme );

void piWindowEvents_Erase( piWindow vme );
void piWindowEvents_EraseFull( piKeyboardInput *me );
int  piWindowEvents_GetMouse_Dx( piMouseInput *me );
int  piWindowEvents_GetMouse_Dy( piMouseInput *me );
int  piWindowEvents_GetMouse_Dz( piMouseInput *me );
void piWindowEvents_GetMouse_D( piMouseInput *me );

} // namespace ImmCore
