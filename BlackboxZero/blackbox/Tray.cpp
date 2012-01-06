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

// Implements a system tray and keeps icons in a linked list of
// 'systemTray' structures so that plugins can display them.

#include "BB.h"
#include "Settings.h"
#include "MessageManager.h"
#include "bbshell.h"
#define INCLUDE_NIDS
#include "Tray.h"

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
#define NIF_GUID        0x00000020

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
#define NIN_POPUPOPEN (WM_USER + 6)
#define NIN_POPUPCLOSE (WM_USER + 7)
*/

//===========================================================================
#pragma pack(push,4)
typedef struct _SHELLTRAYDATA
{
    DWORD dwMagic; // e.g. 0x34753423;
    DWORD dwMessage;
    NOTIFYICONDATA iconData;
} SHELLTRAYDATA;
#pragma pack(pop)

typedef struct systemTrayNode
{
    struct systemTrayNode *next;
    bool hidden;
    bool shared;
    bool added;
    int popup;
    unsigned version;
    HICON orig_icon;
    GUID guidItem;
    HWND hWnd;
    UINT uID;
    UINT uCallbackMessage;
    unsigned uChanged;
    int index;
    POINT pt;
    systemTray t;
} systemTrayNode;

//===========================================================================
// the 'Shell_TrayWnd'
ST HWND hTrayWnd;

// the icon vector
ST systemTrayNode *trayIconList;

// under explorer, setting hTrayWnd to topmost lets it receive the
// messages before explorer does.
ST bool tray_on_top;
ST bool tray_utf8;
ST int tray_edge;
ST HWND tray_mouseover;

ST void RemoveTrayIcon(systemTrayNode *p, bool post);

void LoadShellServiceObjects(void);
void UnloadShellServiceObjects(void);

// Carsomyr's tray redirect trick
// (Vista compatible icons with older plugins, e.f. SystembarEx)
ST int trayredirect_id;
ST UINT trayredirect_message;

// #define TRAY_SHOWPOPUPS

//===========================================================================
// API: GetTraySize
//===========================================================================

int GetTraySize(void)
{
    systemTrayNode *p;
    int n = 0;
    dolist (p, trayIconList)
        p->index = p->hidden ? -1 : n++;
    return n;
}

//===========================================================================
// API: GetTrayIcon
//===========================================================================

static systemTrayNode *nth_icon(int i)
{
    systemTrayNode *p;
    dolist (p, trayIconList)
        if (i == p->index)
            break;
    return p;
}

systemTray* GetTrayIcon(int icon_index)
{
    systemTrayNode *p = nth_icon(icon_index);
    return p ? &p->t : NULL;
}

//===========================================================================
// (API:) EnumTray
//===========================================================================

void EnumTray (TRAYENUMPROC lpEnumFunc, LPARAM lParam)
{
    systemTrayNode *p;
    dolist (p, trayIconList)
        if (false == p->hidden && FALSE == lpEnumFunc(&p->t, lParam))
            break;
}

//===========================================================================
// API: CleanTray
//===========================================================================

void CleanTray(void)
{
    systemTrayNode *p = trayIconList;
    while (p) {
        systemTrayNode *n = p->next;
        if (FALSE == IsWindow(p->hWnd))
            RemoveTrayIcon(p, true);
        p = n;
    }
}

//===========================================================================
// API: ForwardTrayMessage
//===========================================================================

ST void tray_notify(systemTrayNode *p, UINT message)
{
    if (p->version >= 4) {
        WPARAM wParam = MAKELPARAM(p->pt.x, p->pt.y);
        LPARAM lParam = MAKELPARAM(message, p->uID);
        SendNotifyMessage(p->hWnd, p->uCallbackMessage, wParam, lParam);
    } else {
        SendNotifyMessage(p->hWnd, p->uCallbackMessage, p->uID, message);
    }
}

