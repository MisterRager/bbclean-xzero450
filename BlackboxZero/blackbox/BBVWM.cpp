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
#include "BBVWM.h"

#define ST static
#define SCREEN_DIST 10

typedef struct winlist
{
    struct winlist *next;
    HWND hwnd;
    RECT rect;
    int desk;
    int prev_desk;
    int save_desk;

    bool moved;
    bool hidden;
    bool iconic;
    bool sticky_app;
    bool sticky;

    bool istool;
    bool check;

    DWORD threadid;
    HWND root;
} winlist;

ST bool vwm_alt_method;
ST bool vwm_styleXPFix;
ST bool vwm_enabled;
ST struct winlist *vwm_WL;

ST bool belongs_to_app(winlist *wl, winlist *wl2);
ST HWND get_root(HWND hwnd);
ST bool is_shadow(HWND hwnd, LONG ex_style, DWORD threadid);

//=========================================================
// helper functions

winlist* vwm_add_window(HWND hwnd)
{
    bool hidden;
    winlist *wl;

    if (check_sticky_plugin(hwnd))
        return NULL;

    hidden = FALSE == IsWindowVisible(hwnd);
    wl = (winlist*)assoc(vwm_WL, hwnd);

    if (NULL == wl)
    {
        LONG ex_style;
        DWORD threadid;

        if (hidden)
            return NULL;

        ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
        threadid = GetWindowThreadProcessId(hwnd, NULL);

        // exclude blackbox menu drop shadows
        if (usingXP && is_shadow(hwnd, ex_style, threadid))
            return NULL;

        wl = c_new(winlist);
        cons_node (&vwm_WL, wl);
        wl->hwnd = hwnd;
        wl->desk = wl->prev_desk = currentScreen;

        // store some infos used with 'belongs_to_app(...)'
        wl->threadid = threadid;
        wl->root = get_root(hwnd);
        if ((ex_style & (WS_EX_TOOLWINDOW|WS_EX_APPWINDOW))
            == WS_EX_TOOLWINDOW
            && NULL == GetWindow(hwnd, GW_OWNER))
            wl->istool = true;

        // check whether its listed in 'StickyWindows.ini'
        wl->sticky_app = check_sticky_name(hwnd);
    }

    wl->hidden = hidden;
    if (false == hidden || wl->moved)
    {
        wl->check = true;
        wl->iconic = FALSE != IsIconic(hwnd);
        if (false == wl->moved && false == wl->iconic)
            GetWindowRect(hwnd, &wl->rect);
    }
    return wl;
}

ST BOOL CALLBACK win_enum_proc(HWND hwnd, LPARAM lParam)
{
    vwm_add_window(hwnd);
    return TRUE;
}

//=========================================================
// update the list

void vwm_update_winlist(void)
{
    winlist *wl, **wlp, *app;

    // clear check/sticky flags;
    dolist (wl, vwm_WL)
        wl->check = wl->sticky = false;

    if (vwm_enabled)
        EnumWindows(win_enum_proc, 0);

    // clear not listed windows
    for (wlp = &vwm_WL; NULL != (wl = *wlp);) {
        if (wl->check) {
            wlp = &wl->next;
        } else {
            *wlp = wl->next;
            m_free(wl);
        }
    }

    // check what windows belong to sticky apps
    dolist (app, vwm_WL)
        if (app->sticky_app)
            dolist (wl, vwm_WL)
                if (belongs_to_app(wl, app))
                    wl->sticky = true;

#if 0
    // debugging stuff
    dbg_printf("-----------------");
    dolist (wl, vwm_WL)
    {
        char buffer[100];
        char appName[100];
        GetClassName(wl->hwnd, buffer, sizeof buffer);
        LONG style = GetWindowLong(wl->hwnd, GWL_STYLE);
        LONG exstyle = GetWindowLong(wl->hwnd, GWL_EXSTYLE);
        GetAppByWindow(wl->hwnd, appName);
        dbg_printf("hw:%x ws:%d mv:%d hi:%d ic:%d st:%d <%s> (wst:%08x wex:%08x %s)",
            wl->hwnd, wl->desk, wl->moved, wl->hidden, wl->iconic, wl->sticky, buffer, style, exstyle, appName);
    }
    dbg_printf("-----------------");
#endif
}

//============================================================

ST bool is_shadow(HWND hwnd, LONG ex_style, DWORD threadid)
{
    char class_name[20];
    const unsigned wstyle = WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_LAYERED;
    return wstyle == (ex_style & wstyle)
        && threadid == BBThreadId
        && GetClassName(hwnd, class_name, sizeof class_name)
        && 0 == strcmp(class_name, "SysShadow")
        ;
}

