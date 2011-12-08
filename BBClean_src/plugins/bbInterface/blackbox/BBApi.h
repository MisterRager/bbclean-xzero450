/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#ifndef __BBAPI_H_
#define __BBAPI_H_

/*------------------------------------------ */
/* compiler specifics */
/*------------------------------------------ */

#ifdef __GNUC__
  #define _WIN32_IE 0x0500
  #ifndef __BBCORE__
    #define GetBlackboxPath _GetBlackboxPath
  #endif
#endif

#ifdef __BORLANDC__
  #define DLL_EXPORT /* .def file required to work around underscores */
  #define _RWSTD_NO_EXCEPTIONS 1
#endif

#ifndef DLL_EXPORT
  #define DLL_EXPORT __declspec(dllexport)
#endif

/*------------------------------------------ */
/* windows include */
/*------------------------------------------ */

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0500
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

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
/* Better dont use, core doesn't anyway */

#define MAX_LINE_LENGTH 1024

/*------------------------------------------ */
/* BImage definitions */

/* Gradient types + solid */
#define B_HORIZONTAL 0
#define B_VERTICAL   1
#define B_DIAGONAL   2
#define B_CROSSDIAGONAL 3
#define B_PIPECROSS  4
#define B_ELLIPTIC   5
#define B_RECTANGLE  6
#define B_PYRAMID    7
#define B_SOLID      8

/* Bevels */
#define BEVEL_FLAT   0
#define BEVEL_RAISED 1
#define BEVEL_SUNKEN 2
#define BEVEL1 1
#define BEVEL2 2

/*=========================================================================== */
/* Blackbox messages */

#define BB_REGISTERMESSAGE      10001
#define BB_UNREGISTERMESSAGE    10002

/* ----------------------------------- */
#define BB_QUIT                 10101
#define BB_RESTART              10102
#define BB_RECONFIGURE          10103
#define BB_SETSTYLE             10104 /* lParam: const char* stylefile */
#define BB_EXITTYPE             10105 /* lParam: 0=Shutdown/Reboot/Logoff 1=Quit 2=Restart */
  #define B_SHUTDOWN 0
  #define B_QUIT 1
  #define B_RESTART 2
#define BB_TOOLBARUPDATE        10106
#define BB_SETTHEME             10107 /* xoblite */

/* ----------------------------------- */
#define BB_EDITFILE             10201
  /* wParam: 0=CurrentStyle, 1=menu.rc, 2=plugins.rc 3=extensions.rc 4=blackbox.rc
    -1=filename in (LPCSTR)lParam */

#define BB_EXECUTE              10202 /* Send a command or broam for execution (in lParam) */
#define BB_ABOUTSTYLE           10203
#define BB_ABOUTPLUGINS         10204

/* ----------------------------------- */
#define BB_MENU                 10301
  /* wParam: 0=Main menu, 1=Workspaces menu, 2=Toolbar menu */
  #define BB_MENU_ROOT        0
  #define BB_MENU_TASKS       1
  #define BB_MENU_TOOLBAR     2
  #define BB_MENU_ICONS       3
  #define BB_MENU_PLUGIN      4
  #define BB_MENU_CONTEXT     5
  #define BB_MENU_BYBROAM     6
  #define BB_MENU_BYBROAM_KBD 7

#define BB_HIDEMENU             10302
#define BB_TOGGLETRAY           10303   /* toggls system bar's etc. */
#define BB_TOGGLESYSTEMBAR      10303   /* xoblite */
#define BB_SETTOOLBARLABEL      10304   /* Set the toolbar label to (LPCSTR)lParam (returns to normal after 2 seconds) */
#define BB_TOGGLEPLUGINS        10305
#define BB_SUBMENU              10306
#define BB_TOGGLESLIT           10307   /* xoblite */
#define BB_TOGGLETOOLBAR        10308   /* xoblite */

/* ----------------------------------- */
#define BB_SHUTDOWN             10401   /* wParam: 0=Shutdown, 1=Reboot, 2=Logoff, 3=Hibernate, 4=Suspend, 5=LockWorkstation */
#define BB_RUN                  10402

