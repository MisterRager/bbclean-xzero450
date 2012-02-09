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

// This file show bb-stylized window menus with extensions for move to
// workspaces etc for bbLeanBar and bbLeanSkin

#include "BBApi.h"
#include "win0x500.h"
#include "bblib.h"

#define ST static

char usingNT, usingXP;
static const char sys_command[] = "SysCommand %p,%x";

ST HMENU get_sysmenu(HWND Window)
{
    BOOL iconic = IsIconic(Window);
    BOOL zoomed = IsZoomed(Window);
    LONG style = GetWindowLong(Window, GWL_STYLE);
    //LONG exstyle = GetWindowLong(Window, GWL_EXSTYLE);

    HMENU systemMenu = NULL;

    if (is_frozen(Window))
        return systemMenu;

    //XXX GetSystemMenu(Window, TRUE);
    systemMenu = GetSystemMenu(Window, FALSE);
    if (NULL == systemMenu)
        return systemMenu;

    // restore is enabled only if minimized or maximized (not normal)
    EnableMenuItem(systemMenu, SC_RESTORE, MF_BYCOMMAND |
        (iconic || zoomed ? MF_ENABLED : MF_GRAYED));

    // move is enabled only if normal
    EnableMenuItem(systemMenu, SC_MOVE, MF_BYCOMMAND |
        (!(iconic || zoomed) ? MF_ENABLED : MF_GRAYED));

    // size is enabled only if normal
    EnableMenuItem(systemMenu, SC_SIZE, MF_BYCOMMAND |
        (!(iconic || zoomed) && (style & WS_SIZEBOX) ? MF_ENABLED : MF_GRAYED));

    // minimize is enabled only if not minimized
    EnableMenuItem(systemMenu, SC_MINIMIZE, MF_BYCOMMAND |
        (!iconic && (style & WS_MINIMIZEBOX)? MF_ENABLED : MF_GRAYED));

    // maximize is enabled only if not maximized
    EnableMenuItem(systemMenu, SC_MAXIMIZE, MF_BYCOMMAND |
        (!zoomed && (style & WS_MAXIMIZEBOX) ? MF_ENABLED : MF_GRAYED));

    // close is always enabled
    EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);

    // let application modify menu
    //XXX SendMessage(Window, WM_INITMENU, (WPARAM)systemMenu, 0);

    return systemMenu;
}

//===========================================================================
// Function: ShowSysmenu
//===========================================================================

bool ShowSysmenu_default(HWND Window, HWND Owner, RECT *pRect)
{
    HMENU systemMenu = get_sysmenu(Window);
    if (NULL == systemMenu)
        return false;

    if (FALSE == IsIconic(Window))
    {
        SetForegroundWindow(GetLastActivePopup(Window));
        Sleep(10);
    }

    //XXX SendMessage(Window, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, TRUE));

    DWORD ThreadID2 = GetWindowThreadProcessId(Window, NULL);
    DWORD ThreadID1 = GetCurrentThreadId();
    AttachThreadInput(ThreadID1, ThreadID2, TRUE);

    // display the menu
    POINT pt; GetCursorPos(&pt);

    int command =
        TrackPopupMenu(
            systemMenu,
            TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_CENTERALIGN,
            pt.x, pt.y,
            0,
            Owner,
            NULL
            );

    AttachThreadInput(ThreadID1, ThreadID2, FALSE);

    if (command)
        PostMessage(Window, WM_SYSCOMMAND, (WPARAM)command, 0);

    return true;
}

//===========================================================================

//===========================================================================

char *getsysmenu_id(char *IDString, int *n)
{
    sprintf(IDString, "IDSysmenu-%d", ++*n);
    return IDString;
}

bool sysmenu_exists()
{
    return MenuExists("IDSysmenu-");
}

void copymenu (Menu *m, HWND hwnd, HMENU hm, bool sep, int *id, const char *sysBroam)
{
    char text_string[128], *p, id_temp[200];
    MENUITEMINFO MII;
    int n, c;
    static int (WINAPI *pGetMenuStringW)(HMENU,UINT,LPWSTR,int,UINT);

    for (c = GetMenuItemCount (hm), n = 0; n < c; n++)
    {
        memset(&MII, 0, sizeof MII);
        if (usingXP)
            MII.cbSize = sizeof MII;
        else
            MII.cbSize = MENUITEMINFO_SIZE_0400; // to make this work on win95
        MII.fMask  = MIIM_DATA|MIIM_ID|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE;
        GetMenuItemInfo (hm, n, TRUE, &MII);

        if (usingNT
            && load_imp(&pGetMenuStringW, "user32.dll", "GetMenuStringW")) {
            WCHAR wstr[128];
            pGetMenuStringW(hm, n, wstr, array_count(wstr), MF_BYPOSITION);
            bbWC2MB(wstr, text_string, sizeof text_string);
        } else {
            GetMenuString(hm, n, text_string, sizeof text_string, MF_BYPOSITION);
        }

        // dbg_printf("string: <%s>", text_string);
        for (p = text_string; *p; ++p)
            if ('\t' == *p)
                *p = ' ';

        if (MII.hSubMenu)
        {
            //XXX SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)MII.hSubMenu, MAKELPARAM(n, TRUE));
            Menu *s = MakeNamedMenu(text_string, getsysmenu_id(id_temp, id), true);
            copymenu(s, hwnd, MII.hSubMenu, false, id, sysBroam);
            if (sep)
                MakeMenuNOP(m, NULL), sep = false;
            MakeSubmenu(m, s, text_string);
        }
        else
        if (MII.fType & MFT_SEPARATOR)
        {
            sep = true;
        }
        else
        if (!(MII.fState & MFS_DISABLED) && !(MII.fState & MFS_GRAYED))
        {
            char broam[256];
            sprintf(broam, sysBroam, hwnd, MII.wID);
            if (sep)
                MakeMenuNOP(m, NULL), sep = false;
            MakeMenuItem(m, text_string, broam, false);
        }
    }
}