//============================================================
// Some stuff to make shure that windows aren't lost in nirwana

ST void fix_window(int *left, int *top, int width, int height)
{
    RECT x, d, w;
    int x0, y0, dx, dy;

    x0 = VScreenX;
    y0 = VScreenY;
    dx = VScreenWidth;
    dy = VScreenHeight;
    d.right = (d.left = x0) + dx,
    d.bottom = (d.top = y0) + dy;
    w.right = (w.left = *left) + width,
    w.bottom = (w.top = *top) + height;
    if (FALSE == IntersectRect(&x, &w, &d)
        || x.right - x.left < 20
        || x.bottom - x.top < 20)
    {
        *left = iminmax(*left, x0, x0+dx-width);
        *top = iminmax(*top, y0, y0+dy-height);
    }
}

ST void center_window(int *left, int *top, int width, int height)
{
    int x0, dx, w2;
    x0 = VScreenX - SCREEN_DIST;
    dx = VScreenWidth + SCREEN_DIST;
    w2 = width/2;
    while (*left+w2 < x0) *left += dx;
    while (*left+w2 >= x0+dx) *left -= dx;
}

// WINDOWPLACEMENT's rcNormalPosition seems to be relative to the workarea:
ST void add_workarea(RECT *p, int d)
{
    POINT m; RECT w; int dx, dy;
    m.x = (p->left + p->right)/2;
    m.y = (p->top + p->bottom)/2;
    GetMonitorRect(&m, &w, GETMON_WORKAREA|GETMON_FROM_POINT);
    dx = d * w.left;
    dy = d * w.top;
    p->left += dx;
    p->top += dy;
    p->right += dx;
    p->bottom += dy;
}

ST bool set_normal_position(HWND hwnd, RECT *p)
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof wp;
    if (!GetWindowPlacement(hwnd, &wp))
        return false;
    wp.rcNormalPosition = *p;
    p = &wp.rcNormalPosition;
    add_workarea(p, -1);
    return FALSE != SetWindowPlacement(hwnd, &wp);
}

ST bool get_normal_position(HWND hwnd, RECT *p)
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof wp;
    if (!GetWindowPlacement(hwnd, &wp))
        return false;
    *p = wp.rcNormalPosition;
    add_workarea(p, 1);
    return true;
}

//=========================================================

ST HWND get_root(HWND hwnd)
{
    HWND parent, deskwnd = GetDesktopWindow();
    while (NULL != (parent = GetWindow(hwnd, GW_OWNER)) && deskwnd != parent)
        hwnd = parent;
    return hwnd;
}

ST bool belongs_to_app(winlist *win, winlist *app)
{
    if (win->root == app->root)
        return true;
    if (win->threadid == app->threadid && win->istool)
        return true;
    return false;
}

ST void check_appwindows(winlist *wl_app)
{
    winlist *wl;
    if (wl_app->istool)
        dolist (wl, vwm_WL)
            if (false == wl->istool && belongs_to_app(wl_app, wl)) {
                wl_app = wl;
                break;
            }
    dolist (wl, vwm_WL)
        wl->check = belongs_to_app(wl, wl_app);
}