ST int forward_tray_message(systemTrayNode *p, UINT message, systemTrayIconPos *pos)
{
    HWND hwnd;
    systemTrayNode *q;

    if (NULL == p || FALSE == IsWindow(p->hWnd)) {
        CleanTray();
        return 0;
    }

    if (pos) {
        p->pt.x = (pos->r.left + pos->r.right) / 2;
        p->pt.y = (pos->r.top + pos->r.bottom) / 2;
        hwnd = pos->hwnd;
        ClientToScreen(hwnd, &p->pt);
    } else {
        GetCursorPos(&p->pt);
        hwnd = WindowFromPoint(p->pt);
        if (!is_bbwindow(hwnd))
            hwnd = NULL;
    }

    if (hwnd && (message != WM_MOUSEMOVE || hwnd != tray_mouseover)) {

        RECT r, m;
        int x, y;

        tray_mouseover = hwnd;
        x = p->pt.x;
        y = p->pt.y;

        GetWindowRect(hwnd, &r);
        GetMonitorRect(hwnd, &m, GETMON_FROM_WINDOW);
        if (r.right - r.left > r.bottom - r.top) {
            if (y - m.top < m.bottom - y)
                tray_edge = ABE_TOP;
            else
                tray_edge = ABE_BOTTOM;
        } else {
            if (x - m.left < m.right - x)
                tray_edge = ABE_LEFT;
            else
                tray_edge = ABE_RIGHT;
        }

        /* Move/resize the hidden "Shell_TrayWnd" accordingly to
           the plugin where the mouseclick was on, since some tray-apps
           want to align their menu with it */

        SetWindowPos(hTrayWnd, NULL,
            r.left, r.top,
            r.right-r.left, r.bottom-r.top,
            SWP_NOACTIVATE|SWP_NOZORDER);

    }

    if (message != WM_MOUSEMOVE) {
        /* allow the tray-app to grab focus */
        if (have_imp(pAllowSetForegroundWindow))
            pAllowSetForegroundWindow(ASFW_ANY);
        else
            SetForegroundWindow(p->hWnd);
    }

    // dbg_printf("message %x %d %x", p->hWnd, p->uID, message);

    dolist (q, trayIconList)
        if (q->popup && (q != p || message != WM_MOUSEMOVE)) {
            tray_notify(q, NIN_POPUPCLOSE);
            q->popup = 0;
        }

    if (p->version < 3) {
        tray_notify(p, message);
    } else {

#ifdef TRAY_SHOWPOPUPS
        if (message == WM_MOUSEMOVE && p->popup < 3) {
            if (++p->popup == 3)
                tray_notify(p, NIN_POPUPOPEN);
        }
#endif
        if (message == WM_RBUTTONUP) {
            tray_notify(p, WM_CONTEXTMENU);
            tray_notify(p, WM_RBUTTONUP);

        } else if (message == WM_LBUTTONUP) {
            if (usingWin7)
                tray_notify(p, WM_LBUTTONDOWN);
            tray_notify(p, WM_LBUTTONUP);
            tray_notify(p, NIN_SELECT);

        } else if (message == WM_LBUTTONDOWN) {
            if (!usingWin7)
                tray_notify(p, WM_LBUTTONDOWN);
        } else {
            tray_notify(p, message);
        }
    }

    return 1;
}

// Reroute the mouse message to the tray icon's host window...
int ForwardTrayMessage(int icon_index, UINT message, systemTrayIconPos *pos)
{
    return forward_tray_message(nth_icon(icon_index), message, pos);
}

//===========================================================================
// Function: reset_icon - clear the HICON and related entries
//===========================================================================

ST void reset_icon(systemTrayNode *p)
{
    if (false == p->shared) {
        systemTrayNode *s;
        dolist (s, trayIconList)
            if (s->shared && s->orig_icon == p->orig_icon)
                reset_icon(s);
        if (p->t.hIcon)
            DestroyIcon(p->t.hIcon);
    }
    p->t.hIcon = NULL;
    p->orig_icon = NULL;
}

//===========================================================================
// Function: send_tray_message
//===========================================================================

ST LRESULT send_tray_message(systemTrayNode *p, unsigned uChanged, unsigned msg)
{
    if (p->hidden || 0 == uChanged)
        return 0;
    p->uChanged = uChanged;
    return MessageManager_Send(BB_TRAYUPDATE, MAKEWPARAM(p->index, uChanged), msg);
}

//===========================================================================
// Function: RemoveTrayIcon
//===========================================================================

ST void RemoveTrayIcon(systemTrayNode *p, bool post)
{
    reset_icon(p);
    remove_node(&trayIconList, p);
    if (post)
        send_tray_message(p, NIF_ICON, TRAYICON_REMOVED);
    m_free(p);
}

//===========================================================================
// Function: convert_string
//===========================================================================

ST bool convert_string(char *dest, const void *src, int nmax, bool is_unicode)
{
    char buffer[256];
    if (is_unicode) {
        bbWC2MB((const WCHAR*)src, buffer, sizeof buffer);
        src = buffer;
    }
    if (strcmp(dest, (const char *)src)) {
        strcpy_max(dest, (const char *)src, nmax);
        return true;
    }
    return false;
}

//===========================================================================

ST void log_tray(DWORD trayCommand, NIDBB *pnid)
{
    char class_name[100], tip[100];
    tip[0] = class_name[0] = 0;

    GetClassName(pnid->hWnd, class_name, sizeof class_name);

    if ((trayCommand == NIM_ADD || trayCommand == NIM_MODIFY)
        && (pnid->uFlags & NIF_TIP))
        convert_string(tip, pnid->pTip, sizeof tip, pnid->is_unicode);

    log_printf((LOG_TRAY,
        "Tray: %s(%d) "
        "hwnd:%X(%d) "
        "class:\"%s\" "

        "id:%d "
        "flag:%02X "
        "state:%X,%x "
        "msg:%X "
        "icon:%X "
        "tip:\"%s\"",

        trayCommand==NIM_ADD    ? "add" :
        trayCommand==NIM_MODIFY ? "mod" :
        trayCommand==NIM_DELETE ? "del" :
        trayCommand==NIM_SETVERSION ? "ver" : "ukn",
        trayCommand,
        pnid->hWnd, 0 != IsWindow(pnid->hWnd),
        class_name,

        pnid->uID,
        pnid->uFlags,
        pnid->pState ? pnid->pState[1] : 0,
        pnid->pState ? pnid->pState[0] : 0,
        pnid->uCallbackMessage,
        pnid->hIcon,
        tip
        ));
}

//===========================================================================