//===========================================================================
// Function: ShowSysmenu
//===========================================================================

bool ShowSysmenu(HWND Window, HWND Owner, RECT *pRect, const char *plugin_broam)
{
    Menu *m;
    char sysBroam[100];
    char broam[256];
    char temp[200];
    int id = 0;
    char *title = NULL;

    DesktopInfo info;
    bool is_in_current;
    int n, workspace;
    bool bbLeanSkin = 0 != strstr(plugin_broam, "Skin");

    unsigned ver = GetVersion(),
    usingNT = (ver & 0x80000000) == 0;
    usingXP =  usingNT && (ver & 0xFF) >= 5;

#if 0
    if (bbLeanSkin)
        return false;
    else
        return ShowSysmenu_default(Window, Owner, pRect);
#endif
    if (Window != GetLastActivePopup(Window))
        return false;

    HMENU systemMenu = get_sysmenu(Window);
    BOOL iconic = IsIconic(Window);
    LONG style = GetWindowLong(Window, GWL_STYLE);
    LONG exstyle = GetWindowLong(Window, GWL_EXSTYLE);

#if 0
    char buff[32];
    int i = GetWindowText(Window, buff, sizeof buff);
    if (i >= (int)sizeof buff - 1)
        memset(buff + sizeof buff - 4, '.', 3);
    title = buff;
#endif

    GetDesktopInfo(&info);
    workspace = GetTaskWorkspace(Window);
    is_in_current = info.number == workspace;

    sprintf(sysBroam, "%s.%s", plugin_broam, sys_command);
    n = 0;
    m = MakeNamedMenu(title, getsysmenu_id(temp, &id), true);

    if (info.ScreensX > 1)
    {
        //bool is_sticky = 0 != SendMessage(BBhwnd, BB_WORKSPACE, BBWS_ISSTICKY, (LPARAM)Window);
        bool is_sticky = CheckSticky(Window);
        //if (false == is_sticky)
        {
            Menu *s = MakeNamedMenu("Send To", getsysmenu_id(temp, &id), true);
            string_node *p = info.deskNames;
            for (n = 0; p && n < info.ScreensX; ++n, p = p->next)
            {
                sprintf(broam, sysBroam, Window, n+0x1000);
                MakeMenuItem(s, p->str, broam, workspace == n);
            }
            MakeSubmenu(m, s, "Send To");
        }
        sprintf(broam, sysBroam, Window, 0x1100);
        MakeMenuItem(m, "On All &Workspaces", broam, is_sticky);
        n = 1;
    }

    if (is_in_current)
    {
        bool f;
        if (false == iconic && (style & WS_SIZEBOX))
        {
            f = NULL != GetProp(Window, BBSHADE_PROP);
            sprintf(broam, sysBroam, Window, 0x1102);
            MakeMenuItem(m, "Sha&de", broam, f);
        }

        f = 0 != (exstyle & WS_EX_TOPMOST);
        sprintf(broam, sysBroam, Window, 0x1101);
        MakeMenuItem(m, "Always On &Top", broam, f);
        n = 1;
    }

    if (n)
    {
        MakeMenuNOP(m, NULL);
    }

    MakeMenuItem(m, "Minimize &All", "@BBCore.MinimizeAll", false);
    MakeMenuItem(m, "Restore &All", "@BBCore.RestoreAll", false);

    if (is_in_current && systemMenu)
    {
        copymenu(m, Window, systemMenu, true, &id, sysBroam);
    }

    if (false == bbLeanSkin && false == iconic && is_in_current)
    {
        SetForegroundWindow(Window);
        BBSleep(50);
    }

    if (pRect)
        MenuOption(m, BBMENU_RECT|BBMENU_NOTITLE|BBMENU_SYSMENU, pRect);

    ShowMenu(m);
    return true;
}

//===========================================================================
bool exec_sysmenu_command(const char *temp, bool sendToSwitchTo)
{
    HWND task_hwnd = NULL;
    HWND BBhwnd = GetBBWnd();
    unsigned task_msg;

    // dbg_printf("hwnd %x  msg %x : %s", (DWORD)task_hwnd, task_msg, temp);
    if (2 != sscanf(temp, sys_command, &task_hwnd, &task_msg))
        return false;

    if (task_msg >= 0x1000 && task_msg < 0x1100)
    {
        int desk = task_msg - 0x1000;
        if (sendToSwitchTo)
            PostMessage(BBhwnd, BB_MOVEWINDOWTON, desk, (LPARAM)task_hwnd);
        else
            PostMessage(BBhwnd, BB_SENDWINDOWTON, desk, (LPARAM)task_hwnd);
    }
    else
    if (task_msg == 0x1100)
    {
        PostMessage(BBhwnd, BB_WORKSPACE, BBWS_TOGGLESTICKY, (LPARAM)task_hwnd);
    }
    else
    if (task_msg == 0x1101)
    {
        PostMessage(BBhwnd, BB_WORKSPACE, BBWS_TOGGLEONTOP, (LPARAM)task_hwnd);
    }
    else
    if (task_msg == 0x1102)
    {
        PostMessage(BBhwnd, BB_WINDOWSHADE, 0, (LPARAM)task_hwnd);
    }
    else
    {
        PostMessage(task_hwnd, WM_SYSCOMMAND, (WPARAM)task_msg, 0);
    }

    return true;
}

//===========================================================================

