/*---------------------------------------------------------------------------*

  This file is part of the BBNote source code

  Copyright 2003-2009 grischka@users.sourceforge.net

  BBNote is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 *---------------------------------------------------------------------------*/
// bbnote-proxy.cpp - plugin to notify bbnote about the current blackbox-style

#include "BBApi.h"
#include "bblib.h"
#include "bbversion.h"
#include "BBSendData.h"
#include "StyleStruct.h"

const char szVersion      [] = "bbNote-Proxy 1.08";
const char szAppName      [] = "bbNote-Proxy";
const char szInfoVersion  [] = "1.08";
const char szInfoAuthor   [] = "grischka";
const char szInfoRelDate  [] = BBLEAN_RELDATE;
const char szInfoLink     [] = "http://bb4win.sourceforge.net/bblean/";
const char szInfoEmail    [] = "grischka@users.sourceforgs.net";

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
HWND BBhwnd;
HWND hNoteWnd;

LRESULT CALLBACK BBNoteProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case BB_GETSTYLE:
            return BBSendData((HWND)lParam, BB_SENDDATA, wParam, stylePath(), -1);

        case WM_COPYDATA:
            return BBReceiveData(hwnd, lParam, NULL);

        default:
            if (uMsg >= BB_MSGFIRST && uMsg < BB_MSGLAST) {
                if (BB_SETSTYLE == uMsg && 0 == lParam)
                    return PostMessage(BBhwnd, uMsg, wParam, lParam);
                else
                    return SendMessage(BBhwnd, uMsg, wParam, lParam);
            } else {
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            }
    }
}

/*<---------------------------------------------------------------------->*/

int beginPlugin(HINSTANCE hPluginInstance)
{
    if (BBhwnd)
    {
        MessageBox(NULL, "Dont load me twice!", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }

    BBhwnd = GetBBWnd();

    if (0 == memicmp(GetBBVersion(), "bblean", 6))
    {
        MessageBox(NULL, "This plugin is not required with bblean", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }


    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = BBNoteProc;            // our window procedure
    wc.hInstance = hPluginInstance;
    wc.lpszClassName = szAppName;           // our window class name
    RegisterClass(&wc);
    
    hNoteWnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,                   // exstyles
        wc.lpszClassName,                   // our window class name
        NULL,                               // use description for a window title
        WS_POPUP,
        0, 0,                               // position
        0, 0,                               // width & height of window
        NULL,                               // parent window
        NULL,                               // no menu
        (HINSTANCE)wc.hInstance,            // hInstance of DLL
        NULL);

    return 0;
}


void endPlugin(HINSTANCE hPluginInstance)
{
    DestroyWindow(hNoteWnd);
    UnregisterClass(szAppName, hPluginInstance);
}

//===========================================================================