//=========================================================
ST void defer_windows(int newdesk)
{
    winlist *wl;
    HDWP dwp, dwp_new;
    bool gather, winmoved, deskchanged;

    gather = newdesk < 0;

    if (gather)
        newdesk = currentScreen;

    if (newdesk >= nScreens)
        newdesk = nScreens-1;

    deskchanged = currentScreen != newdesk;
    if (deskchanged)
        lastScreen = currentScreen;
    currentScreen = newdesk;
    winmoved = false;

    if (vwm_WL) {
        dwp = BeginDeferWindowPos(listlen(vwm_WL));
        dolist (wl, vwm_WL)
        {
            int left, top, width, height;
            UINT flags;

            // frozen windows would freeze blackbox in
            // "EndDeferWindowPos" below
            if (is_frozen(wl->hwnd))
                goto next;

            if (gather || wl->sticky) {
                wl->desk = newdesk;
                if (false == gather && false == wl->moved)
                    goto next;
            }

            if (wl->desk >= nScreens)
                wl->desk = nScreens-1;

            left = wl->rect.left;
            top = wl->rect.top;
            width = wl->rect.right - left;
            height = wl->rect.bottom - top;

            // hack for some fancy, zero sized but visible message windows
            if ((height == 0 || width == 0) && false == wl->moved)
                goto next;

            if (wl->iconic && false == vwm_alt_method) {
                if (wl->moved && newdesk == wl->desk) {
                    // The window was iconized while on other WS
                    // which should not happen, but just in case:
                    set_normal_position(wl->hwnd, &wl->rect);
                    wl->moved = false;
                }
                goto next;
            }

            wl->moved = newdesk != wl->desk;

            if (vwm_alt_method) {
                if (wl->moved) {
                    // windows on other WS's are hidden
                    flags = SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_HIDEWINDOW;
                } else {
                    flags = SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW;
                }
            } else {
                if (wl->moved) {
                    // windows on other WS's are moved
                    int dx = VScreenWidth + SCREEN_DIST;
                    if (vwm_styleXPFix)
                        left += dx * 2; // move 2 screens to the right
                    else
                        left += dx * (wl->desk - newdesk); // natural order
                } else if (gather) {
                    // with gather, force windows on screen
                    fix_window(&left, &top, width, height);
                }
                flags = SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE;
            }

            dwp_new = DeferWindowPos(dwp, wl->hwnd, NULL, left, top, width, height, flags);
    #if 0
            char buffer[100];
            GetClassName(wl->hwnd, buffer, sizeof buffer);
            dbg_printf ("defer: %x %x <%s>", dwp_new, wl->hwnd, buffer);
    #endif
            if (dwp_new) {
                dwp = dwp_new;
            } else {
                wl->desk = wl->prev_desk;
                wl->moved = newdesk != wl->desk;
            }

        next:
            if (wl->prev_desk != wl->desk) {
                winmoved = true;
                wl->prev_desk = wl->desk;
            }
        }

        EndDeferWindowPos(dwp);
        Sleep(1);
    }

    // update wkspc members in the tasklist
    workspaces_set_desk();
    // send notifications
    if (deskchanged)
        send_desk_refresh();
    if (winmoved)
        send_task_refresh();
}

//=========================================================

ST void explicit_move(winlist *wl)
{
    if (!wl->iconic && !is_frozen(wl->hwnd))
        SetWindowPos(wl->hwnd, NULL,
            wl->rect.left,
            wl->rect.top,
            wl->rect.right - wl->rect.left,
            wl->rect.bottom - wl->rect.top,
            SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
            );
}

ST bool set_location_helper(HWND hwnd, struct taskinfo *t, unsigned flags)
{
    winlist *wl;
    int dx, dy, new_desk, switch_desk, window_desk;
    bool defer, move_before;

    wl = (winlist*)assoc(vwm_WL, hwnd);
    if (NULL == wl)
        return false;

    dx = t->xpos - wl->rect.left;
    dy = t->ypos - wl->rect.top;
    new_desk = t->desk;
    if (flags & (BBTI_SWITCHTO|BBTI_SETDESK))
        if (new_desk < 0 || new_desk >= nScreens)
            return false;

    switch_desk = flags & BBTI_SWITCHTO ? new_desk : currentScreen;
    window_desk = flags & BBTI_SETDESK ? new_desk : wl->desk;
    move_before = switch_desk == window_desk;

    check_appwindows(wl);
    defer = switch_desk != currentScreen;

    dolist (wl, vwm_WL)
        if (wl->check) {
            if (flags & BBTI_SETDESK) {
                if (wl->desk != new_desk) {
                    wl->desk = new_desk;
                    defer = true;
                }
            }

            if (flags & BBTI_SETPOS) {
                wl->rect.top += dy;
                wl->rect.bottom += dy;
                wl->rect.left += dx;
                wl->rect.right += dx;
                if (vwm_alt_method) {
                    if (move_before)
                        explicit_move(wl);
                } else {
                    defer = true;
                }
            }
        }

    if (defer)
        defer_windows(switch_desk);

    if (vwm_alt_method && (flags & BBTI_SETPOS) && false == move_before)
        dolist (wl, vwm_WL)
            if (wl->check)
                explicit_move(wl);

    return true;
}

//=========================================================
//              set workspace
//=========================================================

void vwm_switch(int new_desk)
{
    if (new_desk < 0 || new_desk >= nScreens)
        return;
    vwm_update_winlist();
    defer_windows(new_desk);
}

void vwm_gather(void)
{
    vwm_update_winlist();
    defer_windows(-1);
    if (vwm_alt_method) {
        vwm_alt_method = false;
        vwm_gather();
        vwm_alt_method = true;
    }
}

//=========================================================
// Set window properties

