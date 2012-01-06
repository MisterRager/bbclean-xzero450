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

#include "BB.h"
#include "Settings.h"
#include "Workspaces.h"
#include "Menu/MenuMaker.h"

//===========================================================================
struct en { Menu *m; int desk; HWND hwndTop; int flags, i, n; };
enum e_flags { e_alltasks = 1 };

static BOOL task_enum_func(struct tasklist *tl, LPARAM lParam)
{
    struct en* en = (struct en *)lParam;
    bool is_top = tl->hwnd == en->hwndTop;
    bool iconic = !is_top && IsIconic(tl->hwnd);
    en->i++;
    if ((en->desk == -1 && iconic) // iconic tasks
     || (en->desk == tl->wkspc // tasks for one workspace
        && ((en->flags & e_alltasks) || false == iconic)))
    {
        char buf[100];
        MenuItem *mi;

        sprintf(buf, "@BBCore.ActivateWindow %d", en->i);
        mi = MakeMenuItem(en->m, tl->caption, buf, is_top);
#ifdef BBOPT_MENUICONS
        if (tl->icon)
            MenuItemOption(mi, BBMENUITEM_SETHICON, tl->icon);
#endif
        if (en->desk != -1) {
            sprintf(buf, "@BBCore.MinimizeWindow %d", en->i);
            MenuItemOption(mi, BBMENUITEM_RCOMMAND, buf);
            if (iconic)
                MenuItemOption(mi, BBMENUITEM_DISABLED);
        }
        en->n ++;
    }
    return TRUE;
}

static int fill_task_folder(Menu *m, int desk, int flags)
{
    struct en en;
    en.m = m, en.desk = desk, en.hwndTop = GetActiveTaskWindow(), en.flags = flags, en.i = en.n = 0;
    EnumTasks(task_enum_func, (LPARAM)&en);
    return en.n;
}

static Menu * build_task_folder(int desk, const char *title, bool popup)
{
    Menu *m;
    char buf[100];

    sprintf(buf, (-1 == desk)
        ? "Core_tasks_icons" : "Core_tasks_workspace%d", desk+1);
    m = MakeNamedMenu(title, buf, popup);
    if (m) fill_task_folder(m, desk, e_alltasks);
    return m;
}

//===========================================================================
Menu * MakeTaskFolder(int n, bool popup)
{
    DesktopInfo DI;
    struct string_node *sn;

    GetDesktopInfo(&DI);
    if (n < 0 || n >= DI.ScreensX)
        return NULL;

    sn = (struct string_node *)nth_node(DI.deskNames, n);
    return build_task_folder(n, sn->str, popup);
}

//===========================================================================
Menu* MakeDesktopMenu(int mode, bool popup)
{
    DesktopInfo DI;
    struct string_node *sl;
    int n, d;
    Menu *m, *s, *i = NULL;

    if (mode == 2) {
        m = MakeNamedMenu(NLS0("Tasks"), "Core_tasks_menu", popup);
    } else {
        i = build_task_folder(-1, NLS0("Icons"), popup);
        if (mode == 1)
            return i;
        m = MakeNamedMenu(NLS0("Workspaces"), "Core_tasks_workspaces", popup);
    }
    GetDesktopInfo(&DI);
    for (n = 0, d = DI.ScreensX, sl = DI.deskNames; n < d; ++n, sl = sl->next) {
        if (mode == 0) {
            char buf[100];
            MenuItem *fi;

            fi = MakeSubmenu(m, build_task_folder(n, sl->str, popup), NULL);
            sprintf(buf, "@BBCore.SwitchToWorkspace %d", n+1);
            MenuItemOption(fi, BBMENUITEM_LCOMMAND, buf);
            if (n == DI.number)
                MenuItemOption(fi, BBMENUITEM_CHECKED);
        } else {
            fill_task_folder(m, n, e_alltasks);
            if (n == d-1) return m;
            MakeMenuNOP(m, NULL);
        }
    }
    MakeMenuNOP(m, NULL);
    MakeSubmenu(m, i, NULL);
    s = MakeNamedMenu(NLS0("New/Remove"), "Core_workspaces_setup", popup);
    MakeMenuItem(s, NLS0("New Workspace"), "@BBCore.AddWorkspace", false);
    if (d > 1) MakeMenuItem(s, NLS0("Remove Last"), "@BBCore.DelWorkspace", false);
    MakeMenuItemString(s, NLS0("Workspace Names"), "@BBCore.SetWorkspaceNames", Settings_workspaceNames);
    MakeSubmenu(m, s, NULL);
    return m;
}

//===========================================================================
static BOOL CALLBACK recoverwindow_enum_proc(HWND hwnd, LPARAM lParam)
{
    char windowtext[100];
    char classname[100];
    char appname[100];
    char broam[100];
    char text[400];
    RECT r;
    MenuItem *mi;

    if (IsWindowEnabled(hwnd) && (IsIconic(hwnd)
        || (GetClientRect(hwnd, &r) && r.right && r.bottom))
        && GetWindowText(hwnd, windowtext, sizeof text))
    {
        classname[0] = 0;
        //GetClassName(hwnd, classname, sizeof classname);
        GetAppByWindow(hwnd, appname);

        sprintf (broam, "@BBCore.RecoverWindow %p", hwnd);
        sprintf (text, "%s : \"%s\"", appname, windowtext);
        mi = MakeMenuItem((Menu*)lParam, text, broam, FALSE != IsWindowVisible(hwnd));
        MenuItemOption(mi, BBMENUITEM_JUSTIFY, DT_LEFT);
    }
    return TRUE;
}

Menu *MakeRecoverMenu(bool popup)
{
    Menu *m = MakeNamedMenu(NLS0("Toggle Window Visibilty"), "Core_tasks_recoverwindows", popup);
    EnumWindows(recoverwindow_enum_proc, (LPARAM)m);
    MenuOption(m, BBMENU_MAXWIDTH|BBMENU_SORT|BBMENU_CENTER|BBMENU_PINNED|BBMENU_ONTOP, 400);
    return m;
}

void ShowRecoverMenu(void)
{
    Workspaces_GatherWindows();
    if (Settings_altMethod) {
        ShowMenu(MakeRecoverMenu(true));
    } else {
        BBMessageBox(MB_OK, "Windows gathered in current workspace.");
    }
}

//===========================================================================
