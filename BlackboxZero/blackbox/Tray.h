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

#ifndef _BBTRAY_H_
#define _BBTRAY_H_

//===========================================================================

void CleanTray(void);
void Tray_Init(void);
void Tray_Exit(void);

int GetTraySize(void);
systemTray* GetTrayIcon(int idx);
void Tray_SetEncoding(void);

//===========================================================================
#ifdef INCLUDE_NIDS

#define HWND32 DWORD
#define HICON32 DWORD
#define HANDLE32 DWORD

// Blackbox private:
typedef struct _NIDBB {
    // basic info
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    // some pointers into the original structure
    void *pTip;
    DWORD *pState;
    void *pInfo;
    UINT *pVersion_Timeout;
    void *pInfoTitle;
    DWORD *pInfoFlags;
    GUID *pGuid;
    // flags
    bool is_unicode;
    bool hidden;
    bool shared;
} NIDBB;

// from various Windows versions
typedef struct _NID95 {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    CHAR szTip[64];
} NID95;

typedef struct _NIDNT {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[64];
} NIDNT;

typedef struct _NID2K {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    CHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    CHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    CHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NID2K;

typedef struct _NID2K6 {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    CHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    CHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    CHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
} NID2K6;

typedef struct _NID2KW {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NID2KW;

typedef struct _NID2KW6 {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
} NID2KW6;


// ----------------------------------------
/* 2K */
typedef struct
{
  DWORD cbSize;
  HWND32 hWnd;
  UINT uCallbackMessage;
  UINT uEdge;
  RECT rc;
  LPARAM lParam;
}
APPBARDATAV1;

typedef struct
{
  APPBARDATAV1 abd;
  DWORD  dwMessage;
  HANDLE32 hSharedMemory;
  DWORD  dwSourceProcessId;
}
APPBARMSGDATAV1;

/* XP */
typedef struct
{
  DWORD cbSize;
  HWND32 hWnd;
  UINT uCallbackMessage;
  UINT uEdge;
  RECT rc;
  LPARAM lParam;
#ifndef _WIN64
  DWORD dw64BitAlign;
#endif
}
APPBARDATAV2;

typedef struct
{
  APPBARDATAV2 abd;
  DWORD  dwMessage;
  HANDLE hSharedMemory;
  DWORD  dwSourceProcessId;
#ifndef _WIN64
  DWORD dw64BitAlign;
#endif
}
APPBARMSGDATAV2;

// ----------------------------------------
// 32 bit versions when compiling for x64

typedef struct _NIDNT_32 {
    DWORD cbSize;
    HWND32 hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON32 hIcon;
    WCHAR szTip[64];
} NIDNT_32;

typedef struct _NID2K_32 {
    DWORD cbSize;
    HWND32 hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON32 hIcon;
    CHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    CHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    CHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NID2K_32;

typedef struct _NID2KW_32 {
    DWORD cbSize;
    HWND32 hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON32 hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
} NID2KW_32;

typedef struct _NID2KW6_32 {
    DWORD cbSize;
    HWND32 hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON32 hIcon;
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    };
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID guidItem;
} NID2KW6_32;

struct NOTIFYICONIDENTIFIER_MSGV1
{
    DWORD dwMagic;
    DWORD dwMessage;
    DWORD cbSize;
    DWORD dwPadding;
    HWND32 hWnd;
    UINT uID;
    GUID guidItem;
};

#endif //defINCLUDE_NIDS
#endif //ndef _BBTRAY_H_
