/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  In addition to the GPL, BBApi.h and the needed import library may be
  used to create plugins or programs to be released under any license
  the author wishes.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

/* BBApi.h - bbLean 1.17 plugin API */

#ifndef _BBAPI_H_
#define _BBAPI_H_

/*------------------------------------------ */
/* windows include */
/*------------------------------------------ */
#ifndef WINVER
  #define WINVER 0x0500
  #define _WIN32_WINNT 0x0500
  #define _WIN32_IE 0x0501
#endif

#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

/*------------------------------------------ */
/* compiler specifics */
/*------------------------------------------ */

#if defined __GNUC__ && !defined _WIN64
  /* Convince GNUC to export the __stdcall function with underscore */
  #define GetBlackboxPath _GetBlackboxPath
#endif

#ifdef __BORLANDC__
  /* BORLANDC requires .def files with both the blackbox core and plugins */
  #define DLL_EXPORT
#endif

#ifndef DLL_EXPORT
  #define DLL_EXPORT __declspec(dllexport)
#endif

/*------------------------------------------ */
/* plain C support */

#ifndef __cplusplus
  typedef char bool;
  #define false 0
  #define true 1
  #define ISNULL
  #define class struct
#else
  #define ISNULL =NULL
#endif

/*------------------------------------------ */
/* buffer size for FileRead, ReplaceEnvVars */

#define MAX_LINE_LENGTH 1024

/*------------------------------------------ */
/* BImage definitions */

/* Gradient types */
#define B_HORIZONTAL 0
#define B_VERTICAL 1
#define B_DIAGONAL 2
#define B_CROSSDIAGONAL 3
#define B_PIPECROSS 4
#define B_ELLIPTIC 5
#define B_RECTANGLE 6
#define B_PYRAMID 7
#define B_SOLID 8

/* Bevelstyle */
#define BEVEL_FLAT 0
#define BEVEL_RAISED 1
#define BEVEL_SUNKEN 2

/* Bevelposition */
#define BEVEL1 1
#define BEVEL2 2

/* bullet styles for menus */
#define BS_EMPTY 0
#define BS_TRIANGLE 1
#define BS_SQUARE 2
#define BS_DIAMOND 3
#define BS_CIRCLE 4
#define BS_CHECK 5

/*=========================================================================== */
/* Blackbox messages */

#define BB_REGISTERMESSAGE      10001
#define BB_UNREGISTERMESSAGE    10002

/* ----------------------------------- */
#define BB_QUIT                 10101 /* lParam 0=ask/1=quiet */
#define BB_RESTART              10102
#define BB_RECONFIGURE          10103
#define BB_SETSTYLE             10104 /* lParam: const char* stylefile */

#define BB_EXITTYPE             10105 /* For plugins: receive only */
/* lParam values for BB_EXITTYPE: */
  #define B_SHUTDOWN    0 /* Shutdown/Reboot/Logoff */
  #define B_QUIT        1
  #define B_RESTART     2

#define BB_TOOLBARUPDATE        10106 /* toolbar changed position/size */
#define BB_SETTHEME             10107 /* xoblite */

/* ----------------------------------- */
#define BB_EDITFILE             10201
/* wParam values for BB_EDITFILE:
   0 = Current style,
   1 = menu.rc,
   2 = plugins.rc
   3 = extensions.rc
   4 = blackbox.rc
   -1 = filename in (const char*)lParam */

/* Send a command or broam for execution as (const char*)lParam: */
#define BB_EXECUTE              10202 /* see also BB_EXECUTEASYNC */
#define BB_ABOUTSTYLE           10203
#define BB_ABOUTPLUGINS         10204

/* ----------------------------------- */
/* Show special menu */
#define BB_MENU                 10301
/* wParam values: */
  #define BB_MENU_ROOT      0   /* normal rightclick menu */
  #define BB_MENU_TASKS     1   /* workspaces menu (mid-click) */
  #define BB_MENU_TOOLBAR   2   /* obsolete */
  #define BB_MENU_ICONS     3   /* iconized tasks menu */
  #define BB_MENU_SIGNAL    4   /* do nothing, for BBSoundFX */
  #define BB_MENU_BROAM     6   /* (const char *)lParam: id-string (e.g. "configuration") */
  /* for internal use only: */
  #define BB_MENU_UPDATE    8   /* update the ROOT menu */

#define BB_HIDEMENU             10302 /* hide not-pinned menus */
#define BB_TOGGLETRAY           10303 /* toggles systembar etc. */
#define BB_TOGGLESYSTEMBAR      10303 /* xoblite */
/* Set the toolbar label to (const char*)lParam - returns to normal after 2 seconds) */
#define BB_SETTOOLBARLABEL      10304
#define BB_TOGGLEPLUGINS        10305
#define BB_SUBMENU              10306 /* for BBSoundFX, receive only */
#define BB_TOGGLESLIT           10307 /* xoblite */
#define BB_TOGGLETOOLBAR        10308 /* xoblite */
#define BB_AUTOHIDE             10309 /* wParam: 1=enable/0=disable autohide, lParam: 2 */

