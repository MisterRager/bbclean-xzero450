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

// bbWorkspaceWheel.cpp - switch desktop with mouse wheel

#include "BBApi.h"
#include "bbversion.h"
#include "win0x500.h"

const char szVersion     [] = "bbWorkspaceWheel 0.2";
const char szAppName     [] = "bbWorkspaceWheel";
const char szInfoVersion [] = "0.2";
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = BBLEAN_RELDATE;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";
const char szCopyright   [] = "2004-2009";

//===========================================================================

#ifdef __GNUC__
#define SHARED(T,X) T X __attribute__((section(".shared"), shared)) = (T)0
#endif

#ifdef _MSC_VER
#pragma data_seg( ".shared" )
#define SHARED(T,X) T X = (T)0
#endif

#ifdef __BORLANDC__
#define SHARED(T,X) T X = (T)0
#endif

SHARED(HHOOK, hDeskHook);
SHARED(HWND, DTWnd);
SHARED(HWND, BBhwnd);
SHARED(bool, is_win2k);

#ifdef _MSC_VER
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")
#endif

typedef struct {
    POINT pt;
    HWND hwnd;
    UINT wHitTestCode;
    DWORD_PTR dwExtraInfo;
    DWORD mouseData;
} MOUSEHOOKSTRUCTEX_;

//===========================================================================

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL)
    {
        MOUSEHOOKSTRUCTEX_* pmhExStruct = (MOUSEHOOKSTRUCTEX_*)lParam;
        HWND hw = WindowFromPoint(pmhExStruct->pt);
        if (DTWnd == hw)
        {
            int delta = is_win2k
                ?(short)HIWORD(pmhExStruct->mouseData)
                :(short)LOWORD(pmhExStruct->dwExtraInfo)
                ;

            if (delta > 0)
                PostMessage(BBhwnd, BB_WORKSPACE, BBWS_DESKLEFT, 1);
            else
            if (delta < 0)
                PostMessage(BBhwnd, BB_WORKSPACE, BBWS_DESKRIGHT, 1);

            return TRUE;
        }
    }
    return CallNextHookEx(hDeskHook, nCode, wParam, lParam);
}

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance)
{
    if (BBhwnd)
    {
        MessageBox(BBhwnd, "Dont load me twice!", szAppName, MB_OK|MB_SETFOREGROUND);
        return 1;
    }

    HWND hw;

    hw = FindWindow("BlackBoxClass", "BlackBox");
    if (NULL == hw)
    hw = FindWindow("xoblite", NULL);
    if (NULL == hw) return 1;
    BBhwnd = hw;

    hw = FindWindow("DesktopBackgroundClass", NULL);
    if (NULL == hw) hw = FindWindow("BBLeanDesktop", NULL);
    if (NULL == hw) hw = GetDesktopWindow();
    if (NULL == hw) return 1;
    DTWnd = hw;

    OSVERSIONINFO osInfo = {sizeof osInfo, 0,0};
    GetVersionEx(&osInfo);

    bool usingNT = osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
    bool usingWin2kXP = usingNT && osInfo.dwMajorVersion >= 5;
    is_win2k = usingWin2kXP;

    hDeskHook =  SetWindowsHookEx(WH_MOUSE, MouseProc, hPluginInstance, 0);
    if (NULL == hDeskHook) return 1;

    return 0;
}

void endPlugin(HINSTANCE hPluginInstance)
{
    UnhookWindowsHookEx(hDeskHook);
}

LPCSTR pluginInfo(int field)
{
    switch (field)
    {
        default:
        case 0: return szVersion;
        case 1: return szAppName;
        case 2: return szInfoVersion;
        case 3: return szInfoAuthor;
        case 4: return szInfoRelDate;
        case 5: return szInfoLink;
        case 6: return szInfoEmail;
    }
}

//===========================================================================