int pass_back(DWORD pid, HANDLE shmem, void *mem, int size)
{
    static LPVOID (WINAPI *pSHLockShared) (HANDLE, DWORD);
    static BOOL (WINAPI *pSHUnlockShared) (LPVOID lpData);
    LPVOID hMem;
    int ret = 0;

    if (load_imp(&pSHLockShared, "shlwapi.dll", "SHLockShared")
     && load_imp(&pSHUnlockShared, "shlwapi.dll", "SHUnlockShared")) {
        hMem = pSHLockShared (shmem, pid);
        if (NULL == hMem) {
            //dbg_printf("AppBarEvent: couldn't lock memory");
        } else {
            memcpy(hMem, mem, size);
            pSHUnlockShared(hMem);
            ret = 1;
        }
    } else {
        //dbg_printf("couldn't get SH(Un)LockShared function pointers");
    }
    return ret;
}

//===========================================================================

LRESULT AppBarEvent(void *data, unsigned size)
{
    DWORD *p_message;
    DWORD *p_pid;
    APPBARDATAV1 *abd, abd_ret;
    HANDLE32 *p_shmem;

    abd = (APPBARDATAV1*)data;

    switch (size) {
    case sizeof(APPBARMSGDATAV1):
        p_message = &((APPBARMSGDATAV1*)abd)->dwMessage;
        p_shmem = (HANDLE32*)&((APPBARMSGDATAV1*)abd)->hSharedMemory;
        p_pid = &((APPBARMSGDATAV1*)abd)->dwSourceProcessId;
        break;

    case sizeof(APPBARMSGDATAV2):
        p_message = &((APPBARMSGDATAV2*)abd)->dwMessage;
        p_shmem = (HANDLE32*)&((APPBARMSGDATAV2*)abd)->hSharedMemory;
        p_pid = &((APPBARMSGDATAV2*)abd)->dwSourceProcessId;
        break;

    default:
        dbg_printf("AppBarEvent: unknown size: %d", size);
        return 0;
    }

/*
    dbg_printf("AppBarEventV%d message:%d hwnd:%x pid:%d shmem:%x size:%d",
        size == sizeof(APPBARMSGDATAV2) ? 2 : 1, *p_message, abd->hWnd, *p_pid, *p_shmem, size);
    dbg_window((HWND)abd->hWnd, "appevent window");
*/

    log_printf((LOG_TRAY, "AppBarEventV%d message:%d hwnd:%x pid:%d shmem:%x size:%d",
        size == sizeof(APPBARMSGDATAV2) ? 2 : 1,
        *p_message, abd->hWnd, *p_pid, *p_shmem, size));

    switch (*p_message) {

    case ABM_NEW:  //0
        return TRUE;

    case ABM_REMOVE: //1
        return TRUE;

    case ABM_GETSTATE: //4
        return ABS_ALWAYSONTOP;

    case ABM_GETAUTOHIDEBAR:
        return FALSE; //7

    case ABM_GETTASKBARPOS: //5
        abd_ret = *abd;
        abd_ret.uEdge = tray_edge;
        GetWindowRect(hTrayWnd, &abd_ret.rc);
        return pass_back(*p_pid, (HANDLE)*p_shmem, &abd_ret, sizeof abd_ret);
    }

    return FALSE;
}

//===========================================================================