/* ----------------------------------- */
/* Shutdown etc
    wParam: 0=Shutdown, 1=Reboot, 2=Logoff, 3=Hibernate, 4=Suspend, 5=LockWorkstation
    lParam; 0=ask, 1=quiet */
#define BB_SHUTDOWN             10401

/* Show the 'run' dialog box */
#define BB_RUN                  10402

/* ----------------------------------- */
/* Sent from blackbox on workspace change.  For plugins: receive only */
#define BB_DESKTOPINFO          10501 /* lParam: struct DesktopInfo* */

/* Depreciated, use GetDesktopInfo() */
#define BB_LISTDESKTOPS         10502

/* switch to workspace #n in lParam */
#define BB_SWITCHTON            10503

/* Activate window (restore if minimized). lParam: hwnd to activate */
#define BB_BRINGTOFRONT         10504
  /* wParam flag: Zoom window into current workspace (bblean 1.16+) */
  #define BBBTF_CURRENT 4

/* ----------------------------------- */
#define BB_WORKSPACE            10505
  /* wParam values: */
  #define BBWS_DESKLEFT         0
  #define BBWS_DESKRIGHT        1
  #define BBWS_ADDDESKTOP       2
  #define BBWS_DELDESKTOP       3
  #define BBWS_SWITCHTODESK     4  /* lParam: workspace to switch to. */
  #define BBWS_GATHERWINDOWS    5
  #define BBWS_MOVEWINDOWLEFT   6  /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_MOVEWINDOWRIGHT  7  /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_PREVWINDOW       8  /* lParam: 0=current / 1=all workspaces */
  #define BBWS_NEXTWINDOW       9  /* lParam: 0=current / 1=all workspaces */
  #define BBWS_LASTDESK         10

  /* below: bb4win 0.9x, bblean 1.2+ */
  #define BBWS_TOGGLESTICKY     12 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_EDITNAME         13
  #define BBWS_MAKESTICKY       14 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_CLEARSTICKY      15 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_ISSTICKY         16 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_TOGGLEONTOP      17 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_GETTOPWINDOW     18 /* lParam: hwnd or NULL for foregroundwindow */

  #define BBWS_MINIMIZEALL      20
  #define BBWS_RESTOREALL       21
  #define BBWS_TILEVERTICAL     22
  #define BBWS_TILEHORIZONTAL   23
  #define BBWS_CASCADE          24

/*------------------------------------------ */
#define BB_TASKSUPDATE          10506  /* For plugins: receive only */
  /* lParam for BB_TASKSUPDATE: */
  #define TASKITEM_ADDED 0        /*  wParam: hwnd */
  #define TASKITEM_MODIFIED 1     /*  wParam: hwnd */
  #define TASKITEM_ACTIVATED 2    /*  wParam: hwnd */
  #define TASKITEM_REMOVED 3      /*  wParam: hwnd */
  #define TASKITEM_REFRESH 4      /*  wParam: NULL (sent on window moved to workspace) */
  #define TASKITEM_FLASHED 5      /*  wParam: hwnd */
  #define TASKITEM_LANGUAGE 6     /*  win9x only, wParam: hwnd */

#define BB_TRAYUPDATE           10507 /* For plugins: receive only */
  /* lParam for BB_TRAYUPDATE: */
  #define TRAYICON_ADDED 0
  #define TRAYICON_MODIFIED 1   /* wParam: NIF_XXX flags */
  #define TRAYICON_REMOVED 2

/* cleanup dead trayicons */
/* #define BB_CLEANTRAY         10508 - obsolete */
/* #define BB_CLEANTASKS        10509 - obsolete */

/* File dragged over/dropped on desktop.
   lParam: filename
   wParam 0:drop (on mousebutton up) 1:drag (on mouse over)
   The plugin should return TRUE if it wants the file, FALSE if not */
#define BB_DRAGTODESKTOP        10510 /* For plugins: receive only */

/* Move window to workspace, dont switch. wParam: new desk - lParam: hwnd */
#define BB_SENDWINDOWTON        10511
/* Move window and switch to workspace. wParam: new desk - lParam: hwnd */
#define BB_MOVEWINDOWTON        10512

/* ----------------------------------- */
/* ShellHook messages, obsolete - register for BB_TASKSUPDATE instead */
#define BB_ADDTASK              10601
#define BB_REMOVETASK           10602
#define BB_ACTIVATESHELLWINDOW  10603
#define BB_ACTIVETASK           10604
#define BB_MINMAXTASK           10605
#define BB_REDRAWTASK           10610

