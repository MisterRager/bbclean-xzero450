/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2 or later) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

============================================================================
*/ // hookinfo.h

// This file contains information common to both the loader and the skinner

// strings
#define BBLEANSKIN_ENGINEDLL    "BBLEANSKINENG.DLL"
#define BBLEANSKIN_INFOPROP     "BBLEANSKIN_INFOPROP"
#define BBLEANSKIN_WINDOWMSG    "BBLEANSKIN_WINDOWMSG"
#define BBLEANSKIN_SHMEMID      "BBLEANSKIN_SHMEMID"
#define BBSHADE_PROP            "BBNormalHeight"

// branch versions
#define BBVERSION_LEAN 2
#define BBVERSION_XOB 1
#define BBVERSION_09X 0

// options for 'int EntryFunc(int option, SkinStruct *pSkin);'
#define ENGINE_SETHOOKS 0
#define ENGINE_UNSETHOOKS 1
#define ENGINE_SKINWINDOW 2
#define ENGINE_GETVERSION 3
#define ENGINE_THISVERSION 1140

// wParams for the registered 'BBLEANSKIN_WINDOWMSG'
enum
{
    MSGID_GETSHADEHEIGHT   = 1,
    MSGID_LOAD              ,
    MSGID_UNLOAD            ,
    MSGID_REDRAW            ,
    MSGID_REFRESH           ,
    MSGID_BB_SETSTICKY      ,
    MSGID_BBSM_RESET        ,
    MSGID_BBSM_SETACTIVE    ,
    MSGID_BBSM_SETPRESSED   ,
};

// ---------------------------------------------
struct GradientItem
{
    int bevelstyle;
    int bevelposition;
    int type;
    bool parentRelative;
    bool interlaced;
    COLORREF Color;
    COLORREF ColorTo;
    COLORREF TextColor;
    int borderWidth;
    COLORREF borderColor;
    int marginWidth;
    int validated;
    COLORREF ShadowColor;
    COLORREF OutlineColor;
    COLORREF ColorSplitTo;
    COLORREF ColorToSplitTo;
};

// ---------------------------------------------
struct exclusion_item
{
    unsigned char flen, clen, option;
    char buff[2];
};

struct exclusion_info
{
    int size;
    int count;
    struct exclusion_item ei[1];
};

#define BUTTON_SIZE 9
#define BUTTON_MAP_SIZE ((BUTTON_SIZE*BUTTON_SIZE-1)/8+1)
struct button_bmp
{
    unsigned char data[2][BUTTON_MAP_SIZE];
};

// ---------------------------------------------
struct SkinStruct
{
    GradientItem windowTitleFocus;
    GradientItem windowLabelFocus;
    GradientItem windowHandleFocus;
    GradientItem windowGripFocus;
    GradientItem windowButtonFocus;
    GradientItem windowButtonPressed;
    GradientItem windowButtonCloseFocus;
    GradientItem windowButtonClosePressed;

    GradientItem windowTitleUnfocus;
    GradientItem windowLabelUnfocus;
    GradientItem windowHandleUnfocus;
    GradientItem windowGripUnfocus;
    GradientItem windowButtonUnfocus;
    GradientItem windowButtonCloseUnfocus;

    struct
    {
        int  Height;
        int  Weight;
        int  Justify;   // DT_LEFT, DT_CENTER, DT_RIGHT
        int  validated; // not used
        char Face[128];

    } windowFont;

    COLORREF focus_borderColor;
    COLORREF unfocus_borderColor;
    int borderWidth;
    int handleHeight;

    int gripWidth;
    int buttonSize;
    int labelHeight;
    int rollupHeight;

    int buttonSpace;
    int buttonMargin;
    int labelMargin;
    int ncTop;
    int ncBottom;

    struct button_bmp button_bmp[6];
    char button_string[6];

    struct {
        char Dbl[3];
        char Right[3];
        char Mid[3];
        char Left[3];
    } captionClicks;

    bool snapWindows;
    bool enableLog;
    bool nixShadeStyle;
    bool imageDither;
    bool isNewStyle;
    bool drawLocked;

    int cxSizeFrame;
    int cxFixedFrame;
    int cyCaption;
    int cySmCaption;

    HWND BBhwnd;
    HWND loghwnd;
    HWND skinwnd;
    int BBVersion;

    HHOOK hCallWndHook;
    HHOOK hGetMsgHook;
    struct exclusion_info exInfo;
};

#define offset_exInfo ((int)&((SkinStruct*)NULL)->exInfo)
#define offset_hooks ((int)&((SkinStruct*)NULL)->hCallWndHook)

// ---------------------------------------------

