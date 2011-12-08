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

// implements a system tray and keeps icons in a structure
// for any plugin, that wants to display it.

#include "BB.H"
#include "Settings.h"
#include "Pidl.h"

#define INCLUDE_NIDS
#include "Tray.h"

#include <shlobj.h>
#include <shellapi.h>
#ifdef __GNUC__
#include <shlwapi.h>
#endif
#include <docobj.h>

#define ST static

//===========================================================================
/*
#define NIM_ADD         0x00000000
#define NIM_MODIFY      0x00000001
#define NIM_DELETE      0x00000002

#define NIM_SETFOCUS    0x00000003
#define NIM_SETVERSION  0x00000004
#define NOTIFYICON_VERSION 3

#define NIF_MESSAGE     0x00000001
#define NIF_ICON        0x00000002
#define NIF_TIP         0x00000004
#define NIF_STATE       0x00000008
#define NIF_INFO        0x00000010

#define NIS_HIDDEN      0x00000001
#define NIS_SHAREDICON  0x00000002

// Notify Icon Infotip flags
#define NIIF_NONE       0x00000000
#define NIIF_INFO       0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR      0x00000003

#define NIN_SELECT      WM_USER
#define NINF_KEY        1
#define NIN_KEYSELECT   (NIN_SELECT | NINF_KEY)

#define NIN_BALLOONSHOW (WM_USER + 2)
#define NIN_BALLOONHIDE (WM_USER + 3)
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#define NIN_BALLOONUSERCLICK (WM_USER + 5)
*/

//===========================================================================
typedef struct
{
    DWORD dwMagic; // e.g. 0x34753423;
    DWORD dwMessage;
    NOTIFYICONDATA iconData;
} SHELLTRAYDATA;

typedef struct systemTrayNode
{
    struct systemTrayNode *next;
    bool hidden;
    bool shared;
    bool referenced;
    bool added;
    unsigned version;
    HICON orig_icon;
    systemTray t;
} systemTrayNode;

typedef struct sstraylist
{
    struct sstraylist *next;
    IOleCommandTarget *pOCT;
} sstraylist;

//===========================================================================
// the icon vector
ST systemTrayNode *trayIconList;

// the shell service objects vector
ST sstraylist  *SSOIconList;

ST HWND hTrayWnd;

ST bool tray_on_top;

ST void RemoveTrayIcon(systemTrayNode *p, bool post);

#define SHARED_NOT_FOUND 0x80

//===========================================================================
// API: GetTraySize

int GetTraySize()
{
    systemTrayNode *p; int n = 0;
    dolist (p, trayIconList) if (false == p->hidden) n++;
    return n;
}

//===========================================================================
// API: GetTrayIcon

systemTray* GetTrayIcon(int idx)
{
    systemTrayNode *p; int n = 0;
    dolist (p, trayIconList) if (false == p->hidden && n++ == idx) return &p->t;
    return NULL;
}

//===========================================================================
// API: EnumTray

void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam)
{
    systemTrayNode *p;
    dolist (p, trayIconList)
        if (false == p->hidden && FALSE == lpEnumFunc(&p->t, lParam))
            break;
}

//===========================================================================
// API: CleanTray

void CleanTray()
{
    systemTrayNode *p;
    redo: dolist (p, trayIconList)
        if (FALSE==IsWindow(p->t.hWnd))
        {
            RemoveTrayIcon(p, true);
            goto redo;
        }
}

//===========================================================================
// Function: convert_string
//===========================================================================

ST bool convert_string(char *dest, const void *src, int nmax, bool is_wchar)
{
    char buffer[1024];
    if (is_wchar)
    {
        buffer[0]=0;
        WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)src, -1, buffer, sizeof buffer, NULL, NULL);
        src = buffer;
    }

    if (strcmp(dest, (const char *)src))
    {
        strcpy_max(dest, (const char *)src, nmax);
        return true;
    }

    return false;
}