/* ----------------------------------- */
/* Window commands */
#define BB_WINDOWSHADE          10606 /* lParam: hwnd or NULL for foregroundwindow */
#define BB_WINDOWGROWHEIGHT     10607 /* ... */
#define BB_WINDOWGROWWIDTH      10608 /* ... */
#define BB_WINDOWLOWER          10609 /* ... */
#define BB_WINDOWMINIMIZE       10611 /* ... */
#define BB_WINDOWRAISE          10612 /* ... */
#define BB_WINDOWMAXIMIZE       10613 /* ... */
#define BB_WINDOWRESTORE        10614 /* ... */
#define BB_WINDOWCLOSE          10615 /* ... */
#define BB_WINDOWSIZE           10616 /* ... */
#define BB_WINDOWMOVE           10617 /* ... */
#define BB_WINDOWMINIMIZETOTRAY 10618 /* not implemented */

/* ----------------------------------- */
/* Broadcast a string (Bro@m) to core and all plugins */
#define BB_BROADCAST            10901 /* wParam: 0, (LPCSTR)lParam: command string */

/* ----------------------------------- */
/* BBSlit messages */
#define SLIT_ADD                11001   /* lParam: plugin's hwnd */
#define SLIT_REMOVE             11002   /* lParam: plugin's hwnd */
#define SLIT_UPDATE             11003   /* lParam: plugin's hwnd or NULL */

/*=========================================================================== */
/* BBLean extensions */

/* Kinda opposite of BB_BROADCAST, to get info whether to set checkmarks
   in menu (experimental) */
#define BB_GETBOOL              10870

/* for bbStylemaker: request a redraw of the specified parts */
#define BB_REDRAWGUI            10881
  /* wParam bitflags: */
  #define BBRG_TOOLBAR (1<<0)
  #define BBRG_MENU    (1<<1)
  #define BBRG_WINDOW  (1<<2)
  #define BBRG_DESK    (1<<3)
  #define BBRG_FOCUS   (1<<4)
  #define BBRG_PRESSED (1<<5)
  #define BBRG_STICKY  (1<<6)
  #define BBRG_FOLDER  (1<<7)
  #define BBRG_SLIT    (1<<8)

/* Post a command or broam for execution (in lParam),
   like BB_EXECUTE, but returns immediately. Use with SendMessage */
#define BB_EXECUTEASYNC         10882

/* desktop clicked, lParam: 0=leftdown 1=left, 2=right, 3=mid, 4=x1, 5=x2, 6=x3) */
#define BB_DESKCLICK            10884

/* win9x: left/right winkey pressed */
#define BB_WINKEY               10886

/* bbstylemaker 1.3+ */
#define BB_SENDDATA             10890
#define BB_GETSTYLE             10891
#define BB_GETSTYLESTRUCT       10892
#define BB_SETSTYLESTRUCT       10893

/* ----------------------------------- */
/* internal usage */
#define BB_FOLDERCHANGED        10897   /* folder changed */
#define BB_DRAGOVER             10898   /* dragging over menu */
#define BB_POSTSTRING           10899   /* asynchrone execute command */

/* ----------------------------------- */
#define BB_MSGFIRST             10000
#define BB_MSGLAST              12000

/* ----------------------------------- */
/* bbLeanSkin support, keep in sync with bbLeanSkin/hookinfo.h */
#define BBSHADE_PROP            "BBNormalHeight"
#define BBLEANSKIN_MSG          "BBLEANSKIN_MSG"
// wParams for the registered 'BBLEANSKIN_MSG'
#define BBLS_REDRAW             1
#define BBLS_SETSTICKY          2
#define BBLS_GETSHADEHEIGHT     3

/* =========================================================================== */
/* StyleItem */

typedef struct StyleItem
{
    /* 0.0.80 */
    int bevelstyle;
    int bevelposition;
    int type;
    bool parentRelative;
    bool interlaced;

    /* 0.0.90 */
    COLORREF Color;
    COLORREF ColorTo;
    COLORREF TextColor;
    int FontHeight;
    int FontWeight;
    int Justify;
    int validated;

    char Font[128];

    /* bbLean 1.16 */
    int nVersion;
    int marginWidth;
    int borderWidth;
    COLORREF borderColor;
    COLORREF foregroundColor;
    COLORREF disabledColor;
    bool bordered;
    bool FontShadow; /* xoblite */

    char reserved[102]; /* keep sizeof(StyleItem) = 300 */

} StyleItem;

#define picColor TextColor
#define VALID_TEXTCOLOR 8

typedef class Menu Menu;
typedef class MenuItem MenuItem;

/*=========================================================================== */
/* constants for GetSettingPtr(int index) -> returns: */
enum SN_INDEX {

    SN_STYLESTRUCT      = 0     , /* StyleStruct* */

    SN_TOOLBAR          = 1     , /* StyleItem* */
    SN_TOOLBARBUTTON            , /* StyleItem* */
    SN_TOOLBARBUTTONP           , /* StyleItem* */
    SN_TOOLBARLABEL             , /* StyleItem* */
    SN_TOOLBARWINDOWLABEL       , /* StyleItem* */
    SN_TOOLBARCLOCK             , /* StyleItem* */
    SN_MENUTITLE                , /* StyleItem* */
    SN_MENUFRAME                , /* StyleItem* */
    SN_MENUHILITE               , /* StyleItem* */

