/*
 ============================================================================

  This file is part of bbIconBox source code.
  bbIconBox is a plugin for Blackbox for Windows

  Copyright © 2004-2009 grischka
  http://bb4win.sf.net/bblean

  bbIconBox is free software, released under the GNU General Public License
  (GPL version 2).

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================
*/

#include "bbIconBox.h"
#include <commctrl.h>

inline static int ratioX(Desk *f, int virt_x)
{ return f->mon_rect.left + virt_x * (f->mon_rect.right - f->mon_rect.left) / f->v_rect.right; }

inline static int ratioY(Desk *f, int virt_y)
{ return f->mon_rect.top + virt_y * (f->mon_rect.bottom - f->mon_rect.top) / f->v_rect.bottom; }

inline static int ratioXinv(Desk *f, int screen_x)
{ return f->v_rect.left + (screen_x - f->mon_rect.left) * f->v_rect.right / (f->mon_rect.right - f->mon_rect.left); }

inline static int ratioYinv(Desk *f, int screen_y)
{ return f->v_rect.top + (screen_y - f->mon_rect.top) * f->v_rect.bottom / (f->mon_rect.bottom - f->mon_rect.top); }

void setup_ratio(Desk *f, HWND hwnd, int width, int height)
{
    f->mon = GetMonitorRect(hwnd, &f->mon_rect, GETMON_FROM_WINDOW);
    f->v_rect.left = f->v_rect.top = 0;
    f->v_rect.right  = imax(1, width - 2*f->v_rect.left);
    f->v_rect.bottom = imax(1, height - 2*f->v_rect.top);
}

void get_virtual_rect(Desk *f, RECT *d, winStruct *w, RECT *r)
{
    r->left     = d->left   + ratioXinv (f, w->info.xpos   );
    r->top      = d->top    + ratioYinv (f, w->info.ypos   );
    r->right    = d->left   + ratioXinv (f, w->info.xpos + w->info.width  );
    r->bottom   = d->top    + ratioYinv (f, w->info.ypos + w->info.height );
}

static BOOL TaskEnumFunc(const struct tasklist *p, LPARAM lParam)
{
    HWND hwnd = p->hwnd;
    Desk *f = (Desk*)lParam;
    int n;
    for (n = 0;;)
    {
        winStruct ws;
        memset(&ws, 0, sizeof ws);

        if (false == GetTaskLocation(hwnd, &ws.info))
            break;

        if (GetMonitorRect(hwnd, NULL, GETMON_FROM_WINDOW) != f->mon)
            break;

        ws.hwnd = hwnd;
        ws.iconic = FALSE != IsIconic(hwnd);
        ws.active = p->active;
        ws.index = 0;

        if (0 == ws.info.width && 0 == ws.info.height && !ws.iconic)
        {
            if (2 == ++n)
                break;
            hwnd = GetLastActivePopup(hwnd);
            continue;
        }

        winStruct *p = new winStruct(ws);
        p->next = f->winList;
        f->winList = p;

        ++f->winCount;
        break;
    }
    return TRUE;
}

static BOOL CALLBACK winenumfunc(HWND hwnd, LPARAM lParam)
{
    winStruct *p;
    Desk *f = (Desk*)lParam;
    for (p = f->winList; p; p = p->next)
    {
        if (p->hwnd == hwnd)
        {
            p->index = ++ f->index;
            break;
        }
    }
    return TRUE;
}

static int cmpfn(const void *p1, const void *p2)
{
    winStruct *a = *(winStruct**)p1;
    winStruct *b = *(winStruct**)p2;
    int n = a->iconic - b->iconic;
    if (n) return n;
    return a->index - b->index;
}

static void sort_winlist(winStruct **pp)
{
    winStruct *p, **b, **a;
    int n;
    for (p = *pp, n = 0; p; p = p->next, ++n);
    a = new winStruct*[n];
    for (b = a, p = *pp; p; *b++ = p, p = p->next);
    qsort(a, n, sizeof *a, cmpfn);
    while (b-- > a) (*b)->next = p, p = *b;
    *pp = p;
    delete [] a;
}

void free_winlist(Desk *f)
{
    while (f->winList)
    {
        winStruct *p = f->winList;
        f->winList = f->winList->next;
        delete p;
    }
    f->winCount = 0;
}

void build_winlist(Desk *f, HWND hwnd)
{
    free_winlist(f);
    EnumTasks(TaskEnumFunc, (LPARAM)f);
    f->index = 0;
    EnumWindows(winenumfunc, (LPARAM)f);
    if (f->winCount > 1)
        sort_winlist(&f->winList);
}

winStruct *get_winstruct(Desk *f, int index)
{
    winStruct *p = f->winList;
    while (p && index)
        p = p->next, --index;
    return p;
}

//===========================================================================