ST bool get_balloon(struct systemTrayBalloon *pBalloon, NOTIFYICONDATA *iconData, bool is_wchar)
{
    const void *info, *title; unsigned timeout, flags;
    if (is_wchar)
    {
        NID2KW* data = (NID2KW*)iconData;
        title = data->szInfoTitle;
        info = data->szInfo;
        flags = data->dwInfoFlags;
        timeout = data->uTimeout;
    }
    else
    {
        NID2K* data = (NID2K*)iconData;
        title = data->szInfoTitle;
        info = data->szInfo;
        flags = data->dwInfoFlags;
        timeout = data->uTimeout;
    }

    convert_string(pBalloon->szInfoTitle, title, sizeof pBalloon->szInfoTitle, is_wchar);
    convert_string(pBalloon->szInfo, info, sizeof pBalloon->szInfo, is_wchar);

    pBalloon->dwInfoFlags = flags;
    pBalloon->uInfoTimeout = 0;

    if (0 == pBalloon->szInfo[0] && 0 == pBalloon->szInfoTitle[0])
        return false;

    pBalloon->uInfoTimeout = imax(4000, timeout);
    return true;
}

//===========================================================================
// Function: reset_icon - clear the HICON related entries
//===========================================================================

ST void reset_icon(systemTrayNode *p)
{
    if (false == p->shared)
    {
        if (p->referenced)
        {
            systemTrayNode *s;
            dolist (s, trayIconList)
                if (s->shared && s->orig_icon == p->orig_icon)
                    reset_icon(s);

            p->referenced = false;
        }

        if (p->t.hIcon) DestroyIcon(p->t.hIcon);
    }
    p->t.hIcon = NULL;
    p->orig_icon = NULL;
}

//===========================================================================
// Function: RemoveTrayIcon
//===========================================================================

ST void RemoveTrayIcon(systemTrayNode *p, bool post)
{
    bool hidden = p->hidden;
    reset_icon(p);
    if (p->t.pBalloon) m_free(p->t.pBalloon);
    remove_item(&trayIconList, p);
    if (post && false == hidden)
        PostMessage(BBhwnd, BB_TRAYUPDATE, 0, TRAYICON_REMOVED);
}

//===========================================================================
// Function: ModifyTrayIcon
//===========================================================================

ST UINT ModifyTrayIcon(systemTrayNode *p, NOTIFYICONDATA *iconData, UINT uFlags)
{
    UINT uChanged = 0;
    UINT cbSize = iconData->cbSize;
    bool is_wchar = cbSize >= sizeof(NID2KW) || cbSize == sizeof(NIDNT);

    if (uFlags & NIF_TIP)
    {
        if (convert_string(p->t.szTip, iconData->szTip, sizeof p->t.szTip, is_wchar))
            uChanged |= NIF_TIP;
    }

    if ((uFlags & NIF_INFO) && (cbSize >= sizeof(NID2K)))
    {
        if (NULL == p->t.pBalloon)
            p->t.pBalloon = (systemTrayBalloon*)c_alloc(sizeof *p->t.pBalloon);

        if (get_balloon(p->t.pBalloon, iconData, is_wchar))
            uChanged |= NIF_INFO;
    }

    if ((uFlags & NIF_MESSAGE) && (iconData->uCallbackMessage != p->t.uCallbackMessage))
    {
        p->t.uCallbackMessage = iconData->uCallbackMessage;
        uChanged |= NIF_MESSAGE;
    }

    if (uFlags & NIF_ICON)
    {
        if (p->shared)
        {
            systemTrayNode *o;
            dolist (o, trayIconList)
                if (o->orig_icon == iconData->hIcon && false == o->shared)
                    break;

            if (NULL == o || NULL == o->orig_icon)
            {
                uChanged |= SHARED_NOT_FOUND;
            }
            else
            if (p->orig_icon != iconData->hIcon)
            {
                p->t.hIcon = o->t.hIcon;
                o->referenced = true;
                p->orig_icon = iconData->hIcon;
                uChanged |= NIF_ICON;
            }
        }
        else
        if (p->orig_icon != iconData->hIcon)
        {
            reset_icon(p);
            if (iconData->hIcon)
            {
                p->t.hIcon = CopyIcon(iconData->hIcon);
                p->orig_icon = iconData->hIcon;
            }
            uChanged |= NIF_ICON;
        }
    }
    return uChanged;
}

