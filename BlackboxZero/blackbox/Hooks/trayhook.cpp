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

// trayhook.cpp - hook the systemTray when running under Explorer.

#include "BBApi.h"
#include "win0x500.h"
#include <shellapi.h>

// #define DEBUG

#ifdef __GNUC__
#define SHARED(T,X) T X __attribute__((section(".shared"), shared)) = (T)0
#endif

#ifdef _MSC_VER
#pragma data_seg( ".shared" )
#pragma comment(linker, "/SECTION:.shared,RWS")
#define SHARED(T,X) T X = (T)0
#endif

#ifdef __BORLANDC__
#define SHARED(T,X) extern T X
#define hTrayHook shared_1
#define hShellTrayWnd shared_2
#define bbTrayWnd shared_3
#endif

SHARED(HHOOK, hTrayHook);
SHARED(HWND, hShellTrayWnd);
SHARED(HWND, bbTrayWnd);

#ifdef _MSC_VER
#pragma comment(linker, "/SECTION:.shared,RWS")
#pragma data_seg()
#endif

HINSTANCE hInstance;

#ifdef DEBUG
void dbg_printf (const char *fmt, ...)
{
    char buffer[4096];
    va_list arg; va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}
#endif

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        hInstance = hDLLInst;
        //dbg_printf("Attached to %x", hDLLInst);
        break;

    case DLL_PROCESS_DETACH:
        //dbg_printf("unloaded.");
        break;
    }
    return TRUE;
}

static LRESULT CALLBACK TrayHook_Proc(int ncode, WPARAM wp, LPARAM lp)
{
    if (ncode >= 0)
    {
        CWPSTRUCT* cwps = (CWPSTRUCT*)lp;
        if (cwps->hwnd == hShellTrayWnd
            && cwps->message == WM_COPYDATA
            && (((COPYDATASTRUCT*)cwps->lParam)->dwData == 1
                || ((COPYDATASTRUCT*)cwps->lParam)->dwData == 0
                    ))
        {
            SendMessage(bbTrayWnd, WM_COPYDATA, cwps->wParam, cwps->lParam);
        }
    }
    return CallNextHookEx(hTrayHook, ncode, wp, lp);
}

EXTERN_C DLL_EXPORT int EntryFunc(HWND TrayWnd)
{
    if (TrayWnd) {
        hShellTrayWnd = FindWindow("Shell_TrayWnd", NULL);
        //hShellTrayWnd = FindWindowEx(hShellTrayWnd, NULL, "TrayNotifyWnd", NULL);
        //hShellTrayWnd = FindWindowEx(hShellTrayWnd, NULL, "SysPager", NULL);
        bbTrayWnd = TrayWnd;
        hTrayHook = SetWindowsHookEx(WH_CALLWNDPROC, TrayHook_Proc, hInstance, 0);
        //SendNotifyMessage(HWND_BROADCAST, RegisterWindowMessage("TaskbarCreated"), 0, 0);
    } else {
        if (hTrayHook)
            UnhookWindowsHookEx(hTrayHook);
    }
    return 0;
}