/* ----------------------------------- */
#define BB_DESKTOPINFO          10501
#define BB_LISTDESKTOPS         10502
#define BB_SWITCHTON            10503
#define BB_BRINGTOFRONT         10504   /* lParam: Window to activate */
  /* wParam flag: Zoom window into current workspace and activate: */
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

  #define BBWS_TOGGLESTICKY     12 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_EDITNAME         13 
  #define BBWS_MAKESTICKY       14 /* lParam: hwnd or NULL for foregroundwindow */
  #define BBWS_CLEARSTICKY      15 /* lParam: hwnd or NULL for foregroundwindow */

  #define BBWS_MINIMIZEALL      20
  #define BBWS_RESTOREALL       21
  #define BBWS_TILEVERTICAL     22
  #define BBWS_TILEHORIZONTAL   23
  #define BBWS_CASCADE          24

/*------------------------------------------ */
#define BB_TASKSUPDATE          10506
  /* lParam for BB_TASKSUPDATE: */
  #define TASKITEM_ADDED 0        /*  wParam: hwnd */
  #define TASKITEM_MODIFIED 1     /*  wParam: hwnd */
  #define TASKITEM_ACTIVATED 2    /*  wParam: hwnd */
  #define TASKITEM_REMOVED 3      /*  wParam: hwnd */
  #define TASKITEM_REFRESH 4      /*  wParam: NULL, sent on workspace change of one or more tasks */
  #define TASKITEM_FLASHED 5      /*  wParam: hwnd */

#define BB_TRAYUPDATE           10507
  /* lParam for BB_TRAYUPDATE: */
  #define TRAYICON_ADDED 0
  #define TRAYICON_MODIFIED 1
  #define TRAYICON_REMOVED 2

#define BB_CLEANTRAY            10508
#define BB_DRAGTODESKTOP        10510 /* wParam 0:execute drop  1:test if accepted */

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

/* ----------------------------------- */
/* Broadcast a string (Bro@m) to core and all plugins */
#define BB_BROADCAST            10901 /* lParam: command string */

/* ----------------------------------- */
/* BBSlit messages */
#define SLIT_ADD                11001
#define SLIT_REMOVE             11002
#define SLIT_UPDATE             11003

/* ----------------------------------- */
/* Indices for pluginInfo(int index); */
#define PLUGIN_NAME         1
#define PLUGIN_VERSION      2
#define PLUGIN_AUTHOR       3
#define PLUGIN_RELEASE      4
#define PLUGIN_RELEASEDATE  4   /* xoblite */
#define PLUGIN_LINK         5
#define PLUGIN_EMAIL        6
#define PLUGIN_BROAMS       7   /* xoblite */
#define PLUGIN_UPDATE_URL   8   /* for Kaloth's BBPlugManager */

/*=========================================================================== */
/* BBLean extensions */

/* for bbStylemaker & bbLeanSkin */
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

#define BB_EXECUTEASYNC         10882   /* Post a command or broam for execution (in lParam), like BB_EXECUTE, but returns immediately */
#define BB_DESKCLICK            10884   /* desktop clicked (lParam: 0=leftdown 1=left, 2=right, 3=mid, 4=x1, 5=x2, 6=x3) */
#define BB_WINKEY               10886   /* win9x: for bbKeys, left/right winkey pressed */

#define BB_GETSTYLE_OLD         10887   /* previous implementation, still here for bbstylemaker 1.2 */
#define BB_GETSTYLESTRUCT_OLD   10888   /* also */
#define BB_SETSTYLESTRUCT_OLD   10889   /* also */

#define BB_SENDDATA             10890
#define BB_GETSTYLE             10891   /* SendMessage(GetBBWnd(), BB_GETSTYLE, (WPARAM)buffer, (LPARAM)my_hwnd); */
#define BB_GETSTYLESTRUCT       10892   /* SendMessage(GetBBWnd(), BB_GETSTYLESTRUCT, (WPARAM)buffer, (LPARAM)my_hwnd); */
#define BB_SETSTYLESTRUCT       10893   /* etc. */

/* ----------------------------------- */
/* internal usage */

#ifdef __BBCORE__
#define BB_FOLDERCHANGED        10897   /* folder changed */
#define BB_DRAGOVER             10898   /* dragging over menu */
#define BB_POSTSTRING           10899   /* async command, for BB_EXECUTEASYNC */
#endif

/* ----------------------------------- */
/* done with BB_ messages */

#define BB_MSGFIRST             10000
#define BB_MSGLAST              13000