ST LRESULT TrayEvent(void *data, unsigned size)
{
    DWORD trayCommand = ((SHELLTRAYDATA*)data)->dwMessage;
    void *pData = &((SHELLTRAYDATA*)data)->iconData;
    NIDBB nid;
    systemTrayNode *p;
    UINT bbTrayMessage, uChanged, ret;

    memset(&nid, 0, sizeof nid);

#ifdef _WIN64
    size                    = ((NIDNT_32*)pData)->cbSize        ;
    nid.hWnd                = (HWND)((NIDNT_32*)pData)->hWnd    ;
    nid.uID                 = ((NIDNT_32*)pData)->uID           ;
    nid.uFlags              = ((NIDNT_32*)pData)->uFlags        ;
    nid.uCallbackMessage    = ((NIDNT_32*)pData)->uCallbackMessage  ;
    nid.hIcon        = (HICON)((NIDNT_32*)pData)->hIcon         ;
    if (size >= sizeof(NID2KW_32)) {
        nid.is_unicode          = true;
        nid.pInfoFlags          = &((NID2KW_32*)pData)->dwInfoFlags ;
        nid.pInfoTitle          = &((NID2KW_32*)pData)->szInfoTitle ;
        nid.pVersion_Timeout    = &((NID2KW_32*)pData)->uVersion    ;
        nid.pInfo               = &((NID2KW_32*)pData)->szInfo      ;
        nid.pState              = &((NID2KW_32*)pData)->dwState     ;
        nid.pTip                = &((NID2KW_32*)pData)->szTip       ;
        if (size >= sizeof(NID2KW6_32))
            nid.pGuid           = &((NID2KW6_32*)pData)->guidItem   ;
    } else if (size >= sizeof(NID2K_32)) {
        nid.is_unicode          = false;
        nid.pInfoFlags          = &((NID2K_32*)pData)->dwInfoFlags  ;
        nid.pInfoTitle          = &((NID2K_32*)pData)->szInfoTitle  ;
        nid.pVersion_Timeout    = &((NID2K_32*)pData)->uVersion     ;
        nid.pInfo               = &((NID2K_32*)pData)->szInfo       ;
        nid.pState              = &((NID2K_32*)pData)->dwState      ;
        nid.pTip                = &((NID2K_32*)pData)->szTip        ;
    } else {
        nid.is_unicode          = (size == sizeof(NIDNT_32))        ;
        nid.pTip                = &((NIDNT_32*)pData)->szTip        ;
    }
#else
    size                    = ((NIDNT*)pData)->cbSize           ;
    nid.hWnd                = ((NIDNT*)pData)->hWnd             ;
    nid.uID                 = ((NIDNT*)pData)->uID              ;
    nid.uFlags              = ((NIDNT*)pData)->uFlags           ;
    nid.uCallbackMessage    = ((NIDNT*)pData)->uCallbackMessage ;
    nid.hIcon               = ((NIDNT*)pData)->hIcon            ;
    if (size >= sizeof(NID2KW)) {
        nid.is_unicode          = true;
        nid.pInfoFlags          = &((NID2KW*)pData)->dwInfoFlags    ;
        nid.pInfoTitle          = &((NID2KW*)pData)->szInfoTitle    ;
        nid.pVersion_Timeout    = &((NID2KW*)pData)->uVersion       ;
        nid.pInfo               = &((NID2KW*)pData)->szInfo         ;
        nid.pState              = &((NID2KW*)pData)->dwState        ;
        nid.pTip                = &((NID2KW*)pData)->szTip          ;
        if (size >= sizeof(NID2KW6))
            nid.pGuid           = &((NID2KW6*)pData)->guidItem      ;
    } else if (size >= sizeof(NID2K)) {
        nid.is_unicode          = false;
        nid.pInfoFlags          = &((NID2K*)pData)->dwInfoFlags     ;
        nid.pInfoTitle          = &((NID2K*)pData)->szInfoTitle     ;
        nid.pVersion_Timeout    = &((NID2K*)pData)->uVersion        ;
        nid.pInfo               = &((NID2K*)pData)->szInfo          ;
        nid.pState              = &((NID2K*)pData)->dwState         ;
        nid.pTip                = &((NID2K*)pData)->szTip           ;
    } else {
        nid.is_unicode          = (size == sizeof(NIDNT))           ;
        nid.pTip                = &((NIDNT*)pData)->szTip           ;
    }
#endif

    if (Settings_LogFlag & LOG_TRAY)
        log_tray(trayCommand, &nid);

    // search the list
    dolist (p, trayIconList)
        if (p->hWnd == nid.hWnd && p->uID == nid.uID)
            break;

    switch (trayCommand) {

    case NIM_SETVERSION:
        if (NULL == p || NULL == nid.pVersion_Timeout)
            return FALSE;
        p->version = *nid.pVersion_Timeout;
#ifdef TRAY_SHOWPOPUPS
        if (p->version >= 4 && 0 == (nid.uFlags & NIF_SHOWTIP))
            p->t.szTip[0] = 0;
#endif
        return TRUE;

    case NIM_DELETE:
        if (NULL == p)
            return FALSE;
        RemoveTrayIcon(p, true);
        return TRUE;

    case NIM_MODIFY:
    case NIM_ADD:
        if (FALSE == IsWindow(nid.hWnd)) {
            // has been seen even with NIM_ADD
            if (p)
                RemoveTrayIcon(p, true);
            return FALSE;
        }

        if (p) {
            nid.hidden = p->hidden;
            nid.shared = p->shared;
        } else {
            nid.hidden = nid.shared = false;
        }

        if ((nid.uFlags & NIF_STATE) && nid.pState) {
            if (nid.pState[1] & NIS_HIDDEN)
                nid.hidden = 0 != (nid.pState[0] & NIS_HIDDEN);

            if (nid.pState[1] & NIS_SHAREDICON)
                nid.shared = 0 != (nid.pState[0] & NIS_SHAREDICON);
        }

        if (p) {
            if (NIM_ADD == trayCommand) {
                if (p->added)
                    return FALSE;
                p->added = true;
            }
            if (p->shared != nid.shared)
                reset_icon(p);
            bbTrayMessage = TRAYICON_MODIFIED;

        } else {
            if (NIM_MODIFY == trayCommand) {
#if 0
                if (nid.uFlags != (NIF_ICON|NIF_MESSAGE|NIF_TIP)
                    || NULL == nid.hIcon)
#endif
                    return FALSE;
            }
            p = c_new(systemTrayNode);
            append_node(&trayIconList, p);
            p->hWnd = nid.hWnd;
            p->uID  = nid.uID;
            p->added = NIM_ADD == trayCommand;
            if (trayredirect_message) {
                p->t.hWnd = hTrayWnd;
                p->t.uID = ++trayredirect_id & 0x7fff;
            } else {
                p->t.hWnd = p->hWnd;
                p->t.uID  = p->uID;
            }
            bbTrayMessage = TRAYICON_ADDED;
        }

        p->hidden = nid.hidden;
        p->shared = nid.shared;
        uChanged = 0;
        ret = p->added;

        /* check callback message */
        if ((nid.uFlags & NIF_MESSAGE)
            && (nid.uCallbackMessage != p->uCallbackMessage)) {
            p->uCallbackMessage = nid.uCallbackMessage;
            if (trayredirect_message) {
                p->t.uCallbackMessage = trayredirect_message;
            } else {
                p->t.uCallbackMessage = p->uCallbackMessage;
            }
            uChanged |= NIF_MESSAGE;
        }

        /* check tooltip */
        if ((nid.uFlags & NIF_TIP)) {
            if (convert_string(p->t.szTip, nid.pTip,
                    sizeof p->t.szTip, nid.is_unicode))
                uChanged |= NIF_TIP;
        }

#ifdef TRAY_SHOWPOPUPS
        if (p->version >= 4 && 0 == (nid.uFlags & NIF_SHOWTIP)) {
            p->t.szTip[0] = 0;
        }
#endif

        /* check GUID item */
        if ((nid.uFlags & NIF_GUID) && nid.pGuid) {
            p->guidItem = *nid.pGuid;
        }

        /* check icon */
        if ((nid.uFlags & NIF_ICON)) {
            if (p->shared) {
                systemTrayNode *o;
                dolist (o, trayIconList)
                    if (o->orig_icon == nid.hIcon && false == o->shared)
                        break;

                if (NULL == o || NULL == o->orig_icon) {
                    // shared icons with an invalid reference are not added
                    RemoveTrayIcon(p, false);
                    return FALSE;
                }

                if (p->orig_icon != o->orig_icon) {
                    p->orig_icon = o->orig_icon;
                    p->t.hIcon = o->t.hIcon;
                    uChanged |= NIF_ICON;
                }

            } else {
                reset_icon(p);
                if (nid.hIcon) {
                    p->t.hIcon = CopyIcon(nid.hIcon);
                    p->orig_icon = nid.hIcon;
                }
                uChanged |= NIF_ICON;
            }
        }

        /* check balloon */
        if ((nid.uFlags & NIF_INFO) && nid.pInfo) {

            convert_string(p->t.balloon.szInfoTitle, nid.pInfoTitle,
                sizeof p->t.balloon.szInfoTitle, nid.is_unicode);

            convert_string(p->t.balloon.szInfo, nid.pInfo,
                sizeof p->t.balloon.szInfo, nid.is_unicode);

            p->t.balloon.dwInfoFlags = *nid.pInfoFlags;
            p->t.balloon.uInfoTimeout = 0;

            if (p->t.balloon.szInfo[0] || p->t.balloon.szInfoTitle[0]) {
                p->t.balloon.uInfoTimeout = iminmax(*nid.pVersion_Timeout, 4000, 20000);
                uChanged |= NIF_ICON;
            }
        }

        /* notify plugins about change */
        send_tray_message(p, uChanged, bbTrayMessage);
        return ret;

    default:
        return FALSE;
    }
}

