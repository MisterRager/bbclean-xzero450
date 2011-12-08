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

#ifndef __Tray_H
#define __Tray_H

//===========================================================================

void CleanTray(void);
void Tray_Init(void);
void Tray_Exit(void);

int GetTraySize();
systemTray* GetTrayIcon(int idx);

/* experimental: */
typedef BOOL (*TRAYENUMPROC)(struct systemTray *, LPARAM);
API_EXPORT void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam);

//===========================================================================
#ifdef INCLUDE_NIDS

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

#endif

//===========================================================================
#endif