    SN_MENUBULLET               , /* char* */
    SN_MENUBULLETPOS              /* char* */
                                ,
    SN_BORDERWIDTH              , /* int* */
    SN_BORDERCOLOR              , /* COLORREF* */
    SN_BEVELWIDTH               , /* int* */
    SN_FRAMEWIDTH               , /* int* */
    SN_HANDLEHEIGHT             , /* int* */
    SN_ROOTCOMMAND              , /* char* */

    SN_MENUALPHA                , /* int* */
    SN_TOOLBARALPHA             , /* int* */
    SN_METRICSUNIX              , /* bool* */
    SN_BULLETUNIX               , /* bool* */

    SN_WINFOCUS_TITLE           , /* StyleItem* */
    SN_WINFOCUS_LABEL           , /* StyleItem* */
    SN_WINFOCUS_HANDLE          , /* StyleItem* */
    SN_WINFOCUS_GRIP            , /* StyleItem* */
    SN_WINFOCUS_BUTTON          , /* StyleItem* */
    SN_WINFOCUS_BUTTONP           /* StyleItem* */
                                ,
    SN_WINUNFOCUS_TITLE         , /* StyleItem* */
    SN_WINUNFOCUS_LABEL         , /* StyleItem* */
    SN_WINUNFOCUS_HANDLE        , /* StyleItem* */
    SN_WINUNFOCUS_GRIP          , /* StyleItem* */
    SN_WINUNFOCUS_BUTTON        , /* StyleItem* */

    SN_WINFOCUS_FRAME_COLOR     , /* COLORREF* */
    SN_WINUNFOCUS_FRAME_COLOR   , /* COLORREF* */

    SN_ISSTYLE070               , /* bool* */
    SN_SLIT                     , /* StyleItem* */

    SN_LAST
};

/*=========================================================================== */
/* Plugin API */

#ifdef __BBCORE__
  #define API_EXPORT DLL_EXPORT
