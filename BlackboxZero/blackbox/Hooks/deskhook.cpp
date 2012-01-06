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

// deskkook.cpp

#include "BBApi.h"
#include "win0x500.h"

// #define DEBUG

#if 0
typedef struct tagMOUSEHOOKSTRUCTEX : public tagMOUSEHOOKSTRUCT
{
    DWORD mouseData;
} MOUSEHOOKSTRUCTEX;
#endif

#ifdef __GNUC__
#define SHARED(T,X) T X __attribute__((section(".shared"), shared)) = (T)0
#endif

#ifdef _MSC_VER
#pragma data_seg( ".shared" )
#define SHARED(T,X) T X = (T)0
#endif

#ifdef __BORLANDC__
#define SHARED(T,X) extern T X
#define hDeskHook shared_1
#define g_hShellHook shared_2
#define WM_ShellHook shared_3
#define BBhwnd shared_4
#define DTWnd shared_5
#define underExplorer shared_6
#define progman_present shared_7
#define is_win2k shared_8
#endif

SHARED(HHOOK, hDeskHook);
SHARED(HHOOK, g_hShellHook);
SHARED(unsigned, WM_ShellHook);
SHARED(HWND, BBhwnd);
SHARED(HWND, DTWnd);
SHARED(bool, underExplorer);
SHARED(bool, progman_present);
SHARED(bool, is_win2k);

#ifdef _MSC_VER
#pragma comment(linker, "/SECTION:.shared,RWS")
#pragma data_seg()
#endif

#ifndef MK_ALT
#define MK_ALT 0x20
#endif

HINSTANCE hInstance = NULL;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam);

void dbg_printf (const char *fmt, ...)
{
    char buffer[4096];
    va_list arg; va_start(arg, fmt);
    vsprintf (buffer, fmt, arg);
    strcat(buffer, "\n");
    OutputDebugString(buffer);
}

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (DLL_PROCESS_ATTACH == fdwReason)
    {
        //dbg_printf("hook attached");

        hInstance = hinstDLL;
    }
    else
    if (DLL_PROCESS_DETACH == fdwReason)
    {
        //dbg_printf("hook detached");
    }
    return TRUE;
}

void *memset(void *d, int c, unsigned l)
{
    char *p = (char *)d;
    while (l) *p++ = c, --l;
    return d;
}

void post_click(int n)
{
    unsigned wParam = 0;
    if (0x8000 & GetAsyncKeyState(VK_MENU))
        wParam |= MK_ALT;
    if (0x8000 & GetAsyncKeyState(VK_CONTROL))
        wParam |= MK_CONTROL;
    if (0x8000 & GetAsyncKeyState(VK_SHIFT))
        wParam |= MK_SHIFT;
    PostMessage(BBhwnd, BB_DESKCLICK, wParam, n);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION
     && ((MOUSEHOOKSTRUCT*)lParam)->hwnd == DTWnd
     && (false == underExplorer
         || false == progman_present
         || 0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000)
         ))
    {
        switch(wParam)
        {
            case WM_LBUTTONDOWN:
                if (progman_present)
                    PostMessage(BBhwnd, BB_HIDEMENU, 0, 0);
                else
                    post_click(0);
                break;

            case WM_LBUTTONDBLCLK:
                if (progman_present)
                    break;
                post_click(7);
                // prevent explorer showing the start menu
                return TRUE;

            //====================
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
                return TRUE;

            case WM_LBUTTONUP:
                post_click(1);
                break;

            case WM_RBUTTONUP:
                post_click(2);
                return TRUE;

            case WM_MBUTTONUP:
                post_click(3);
                break;

            case WM_XBUTTONUP:
                if (HIWORD(wParam) == XBUTTON1)
                    post_click(4);
                else
                if (HIWORD(wParam) == XBUTTON2)
                    post_click(5);
                else
                if (HIWORD(wParam) == XBUTTON3)
                    post_click(6);
                break;
        }
    }
    return CallNextHookEx(hDeskHook, nCode, wParam, lParam);
}

#ifdef BBTINY
LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    //dbg_printf("ShellProc %x %x %x", nCode, wParam, lParam);
    if (nCode >= 0)
    {
        if (HSHELL_WINDOWACTIVATED == nCode && lParam)
            nCode |= 0x8000;
        else
        if (HSHELL_REDRAW == nCode && lParam)
            nCode |= 0x8000;

        PostMessage(BBhwnd, WM_ShellHook, nCode, wParam);
    }

    return CallNextHookEx( g_hShellHook, nCode, wParam, lParam );
}
#endif

EXTERN_C DLL_EXPORT void SetHooks(HWND BlackboxWnd, int flags)
{
    if (BlackboxWnd)
    {
        BBhwnd = BlackboxWnd;
        underExplorer = flags & 1;

        OSVERSIONINFO osInfo;
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        GetVersionEx(&osInfo);
        bool usingNT         = osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
        bool usingWin2kXP    = usingNT && osInfo.dwMajorVersion >= 5;
        is_win2k = usingWin2kXP;

        DTWnd = GetDesktopWindow();

        HWND hw = FindWindow("Progman", "Program Manager");
        if (hw && IsWindowVisible(hw))
        {
            DTWnd = hw;
            hw = FindWindowEx(hw, NULL, "SHELLDLL_DefView", NULL);
            if (hw && IsWindowVisible(hw))
            {
                DTWnd = hw;
                hw = FindWindowEx(hw, NULL, "SysListView32", NULL);
                if (hw && IsWindowVisible(hw))
                {
                    DTWnd = hw;
                }
            }
            progman_present = true;
        }
        hDeskHook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, 0);

#ifdef BBTINY
        if (NULL == GetModuleHandle("SHELL32.DLL"))
        {
            WM_ShellHook = RegisterWindowMessage("SHELLHOOK");
            g_hShellHook = SetWindowsHookEx(WH_SHELL, (HOOKPROC)ShellProc, hInstance, 0);
        }
#endif

    }
    else
    {
        if (hDeskHook)
        {
            UnhookWindowsHookEx(hDeskHook);
            hDeskHook = NULL;
        }

        if (g_hShellHook)
        {
            UnhookWindowsHookEx(g_hShellHook);
            g_hShellHook = NULL;
        }
    }
}