/*=========================================================================== */
/* extended Style-info for convenience and other purposes (backwards compatible) */

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

    /* below: experimental */
    int nVersion;
    int marginWidth;
    int borderWidth;
    COLORREF borderColor;
    COLORREF foregroundColor;
    COLORREF disabledColor;
    bool bordered;
    bool FontShadow; /* xoblite */

    char reserved[128 - (6*4 + 2*1)]; /* keep structure size */

    COLORREF ShadowColor;

} StyleItem;

#define picColor TextColor

/* StyleItem.validated flags */
#define VALID_TEXTURE       (1<<0)  /* gradient & bevel definition */

#define VALID_COLORFROM     (1<<1)  /* Color */
#define VALID_COLORTO       (1<<2)  /* ColorTo */
#define VALID_TEXTCOLOR     (1<<3)  /* TextColor */
#define VALID_PICCOLOR      (1<<4)  /* picColor */

#define VALID_FONT          (1<<5)  /* Font */
#define VALID_FONTHEIGHT    (1<<6)  /* FontHeight */
#define VALID_FONTWEIGHT    (1<<7)  /* FontWeight */
#define VALID_JUSTIFY       (1<<8)  /* Justify */

#define VALID_MARGIN        (1<<9)
#define VALID_BORDER        (1<<10)
#define VALID_BORDERCOLOR   (1<<11)

#define VALID_DISABLEDCOLOR (1<<12)

#define VALID_SHADOWCOLOR   (1<<13)

typedef class Menu Menu;
typedef class MenuItem MenuItem;

/*=========================================================================== */
/* constants for GetSettingPtr(int index) -> returns: */
enum {

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

    SN_NEWMETRICS               , /* bool (not a ptr) */

    SN_MENUSEPARATOR            , /* StyleItem* */
    SN_MENUVOLUME               , /* StyleItem* */

    SN_LAST
};