//===========================================================================
#if 0
ST void log_tray(DWORD trayCommand, NOTIFYICONDATA  *iconData, unsigned statemask, unsigned state)
{
    char tip[256]; tip[0] = 0;
    if ((trayCommand == NIM_ADD || trayCommand == NIM_MODIFY)
        && (iconData->uFlags & NIF_TIP)
        )
        convert_string(tip, iconData->szTip, sizeof tip,
            iconData->cbSize >= sizeof(NID2KW) || iconData->cbSize == sizeof(NIDNT));

    char class_n[200];class_n[0] = 0;
    GetClassName(iconData->hWnd, class_n, sizeof class_n);
#if 0
    log_printf(4,
#else
    dbg_printf(
#endif
        "Tray %d %s hw %X %d id %d flag %02X cb %X"
        " sm %X st %X icon %X class \"%s\" tip \"%s\"",
        trayCommand,
        trayCommand==NIM_ADD    ? "add" :
        trayCommand==NIM_MODIFY ? "mod" :
        trayCommand==NIM_DELETE ? "del" : "ukn",
        iconData->hWnd,
        0!=IsWindow(iconData->hWnd),
        iconData->uID,
        iconData->uFlags,
        iconData->uCallbackMessage,
        statemask,
        state,
        iconData->hIcon,
        class_n,
        tip
        );
}
#define LOG_TRAY
#endif

//===========================================================================
// TrayWnd Callback
//===========================================================================

ST LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_WINDOWPOSCHANGED && tray_on_top)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
        return 0;
    }

    if (message != WM_COPYDATA)
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    if (((COPYDATASTRUCT *)lParam)->dwData != 1)
        return FALSE;

    // --------------------------------------------

    NOTIFYICONDATA  *iconData;
    DWORD           trayCommand;
    UINT            uFlags;
    UINT            uChanged;
    bool            hidden = false;
    bool            shared = false;
    unsigned        state = 0;
    unsigned        statemask = 0;
    unsigned        version = 0;

    trayCommand =  ((SHELLTRAYDATA*)((COPYDATASTRUCT*)lParam)->lpData)->dwMessage;
    iconData    = &((SHELLTRAYDATA*)((COPYDATASTRUCT*)lParam)->lpData)->iconData;

    // search the list and set defaults for hidden & shared
    systemTrayNode *p;
    dolist (p, trayIconList)
        if (p->t.hWnd == iconData->hWnd && p->t.uID == iconData->uID)
        {
            hidden = p->hidden;
            shared = p->shared;
            break;
        }

    uFlags = iconData->uFlags;

    if (uFlags & NIF_STATE)
    {
        if (iconData->cbSize >= sizeof(NID2KW))
        {
            statemask   = ((NID2KW*)iconData)->dwStateMask;
            state       = ((NID2KW*)iconData)->dwState;
            version     = ((NID2KW*)iconData)->uVersion;
        }
        else
        if (iconData->cbSize >= sizeof(NID2K))
        {
            statemask   = ((NID2K*)iconData)->dwStateMask;
            state       = ((NID2K*)iconData)->dwState;
            version     = ((NID2K*)iconData)->uVersion;
        }

        if (statemask & NIS_HIDDEN)
            hidden = 0 != (state & NIS_HIDDEN);

        if (statemask & NIS_SHAREDICON)
            shared = 0 != (state & NIS_SHAREDICON);
    }

#ifdef LOG_TRAY
    if (Settings_LogFlag & 4) log_tray(trayCommand, iconData, statemask, state);
#endif

    if (NIM_DELETE == trayCommand) // NIM_DELETE does not care for a valid hwnd
    {
        if (NULL == p) return FALSE;
        RemoveTrayIcon(p, true);
        return TRUE;
    }

    if (FALSE == IsWindow(iconData->hWnd)) // has been seen even with NIM_ADD (crap:)
    {
        if (p) RemoveTrayIcon(p, true);
        return FALSE;
    }

    if (NIM_MODIFY == trayCommand)
    {
        if (NULL == p)
        {
            // here explorer just returns FALSE;

            // we try to be a bit smarter for simple icons, since
            // bblean might be more affected by crashes (plugins, plugins :),
            // and not all apps are always resending their icons as
            // they should with the "TaskbarCreated" message.

            if (false == shared
             && iconData->hIcon
             && (NIF_ICON|NIF_MESSAGE|NIF_TIP) == uFlags
                )
                goto add_icon;

            return FALSE;
        }

        p->hidden = hidden;
        if (p->shared != shared) // just in case, dont know if it ever happens
        {
            reset_icon(p);
            p->shared = shared;
        }

        uChanged = ModifyTrayIcon(p, iconData, uFlags);

        if (uChanged & SHARED_NOT_FOUND)
            return FALSE; // icon remains unchanged

        if (uChanged && false == p->hidden)
            PostMessage(BBhwnd, BB_TRAYUPDATE, uChanged, TRAYICON_MODIFIED);

        return TRUE;
    }

    if (NIM_ADD == trayCommand)
    {
add_icon:
        if (NULL == p)
        {
            p = (systemTrayNode*)c_alloc(sizeof *p);
            append_node(&trayIconList, p);
            p->t.hWnd = iconData->hWnd;
            p->t.uID  = iconData->uID;
        }
        else
        if (p->added)
            return FALSE; // that's what explorer does

        p->hidden = hidden;
        p->shared = shared;
        p->added = NIM_ADD == trayCommand;

        uChanged = ModifyTrayIcon(p, iconData, uFlags);

        if (uChanged & SHARED_NOT_FOUND)
        {
            // shared icons with an invalid reference are not added
            // this should handle network icons correctly, finally.
            RemoveTrayIcon(p, false);
            return FALSE;
        }

        if (uChanged && false == p->hidden)
            PostMessage(BBhwnd, BB_TRAYUPDATE, uChanged, TRAYICON_ADDED);

        return TRUE;
    }

    if (NIM_SETVERSION == trayCommand)
    {
        if (NULL == p) return FALSE;
        p->version = version; // affects supposed keyboard behaviour, unused still.
        return TRUE;
    }

    return FALSE;
}

