/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003-2009 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

// This file contains information private to the skinner

#define NUMOFGDIOBJS (12+1) // 12 bitmaps, 1 font
#define hFont gdiobjs[12].hObj

struct SizeInfo
{
    int width, height;
    int HiddenTop;
    int HiddenBottom;
    int HiddenSide;
    int BottomAdjust;
    int BottomHeight;
    RECT rcClient;
};

struct GdiInfo
{
    int cx, cy;
    HGDIOBJ hObj;
};

struct button_set
{
    int set, pos;
};

struct WinInfo
{
    HMODULE hModule;
    HWND hwnd;
    LRESULT (WINAPI *pCallWindowProc)(WNDPROC,HWND,UINT,WPARAM,LPARAM);
    WNDPROC wpOrigWindowProc;
    LONG style, exstyle;

    struct SizeInfo S;
    struct GdiInfo gdiobjs[NUMOFGDIOBJS];
    HDC buf;

    bool is_unicode;
    bool apply_skin;

    bool in_set_region;
    bool dont_paint;
    bool sync_paint;

    bool is_active;
    bool is_active_app;
    bool is_zoomed;
    bool is_iconic;
    bool is_moving;

    bool is_rolled;
    bool is_ontop;
    bool is_sticky;
    bool has_sticky;

    bool dblclk_timer_set;

    char capture_button;
    char button_down;
    char button_count;
    struct button_set button_set[BUTTON_COUNT];
};

enum button_types {
    btn_None      = 0,

    btn_Close     = 1,
    btn_Max       = 2,
    btn_Min       = 3,
    btn_Rollup    = 4,
    btn_OnTop     = 5,
    btn_Sticky    = 6,
    btn_Icon      = 7,

    btn_Lower     ,
    btn_TMin      ,

    btn_VMax      ,
    btn_HMax      ,

    btn_Caption   ,
    btn_Nowhere   ,

    btn_Topleft   ,
    btn_Topright  ,
    btn_Top       ,

    btn_SysMenu
};
    
#define btn_Last btn_HMax

//-----------------------------------------------------------------

extern HINSTANCE hInstance;
extern unsigned bbSkinMsg;
extern SkinStruct mSkin;

bool GetSkin(void);
void send_log(HWND, const char *msg);
void subclass_window(HWND hwnd);
HWND GetRootWindow(HWND hwnd);
int imax(int a, int b);
int imin(int a, int b);

#define get_WinInfo(hwnd) ((struct WinInfo*)GetProp(hwnd, BBLEANSKIN_INFOPROP))
#define set_WinInfo(hwnd, WI) SetProp(hwnd, BBLEANSKIN_INFOPROP, WI)
#define del_WinInfo(hwnd) RemoveProp(hwnd, BBLEANSKIN_INFOPROP)

LRESULT APIENTRY WindowSubclassProc(HWND, UINT, WPARAM, LPARAM);

#define CALLORIGWINDOWPROC(hwnd, msg, wp, lp) \
    WI->pCallWindowProc(WI->wpOrigWindowProc, hwnd, msg, wp, lp)

//-----------------------------------------------------------------
