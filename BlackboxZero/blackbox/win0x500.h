/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

/* compiler and version dependent additional defines for windows */

#ifndef _WIN0X500_H_
#define _WIN0X500_H_

/* Layered Windows */
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED 0x00080000
#endif
#ifndef LWA_COLORKEY
#define LWA_COLORKEY 1
#endif
#ifndef LWA_ALPHA
#define LWA_ALPHA 2
#endif
#ifndef ULW_ALPHA
#define ULW_COLORKEY 1
#define ULW_ALPHA 2
#define ULW_OPAQUE 4
#endif
#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW   0x00020000
#endif


/* Mouse */
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#define WM_XBUTTONUP 0x020C
#endif
#ifndef XBUTTON1
#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#endif
#ifndef XBUTTON3
#define XBUTTON3 0x0004
#endif

/* MultiMon */
#ifndef SM_XVIRTUALSCREEN
#define SM_XVIRTUALSCREEN 76
#define SM_YVIRTUALSCREEN 77
#define SM_CXVIRTUALSCREEN 78
#define SM_CYVIRTUALSCREEN 79
#define SM_CMONITORS 80
#define SM_SAMEDISPLAYFORMAT 81
#ifndef HMONITOR_DECLARED
typedef HANDLE HMONITOR;
#define HMONITOR_DECLARED
typedef struct tagMONITORINFO {
    DWORD cbSize;
    RECT rcMonitor;
    RECT rcWork;
    DWORD dwFlags;
} MONITORINFO,*LPMONITORINFO;
typedef BOOL(CALLBACK* MONITORENUMPROC)(HMONITOR,HDC,LPRECT,LPARAM);
#endif
#endif

#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONULL 0 
#define MONITOR_DEFAULTTOPRIMARY 1 
#define MONITOR_DEFAULTTONEAREST 2 
#define MONITORINFOF_PRIMARY 1 
#endif

/* SystemParametersInfo */
#ifndef SPI_SETACTIVEWNDTRKTIMEOUT
#define SPI_SETACTIVEWNDTRKTIMEOUT 0x2003
#endif
#ifndef SPI_SETFOREGROUNDFLASHCOUNT
#define SPI_SETFOREGROUNDFLASHCOUNT 0x2005
#endif
#ifndef SPI_SETACTIVEWNDTRKZORDER
#define SPI_SETACTIVEWNDTRKZORDER 0x100D
#endif
#ifndef SPI_SETACTIVEWINDOWTRACKING
#define SPI_SETACTIVEWINDOWTRACKING 0x1001
#endif

/* TrayIcon Messages */
#ifndef NIM_SETFOCUS
#define NIM_SETFOCUS 0x00000003
#endif
#ifndef NIM_SETVERSION
#define NIM_SETVERSION 0x00000004
#define NOTIFYICON_VERSION 3
#endif
#ifndef NIF_STATE
#define NIF_STATE 0x00000008
#define NIS_HIDDEN 0x00000001
#define NIS_SHAREDICON 0x00000002
#endif
#ifndef NIF_INFO
#define NIF_INFO 0x00000010
#endif
#ifndef NIF_GUID
#define NIF_GUID 0x00000020
#endif
#ifndef NIF_SHOWTIP
#define NIF_SHOWTIP 0x00000080
#endif

#ifndef NIIF_ERROR
#define NIIF_NONE   0x00000000
#define NIIF_INFO   0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR  0x00000003
#define NIIF_LARGE_ICON 0x00000020
#endif

#ifndef NIN_SELECT
#define NIN_SELECT (WM_USER + 0)
#define NINF_KEY 0x1
#define NIN_KEYSELECT (NIN_SELECT | NINF_KEY)
#endif

#ifndef NIN_BALLOONSHOW
#define NIN_BALLOONSHOW (WM_USER + 2)
#define NIN_BALLOONHIDE (WM_USER + 3)
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
#endif

#ifndef NIN_POPUPOPEN
#define NIN_POPUPOPEN (WM_USER+6)
#endif

#ifndef NIN_POPUPCLOSE
#define NIN_POPUPCLOSE (WM_USER+7)
#endif

#ifndef HSHELL_WINDOWREPLACED
#define HSHELL_WINDOWREPLACED 13
#define HSHELL_WINDOWREPLACING 14
#endif

/* _WIN64 preview */
#ifndef _WIN64

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_HINSTANCE GWL_HINSTANCE
#define GetWindowLongPtr GetWindowLong
#define GetWindowLongPtrW GetWindowLongW
#define GetWindowLongPtrA GetWindowLongA
#define SetWindowLongPtr SetWindowLong
#define SetWindowLongPtrW SetWindowLongW
#define SetWindowLongPtrA SetWindowLongA
#define DWORD_PTR DWORD
#define PDWORD_PTR PDWORD
#define UINT_PTR UINT
#define LONG_PTR LONG
#define ULONG_PTR ULONG
#endif

#ifndef GCLP_HICON
#define GetClassLongPtr GetClassLong
#define GetClassLongPtrW GetClassLongW
#define GetClassLongPtrA GetClassLongA
#define SetClassLongPtr SetClassLong
#define SetClassLongPtrW SetClassLongW
#define SetClassLongPtrA SetClassLongA
#define GCLP_HICON GCL_HICON
#define GCLP_HICONSM GCL_HICONSM
#endif

#endif /* ndef _WIN64 */

/* Misc */
#ifndef ENDSESSION_LOGOFF
#define ENDSESSION_LOGOFF 0x80000000
#endif
#ifndef OFN_DONTADDTORECENT
#define OFN_DONTADDTORECENT 0x02000000
#endif
#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

#ifndef ASFW_ANY
EXTERN_C BOOL WINAPI AllowSetForegroundWindow(DWORD dwProcessId);
#define ASFW_ANY 0xFFFFFFFF
#endif

#if defined _MSC_VER || defined __GNUC__
  #define stricmp _stricmp
  #define strnicmp _strnicmp
  #define memicmp _memicmp
  #define strlwr _strlwr
  #define strupr _strupr
  #define itoa _itoa
#endif

#ifndef CLR_INVALID
#define CLR_INVALID 0xFFFFFFFF
#endif

// WtsApi32.h
#ifndef WTS_CURRENT_SERVER_HANDLE
#define WTS_CURRENT_SERVER_HANDLE ((HANDLE)NULL)
#define WTS_CURRENT_SESSION ((DWORD)-1)
#endif

#if defined __GNUC__ && __GNUC__ < 3
typedef struct {
    RECT rgrc[3];
    PWINDOWPOS lppos;
} NCCALCSIZE_PARAMS;
#define MDEFINE_GUID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
    EXTERN_C const GUID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}
#endif

#define WM_WTSSESSION_CHANGE 0x02B1
#define NOTIFY_FOR_THIS_SESSION 0
#define NOTIFY_FOR_ALL_SESSIONS 1
#define WTS_CONSOLE_CONNECT 0x1
#define WTS_CONSOLE_DISCONNECT 0x2
#define WTS_REMOTE_CONNECT 0x3
#define WTS_REMOTE_DISCONNECT 0x4
#define WTS_SESSION_LOGON 0x5
#define WTS_SESSION_LOGOFF 0x6
#define WTS_SESSION_LOCK 0x7
#define WTS_SESSION_UNLOCK 0x8

#define MENUITEMINFO_SIZE_0400 \
    ((DWORD_PTR)&((MENUITEMINFO*)NULL)->cch + sizeof((MENUITEMINFO*)NULL)->cch)

#endif //ndef _WIN0X500_H_