//===========================================================================
// Function: LoadShellServiceObjects
// Purpose: Initializes COM, then starts all ShellServiceObjects listed in
//          the ShellServiceObjectDelayLoad registry key, e.g. Volume,
//          Power Management, PCMCIA, etc.
// In: void
// Out: void
//===========================================================================

ST void LoadShellServiceObjects()
{
    // redundant by 'OleInitialize' in the main startup
    //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    //CoInitialize(NULL); // win95 compatible

    HKEY hkeyServices;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        "Software\\Microsoft\\Windows\\CurrentVersion\\"
        "ShellServiceObjectDelayLoad", 0, KEY_READ, &hkeyServices))
        return;

    for (int i=0;;i++)
    {
        char szValueName[100]; char szData[200];
        DWORD cbValueName   = sizeof szValueName;
        DWORD cbData        = sizeof szData;
        DWORD dwDataType;

        if (ERROR_SUCCESS != RegEnumValue(hkeyServices, i,
            szValueName, &cbValueName, 0,
            &dwDataType, (LPBYTE) szData, &cbData))
            break;

        //dbg_printf("ShellService %s %s", szValueName, szData);

        WCHAR wszCLSID[sizeof szData];
        MultiByteToWideChar(CP_ACP, 0, szData, cbData, wszCLSID, sizeof szData);

        CLSID clsid;
        CLSIDFromString(wszCLSID, &clsid);

        IOleCommandTarget *pOCT;
        HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IOleCommandTarget, (void **) &pOCT);

        if (SUCCEEDED(hr))
        {
            // Open ShellServiceObject...
            pOCT->Exec(&CGID_ShellServiceObject,
                2, // start
                0,
                NULL, NULL);

            append_node(&SSOIconList, new_node(pOCT));
        }
    }
    RegCloseKey(hkeyServices);
}

//===========================================================================
// Function: UnloadShellServiceObjects
//===========================================================================

ST void UnloadShellServiceObjects()
{
    // Go through each element of the array and stop it..
    sstraylist *t;
    dolist(t, SSOIconList)
    {
        t->pOCT->Exec(&CGID_ShellServiceObject,
            3, // stop
            0,
            NULL, NULL);

        t->pOCT->Release();
    }
    freeall(&SSOIconList);
    //redundant by 'OleUninitialize' in the main shutdown
    //CoUninitialize();
}

//===========================================================================

ST void broadcast_tbcreated(void)
{
    PostMessage(HWND_BROADCAST, RegisterWindowMessage("TaskbarCreated"), 0, 0);
}

ST const char TrayNotifyClass [] = "TrayNotifyWnd";
ST const char TrayClockClass [] = "TrayClockWClass";

ST LRESULT CALLBACK TrayNotifyWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //dbg_printf("TrayNotifyWnd %04x msg %04x wp %08x lp %08x", hwnd, message, wParam, lParam);
    return DefWindowProc(hwnd, message, wParam, lParam);
}