#else
  #define API_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /* ------------------------------------ */
    /* Resource File API */

    /* Read Settings */
    API_EXPORT bool ReadBool(const char* fileName, const char* szKey, bool defaultBool);
    API_EXPORT int ReadInt(const char* fileName, const char* szKey, int defaultInt);
    API_EXPORT COLORREF ReadColor(const char* fileName, const char* szKey, const char* defaultColor);
    API_EXPORT const char* ReadString(const char* fileName, const char* szKey, const char* defaultString);

    /* Read a rc-value as string. 'ptr' is optional, if present, at input indicates the line
       from where the search starts, at output is set to the line that follows the match. */
    API_EXPORT const char* ReadValue(const char* fileName, const char* szKey, long* ptr ISNULL);

    /* Note that pointers returned from 'ReadString' and 'ReadValue' are valid only
       until the next Read/Write call. For later usage, you need to copy the string
       into a place within your code. */

    API_EXPORT int FoundLastValue(void);
    /* Returns: 0=not found, 1=found exact value, 2=found matching wildcard */

    /* Write Settings */
    API_EXPORT void WriteBool(const char* fileName, const char* szKey, bool value);
    API_EXPORT void WriteInt(const char* fileName, const char* szKey, int value);
    API_EXPORT void WriteString(const char* fileName, const char* szKey, const char* value);
    API_EXPORT void WriteColor(const char* fileName, const char* szKey, COLORREF value);

    /* Delete rc-entry - wildcards supported for keyword */
    API_EXPORT bool DeleteSetting(LPCSTR fileName, LPCSTR szKey);
    /* Rename Setting (or delete with new_keyword=NULL) */
    API_EXPORT bool RenameSetting(const char* fileName, const char* old_keyword, const char* new_keyword);

    /* Direct access to Settings variables / styleitems / colors
       See the "SN_XXX" constants above */
    API_EXPORT void* GetSettingPtr(int sn_index);

    /* Read an entire StyleItem. This is not meant for the standard style items,
       but for a plugin to read in custom style extensions. Returns nonzero if
       any of the requested properties were read */
    API_EXPORT int ReadStyleItem(
        const char* fileName, /* style filename */
        const char* szKey,  /* keyword until the last dot, e.g. "bbpager.frame" */
        StyleItem* pStyleItemOut, /* pointer to receiving StyleItem struct */
        StyleItem* pStyleItemDefault /* pointer to a default StyleItem or NULL */
        );

    /* ------------------------------------ */
    /* Get paths */

    /* Path where blackbox.exe is (including trailing backslash) */
    API_EXPORT char* WINAPI GetBlackboxPath(char* path, int maxLength);
    /*
       Get full Path for a given filename. The file is searched:
       1) If pluginInstance is not NULL: In the directory that contains the DLL
       2) In the blackbox directory.
       Returns true if the file was found.
       If not, 'pszOut' is set to the default location (plugin directory)
    */
    API_EXPORT bool FindRCFile(char* pszOut, const char* fileName, HINSTANCE pluginInstance);

    /* Get configuration filepaths */
    API_EXPORT const char* bbrcPath(const char* bbrcFileName ISNULL);
    API_EXPORT const char* extensionsrcPath(const char* extensionsrcFileName ISNULL);
    API_EXPORT const char* menuPath(const char* menurcFileName ISNULL);
    API_EXPORT const char* plugrcPath(const char* pluginrcFileName ISNULL);
    API_EXPORT const char* stylePath(const char* styleFileName ISNULL);
    API_EXPORT const char *defaultrcPath(void);

    /* As configured in exensions.rc: */
    API_EXPORT void GetBlackboxEditor(/*OUT*/ char* editor);

    /* ------------------------------------ */
    /* File functions */

    /* Check if 'pszPath' exists as a regular file */
    API_EXPORT bool FileExists(const char* pszPath);

    /* Open/Close a file for use with 'FileRead' or 'ReadNextCommand'
       (Do not use 'fopen/fclose' in combination with these) */
    API_EXPORT FILE *FileOpen(const char* pszPath);
    API_EXPORT bool FileClose(FILE *fp);

    /* Read one line from file. Leading and trailing spaces are removed.
       Tabs are converted to spaces. */
    API_EXPORT bool FileRead(FILE *fp, char* buffer);

    /* Similar, additionally skips comments and empty lines */
    API_EXPORT bool ReadNextCommand(FILE *fp, char* buffer, unsigned maxLength);

    /* ------------------------------------ */
    /* Make a plugin sticky on all workspaces */

    API_EXPORT void MakeSticky(HWND hwnd);
    API_EXPORT void RemoveSticky(HWND hwnd);
    API_EXPORT bool CheckSticky(HWND hwnd);

    /* Note: Please make shure to call RemoveSticky before your
       plugin-window is destroyed */

    /* ------------------------------------ */
    /* Window utilities */

    API_EXPORT HMONITOR GetMonitorRect(void *from, RECT *r, int Flags);
    /* Flags: */
    #define GETMON_FROM_WINDOW    1 /* 'from' is HWND */
    #define GETMON_FROM_POINT     2 /* 'from' is POINT* */
    #define GETMON_FROM_MONITOR   4 /* 'from' is HMONITOR */
    #define GETMON_WORKAREA      16 /* get working area rather than full screen */

    API_EXPORT void SnapWindowToEdge(WINDOWPOS* wp, LPARAM nTriggerDist, UINT Flags, ...);
    /* Flags: */
    #define SNAP_FULLSCREEN 1  /* use full screen rather than workarea */
    #define SNAP_NOPLUGINS  2 /* dont snap to other plugins */
    #define SNAP_SIZING     4 /* window is resized (bottom-right sizing only) */

    /* Wrapper for 'SetLayeredWindowAttributes', win9x compatible */
    API_EXPORT bool SetTransparency(HWND hwnd, BYTE alpha);
    /* alpha: 0-255, 255=transparency off */

    /* Get application's name from window, returns lenght of string (0 on failure) */
    API_EXPORT int GetAppByWindow(HWND hwnd, char* pszOut);

    /* Is this window considered as an application */
    API_EXPORT bool IsAppWindow(HWND hwnd);

    /* ------------------------------------ */
    /* Desktop margins: */

    /* Add a screen margin at the indicated location */
    API_EXPORT void SetDesktopMargin(HWND hwnd, int location, int marginWidth);
    /* with hwnd is the plugin's hwnd, location is one of the following: */
    enum SetDesktopMargin_locations
    {
        BB_DM_TOP, BB_DM_BOTTOM, BB_DM_LEFT, BB_DM_RIGHT
    };

    /* Note: Make shure to remove the margin before your plugin-window
       is destroyed: SetDesktopMargin(hwnd_plugin, 0, 0); */

    /* ------------------------------------ */
    /* Info */

    /* Get the main window */
    API_EXPORT HWND GetBBWnd(void);
    /* Version string (as specified in resource.rc, e.g. "bbLean 1.17") */
    API_EXPORT const char* GetBBVersion(void);
    /* True if running on top of explorer shell: */
    API_EXPORT bool GetUnderExplorer(void);
    /* Get some string about the OS */
    API_EXPORT LPCSTR GetOSInfo(void);

    /* ------------------------------------ */
    /* String tokenizing etc. */

    /* Put first token from source into target. Returns: ptr to rest of source */
    API_EXPORT const char* Tokenize(
        const char* source, char* target, const char* delimiters);

    /* Put first 'numTokens' from 'source' into 'targets', copy rest into
       'remaining', if it's not NULL. The 'targets' and 'remaining' buffers
       are zero-terminated always. Returns the number of actual tokens found */
    API_EXPORT int BBTokenize (
        const char* source, char** targets, unsigned numTokens, char* remaining);

    /* Parse a texture specification, e.g. 'raised vertical gradient' */
    API_EXPORT void ParseItem(const char* szItem, StyleItem *item);
    /* Is searchString part of inputString (letter case ignored) */
    API_EXPORT bool IsInString(const char* inputString, const char* searchString);
    /* Remove first and last character from string */
    API_EXPORT char* StrRemoveEncap(char* string);
    /* Replace Environment Variables in string (like %USERNAME%) */
    API_EXPORT void ReplaceEnvVars(char* string);
    /* Replace special folders in string (like APPDATA) */
    API_EXPORT char* ReplaceShellFolders(char* path);

    /* ------------------------------------ */
    /* Shell execute a command */

    API_EXPORT BOOL BBExecute(
        HWND Owner,         /*  normally NULL */
        const char* szVerb,      /*  normally NULL */
        const char* szFile,      /*  required */
        const char* szArgs,      /*  or NULL */
        const char* szDirectory, /*  or NULL */
        int nShowCmd,       /*  normally SW_SHOWNORMAL */
        int noErrorMsgs    /*  if true, suppresses errors */
        );

    /* ------------------------------------ */
    /* Logging and error messages */

    /* printf-like message box, flag is MB_OK etc */
    API_EXPORT int BBMessageBox(int flag, const char *fmt, ...);
    /* Pass formatted message to 'OutputDebugString (for plugin developers)' */
    API_EXPORT void dbg_printf(const char *fmt, ...);
    /* Write to blackbox.log, not implemented in bblean */
    API_EXPORT void Log(const char* Title, const char* Line);
    /* Complain about missing file */
    API_EXPORT int MBoxErrorFile(const char* szFile);
    /* Complain about something */
    API_EXPORT int MBoxErrorValue(const char* szValue);

    /* ------------------------------------ */
    /* Painting */

    /* Generic Gradient Function */
    API_EXPORT void MakeGradient(
        HDC hdc,
        RECT rect,
        int type,
        COLORREF color1,
        COLORREF color2,
        bool interlaced,
        int bevelstyle,
        int bevelposition,
        int bevelWidth,   /* not used, should be 0 */
        COLORREF borderColour,
        int borderWidth
        );

    /* Draw a Gradient Rectangle from StyleItem, optional using the style border. */
    API_EXPORT void MakeStyleGradient(HDC hDC, RECT* p_rect, StyleItem * m_si, bool withBorder);
    /* Draw a Border */
    API_EXPORT void CreateBorder(HDC hdc, RECT* p_rect, COLORREF borderColour, int borderWidth);
    /* Draw a Pixmap for buttons, menu bullets, checkmarks ... */
    API_EXPORT void bbDrawPix(HDC hDC, RECT *p_rect, COLORREF picColor, int style);
    /* Create a font handle from styleitem, with parsing and substitution. */
    API_EXPORT HFONT CreateStyleFont(StyleItem * si);

    /* ------------------------------------ */
    /* UTF-8 support */

    /* Draw a textstring with color, possibly translating from UTF-8 */
    API_EXPORT void bbDrawText(HDC hDC, const char *text, RECT *p_rect, unsigned just, COLORREF c);
    /* convert from Multibyte string to WCHAR */
    API_EXPORT int bbMB2WC(const char *src, WCHAR *wchar_buffer, int wchar_len);
    /* convert from WCHAR to Multibyte string */
    API_EXPORT int bbWC2MB(const WCHAR *src, char *char_buffer, int char_len);

    /* ------------------------------------ */
    /* Plugin Menu API - See the SDK for application examples */

    /* creates a Menu or Submenu, Id must be unique, fshow indicates whether
       the menu should be shown (true) or redrawn (false) */
    API_EXPORT Menu *MakeNamedMenu(const char* HeaderText, const char* Id, bool fshow);

    /* inserts an item to execute a command or to set a boolean value */
    API_EXPORT MenuItem *MakeMenuItem(
        Menu *PluginMenu, const char* Title, const char* Cmd, bool ShowIndicator);

    /* inserts an inactive item, optionally with text. 'Title' may be NULL. */
    API_EXPORT MenuItem *MakeMenuNOP(Menu *PluginMenu, const char* Title ISNULL);

    /* inserts an item to adjust a numeric value */
    API_EXPORT MenuItem *MakeMenuItemInt(
        Menu *PluginMenu, const char* Title, const char* Cmd,
        int val, int minval, int maxval);

    /* inserts an item to edit a string value */
    API_EXPORT MenuItem *MakeMenuItemString(
        Menu *PluginMenu, const char* Title, const char* Cmd,
        const char* init_string ISNULL);

    /* inserts an item, which opens a submenu */
    API_EXPORT MenuItem *MakeSubmenu(
        Menu *ParentMenu, Menu *ChildMenu, const char* Title ISNULL);

    /* inserts an item, which opens a submenu from a system folder.
      'Cmd' optionally may be a Broam which then is sent on user click
      with "%s" in the broam string replaced by the selected filename */
    API_EXPORT MenuItem* MakeMenuItemPath(
        Menu *ParentMenu, const char* Title, const char* path, const char* Cmd ISNULL);

    /* Context menu for filesystem items. One of path or pidl can be NULL */
    API_EXPORT Menu *MakeContextMenu(const char *path, const void *pidl);

    /* shows the menu */
    API_EXPORT void ShowMenu(Menu *PluginMenu);

    /* checks whether a menu with ID starting with 'IDString_start', still exists */
    API_EXPORT bool MenuExists(const char* IDString_start);

    /* set option for MenuItem  */
    API_EXPORT void MenuItemOption(MenuItem *pItem, int option, ...);
    #define BBMENUITEM_DISABLED   1 /* set disabled state */
    #define BBMENUITEM_CHECKED    2 /* set checked state */
    #define BBMENUITEM_LCOMMAND   3 /* next arg is command for left click */
    #define BBMENUITEM_RCOMMAND   4 /* next arg is command for right click */
    #define BBMENUITEM_OFFVAL     5 /* next args are offval, offstring (with Int-Items) */
    #define BBMENUITEM_UPDCHECK   6 /* update checkmarks on the fly */
    #define BBMENUITEM_JUSTIFY    7 /* next arg is DT_LEFT etc... */
    #define BBMENUITEM_SETICON    8 /* next arg is "path\to\icon[,iconindex]" */
    #define BBMENUITEM_SETHICON   9 /* next arg is HICON */
    #define BBMENUITEM_RMENU     10 /* next arg is Menu* for right-click menu */

    API_EXPORT void MenuOption(Menu *pMenu, int flags, ...);
    #define BBMENU_XY             0x0001 /* next arg is x/y position */
    #define BBMENU_RECT           0x0002 /* next arg is *pRect to show above/below */
    #define BBMENU_CENTER         0x0003 /* center menu on screen */
    #define BBMENU_CORNER         0x0004 /* align with corner on mouse */
    #define BBMENU_POSMASK        0x0007 /* bit mask for above positions */
    #define BBMENU_KBD            0x0008 /* use position from blackbox.rc */
    #define BBMENU_XRIGHT         0x0010 /* x is menu's right */
    #define BBMENU_YBOTTOM        0x0020 /* y is menu's bottom */
    #define BBMENU_PINNED         0x0040 /* show menu initially pinned */
    #define BBMENU_ONTOP          0x0080 /* show menu initially on top */
    #define BBMENU_NOFOCUS        0x0100 /* dont set focus on menu */
    #define BBMENU_NOTITLE        0x0200 /* no title */
    #define BBMENU_MAXWIDTH       0x0400 /* next arg is maximal menu width */
    #define BBMENU_SORT           0x0800 /* sort menu alphabetically */
    #define BBMENU_ISDROPTARGET   0x1000 /* register as droptarget */
    #define BBMENU_HWND           0x2000 /* next arg is HWND to send notification on menu-close */
    #define BBMENU_SYSMENU        0x4000 /* is a system menu (for bbLeanSkin/Bar) */

    /* ------------------------------------ */
    /* obsolete: */
    API_EXPORT Menu *MakeMenu(const char* HeaderText);
    API_EXPORT void DelMenu(Menu *PluginMenu); /* does nothing */

    /* ------------------------------------ */
    /* Tray icon access */

    /* A plugin can register BB_TRAYUPDATE to receive notification about
       changes with tray icons */

    typedef struct systemTrayBalloon
    {
        UINT    uInfoTimeout;
        DWORD   dwInfoFlags;
        char    szInfoTitle[64];
        char    szInfo[256];
    } systemTrayBalloon;

    typedef struct systemTray
    {
        HWND    hWnd;
        UINT    uID;
        UINT    uCallbackMessage;
        HICON   hIcon;
        char    szTip[256];
        systemTrayBalloon balloon;
    } systemTray;

    API_EXPORT int GetTraySize(void);
    API_EXPORT systemTray* GetTrayIcon(int icon_index);

    typedef struct systemTrayIconPos
    {
        HWND hwnd; /* the plugin's hwnd */
        RECT r; /* icon rectangle on plugin's hwnd */
    } systemTrayIconPos;

    API_EXPORT int ForwardTrayMessage(int icon_index, UINT message, systemTrayIconPos *pos);

    /* ------------------------------------ */
    /* Task items access */

    /* A plugin can register BB_TASKSUPDATE to receive notification about
       changes with tasks */

    /* get the size */
    API_EXPORT int GetTaskListSize(void);
    /* get a task's HWND by index [0..GetTaskListSize()] */
    API_EXPORT HWND GetTask(int index);
    /* get the index of the currently active task */
    API_EXPORT int GetActiveTask(void);
    /* Get the workspace number for a task */
    API_EXPORT int GetTaskWorkspace(HWND hwnd);
    /* For backwards compatibility only. (because of unclear implementation) */
    API_EXPORT void SetTaskWorkspace(HWND hwnd, int workspace);

    typedef struct tasklist /* bb4win 0.90 compatible layout */
    {
        struct tasklist* next;
        HWND    hwnd;
        HICON   icon;
        int     wkspc;
        char    caption[248];
        bool    active;
        bool    flashing;
        /* below: obsolete */
        bool    _former_hidden;
        bool    _former_reserved;
        HICON   _former_icon_big;
    } tasklist;

    /* Direct access: get the internal TaskList. */
    API_EXPORT const struct tasklist *GetTaskListPtr(void);

    /* ------------------------------------ */
    /* Workspace (aka Desktop) Information */

    /* A plugin can register BB_DESKTOPINFO to receive notification about
       workspace changes */

    typedef struct DesktopInfo /* do not change this structure */
    {
        char name[32];  /* name of the desktop */
        bool isCurrent; /* if it's the current desktop */
        int number;     /* desktop number */
        int ScreensX;   /* total number of screens */
        struct string_node *deskNames; /* list of all names */
    } DesktopInfo;

    /* Get current Desktop information: */
    API_EXPORT void GetDesktopInfo(DesktopInfo *deskInfo);

    /* ------------------------------------ */
    /* often used structure */

    typedef struct string_node
    {
        struct string_node *next;
        char str[1];
    } string_node;

    #define STRING_NODE_DEFINED

    /* ------------------------------------ */
    /* windows on workspace placement interface */

    typedef struct taskinfo
    {
        int xpos, ypos;     /* position */
        int width, height;  /* size, ignored with 'SetTaskLocation' */
        int desk;           /* workspace */
    } taskinfo;

    /* get workspace and original position/size for window */
    API_EXPORT bool GetTaskLocation(HWND hwnd, struct taskinfo *pti);

    /* set workspace and/or position for window */
    API_EXPORT bool SetTaskLocation(HWND hwnd, struct taskinfo *pti, UINT flags);
    /* where flags are any combination of: */
    #define BBTI_SETDESK    1 /* move window to desk as specified */
    #define BBTI_SETPOS     2 /* move window to x/ypos as specified */
    #define BBTI_SWITCHTO   4 /* switch workspace after move */

    /* ------------------------------------ */
    /*  Toolbar Info (for systembars that want to 'link to the toolbar') */

    typedef struct ToolbarInfo
    {
        /* rc - settings */
        int     placement;      /* 0..5, see Toolbar placements below */
        int     widthPercent;   /* 0..100 */
        bool    onTop;
        bool    autoHide;
        bool    pluginToggle;
        bool    disabled;       /* completely */

        bool    transparency;
        BYTE    alphaValue;     /* 0..255 */
        /* current status */
        bool    autohidden;     /* currently by autoHide */
        bool    hidden;         /* currently by pluginToggle */
        int     xpos;
        int     ypos;
        int     width;
        int     height;
        HWND    hwnd;
        /* bbsb info, these can be set by a systembar */

        HWND    bbsb_hwnd;
        bool    bbsb_linkedToToolbar;
        bool    bbsb_reverseTasks;
        bool    bbsb_currentOnly;
     } ToolbarInfo;

    /* get a pointer to core's static ToolbarInfo */
    API_EXPORT ToolbarInfo *GetToolbarInfo(void);

    /* Toolbar placements */
    #define PLACEMENT_TOP_LEFT 0
    #define PLACEMENT_TOP_CENTER 1
    #define PLACEMENT_TOP_RIGHT 2
    #define PLACEMENT_BOTTOM_LEFT 3
    #define PLACEMENT_BOTTOM_CENTER 4
    #define PLACEMENT_BOTTOM_RIGHT 5

    /* ------------------------------------ */
    /* plugin interface declarations */