//===========================================================================
ST LRESULT TrayInfoEvent(void *data, unsigned size)
{
    struct NOTIFYICONIDENTIFIER_MSGV1* s = (NOTIFYICONIDENTIFIER_MSGV1*)data;

    systemTrayNode *p;
    dolist (p, trayIconList)
        if (IsEqualIID(p->guidItem, s->guidItem))
            break;
#if 0
    char s_guid[40];
    guid_to_string(&s->guidItem, s_guid);
    dbg_printf("%x %d %d %d %04x %d - %04x %s",
        s->dwMagic,
        s->dwMessage,
        s->cbSize,
        s->dwPadding,
        s->hWnd,
        s->uID,
        p ? (unsigned)p->hWnd : 0,
        s_guid
        );
    if (p)
        dbg_window(p->hWnd, "TrayInfoEvent window");

#endif

    if (NULL == p)
        return 0;

    if (s->dwMessage == 2) {
        return MAKELONG(16,16);
    }

    if (s->dwMessage == 1) {
        return MAKELPARAM(p->pt.x, p->pt.y);
    }

    return 0;
}

ST LRESULT TrayTestEvent(void *data, unsigned size)
{
#if 0
    char s_guid[40];
    guid_to_string((GUID*)data, s_guid);

    char buffer[1000];
    unsigned n, x;
    x = sprintf(buffer, "size: %04x", size);
    for (n = 0; n < size && x < sizeof buffer - 10; ++n) {
        if (0 == n % 16)
            x += sprintf(buffer + x, "\n%02x: ", n);
        else if (0 == n % 4)
            buffer[x++] = ' ';
        x += sprintf(buffer + x, " %02x", ((unsigned char*)data)[n]);
    }
    dbg_printf("%s\n%s\n", buffer, s_guid);
#endif
    return 0;
}

//===========================================================================
// TrayWnd Callback
//===========================================================================

ST LRESULT CALLBACK TrayWndProc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COPYDATA) {
        void *data;
        unsigned size;
        int id;

        data = ((COPYDATASTRUCT*)lParam)->lpData;
        size = ((COPYDATASTRUCT*)lParam)->cbData;
        id = ((COPYDATASTRUCT*)lParam)->dwData;

        if (size >= sizeof (DWORD)
            && ((SHELLTRAYDATA*)data)->dwMagic == 0x34753423) {
            if (id == 1)
                return TrayEvent(data, size);
            if (id == 3)
                return TrayInfoEvent(data, size);
        }
        if (id == 0)
            return AppBarEvent(data, size);
        return TrayTestEvent(data, size);
    }

    if (message == WM_WINDOWPOSCHANGED && tray_on_top) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0,
            SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
        return 0;
    }

    if (message == trayredirect_message && message) {
        systemTrayNode *p;
        dolist (p, trayIconList)
            if (p->t.uID == wParam)
                break;
        forward_tray_message(p, lParam, NULL);
        return 0;
    }

    //dbg_printf("other message: %x %x %x", message, wParam, lParam);
    return DefWindowProc(hwnd, message, wParam, lParam);
}