bool vwm_set_location(HWND hwnd, struct taskinfo *t, unsigned flags)
{
    vwm_update_winlist();
    return set_location_helper(hwnd, t, flags);
}

bool vwm_set_desk(HWND hwnd, int new_desk, bool switchto)
{
    struct taskinfo t;
    unsigned flags;

    if (new_desk < 0 || new_desk >= nScreens)
        return false;

    t.desk = new_desk;
    t.xpos = t.ypos = 0;
    flags = switchto ? BBTI_SWITCHTO|BBTI_SETDESK : BBTI_SETDESK;
    return vwm_set_location(hwnd, &t, flags);
}

bool vwm_set_workspace(HWND hwnd, int new_desk)
{
    // BBPager hack: redo it's move (cannot call vwm_update_winlist here)
    RECT r;
    struct taskinfo t;

    GetWindowRect(hwnd, &r);
    center_window((int*)&r.left, (int*)&r.top, r.right - r.left, r.bottom - r.top);
    t.desk  = new_desk;
    t.xpos  = r.left;
    t.ypos  = r.top;
    return set_location_helper(hwnd, &t, BBTI_SETDESK|BBTI_SETPOS);
}

bool vwm_set_sticky(HWND hwnd, bool set)
{
    winlist *wl;
    if (FALSE == IsWindow(hwnd))
        return false;
    wl = vwm_add_window(hwnd);
    if (NULL == wl)
        return false;
    wl->sticky_app = set;
    return true;
}

static void bottomize(HWND hwnd)
{
    if (!is_frozen(hwnd))
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
            SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOSENDCHANGING);
}

bool vwm_lower_window(HWND hwnd)
{
    winlist *wl;
    vwm_update_winlist();
    wl = (winlist*)assoc(vwm_WL, hwnd);
    if (NULL == wl)
        return false;
    check_appwindows(wl);
    dolist (wl, vwm_WL)
        if (wl->check)
            bottomize(wl->hwnd);
    return true;
}

//=========================================================
// retrieve infos

int vwm_get_desk(HWND hwnd)
{
    winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
    return wl ? wl->desk : currentScreen;
}

bool vwm_get_location(HWND hwnd, struct taskinfo *t)
{
    winlist *wl; RECT rp, *p;

    wl = (winlist*)assoc(vwm_WL, hwnd);
    if (wl && wl->moved) {
        p = &wl->rect;
    } else {
        p = &rp;
        if (IsIconic(hwnd)) {
            if (false == get_normal_position(hwnd, p))
                return false;
        } else {
            if (FALSE == GetWindowRect(hwnd, p))
                return false;
        }
    }
    t->desk = wl ? wl->desk : currentScreen;
    t->xpos = p->left;
    t->ypos = p->top;
    t->width = p->right - p->left;
    t->height = p->bottom - p->top;
    return true;
}

bool vwm_get_status(HWND hwnd, int what)
{
    winlist *wl;
    wl = (winlist*)assoc(vwm_WL, hwnd);
    if (wl) switch (what) {
        case VWM_MOVED: return wl->moved;
        case VWM_STICKY: return wl->sticky_app;
        case VWM_HIDDEN: return wl->hidden;
        case VWM_ICONIC: return wl->iconic;
    } else switch (what) {
        case VWM_STICKY: return check_sticky_name(hwnd);
    }
    return false;
}

//=========================================================
// Init/exit

void vwm_init(void)
{
    vwm_alt_method = Settings_altMethod;
    vwm_styleXPFix = Settings_styleXPFix;
    vwm_enabled = nScreens > 1;
}

void vwm_exit(void)
{
    freeall(&vwm_WL);
}

void vwm_reconfig(bool defer)
{
    winlist *wl;

    if (vwm_alt_method != Settings_altMethod || vwm_styleXPFix != Settings_styleXPFix)
    {
        // backup desk:
        dolist (wl, vwm_WL)
            wl->save_desk = wl->desk;
        // gather
        vwm_gather();
        // restore desk:
        dolist (wl, vwm_WL)
            wl->desk = wl->save_desk;
        vwm_alt_method = Settings_altMethod;
        vwm_styleXPFix = Settings_styleXPFix;
        defer = true;
    }

    dolist (wl, vwm_WL)
        if (wl->desk >= nScreens)
            defer = true;

    if (currentScreen >= nScreens) {
        currentScreen = nScreens - 1;
        defer = true;
    }

    if (defer)
        vwm_switch(currentScreen);

    vwm_enabled = nScreens > 1;
}

//=========================================================