ST HWND create_tray_child(HWND hwndParent, const char *class_name)
{
    WNDCLASS wc;
    ZeroMemory(&wc,sizeof(wc));
    wc.hInstance        = hMainInstance;       
    wc.lpszClassName    = class_name;
    wc.lpfnWndProc      = TrayNotifyWndProc;
    BBRegisterClass(&wc);

    return CreateWindowEx(0, wc.lpszClassName, NULL, WS_CHILD|WS_DISABLED,
        0, 0, 0, 0,
        hwndParent, NULL, hMainInstance, NULL
        );
}

//===========================================================================
// Public Tray interface

void Tray_Init()
{
    if (dont_hide_tray) return;

    SSOIconList  = NULL;
    trayIconList = NULL;
    tray_on_top = underExplorer;

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc      = TrayWndProc;
    wc.hInstance        = hMainInstance;       
    wc.lpszClassName    = ShellTrayClass;
    BBRegisterClass(&wc);

    hTrayWnd = CreateWindowEx(
        tray_on_top ? WS_EX_TOPMOST|WS_EX_TOOLWINDOW : WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        NULL,
        WS_POPUP|WS_DISABLED,
        0, 0, 0, 0,
        NULL,
        NULL,
        hMainInstance,
        NULL
        );

    // Some programs want these child windows so they can
    // figure out the presence/location of the tray.
    create_tray_child(
        create_tray_child(hTrayWnd, TrayNotifyClass),
        TrayClockClass);

    broadcast_tbcreated();

    if (false == underExplorer)
    {
#ifdef BBOPT_DEVEL
        #pragma message("\n"__FILE__ "(665) : warning X0: LoadShellServiceObjects() disabled.\n")
#else
        LoadShellServiceObjects();
#endif
    }
}

void Tray_Exit()
{
    if (NULL == hTrayWnd)
        return;

    UnloadShellServiceObjects();
    DestroyWindow(hTrayWnd);
    hTrayWnd = NULL;

    UnregisterClass(ShellTrayClass, hMainInstance);
    UnregisterClass(TrayNotifyClass, hMainInstance);
    UnregisterClass(TrayClockClass, hMainInstance);

    while (trayIconList)
        RemoveTrayIcon(trayIconList, false);

    if (underExplorer)
        broadcast_tbcreated();
}

//===========================================================================
/*
                        About BALLOON Tooltips
                        ======================

The taskbar notification area is sometimes erroneously called the "tray."

Version 5.0 of the Shell, found on Windows 2000, handles Shell_NotifyIcon
mouse and keyboard events differently than earlier Shell versions, found on
Microsoft Windows NT® 4.0, Windows 95, and Windows 98. The differences are:

- If a user selects a notify icon's shortcut menu with the keyboard, the
  version 5.0 Shell sends the associated application a WM_CONTEXTMENU message.
  Earlier versions send WM_RBUTTONDOWN and WM_RBUTTONUP messages. 

- If a user selects a notify icon with the keyboard and activates it with
  the SPACEBAR or ENTER key, the version 5.0 Shell sends the associated
  application an NIN_KEYSELECT notification. Earlier versions send
  WM_RBUTTONDOWN and WM_RBUTTONUP messages. 

- If a user selects a notify icon with the mouse and activates it with
  the ENTER key, the version 5.0 Shell sends the associated application an
  NIN_SELECT notification. Earlier versions send WM_RBUTTONDOWN and
  WM_RBUTTONUP messages. 

- If a user passes the mouse pointer over an icon with which a balloon ToolTip
  is associated, the version 6.0 Shell (Windows XP) sends the following messages: 

NIN_BALLOONSHOW
- Sent when the balloon is shown (balloons are queued). 

NIN_BALLOONHIDE
- Sent when the balloon disappears/when the icon is deleted, for example.
This message is not sent if the balloon is dismissed because of a timeout
or mouse click by the user. 

NIN_BALLOONTIMEOUT
- Sent when the balloon is dismissed because of a timeout. 

NIN_BALLOONUSERCLICK
- Sent when the balloon is dismissed because the user clicked the mouse. 

You can select which way the Shell should behave by calling Shell_NotifyIcon
with dwMessage set to NIM_SETVERSION. Set the uVersion member of the
NOTIFYICONDATA structure to indicate whether you want version 5.0 or
pre-version 5.0 behavior. 

Note:
    The messages discussed above are not conventional Windows messages.
    They are sent as the lParam value of the application-defined message
    that is specified when the icon is added with NIM_ADD.
*/
//===========================================================================