//===========================================================================
// Function: LoadShellServiceObjects
// Purpose: Initializes COM, then starts all ShellServiceObjects listed in
//          the ShellServiceObjectDelayLoad registry key, e.g. Volume,
//          Power Management, PCMCIA, etc.
// In: void
// Out: void
//===========================================================================
#ifndef BBTINY

#include <docobj.h>

typedef struct sso_list_t
{
    struct sso_list_t *next;
    IOleCommandTarget *pOCT;
    char name[1];
} sso_list_t;

// the shell service objects vector
ST sso_list_t *sso_list;
ST HANDLE BBSSO_Stop;
ST HANDLE BBSSO_Thread;

#if defined __GNUC__ && __GNUC__ < 3
MDEFINE_GUID(CGID_ShellServiceObject, 0x000214D2L, 0, 0,0xC0,0,0,0,0,0,0,0x46);
#endif

// vista uses stobject.dll to load ShellServiceObjects
// {35CEC8A3-2BE6-11D2-8773-92E220524153}
// {730F6CDC-2C86-11D2-8773-92E220524153}

void LSDeactivateActCtx(HANDLE hActCtx,  DWORD_PTR* pulCookie);
HANDLE LSActivateActCtxForClsid(REFCLSID rclsid, DWORD_PTR* pulCookie);

ST int sso_load(const char *name, const char *guid)
{
    WCHAR wszCLSID[200];
    CLSID clsid;
    IOleCommandTarget *pOCT = NULL;
    HRESULT hr;
    sso_list_t *t;
    DWORD_PTR ulCookie;
    HANDLE hContext;

    MultiByteToWideChar(CP_ACP, 0, guid, 1+strlen(guid), wszCLSID, array_count(wszCLSID));
    CLSIDFromString(wszCLSID, &clsid);

    hContext = LSActivateActCtxForClsid(clsid, &ulCookie);

    hr = CoCreateInstance(
        COMREF(clsid),
        NULL, 
        CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
        COMREF(IID_IOleCommandTarget),
        (void **) &pOCT
        );

    log_printf((LOG_TRAY, "Tray: Load ShellServiceObject(%s): %s: %s",
        SUCCEEDED(hr)?"ok":"failed", name, guid));

    if (SUCCEEDED(hr)) {
        hr = COMCALL5(pOCT, Exec,
            &CGID_ShellServiceObject,
            2, // start
            0,
            NULL,
            NULL
            );
        //dbg_printf("Starting ShellService %lx %d %s %s", (DWORD)hr, (pOCT->AddRef(), pOCT->Release()), name, guid);
        if (SUCCEEDED(hr)) {
            t = (sso_list_t*)m_alloc(sizeof(sso_list_t) + strlen(name));
            t->pOCT = pOCT;
            strcpy(t->name, name);
            append_node(&sso_list, t);
        } else {
            pOCT->Release();
        }
    }

    LSDeactivateActCtx(hContext, &ulCookie);
    return 1;
}

ST DWORD WINAPI SSO_Thread(void *pv)
{
    HKEY hk0, hk1;
    sso_list_t *t;
    const char *key;

    hk0 = HKEY_LOCAL_MACHINE;
    key = "Software\\Microsoft\\Windows\\CurrentVersion\\ShellServiceObjectDelayLoad";

    // CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_NOOLE1DDE);
    CoInitialize(0); // win95 compatible

    BBSleep(1000);

    if (usingVista)
        sso_load("stobject", "{35CEC8A3-2BE6-11D2-8773-92E220524153}");
    else
    if (ERROR_SUCCESS == RegOpenKeyEx(hk0, key, 0, KEY_READ, &hk1)) {
        int index;
        for (index = 0; ; ++index) {
            char szValueName[MAX_PATH];
            char szData[MAX_PATH];
            DWORD cbValueName, cbData, dwDataType;

            cbValueName = sizeof szValueName;
            cbData = sizeof szData;

            if (ERROR_SUCCESS != RegEnumValue(hk1, index, szValueName, &cbValueName,
                    0, &dwDataType, (LPBYTE) szData, &cbData))
                break;

            sso_load(szValueName, szData);
        }
        RegCloseKey(hk1);
    }

    // Wait for the exit event
    BBWait(0, 1, &BBSSO_Stop);

    // Go through each element of the array and stop it..
    dolist(t, sso_list) {
        IOleCommandTarget *pOCT = t->pOCT;
        HRESULT hr;
        /* sometimes for some reason trying to access the SSObject's vtbl
           here caused a GPF. Maybe it was already released. */
        if (IsBadReadPtr(*(void**)pOCT, 5*sizeof(void*)/*&Exec+1*/)) {
#ifdef BBOPT_MEMCHECK
            BBMessageBox(MB_OK, "Bad ShellService Object: %s", t->name);
#endif
            continue;
        }
        hr = COMCALL5(pOCT, Exec,
            &CGID_ShellServiceObject,
            3, // stop
            0,
            NULL,
            NULL
            );
        //dbg_printf("Stopped ShellService %lx %d %s", (DWORD)hr, (pOCT->AddRef(), pOCT->Release()), t->name);
        COMCALL0(pOCT, Release);
    }

    freeall(&sso_list);
    CoUninitialize();
    return 0;
}