/*=========================================================================== */
/* Exported functions */

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
    API_EXPORT bool ReadBool(LPCSTR fileName, LPCSTR szKey, bool defaultBool);
    API_EXPORT int ReadInt(LPCSTR fileName, LPCSTR szKey, int defaultInt);
    API_EXPORT LPCSTR ReadString(LPCSTR fileName, LPCSTR szKey, LPCSTR defaultString);
    API_EXPORT COLORREF ReadColor(LPCSTR fileName, LPCSTR szKey, LPCSTR defaultString);

    /* Read a rc-value as string. If 'ptr' is specified, it can read a sequence of items with the same name. */
    API_EXPORT LPCSTR ReadValue(LPCSTR fileName, LPCSTR szKey, LPLONG ptr ISNULL);

    /* Was the last Value actually read from the rc-file ? */
    API_EXPORT int FoundLastValue(void);
    /* Returns: 0=not found, 1=found exact value, 2=found matching wildcard*/

    /* Write Settings */
    API_EXPORT void WriteBool(LPCSTR fileName, LPCSTR szKey, bool value);
    API_EXPORT void WriteInt(LPCSTR fileName, LPCSTR szKey, int value);
    API_EXPORT void WriteString(LPCSTR fileName, LPCSTR szKey, LPCSTR value);
    API_EXPORT void WriteColor(LPCSTR fileName, LPCSTR szKey, COLORREF value);

    /* Delete/Rename Setting */
    API_EXPORT bool DeleteSetting(LPCSTR fileName, LPCSTR szKey); /* wildcards supported for keyword */
    API_EXPORT bool RenameSetting(LPCSTR fileName, LPCSTR old_keyword, LPCSTR new_keyword);

    /* ------------------------------------ */
    /* Direct access to Settings variables / styleitems / colors */
    /* See the "SN_XXX" constants for possible values */
    API_EXPORT void* GetSettingPtr(int index);

    /* ------------------------------------ */
    /* File functions */
    API_EXPORT bool FileExists(LPCSTR fileName);
    API_EXPORT FILE *FileOpen(LPCSTR fileName);
    API_EXPORT bool FileClose(FILE *filePointer);
    API_EXPORT bool FileRead(FILE *filePointer, LPSTR buffer);
    API_EXPORT bool ReadNextCommand(FILE *filePointer, LPSTR buffer, DWORD maxLength);

    API_EXPORT bool FindConfigFile(LPSTR pszOut, LPCSTR fileName, LPCSTR pluginDir);
    API_EXPORT LPCSTR ConfigFileExists(LPCSTR filename, LPCSTR pluginDir);

    /* ------------------------------------ */
    /* Make a window visible on all workspaces */
    API_EXPORT void MakeSticky(HWND window);
    API_EXPORT void RemoveSticky(HWND window);
    API_EXPORT bool CheckSticky(HWND window);

    /* ------------------------------------ */
    /* Window utilities */
    API_EXPORT int GetAppByWindow(HWND Window, char*);
    API_EXPORT bool IsAppWindow(HWND hwnd);

    API_EXPORT HMONITOR GetMonitorRect(void *from, RECT *r, int Flags);
    /* Flags: */
    #define GETMON_FROM_WINDOW 1    /* 'from' is HWND */
    #define GETMON_FROM_POINT 2     /* 'from' is POINT* */
    #define GETMON_FROM_MONITOR 4   /* 'from' is HMONITOR */
    #define GETMON_WORKAREA 16      /* get working area rather than full screen */

    API_EXPORT void SnapWindowToEdge(WINDOWPOS* windowPosition, LPARAM nDist_or_pContent, UINT Flags);
    /* Flags: */
    #define SNAP_FULLSCREEN 1  /* use full screen rather than workarea */
    #define SNAP_NOPLUGINS  2  /* dont snap to other plugins */
    #define SNAP_CONTENT    4  /* the "nDist_or_pContent" parameter points to a SIZE struct */
    #define SNAP_NOPARENT   8  /* dont snap to parent window edges */
    #define SNAP_SIZING    16  /* window is resized (bottom-right sizing only) */

    API_EXPORT bool SetTransparency(HWND hwnd, BYTE alpha);
    /* alpha: 0-255, 255=transparency off */

    /* ------------------------------------ */
    /* Desktop margins: */

    /* Add a screen margin at the indicated location */
    API_EXPORT void SetDesktopMargin(HWND hwnd, int location, int margin);

    /* with hwnd is the plugin's hwnd, location is one of the following: */
    enum { BB_DM_TOP, BB_DM_BOTTOM, BB_DM_LEFT, BB_DM_RIGHT };

    /* Dont forget to remove a margin before the plugin-window is destroyed. */
    /* Use SetDesktopMargin(hwnd, 0, 0) to remove it. */

    /* ------------------------------------ */
    /* Get paths */
    API_EXPORT void GetBlackboxEditor(LPSTR editor);
    API_EXPORT LPSTR WINAPI GetBlackboxPath(LPSTR path, int maxLength); /* with trailing backslash */

    API_EXPORT LPCSTR bbrcPath(LPCSTR bbrcFileName ISNULL);
    API_EXPORT LPCSTR extensionsrcPath(LPCSTR extensionsrcFileName ISNULL);
    API_EXPORT LPCSTR menuPath(LPCSTR menurcFileName ISNULL);
    API_EXPORT LPCSTR plugrcPath(LPCSTR pluginrcFileName ISNULL);
    API_EXPORT LPCSTR stylePath(LPCSTR styleFileName ISNULL);

    /* ------------------------------------ */
    /* Get the main window and other info */
    API_EXPORT HWND GetBBWnd();
    API_EXPORT LPCSTR GetBBVersion(); /* for instance: "bbLean 1.13" */
    API_EXPORT LPCSTR GetOSInfo();
    API_EXPORT bool GetUnderExplorer();

    /* ------------------------------------ */
    /* String tokenizing */
    API_EXPORT LPSTR Tokenize(LPCSTR sourceString, LPSTR targetString, LPCSTR delimiter);
    API_EXPORT int BBTokenize (LPCSTR sourceString, LPSTR* targetStrings, DWORD numTokensToParse, LPSTR remainingString);

    /* ------------------------------------ */
    /* Shell execute a command */
    API_EXPORT BOOL BBExecute(HWND Owner, LPCSTR szOperation, LPCSTR szCommand, LPCSTR szArgs, LPCSTR szDirectory, int nShowCmd, bool noErrorMsgs);

    /* ------------------------------------ */
    /* Logging and error messages */
    /* API_EXPORT void Log(LPCSTR format, ...); */
    API_EXPORT void Log(LPCSTR Title, LPCSTR Line);
    API_EXPORT int MBoxErrorFile(LPCSTR szFile);
    API_EXPORT int MBoxErrorValue(LPCSTR szValue);
    API_EXPORT int BBMessageBox(int flg, const char *fmt, ...);

    /* debugging */
    API_EXPORT void dbg_printf(const char *fmt, ...);

    /* ------------------------------------ */
    /* Helpers */
    API_EXPORT void ParseItem(LPCSTR szItem, StyleItem *item);
    API_EXPORT bool IsInString(LPCSTR inputString, LPCSTR searchString);
    API_EXPORT LPSTR StrRemoveEncap(LPSTR string);
    API_EXPORT void ReplaceEnvVars(LPSTR string);
    API_EXPORT LPSTR ReplaceShellFolders(LPSTR path);

    /* ------------------------------------ */
    /* Painting */

    /* Generic Gradient Function */
    API_EXPORT void MakeGradient(HDC hdc, RECT rect, int gradientType, COLORREF colourFrom, COLORREF colourTo, bool interlaced, int bevelStyle, int bevelPosition, int bevelWidth, COLORREF borderColour, int borderWidth);
    /* Draw a Gradient Rectangle from StyleItem, optional using the style border. */
    API_EXPORT void MakeStyleGradient(HDC hDC, RECT* p_rect, StyleItem * m_si, bool withBorder);
    /* Draw a Border */
    API_EXPORT void CreateBorder(HDC hdc, RECT* p_rect, COLORREF borderColour, int borderWidth);
    /* Create a font handle from styleitem, with parsing and substitution. */
    API_EXPORT HFONT CreateStyleFont(StyleItem * si);

    /* ------------------------------------ */

    /* ------------------------------------ */
    /* Plugin Menu API - See the SDK for application examples */

    /* creates a Menu or Submenu, with an unique id, fshow indicates whether the menu is to be shown (true) or redrawn (false) */
    API_EXPORT Menu *MakeNamedMenu(LPCSTR HeaderText, LPCSTR Id, bool fshow);

    /* inserts an item to execute a command or to set a boolean value */
    API_EXPORT MenuItem *MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator);

    /* inserts an inactive item, optionally with text. 'Title' may be NULL. */
    API_EXPORT MenuItem *MakeMenuNOP(Menu *PluginMenu, LPCSTR Title ISNULL);

    /* inserts an item to adjust a numeric value */
    API_EXPORT MenuItem *MakeMenuItemInt(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, int val, int minval, int maxval);

    /* inserts an item to edit a string value */
    API_EXPORT MenuItem *MakeMenuItemString(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, LPCSTR init_string ISNULL);

    /* inserts an item, which opens a submenu */
    API_EXPORT MenuItem *MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCSTR Title);

    /* inserts an item, which opens a submenu from a system folder.
      'Cmd' optionally may be a Broam which is then sent on user click
      with "%s" in the broam string replaced by the selected filename */
    API_EXPORT MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd ISNULL);

    /* set disabled state (menu.frame.disabledColor) for last added MenuItem */
    API_EXPORT void DisableLastItem(Menu *PluginMenu);

    /* shows the menu */
    API_EXPORT void ShowMenu(Menu *PluginMenu);

    /* checks whether a menu is still on screen */
    API_EXPORT bool MenuExists(LPCSTR IDString_start);

    /* ------------------------------------ */
    /* obsolete with MakeNamedMenu: */
    API_EXPORT Menu *MakeMenu(LPCSTR HeaderText);
    API_EXPORT void DelMenu(Menu *PluginMenu); /* does nothing */

    /* ------------------------------------ */
    /* Draw Text With Shadow */
    API_EXPORT void DrawTextWithShadow(HDC hdc, const char* text, int textLength, RECT* r, unsigned int format, COLORREF textColor, COLORREF shadowColor, bool shadow);
    /* ------------------------------------ */
    /* Tray icon access */

    typedef struct systemTray
    {
        HWND    hWnd;
        UINT    uID;
        UINT    uCallbackMessage;
        HICON   hIcon;
        char    szTip[256 - 4];
        struct systemTrayBalloon *pBalloon; /* NULL when not present */
    } systemTray;

    typedef struct systemTrayBalloon
    {
        UINT    uInfoTimeout;
        DWORD   dwInfoFlags;
        char    szInfoTitle[64];
        char    szInfo[256];
    } systemTrayBalloon;

    API_EXPORT int GetTraySize(void);
    API_EXPORT systemTray* GetTrayIcon(int pointer);

    /* experimental: */
    typedef BOOL (*TRAYENUMPROC)(struct systemTray *, LPARAM);
    API_EXPORT void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam);

    /* ------------------------------------ */

    /* ------------------------------------ */
    /* Task items access */

    typedef struct tasklist /* current 0.90 compatibility outlay */
    {
        struct tasklist* next;
        HWND    hwnd;
        HICON   icon;
        int     wkspc;
        char    caption[244];
        HICON   orig_icon;
        bool    active;
        bool    flashing;
        bool    _former_hidden;
        bool    _former_reserved;
        HICON   _former_icon_big;
    } tasklist;

    /* get the size */
    API_EXPORT int GetTaskListSize(void);
    /* get task's HWND by index */
    API_EXPORT HWND GetTask(int index);
    /* get the index of the currently active task */
    API_EXPORT int GetActiveTask(void);
    /* Retrieve a pointer to the internal TaskList. */
    API_EXPORT struct tasklist *GetTaskListPtr(void);

    typedef struct taskinfo
    {
        int xpos, ypos;     /* position */
        int width, height;  /* size, ignored with 'SetTaskLocation' */
        int desk;           /* workspace */
    } taskinfo;

    /* get workspace and original position/size for window */
    API_EXPORT bool GetTaskLocation(HWND, struct taskinfo *pti);

    /* set workspace and/or position for window */
    API_EXPORT bool SetTaskLocation(HWND, struct taskinfo *pti, UINT flags);
    /* where flags are: */
    #define BBTI_SETDESK    1 /* move window to desk as specified */
    #define BBTI_SETPOS     2 /* move window to x/ypos as spec'd */
    #define BBTI_SWITCHTO   4 /* switch workspace after move */

    /* experimental: */
    typedef BOOL (*TASKENUMPROC)(struct tasklist *, LPARAM);
    API_EXPORT void EnumTasks (TASKENUMPROC lpEnumFunc, LPARAM lParam);

    /* ------------------------------------ */
    /* Get the workspace number for a task */
    API_EXPORT  int GetTaskWorkspace(HWND hwnd);

    /* For backwards compatibility only, depreciated! */
    API_EXPORT void SetTaskWorkspace(HWND hwnd, int workspace);

    /* ------------------------------------ */

    /* ------------------------------------ */
    /* Desktop Information: */
    typedef struct string_node { struct string_node *next; char str[1]; } string_node;

    typedef struct DesktopInfo
    {
        char name[32];  /* name of the desktop */
        bool isCurrent; /* if it's the current desktop */
        int number;     /* desktop number */
        int ScreensX;   /* total number of screens */
        string_node *deskNames; /* list of all names */
    } DesktopInfo;

    /* Get the current Workspace number and name */
    API_EXPORT void GetDesktopInfo(DesktopInfo *deskInfo);
    /* Also, BB sends a BB_DESKTOPINFO message on workspace changes */
    /* with lParam pointing to a DesktopInfo structure */

    /* experimental: */
    typedef BOOL (*DESKENUMPROC)(struct DesktopInfo *, LPARAM);
    API_EXPORT void EnumDesks (DESKENUMPROC lpEnumFunc, LPARAM lParam);

    /* ------------------------------------ */

    /* ------------------------------------ */
    /* Get position and other info about the toolbar */

    typedef struct ToolbarInfo
    {
        /* rc - settings */
        int     placement;      /* 0 - 5 (top-left/center/right, bottom-left/center/right) */
        int     widthPercent;   /* 0 - 100 */
        bool    onTop;
        bool    autoHide;
        bool    pluginToggle;
        bool    disabled;       /* completely */

        bool    transparency;
        BYTE    alphaValue;     /* 0 - 255 (255 = transparency off) */

        /* current status */
        bool    autohidden;     /* currently by autoHide */
        bool    hidden;         /* currently by pluginToggle */
        int     xpos;
        int     ypos;
        int     width;
        int     height;
        HWND    hwnd;

        /* bbsb info, these are set by a systembar if present */
        HWND    bbsb_hwnd;
        bool    bbsb_linkedToToolbar;
        bool    bbsb_reverseTasks;
        bool    bbsb_currentOnly;
     } ToolbarInfo;

    /* retrieve a pointer to core's static ToolbarInfo */
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
    DLL_EXPORT int beginSlitPlugin(HINSTANCE, HWND hSlit);
    DLL_EXPORT int beginPluginEx(HINSTANCE, HWND hSlit);
    DLL_EXPORT void endPlugin(HINSTANCE);
    DLL_EXPORT LPCSTR pluginInfo(int index);
#endif

#ifdef __cplusplus
}
#endif

/*=========================================================================== */
#endif /* __BBAPI_H_ */