#ifndef __BBCORE__
    DLL_EXPORT int beginPlugin(HINSTANCE);
    DLL_EXPORT int beginSlitPlugin(HINSTANCE, HWND hSlit); /* obsolete */
    DLL_EXPORT int beginPluginEx(HINSTANCE, HWND hSlit);
    DLL_EXPORT void endPlugin(HINSTANCE);
    DLL_EXPORT const char* pluginInfo(int index);
#endif

    /* return values for beginPlugin functions */
    #define BEGINPLUGIN_OK 0
    #define BEGINPLUGIN_FAILED 1
    #define BEGINPLUGIN_FAILED_QUIET 2

    /* possible index values for pluginInfo */
    #define PLUGIN_NAME         1
    #define PLUGIN_VERSION      2
    #define PLUGIN_AUTHOR       3
    #define PLUGIN_RELEASE      4
    #define PLUGIN_RELEASEDATE  4   /* xoblite */
    #define PLUGIN_LINK         5
    #define PLUGIN_EMAIL        6
    #define PLUGIN_BROAMS       7   /* xoblite */
    #define PLUGIN_UPDATE_URL   8   /* Kaloth's BBPlugManager */

#ifdef __cplusplus
}
#endif

/* =========================================================================== */
#endif /* _BBAPI_H_ */