void LoadShellServiceObjects(void)
{
    DWORD threadid;
    BBSSO_Stop = CreateEvent(NULL, FALSE, FALSE, "BBSSO_Stop");
    BBSSO_Thread = CreateThread(NULL, 0, SSO_Thread, NULL, 0, &threadid);
}

void UnloadShellServiceObjects(void)
{
    if (BBSSO_Thread) {
        SetEvent(BBSSO_Stop);
        BBWait(0, 1, &BBSSO_Thread);
        CloseHandle(BBSSO_Stop);
        CloseHandle(BBSSO_Thread);
        BBSSO_Thread = NULL;
    }
}

//===========================================================================
#endif //ndef BBTINY
//===========================================================================

ST void broadcast_tbcreated(void)
{
    SendNotifyMessage(HWND_BROADCAST, RegisterWindowMessage("TaskbarCreated"), 0, 0);
}

ST const char TrayNotifyClass [] = "TrayNotifyWnd";
ST const char TrayClockClass [] = "TrayClockWClass";

ST LRESULT CALLBACK TrayNotifyWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //log_printf((LOG_TRAY, "Tray: TrayNotifyWnd %04x msg %04x wp %08x lp %08x", hwnd, message, wParam, lParam));
    //dbg_printf("Tray: TrayNotifyWnd %04x msg %04x wp %08x lp %08x", hwnd, message, wParam, lParam);
    return DefWindowProc(hwnd, message, wParam, lParam);
}

ST HWND create_tray_child(HWND hwndParent, const char *class_name)
{
    BBRegisterClass(class_name, TrayNotifyWndProc, 0);
    return CreateWindow(
        class_name,
        NULL,
        WS_CHILD,
        0, 0, 0, 0,
        hwndParent,
        NULL,
        hMainInstance,
        NULL
        );
}

void Tray_SetEncoding(void)
{
    systemTrayNode *p;
    bool utf8 = 0 != Settings_UTF8Encoding;
    if (utf8 != tray_utf8)
        dolist (p, trayIconList) {
            WCHAR wstr[256];
            MultiByteToWideChar(tray_utf8 ? CP_UTF8 : CP_ACP, 0,
                p->t.szTip, -1, wstr, array_count(wstr));
            bbWC2MB(wstr, p->t.szTip, sizeof p->t.szTip);
        }
    tray_utf8 = utf8;
}

//===========================================================================
// hook tray when running under explorer

ST const char *trayClassName;
ST int (*trayHookDll_EntryFunc)(HWND);


//===========================================================================
// Public Tray interface

void Tray_Init(void)
{
    if (Settings_disableTray)
        return;

    log_printf((LOG_TRAY, "Tray: Starting"));

    trayClassName = "Shell_TrayWnd";
    tray_on_top = false;
    Tray_SetEncoding();

    if (underExplorer) {
        if (load_imp(&trayHookDll_EntryFunc, "trayhook.dll", "EntryFunc")) {
            // the trayhook will redirect messages from the real
            // "Shell_TrayWnd" to our window
            trayClassName = "bbTrayWnd";
        } else {
            // otherwise we hope that our window will get messages
            // first if it is on top.
            tray_on_top = true;
        }
    }

    BBRegisterClass(trayClassName, TrayWndProc, 0);
    if (Settings_OldTray)
        trayredirect_message = RegisterWindowMessage("BBTrayMessage");

    hTrayWnd = CreateWindowEx(
        tray_on_top ? WS_EX_TOOLWINDOW|WS_EX_TOPMOST : WS_EX_TOOLWINDOW,
        trayClassName,
        NULL,
        WS_POPUP,
        0, 0, 0, 0,
        NULL,
        NULL,
        hMainInstance,
        NULL
        );

    if (have_imp(trayHookDll_EntryFunc)) {
        trayHookDll_EntryFunc(hTrayWnd);
    } else {
        // Some programs want these child windows so they can
        // figure out the presence/location of the tray.
        create_tray_child(
            create_tray_child(hTrayWnd, TrayNotifyClass),
            TrayClockClass);
    }

    if (false == underExplorer)
        LoadShellServiceObjects();
    broadcast_tbcreated();
}

void Tray_Exit(void)
{
    bool use_hook = have_imp(trayHookDll_EntryFunc);
    UnloadShellServiceObjects();
    if (hTrayWnd) {
        DestroyWindow(hTrayWnd);
        hTrayWnd = NULL;
        UnregisterClass(trayClassName, hMainInstance);
        if (false == use_hook) {
            UnregisterClass(TrayNotifyClass, hMainInstance);
            UnregisterClass(TrayClockClass, hMainInstance);
            if (underExplorer)
                broadcast_tbcreated();
        }
    }
    while (trayIconList)
        RemoveTrayIcon(trayIconList, false);
}

//===========================================================================
/* stuff incoming on Win-7 with WM_COPYDATA:dwData = 2

 {68DDBB56-9D1D-4FD9-89C5-C0DA2A625392} UnexpectedShutdownReason

 size: 0014
 00:  56 bb dd 68  1d 9d d9 4f  89 c5 c0 da  2a 62 53 92
 10:  02 00 00 00

 {7849596A-48EA-486E-8937-A2A3009F31A9} PostBootReminder object

 size: 0014
 00:  6a 59 49 78  ea 48 6e 48  89 37 a2 a3  00 9f 31 a9
 10:  02 00 00 00

 {900C0763-5CAD-4A34-BC1F-40CD513679D5} User Account Control Check Service

 size: 0014
 00:  63 07 0c 90  ad 5c 34 4a  bc 1f 40 cd  51 36 79 d5
 10:  03 00 00 00

 {FBEB8A05-BEEE-4442-804E-409D6C4515E9} ShellFolder for CD Burning

 size: 0014
 00:  05 8a eb fb  ee be 42 44  80 4e 40 9d  6c 45 15 e9
 10:  02 00 00 00

*/

#ifndef BBTINY
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// LSActivateActCtxForDll
// Activates the custom activation context for the specified DLL
//

#ifndef ACTCTX_FLAG_RESOURCE_NAME_VALID
#define ACTCTX_FLAG_RESOURCE_NAME_VALID 0x00000008
typedef struct tagACTCTXA {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCSTR      lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCSTR      lpAssemblyDirectory;
    LPCSTR      lpResourceName;
    LPCSTR      lpApplicationName;
    HMODULE     hModule;
} ACTCTX, *PACTCTX;
#endif

int CLSID_to_string(REFCLSID rclsid, LPSTR ptzBuffer, size_t cchBuffer)
{
    LPOLESTR pOleString = NULL;
    int len;
    HRESULT hr;

    hr = ProgIDFromCLSID(rclsid, &pOleString);
    if (FAILED(hr))
        hr = StringFromCLSID(rclsid, &pOleString);
    if (FAILED(hr) || NULL == pOleString)
        return 0;
    len = WideCharToMultiByte(CP_ACP, 0, pOleString, -1, ptzBuffer,
        (int)cchBuffer, NULL, NULL);
    CoTaskMemFree(pOleString);
    return len;
}

HANDLE LSActivateActCtxForDll(LPCSTR pszDll, DWORD_PTR *pulCookie)
{
    HANDLE hContext;
    ACTCTX act = { 0 };

    HANDLE (WINAPI* fnCreateActCtx)(PACTCTX pCtx);
    BOOL (WINAPI* fnActivateActCtx)(HANDLE hCtx, ULONG_PTR* pCookie);

    if (!_load_imp(&fnCreateActCtx, "kernel32.dll", "CreateActCtxA")
     || !_load_imp(&fnActivateActCtx, "kernel32.dll", "ActivateActCtx"))
        return NULL;

    act.cbSize = sizeof(act);
    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
    act.lpSource = pszDll;
    act.lpResourceName = MAKEINTRESOURCE(123);

    hContext = fnCreateActCtx(&act);
    if (hContext == INVALID_HANDLE_VALUE)
        return NULL;
    if (fnActivateActCtx(hContext, pulCookie))
        return hContext;
    LSDeactivateActCtx(hContext, NULL);
    return NULL;
}

//
//  Activate the the custom manifest (if any) of the DLL that implements
//  the COM object in question
//

HANDLE LSActivateActCtxForClsid(REFCLSID rclsid, DWORD_PTR *pulCookie)
{
    char szCLSID[39];
    char szSubkey[MAX_PATH];
    char szDll[MAX_PATH];
    DWORD cbDll;
    LONG lres;

    LRESULT (WINAPI *pSHGetValue)(
        HKEY hkey,
        LPCSTR pszSubKey,
        LPCSTR pszValue,
        LPDWORD pdwType,
        LPVOID pvData,
        LPDWORD pcbData
        );

    if (!_load_imp(&pSHGetValue, "shlwapi.dll", "SHGetValueA"))
        return NULL;

    if (0 == CLSID_to_string(rclsid, szCLSID, sizeof szCLSID))
        return NULL;

    sprintf(szSubkey, "CLSID\\%s\\InProcServer32", szCLSID);
    cbDll = sizeof(szDll);

    lres = pSHGetValue(HKEY_CLASSES_ROOT, szSubkey, NULL, NULL, szDll, &cbDll);
    if (lres != ERROR_SUCCESS)
        return NULL;
    return LSActivateActCtxForDll(szDll, pulCookie);
}

void LSDeactivateActCtx(HANDLE hActCtx, ULONG_PTR* pulCookie)
{
    BOOL (WINAPI* fnDeactivateActCtx)(DWORD dwFlags, ULONG_PTR ulc);
    void (WINAPI* fnReleaseActCtx)(HANDLE hActCtx);

    if (!_load_imp(&fnDeactivateActCtx, "kernel32.dll", "DeactivateActCtx")
     || !_load_imp(&fnReleaseActCtx, "kernel32.dll", "ReleaseActCtx"))
        return;
    if (NULL == hActCtx)
        return;
    if (pulCookie)
        fnDeactivateActCtx(0, *pulCookie);
    fnReleaseActCtx(hActCtx);
}
#endif
