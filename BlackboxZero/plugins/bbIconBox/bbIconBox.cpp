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
#include "bbversion.h"

const char szVersion     [] = "bbIconBox "BBLEAN_VERSION;
const char szAppName     [] = "bbIconBox";
const char szInfoVersion [] = BBLEAN_VERSION;
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = BBLEAN_RELDATE;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";
const char szCopyright   [] = "2005-2009";

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

#define INCLUDE_MODE_PAGER

// ----------------------------------------------
// Global variables

HWND BBhwnd;    // Blackbox window handle
struct icon_box *g_PI;
int currentDesk;
int BBVersion;
int box_count;
char create_path[MAX_PATH];

HINSTANCE g_hInstance;
HWND g_hSlit;
char g_rcpath[MAX_PATH];
bool new_folder (const char *name, const char *path);

HWND hwnd_now_active;
HWND hwnd_last_active;

// ----------------------------------------------
// Unspecific utility

void switchto_desk(int desk);
void activate_window(HWND taskwnd);
void zoom_window(HWND taskwnd);
void minimize_window(HWND taskwnd);
void close_window(HWND taskwnd);
void move_window_left(HWND taskwnd);
void move_window_right(HWND taskwnd);
void move_window(HWND taskwnd, int desk);
void send_window(HWND taskwnd, int desk);

void switchto_desk(int desk)
{
    if (BBP_bbversion() < 1160) {
        PostMessage(BBhwnd, BB_SWITCHTON, desk, 0);
    } else {
        PostMessage(BBhwnd, BB_WORKSPACE, BBWS_SWITCHTODESK, desk);
    }
}

void activate_window(HWND taskwnd)
{
    if (BBP_bbversion() < 2) {
        int desk = GetTaskWorkspace(taskwnd);
        PostMessage(BBhwnd, BB_SWITCHTON, desk, 0);
        PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)taskwnd);
    } else {
        PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)taskwnd);
    }

}

void zoom_window(HWND taskwnd)
{
    if (BBP_bbversion() < 2) {
        SetTaskWorkspace(taskwnd, currentDesk);
        PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)taskwnd);
    } else {
        PostMessage(BBhwnd, BB_BRINGTOFRONT, BBBTF_CURRENT, (LPARAM)taskwnd);
    }
}

void minimize_window(HWND taskwnd)
{
    if (BBP_bbversion() < 2) {
        PostMessage(taskwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    } else {
        PostMessage(BBhwnd, BB_WINDOWMINIMIZE, 0, (LPARAM)taskwnd);
    }

}

void close_window(HWND taskwnd)
{
    if (BBP_bbversion() < 2) {
        PostMessage(taskwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    } else {
        PostMessage(BBhwnd, BB_WINDOWCLOSE, 0, (LPARAM)taskwnd);
    }
}

void move_window_left(HWND taskwnd)
{
    PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWLEFT, (LPARAM)taskwnd);
}

void move_window_right(HWND taskwnd)
{
    PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWRIGHT, (LPARAM)taskwnd);
}

void move_window(HWND taskwnd, int desk)
{
    if (BBP_bbversion() < 2) {
        SetTaskWorkspace(taskwnd, desk);
        PostMessage(BBhwnd, BB_SWITCHTON, desk, 0);
        PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)taskwnd);
    } else if (BBP_bbversion() < 1170) {
        taskinfo ti;
        ti.desk = desk;
        SetTaskLocation(taskwnd, &ti, BBTI_SETDESK|BBTI_SWITCHTO);
        PostMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)taskwnd);
    } else {
        PostMessage(BBhwnd, BB_MOVEWINDOWTON, desk, (LPARAM)taskwnd);
    }
}

void send_window(HWND taskwnd, int desk)
{
    if (BBP_bbversion() < 2) {
        SetTaskWorkspace(taskwnd, desk);
        // PostMessage(BBhwnd, BB_SWITCHTON, currentDesk, 0);
    } else if (BBP_bbversion() < 1170) {
        taskinfo ti;
        ti.desk = desk;
        SetTaskLocation(taskwnd, &ti, BBTI_SETDESK);
    } else {
        PostMessage(BBhwnd, BB_SENDWINDOWTON, desk, (LPARAM)taskwnd);
    }
}

// ----------------------------------------------
// The icon_box class

struct icon_box : public plugin_info
{
    #define FIRSTITEM m_index

    // box properties
    int m_index;
    char m_szInstName [40];
    char m_name [40];
    char m_title [80];
    Folder my_Folder;

    // settings
    int rows;
    int columns;
    int iconWidth;
    int iconHeight;
    int saturationValue;
    int hueIntensity;
    bool drawBorder;
    bool drawTitle;
    bool toolTips;

    // runtime variables
    int titleHeight;
    int titleBorder;
    int frameBorder;
    int frameMargin;
    int iconPadding;
    int folderSize;

    int activeIcon;
    int capturedIcon;
    int mouse_in;
    int activeTemp;
    bool is_single_task;
    bool no_items;

    bool dragging;
    bool capture;
    POINT mouse_down;

    HBITMAP my_bmp;

    struct rc { const char *key; int m; void *v; void *d; };
    struct rc * m_rc;

    #define M_BOL 1
    #define M_INT 2
    #define M_STR 3

    // class methods:
    icon_box()
    {
        BBP_clear(this, FIRSTITEM);

        struct rc tmp_rc [] = {
        { "path",             M_STR, my_Folder.path    , (void*)0     },
        { "title",            M_STR, m_title           , (void*)0     },
        { "rows",             M_INT, &rows             , (void*)4     },
        { "columns",          M_INT, &columns          , (void*)4     },
        { "icon.size",        M_INT, &iconWidth        , (void*)16    },
        { "icon.saturation",  M_INT, &saturationValue  , (void*)80    },
        { "icon.hue",         M_INT, &hueIntensity     , (void*)60    },
        { "drawTitle",        M_BOL, &drawTitle        , (void*)false },
        { "drawBorder",       M_BOL, &drawBorder       , (void*)true  },
        { "toolTips",         M_BOL, &toolTips         , (void*)true  },
        { NULL, 0, NULL, 0 }
        };

        m_rc = (struct rc*)m_alloc(sizeof tmp_rc);
        memcpy(m_rc, tmp_rc, sizeof tmp_rc);

        icon_box **pp = &g_PI;
        while (*pp) pp = (icon_box**)&(*pp)->next;
        *pp = this;
        this->next = NULL;
    }

    ~icon_box();

    void about_box()
    {
        BBP_messagebox(this, MB_OK, "%s - © %s %s\n", szVersion, szCopyright, szInfoEmail);
    }

    LRESULT wnd_proc(HWND hwnd, UINT message,
        WPARAM wParam, LPARAM lParam, LRESULT *ret);
    void GetIconRect(int i, LPRECT rect);
    void GetStyleSettings();
    bool GetRCSettings(void);
    void Paint(HDC hdc);
    void show_menu(bool f);
    void write_rc(void *v);
    void common_broam(const char *temp);

    int CheckMouse(POINT mousepos);

    void CalcMetrics(int*, int*);
    void UpdateMetrics();
    void process_broam(const char *temp, int f);
    void invalidate(void);
    void register_msg(bool);

    void paint_background(HDC hdc);
    void paint_icon(HDC hdc, Item *icon, bool active, bool sunken);
    void paint_text(HDC hdc, RECT *r, StyleItem *pSI, StyleItem *pTC,
        bool centerd, const char *text);

    void LoadFolder();
    void ClearFolder();

    virtual void FillFolder() {}
    Item *GetFolderIcon(int n);

    int MouseEvent(HWND hwnd, unsigned message,
        WPARAM wParam, LPARAM lParam, int action);

    virtual int MouseAction(HWND hwnd, unsigned message,
        WPARAM wParam, POINT *p, int index) { return 0; }

    int get_other_desk(void);
    virtual LPCITEMIDLIST do_dragover(Item *icon) { return NULL; }
};

// ----------------------------------------------
// destructor

icon_box::~icon_box()
{
    // dbg_printf("delete %s", m_name);

    ClearFolder();
    if (my_Folder.drop_target)
        exit_drop_targ(my_Folder.drop_target);

    if (hwnd)
    {
        ClearToolTips(hwnd);
        BBP_Exit_Plugin(this);
    }

    if (my_bmp)
        DeleteObject(my_bmp);

    m_free(m_rc);
    struct icon_box **p = &g_PI; int n = 0;
    while (*p)
    {
        if (this == *p)
            *p = (struct icon_box*)(*p)->next;
        else
        {
            (*p)->m_index = n++;
            p = (struct icon_box**)&(*p)->next;
        }
    }

    --box_count;
}

// ----------------------------------------------
void write_boxes(void)
{
    char rckey[80];
    sprintf(rckey, "%s.id.count:", szAppName);
    WriteInt(g_rcpath, rckey, box_count);

    struct icon_box *p = g_PI; int n = 0;
    while (p)
    {
        sprintf(rckey, "%s.id.%d:", szAppName, ++n);
        WriteString(g_rcpath, rckey, p->m_name);
        p = (struct icon_box *)p->next;
    }
}


// ----------------------------------------------
void icon_box::ClearFolder()
{
    ::ClearFolder(&this->my_Folder);
}

void icon_box::LoadFolder(void)
{
    ClearFolder();
    FillFolder();
    int n = 0;
    Item *p = my_Folder.items;
    while (p) p->index = n++, p = p -> next;
    folderSize = n;
}

Item *icon_box::GetFolderIcon(int n)
{
    Item *p; int i;
    if (n < 0)
        return NULL;
    for (p = my_Folder.items, i = 0; i < n && p; i++, p = p -> next);
    return p;
}

int icon_box::get_other_desk(void)
{
    POINT pt;
    GetCursorPos(&pt);
    HWND other = WindowFromPoint(pt);
    icon_box *p;

    for (p = g_PI; p; p = (icon_box*)p->next)
        if (p->hwnd == other)
            break;

    if (NULL == p)
        return 0;

    if (p->my_Folder.mode == MODE_TASK)
        return p->my_Folder.desk;

    if (p->my_Folder.mode == MODE_PAGER) {
        ScreenToClient(other, &pt);
        int n = p->CheckMouse(pt);
        if (n > 0)
            return n;
    }

    return 0;
}

// ----------------------------------------------
BOOL tray_enum_func(const systemTray *icon, LPARAM lParam)
{
    Item * item = new Item;
    *(Item ***)lParam = &(**(Item ***)lParam=item)->next;
    item->next = NULL;
    item->data = NULL;
    item->hIcon = icon->hIcon;
    strcpy_max(item->szTip, icon->szTip, sizeof item->szTip);
    item->is_folder = false;
    return TRUE;
}

struct task_enum { Item **pItems; int desk; int icon_size; };

BOOL task_enum_func(const tasklist *icon, LPARAM lParam)
{
    task_enum *te = (task_enum*)lParam;
    if (te->desk == 0 || icon->wkspc + 1 == te->desk)
    {
        Item * item = new Item;
        te->pItems = &(*te->pItems=item)->next;
        item->next = NULL;
        item->data = icon->hwnd;
        strcpy_max(item->szTip, icon->caption, sizeof item->szTip);
        item->is_folder = false;
        item->active = icon->active || icon->flashing;

        item->hIcon = icon->icon;
        if (te->icon_size > 24)
        {
            HICON hIco = NULL;
            HWND hwnd = icon->hwnd;
            SendMessageTimeout(hwnd, WM_GETICON,
                ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_BLOCK, 300, (DWORD_PTR*)&hIco);
            if (NULL==hIco)
                hIco = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
            if (hIco)
                item->hIcon = hIco;
        }

        if (NULL == item->hIcon)
            item->hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    return TRUE;
}

BOOL desk_enum_func(const DesktopInfo *icon, LPARAM lParam)
{
    Item * item = new Item;
    *(Item ***)lParam = &(**(Item ***)lParam=item)->next;
    item->next = NULL;
    item->data = (void*)icon->number;
    item->hIcon = NULL;
    item->is_folder = false;
    item->active = icon->isCurrent;
    strcpy_max(item->szTip, icon->name, sizeof item->szTip);
    return TRUE;
}

// ----------------------------------------------
struct tray_box : icon_box
{
    tray_box()
    {
        my_Folder.mode = MODE_TRAY;
    }

    void FillFolder(void)
    {
        Item **pItems = &my_Folder.items;
        EnumTray(tray_enum_func, (LPARAM)&pItems);
    }

    int MouseAction(HWND hwnd, unsigned message, WPARAM wParam, POINT *p, int index)
    {
        if (index > 0)
        {
            systemTrayIconPos pos;
            pos.hwnd = this->hwnd;
            GetIconRect(index-1, &pos.r);
            ForwardTrayMessage(index-1, message, &pos);
        }
        return 0;
    }
};

// ----------------------------------------------
struct task_box : icon_box
{
    task_box(int n)
    {
        my_Folder.mode = MODE_TASK;
        my_Folder.desk = n;
        is_single_task = n > 0;
    }

    void FillFolder(void)
    {
        DesktopInfo DI;
        GetDesktopInfo (&DI);
        currentDesk = DI.number;

        task_enum te = { &my_Folder.items, my_Folder.desk, iconWidth };
        EnumTasks(task_enum_func, (LPARAM)&te);
    }

    LPCITEMIDLIST do_dragover(Item *icon)
    {
        HWND task = NULL;
        if (icon)
            task = (HWND)icon->data;

        if (my_Folder.task_over != task) {
            my_Folder.task_over = task;
            if (task)
                SetTimer(hwnd, TASK_RISE_TIMER, 200, 0);
        }
        return NULL;
    }

    int MouseAction(HWND hwnd, unsigned int message, WPARAM wParam, POINT *p, int index)
    {
        Item *item;
        int desk;

        if (BBIB_DRAG == message)
        {
            desk = get_other_desk();
            if (desk > 0)
                SetCursor(LoadCursor(hInstance, (LPCSTR)(wParam & MK_RBUTTON?102:101)));
            else
                SetCursor(LoadCursor(NULL, IDC_NO));
            return desk > 0;
        }

        if (0 == index) {
            // just switch workspace when clicked outside icon
            if (my_Folder.desk && message == WM_LBUTTONUP && capture)
                switchto_desk(my_Folder.desk-1);
            return 0;
        }

        item = GetFolderIcon(index-1);
        if (NULL == item)
            return 0;

        HWND taskwnd = (HWND)item->data;
        bool shift_pressed = 0 != (wParam & MK_SHIFT);
        bool sysmenu = false;

        switch (message)
        {
            //====================

            case BBIB_PICK:
                if (wParam & (MK_LBUTTON|MK_RBUTTON))
                    dragging = true;
                return dragging;

            case BBIB_DROP:
                desk = get_other_desk();
                if (desk > 0) {
                    --desk;
                    //if (wParam == WM_LBUTTONUP)
                    if (taskwnd == hwnd_last_active)
                        move_window(taskwnd, desk);
                    else
                        send_window(taskwnd, desk);
                }
                break;

            // Restore and focus window
            case WM_LBUTTONUP:
                if (sysmenu
                    && item->active
                    && !IsIconic(taskwnd)) {
                    minimize_window(taskwnd);
                    break;
                }

                if (shift_pressed)
                    zoom_window(taskwnd);
                else
                    activate_window(taskwnd);
                SetTimer(hwnd, TASK_ACTIVATE_TIMER, 200, NULL);
                activeTemp = index;
                break;

            //====================
            // Minimize (iconify) window

            case WM_RBUTTONUP:
                if (sysmenu)
                    ;//ShowSysmenu(taskwnd, hBBSystembarWnd);
                else
                if (shift_pressed)
                    close_window(taskwnd);
                else
                    minimize_window(taskwnd);
                break;

            //====================
            // Move window to the next/previous workspace

            case WM_MBUTTONUP:
                if (shift_pressed)
                    move_window_left(taskwnd);
                else
                    move_window_right(taskwnd);
                activate_window(taskwnd);
                break;
        }
        return 0;
    }
};

// ----------------------------------------------
#ifdef INCLUDE_MODE_PAGER
struct desk_box : icon_box
{
    winStruct W;
    int dx, dy;
    Desk my_Desk;


    desk_box()
    {
        my_Folder.mode = MODE_PAGER;
        W.hwnd = NULL;
        memset(&my_Desk, 0, sizeof my_Desk);
    }

    ~desk_box()
    {
        free_winlist(&my_Desk);
    }

    void FillFolder(void)
    {
        Item **pItems = &my_Folder.items;
        EnumDesks(desk_enum_func, (LPARAM)&pItems);
    }

    int MouseAction(HWND hwnd, unsigned int message, WPARAM wParam, POINT *p, int index)
    {
        int desk;
        switch(message)
        {
            case BBIB_PICK:
                if (wParam & (MK_LBUTTON|MK_RBUTTON)) {
                    if (W.hwnd)
                        dragging = true;
                }
                return dragging;

            case BBIB_DRAG:
                desk = get_other_desk();
                if (desk > 0) {
                    if (index < 0)
                        SetCursor(LoadCursor(hInstance, (LPCSTR)(wParam & MK_RBUTTON?102:101)));
                    else
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                } else {
                    if (index < 0 || p->y < titleHeight)
                        SetCursor(LoadCursor(NULL, IDC_NO));
                    else
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                }

                dx = p->x - mouse_down.x;
                dy = p->y - mouse_down.y;
                InvalidateRect(hwnd, NULL, FALSE);
                return desk > 0;

            case BBIB_DROP:
                desk = get_other_desk();
                if (desk > 0) {
                    --desk;
                    //if (wParam == WM_LBUTTONUP)
                    if (W.hwnd == hwnd_last_active)
                        move_window(W.hwnd, desk);
                    else
                        send_window(W.hwnd, desk);
                }
                return 0;
        }

        winStruct *w = NULL;
        desk = -1;

        if (index > 0)
        {
            Item *item = GetFolderIcon(index-1);
            RECT iconRect, r;
            winStruct *w2;
            int n;

            GetIconRect(index-1, &iconRect);
            desk = (DWORD_PTR)item->data;

            for (n = 0; n < my_Desk.winCount; n++) {
                w2 = get_winstruct(&my_Desk, n);
                if (w2->info.desk == desk) {
                    get_virtual_rect(&my_Desk, &iconRect, w2, &r);
                    if (PtInRect(&r, *p)) {
                        w = w2;
                        break;
                    }
                }
            }
        }

        switch(message)
        {
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                if (w) W = *w;
                else W.hwnd = NULL;
                break;

            case WM_RBUTTONUP:
            case WM_LBUTTONUP:
                if (w)
                    activate_window(w->hwnd);
                else if (desk >= 0)
                    switchto_desk(desk);
                W.hwnd = NULL;
                break;
        }
        return 0;
    }

    void draw_windows(HDC hdc, Item *icon, RECT *iconRect, StyleItem *N, StyleItem *A, bool active)
    {
        StyleItem *T, *U;
        RECT r, d;
        int n, desk;
        winStruct *w;

        if (icon->active)
            T = A, U = N;
        else
            T = N, U = A;

        desk = (DWORD_PTR)icon->data;

        if (iconWidth >= 16)
        {
            for (n = my_Desk.winCount; n--;) {
                w = get_winstruct(&my_Desk, n);
                if (w->info.desk == desk) {
                    get_virtual_rect(&my_Desk, &*iconRect, w, &r);
                    if (w->iconic)
                        CreateBorder(hdc, &r, U->borderColor, 1);
                    else
                        MakeStyleGradient(hdc, &r, U, U->bordered);
                }
            }

            if (active && dragging) {
                w = &W;
                GetIconRect(w->info.desk, &d);
                get_virtual_rect(&my_Desk, &d, w, &r);
                OffsetRect(&r, dx, dy);
                MakeStyleGradient(hdc, &r, A, A->bordered);
            }
        }

        char text[20];
        sprintf(text, "%d", desk + 1);
        paint_text(hdc, &*iconRect, N, T, true, text);
    }

};
#endif

// ----------------------------------------------
struct folder_box : icon_box
{
    folder_box()
    {
        my_Folder.mode = MODE_FOLDER;
    }

    void FillFolder(void)
    {
        ::LoadFolder(&this->my_Folder, this->iconWidth, this->hwnd);
    }

    LPCITEMIDLIST do_dragover(Item *icon)
    {
        if (icon)
            return icon->data ? first_pidl((struct pidl_node*)icon->data) : NULL;
        if (my_Folder.pidl_list)
            return first_pidl(my_Folder.pidl_list);
        return NULL;
    }

    int MouseAction(HWND hwnd, unsigned int message, WPARAM wParam, POINT *p, int index)
    {
        Item *icon = GetFolderIcon(index-1);
        if (icon)
        {
            LPCITEMIDLIST pidl = NULL;
            if (icon->data)
                pidl = first_pidl((struct pidl_node*)icon->data);

            if (message == WM_LBUTTONUP) {
                exec_pidl(pidl, NULL);
            }

            if (message == WM_RBUTTONUP) {
                Menu * m = MakeContextMenu(NULL, pidl);
                BBP_showmenu(this, m, true|BBMENU_CORNER);
            }

        } else if (0 == index) {
            if (message == WM_RBUTTONUP && this->my_Folder.pidl_list) {
                Menu * m = MakeContextMenu(NULL, first_pidl(this->my_Folder.pidl_list));
                BBP_showmenu(this, m, true|BBMENU_CORNER);
            }
        }
        return 0;
    }

};

// ----------------------------------------------
// Get info about the item under mouse while in a
// drag & drop operation

LPCITEMIDLIST indrag_get_pidl(HWND hwnd, POINT *pt)
{
    icon_box *DI = (icon_box *)GetWindowLongPtr(hwnd, 0);
    POINT p = *pt;
    ScreenToClient(hwnd, &p);
    SendMessage(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(p.x, p.y));

    int index = DI->CheckMouse(p);
    Item *icon = index ? DI->GetFolderIcon(index-1) : NULL;

    return DI->do_dragover(icon);
}

void handle_task_timer(HWND task_over)
{
    if (NULL == task_over)
        return;
    if (task_over == GetTask(GetActiveTask()))
        return;
    DWORD ThreadID1 = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD ThreadID2 = GetCurrentThreadId();
    if (ThreadID1 != ThreadID2) {
        AttachThreadInput(ThreadID1, ThreadID2, TRUE);
        SetForegroundWindow(BBhwnd);
        AttachThreadInput(ThreadID1, ThreadID2, FALSE);
    }
    activate_window(task_over);
}

// ----------------------------------------------
// New bitmap required...

void icon_box::invalidate()
{
    if (my_bmp) DeleteObject(my_bmp), my_bmp = NULL;
    InvalidateRect(this->hwnd, NULL, FALSE);
}

// ----------------------------------------------
// Function: CalcMetrics

void icon_box::CalcMetrics(int *w, int *h)
{
    iconHeight = iconWidth;
    no_items = false;
    columns = imax(1, columns);
    rows = imax(1, rows);

#ifdef INCLUDE_MODE_PAGER
    if (MODE_PAGER == my_Folder.mode)
    {
        Desk *f = &((desk_box*)this)->my_Desk;
        if (iconWidth >= 24)
            iconHeight = iconWidth * 3 / 4;
        setup_ratio(f, this->hwnd, iconWidth, iconHeight);
        build_winlist(f, this->hwnd);
    }
#endif

    int spacing_x = 2 * frameMargin - iconPadding;
    int spacing_y = spacing_x;
    int icon_dist_x = iconWidth + iconPadding;
    int icon_dist_y = iconHeight + iconPadding;
    int xn, yn, fn = folderSize;

    if (0 == fn) {
#if 0
        if (titleHeight) {
            spacing_y = 2*frameBorder-titleBorder;
            no_items = true;
        } else
#endif
            fn = 1;
    }

    if (this->orient_vertical) {
        xn = columns;
        yn = (fn + columns - 1)/columns;
    } else {
        yn = rows;
        xn = (fn + rows - 1)/rows;
    }

    *w = xn * icon_dist_x + spacing_x;
    *h = yn * icon_dist_y + spacing_y + titleHeight;
}

// ----------------------------------------------
// Function: UpdateMetrics

void icon_box::UpdateMetrics()
{
    int w, h;
    CalcMetrics(&w, &h);
    invalidate();
    BBP_set_size(this, w, h);
}

// ----------------------------------------------
// Function: GetIconRect

void icon_box::GetIconRect(int i, LPRECT rect)
{
    int is_x = iconWidth;
    int is_y = iconHeight;
    int icon_dist_x = is_x + iconPadding;
    int icon_dist_y = is_y + iconPadding;

    int row, col;

    if (false == orient_vertical)
        col = i / rows, row = i % rows;
    else
        row = i / columns, col = i % columns;

    rect->left   = frameMargin + col * icon_dist_x;
    rect->top    = frameMargin + row * icon_dist_y + titleHeight;
    rect->right  = rect->left + is_x;
    rect->bottom = rect->top  + is_y;
}

// ----------------------------------------------
// Function: CheckMouse

int icon_box::CheckMouse(POINT mousepos)
{
    RECT r;
    int i, n;

    for (i = 0, n = folderSize; i < n; i++) {
        GetIconRect(i, &r);
        if (PtInRect(&r, mousepos))
            return i+1;
    }

    r.left = r.top = 0;
    r.right = width;
    r.bottom = height;

    if (PtInRect(&r, mousepos))
        return 0;

    return -1;
}

// ----------------------------------------------
// Function: Paint

void icon_box::paint_text(HDC hdc, RECT *r, StyleItem *pSI, StyleItem *pTC, bool centerd, const char *text)
{
     SetTextColor(hdc, pTC->TextColor);
     SetBkMode(hdc, TRANSPARENT);
     HFONT hf = CreateStyleFont(pSI);
     HGDIOBJ oldfont = SelectObject(hdc, hf);
     int just = DT_CENTER;

     if (false == centerd)
     {
         RECT s = {0,0,0,0};
         DrawText(hdc, text, -1, &s, DT_LEFT|DT_EXPANDTABS|DT_CALCRECT);
         if (s.right < r->right - r->left)
             just = pSI->Justify;
         else
             just = DT_LEFT;
     }

     DrawText(hdc, text, -1, r, just | DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS);
     DeleteObject (SelectObject(hdc, oldfont));
}

// ----------------------------------------------
void icon_box::paint_icon(HDC hdc, Item *icon, bool active, bool sunken)
{
    RECT iconRect;
    RECT r;
    GetIconRect(icon->index, &iconRect);

    StyleItem *A, *N;
    if (titleHeight) {
        N = (StyleItem*)GetSettingPtr(SN_MENUFRAME);
        A = (StyleItem*)GetSettingPtr(SN_MENUHILITE);
    } else {
        N = (StyleItem*)GetSettingPtr(SN_TOOLBAR);
        A = (StyleItem*)GetSettingPtr(SN_TOOLBARWINDOWLABEL);
    }

    if (sunken)
    {
        StyleItem SI, *H;
        SI = *A, H = &SI;
        const int d = 2;

        r = iconRect;
        InflateRect(&r, d, d);

        if (false == H->parentRelative)
        {
            H->bevelposition = BEVEL1;
            H->bevelstyle = BEVEL_SUNKEN;
            MakeStyleGradient(hdc, &r, H, false);
        }
        else
        {
            CreateBorder(hdc, &r, N->borderColor, d);
        }
    }

    if (icon->hIcon)
    {
        DrawIconSatnHue(hdc, iconRect.left, iconRect.top, icon->hIcon, iconWidth, iconWidth, 0, NULL, DI_NORMAL, false == active, saturationValue, hueIntensity);
#if 0
        if (active && ! sunken) {
            const int d = 1;
            r = iconRect;
            InflateRect(&r, d, d);
            CreateBorder(hdc, &r, N->borderColor, 1);
        }
#endif
    }
#ifdef INCLUDE_MODE_PAGER
    else
    if (MODE_PAGER == my_Folder.mode)
    {
        ((desk_box*)this)->draw_windows(hdc, icon, &iconRect, N, A, active);
    }
#endif
}

// ----------------------------------------------
void icon_box::Paint(HDC hdc)
{
    int i, n;
    Item *icon;

    icon = my_Folder.items;
    for (i = 0, n = folderSize; i < n && icon; i++, icon = icon->next)
    {
        paint_icon(hdc, icon, false, false);
        if (false == toolTips)
            continue;

        RECT iconRect;
        GetIconRect(i, &iconRect);
        iconRect.right  += iconPadding;
        iconRect.bottom += iconPadding;
        SetToolTip(this->hwnd, &iconRect, icon->szTip);

        if (MODE_TRAY == my_Folder.mode)
            make_bb_balloon(this, GetTrayIcon(i), &iconRect);
    }
}

// ----------------------------------------------
// Function: MouseEvent

int icon_box::MouseEvent(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, int action)
{
    POINT pt;
    int index, active;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);
    index = CheckMouse(pt);

    if (index != mouse_in) {
        if (mouse_in > 0)
            this->MouseAction(hwnd, WM_MOUSELEAVE, wParam, &pt, mouse_in);
        mouse_in = index;
    }

    switch (action) {
    case 0: // down
        capturedIcon = index;
        if (my_Folder.mode != MODE_PAGER)
            activeIcon = index;
        mouse_down = pt;
        this->MouseAction(hwnd, message, wParam, &pt, index);
        break;

    case 1: // move
        active = index;
        if (MK_CONTROL & wParam) {
            active = 0;
        } else if (my_Folder.mode == MODE_TASK) {
            if (this->capturedIcon)
                active = this->capturedIcon;
        } else if (my_Folder.mode == MODE_PAGER) {
            active = 0;
        } else if (capturedIcon && index != capturedIcon) {
            active = 0;
        }

        if (active != this->activeIcon) {
            this->activeIcon = active;
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (dragging) {
            message = BBIB_DRAG;

        } else if (capturedIcon) {

            int dx = mouse_down.x - pt.x;
            int dy = mouse_down.y - pt.y;
            if (dx*dx + dy*dy > 50) {
                message = BBIB_PICK;
                index = capturedIcon;
            }
        }
        this->MouseAction(hwnd, message, wParam, &pt, index);
        break;

    case 2: // up
        if (dragging) {
            dragging = false;
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            wParam = message;
            message = BBIB_DROP;
            index = capturedIcon;
        } else if (index != capturedIcon)
            break;
        this->MouseAction(hwnd, message, wParam, &pt, index);
        break;

    case 3: // leave
        break;

    default:
        return 0;

    }
    return index;
}

// ----------------------------------------------
// Function: GetStyleSettings

void icon_box::GetStyleSettings()
{
    // Get the path to the current style file.
    StyleItem *Frame;

    if (drawTitle)
    {
        StyleItem *MTitle = (StyleItem*)GetSettingPtr(SN_MENUTITLE);
        HFONT hFont = CreateStyleFont(MTitle);
        int tfh = get_fontheight(hFont);
        DeleteObject(hFont);

        titleBorder = MTitle->borderWidth;

        void *p = GetSettingPtr(SN_ISSTYLE070);
        bool is_style070 = HIWORD(p) ? *(bool*)p : NULL!=p;

        if (MTitle->nVersion < 4 || false == is_style070)
            tfh = MTitle->FontHeight - 2;

        titleHeight = tfh + 2*MTitle->marginWidth + titleBorder;

        Frame = (StyleItem*)GetSettingPtr(SN_MENUFRAME);
    } else {
        titleHeight = titleBorder = 0;
        Frame = (StyleItem*)GetSettingPtr(SN_TOOLBAR);
    }

    frameBorder = drawBorder ? Frame->borderWidth : 0;
    frameMargin = frameBorder + Frame->marginWidth + 2;
    iconPadding = 3;
}

// ----------------------------------------------
// Function: GetRCSettings

void icon_box::write_rc(void *v)
{
    struct rc *p = m_rc;
    do
    {
        if (NULL == v || p->v == v) switch (p->m)
        {
            case M_BOL: BBP_write_bool(this, p->key, *(bool*)p->v); break;
            case M_INT: BBP_write_int(this, p->key, *(int*)p->v); break;
            case M_STR: BBP_write_string(this, p->key, (char*)p->v); break;
        }
    } while ((++p)->key);

}
bool icon_box::GetRCSettings(void)
{
    struct rc *p = m_rc;
    do
    {
        switch (p->m)
        {
            case M_BOL: *(bool*)p->v = BBP_read_bool(this, p->key, 0 != (int)(DWORD_PTR)p->d); break;
            case M_INT: *(int*)p->v = BBP_read_int(this, p->key, (int)(DWORD_PTR)p->d); break;
            case M_STR: BBP_read_string(this, (char*)p->v, p->key, (const char*)p->d); break;
        }
    } while ((++p)->key);

    if (is_single_task)
        drawBorder = true;
    BBP_read_window_modes(this, szAppName);

    return true;
}

// ----------------------------------------------
void icon_box::process_broam(const char *temp, int f)
{
    const char *rest;

    if (f & BBP_BROAM_HANDLED)
    {
        if (f & BBP_BROAM_METRICS)
            UpdateMetrics();
        show_menu(false);
        return;
    }

    if (f == BBP_BROAM_COMMON) {
        common_broam(temp);
        return;
    }

    if (BBP_broam_bool(this, temp, "drawBorder", &drawBorder))
    {
        GetStyleSettings();
        UpdateMetrics();
        show_menu(false);
        return;
    }

    if (BBP_broam_bool(this, temp, "toolTips", &toolTips))
    {
        UpdateMetrics();
        show_menu(false);
        return;
    }

    if (BBP_broam_bool(this, temp, "drawTitle", &drawTitle))
    {
        GetStyleSettings();
        UpdateMetrics();
        show_menu(false);
        return;
    }

    if (BBP_broam_int(this, temp, "rows", &rows))
    {
        UpdateMetrics();
        return;
    }

    if (BBP_broam_int(this, temp, "columns", &columns))
    {
        UpdateMetrics();
        return;
    }

    if (BBP_broam_int(this, temp, "icon.hue", &hueIntensity))
    {
        UpdateMetrics();
        return;
    }

    if (BBP_broam_int(this, temp, "icon.saturation", &saturationValue))
    {
        UpdateMetrics();
        return;
    }

    if (BBP_broam_int(this, temp, "icon.size", &iconWidth))
    {
        LoadFolder();
        UpdateMetrics();
        return;
    }

    if (BBP_broam_string(this, temp, "path", &rest))
    {
        strcpy(my_Folder.path, rest);
        LoadFolder();
        UpdateMetrics();
        show_menu(false);
        return;
    }

    if (BBP_broam_string(this, temp, "title", &rest))
    {
        strcpy(m_title, rest);
        UpdateMetrics();
        show_menu(false);
        return;
    }

    if (!stricmp(temp, "remove"))
    {
        PostMessage(hwnd, BBIB_DELETE, 0, 0);
        return;
    }
}

// ----------------------------------------------
void icon_box::common_broam(const char *temp)
{
    char buffer[MAX_PATH];
    char name[MAX_PATH];
    char path[MAX_PATH];
    int n, x;
    const char *rest;
    plugin_info *p;

    path[0] = 0;

    if (BBP_broam_string(NULL, temp, "remove", &rest)) {
        if (g_PI && g_PI->next)
            dolist (p, g_PI)
                if (0 == stricmp(((icon_box*)p)->m_name, rest)) {
                    PostMessage(p->hwnd, BBIB_DELETE, 0, 0);
                    break;
                }

    } else if (BBP_broam_string(NULL, temp, "create.set", &rest)) {
        strcpy(create_path, rest);

    } else if (0 == stricmp(temp, "create.path")) {
        strcpy(path, create_path);

    } else if (0 == stricmp(temp, "create.browse")) {
        GetBlackboxPath(path, sizeof path);
        if (false == select_folder(NULL, szAppName, name, path))
            path[0] = 0;

    } else if (BBP_broam_string(NULL, temp, "create", &rest)) {
        strcpy(path, rest);
    }

    if (path[0]) {
        if (0 == strcmp(path, "TRAY")
            || 0 == memcmp(path, "TASK", 4)
            || 0 == strcmp(path, "PAGER")
            ) {
            strlwr(strcpy(name, path));

        } else {
            struct pidl_node *pidl_list = get_folder_pidl_list(path);
            if (NULL == pidl_list) {
                sprintf(buffer, "Invalid Path: %s", path);
                BBP_messagebox(this, MB_OK, buffer, szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
                return;
            }
            sh_getdisplayname(first_pidl(pidl_list), name);
            delete_pidl_list (&pidl_list);
        }

        n = strlen(name);
        x = 1;

        for (;;) {
            dolist (p, g_PI)
                if (0 == stricmp(((icon_box*)p)->m_name, name)) break;
            if (NULL == p) break;
            sprintf(name + n, "/%d", ++x);
        }

        new_folder(name, path);
        write_boxes();
    }

    dolist (p, g_PI)
        PostMessage(p->hwnd, BBIB_UPDATE, 0, 0);
}

// ----------------------------------------------
// Function: show_menu

void icon_box::show_menu(bool popup)
{
    n_menu *main, *sub, *sub2;
    char b1[80], b2[80];
    DesktopInfo D;
    int n;

    main = n_makemenu(m_name);
    BBP_n_orientmenu(this, main);
    if (orient_vertical) n_menuitem_int(main, "Columns", "columns", columns, 1, 32);
    else n_menuitem_int(main, "Rows", "rows", rows, 1, 32);
    n_menuitem_int(main, "Icon Size", "icon.size", iconWidth, 12, 64);
    n_menuitem_bol(main, "Draw Title", "drawTitle", drawTitle);
    if (drawTitle) n_menuitem_str(main, "Title Text", "title", m_title);
    if (this->my_Folder.mode == MODE_FOLDER)
        n_menuitem_str(main, "Path", "Path", this->my_Folder.path);
    n_menuitem_nop(main, NULL);

    sub = n_submenu(main, "Configuration");
    BBP_n_insertmenu(this, sub);
    n_menuitem_nop(sub, NULL);
    if (false == is_single_task)
        n_menuitem_bol(sub, "Border", "drawBorder", drawBorder);
    n_menuitem_bol(sub, "Tooltips", "toolTips", toolTips);
    n_menuitem_int(sub, "Saturation", "icon.saturation", saturationValue, 0, 255);
    n_menuitem_int(sub, "Hue", "icon.hue", hueIntensity, 0, 255);

    if (false == this->inSlit)
        BBP_n_placementmenu(this, main);
    sub = n_submenu(main, "New");
    sub2 = n_submenu(sub, "Folder");
    if (popup) create_path[0] = 0;

    if (BBVERSION_XOB) {
        n_menuitem_str(sub2, "Path", "@bbIconBox.create", "");
    } else {
        n_menuitem_str(sub2, "Path", "@bbIconBox.create.set", create_path);
        n_menuitem_cmd(sub2, "Create", "@bbIconBox.create.path");
        if (0 == create_path[0])
            n_disable_lastitem(sub2);
        n_menuitem_nop(sub2, NULL);
    }

    n_menuitem_cmd(sub2, "Desktop", "@bbIconBox.create DESKTOP");
    n_menuitem_cmd(sub2, "Quick Launch", "@bbIconBox.create APPDATA\\Microsoft\\Internet Explorer\\Quick Launch");
    n_menuitem_cmd(sub2, "Browse ...", "@bbIconBox.create.browse");

    sub2 = n_submenu(sub, "Task");
    n_menuitem_cmd(sub2, "All Workspaces", "@bbIconBox.create TASK");
    n_menuitem_nop(sub2, NULL);

    GetDesktopInfo(&D);
    for (n = 0; n < D.ScreensX; ++n) {
        sprintf(b1, "Workspace %d", 1+n);
        sprintf(b2, "@bbIconBox.create TASK%d", 1+n);
        n_menuitem_cmd(sub2, b1, b2);
    }
    n_menuitem_cmd(sub, "Tray", "@bbIconBox.create TRAY");
#ifdef INCLUDE_MODE_PAGER
    n_menuitem_cmd(sub, "Pager", "@bbIconBox.create PAGER");
#endif

#if 0
    n_menuitem_cmd(main, "Remove This", "remove");
    n_menuitem_nop(main, NULL);
#else
    sub = n_submenu(main, "Remove");
    plugin_info *p; dolist (p, g_PI) {
        const char *name = ((icon_box*)p)->m_name;
        if (*name) {
            sprintf(b2, "@bbIconBox.remove %s", name);
            n_menuitem_cmd(sub, name, b2);
        }
    }
#endif
    n_menuitem_nop(main, NULL);
    n_menuitem_cmd(main, "Edit Settings",  "@bbIconBox.editRC");
    n_menuitem_cmd(main, "About", "@bbIconBox.about");
    n_showmenu(this, main, popup, 0);
}

// ----------------------------------------------
void icon_box::register_msg(bool set)
{
    static UINT msg_common[] = { BB_REDRAWGUI, 0 };
    static UINT msg_tray[] = { BB_TRAYUPDATE, 0 };
    static UINT msg_task1[] = { BB_DESKTOPINFO, BB_TASKSUPDATE, 0 };
    static UINT msg_task2[] = { BB_DESKTOPINFO, BB_TASKSUPDATE, BB_WORKSPACE, BB_BRINGTOFRONT, 0 };
    UINT bb = set ? BB_REGISTERMESSAGE : BB_UNREGISTERMESSAGE;
    SendMessage(BBhwnd, bb, (WPARAM)hwnd, (LPARAM)msg_common);
    if (MODE_TASK == my_Folder.mode || MODE_PAGER == my_Folder.mode)
        SendMessage(BBhwnd, bb, (WPARAM)hwnd, (LPARAM)(BBVERSION_XOB?msg_task2:msg_task1));
    if (MODE_TRAY == my_Folder.mode)
        SendMessage(BBhwnd, bb, (WPARAM)hwnd, (LPARAM)msg_tray);

    if (NULL == my_Folder.drop_target)
        my_Folder.drop_target = init_drop_targ(hwnd);
}

void draw_line(HDC hDC, int x1, int x2, int y, int w, COLORREF C)
{
    if (w)
    {
        HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
        while (w--)
        {
            MoveToEx(hDC, x1, y, NULL);
            LineTo  (hDC, x2, y);
            ++y;
        }
        DeleteObject(SelectObject(hDC, oldPen));
    }
}

// ----------------------------------------------
void icon_box::paint_background(HDC hdc)
{
    RECT r = {0, 0, this->width, this->height };

    StyleItem * Frame = this->titleHeight
       ? (StyleItem*)GetSettingPtr(SN_MENUFRAME)
       : (StyleItem*)GetSettingPtr(SN_TOOLBAR)
       ;

    StyleItem *MT = (StyleItem*)GetSettingPtr(SN_MENUTITLE);

    int frame_border = frameBorder;
    int title_height = this->titleHeight;
    int title_border = this->titleBorder;

    if (no_items) {
        title_height += frame_border - title_border;
        title_border = frame_border;
    }

    if (this->is_single_task && this->my_Folder.desk-1 != currentDesk)
    {
        if (title_height)
            title_height += (no_items?2:1)*frame_border;
        frame_border = 0;
    }

    CreateBorder(hdc, &r, Frame->borderColor, frame_border);

    r.top = r.left = frame_border;
    r.right -= frame_border;
    r.bottom -= frame_border;
    if (title_height && false == MT->parentRelative)
        r.top += title_height;

    MakeStyleGradient(hdc, &r, Frame, false);

    if (title_height)
    {
        r.top = frame_border;
        r.bottom = r.top + title_height - title_border;
        if (false == MT->parentRelative)
            MakeStyleGradient(hdc, &r, MT, false);

        int margin = imax(Frame->marginWidth, 2);
        int lm = r.left + margin;
        int rm = r.right - margin;

        if (title_border && false == no_items) {
            if (MT->parentRelative)
                draw_line(hdc, lm, rm, r.bottom, title_border, MT->borderColor);
            else
                draw_line(hdc, r.left, r.right, r.bottom, title_border, MT->borderColor);
        }

        if (m_title[0]) {
            r.left = lm;
            r.right = rm;
            this->paint_text(hdc, &r, MT, MT, false, m_title);
        }
    }

    this->Paint(hdc);
    ClearToolTips(hwnd);
}

// ----------------------------------------------
LRESULT icon_box::wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret)
{
    if (ret)
        return *ret;

    int index;

    switch (message)
    {       

        //====================
        case WM_CREATE:
            this->register_msg(true);
            break;

        case WM_DESTROY:
            this->register_msg(false);
            break;

        //====================
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC buf = CreateCompatibleDC(NULL);
            HGDIOBJ other;

            if (NULL == this->my_bmp)
            {
                this->my_bmp = CreateCompatibleBitmap(hdc, this->width, this->height);
                other = SelectObject(buf, this->my_bmp);
                paint_background(buf);
            }
            else
            {
                other = SelectObject(buf, this->my_bmp);
            }

            HDC buf2 = CreateCompatibleDC(NULL);
            HGDIOBJ otherbmp = SelectObject(buf2, CreateCompatibleBitmap(hdc, this->width, this->height ));
            BitBltRect(buf2, buf, &ps.rcPaint);

            if (this->activeIcon)
            {
                Item *icon = GetFolderIcon(this->activeIcon-1);
                bool f
                    = this->capturedIcon == this->activeIcon
                    //&& this->my_Folder.mode != MODE_TRAY
                    ;
                if (icon)
                    this->paint_icon(buf2, icon, true, f);
            }

            if (this->my_Folder.mode == MODE_TASK
                || this->my_Folder.mode == MODE_PAGER)
            {
                Item *icon = this->my_Folder.items;
                int a = 0;
                while (icon) {
                    ++a;
                    if (icon->active || a == activeTemp)
                        this->paint_icon(buf2, icon, true, true);
                    icon = icon->next;
                }
            }

            BitBltRect(hdc, buf2, &ps.rcPaint);

            DeleteObject(SelectObject(buf2, otherbmp));
            DeleteDC(buf2);

            SelectObject(buf, other);
            DeleteDC(buf);

            EndPaint(hwnd, &ps);
        }
        break;

        //====================
        // xoblite workaround
        case BB_WORKSPACE:
        case BB_BRINGTOFRONT:
            PostMessage(hwnd, BB_TASKSUPDATE, 0, 0);
            break;

        //====================
        case BB_TASKSUPDATE:
            if (TASKITEM_ACTIVATED == lParam && wParam) {
                hwnd_last_active = hwnd_now_active;
                hwnd_now_active = (HWND)wParam;
            }
            new_task_list();
            goto update;

        case BB_DESKTOPINFO:
            hwnd_last_active = hwnd_now_active = NULL;
            goto update;

        case BB_TRAYUPDATE:
            goto update;

        update:
            this->LoadFolder();
            this->UpdateMetrics();
            break;

        //====================

        // If Blackbox sends a reconfigure message, load the new style settings and update window...
        case BB_RECONFIGURE:
            this->GetRCSettings();

        case BB_REDRAWGUI:
            this->GetStyleSettings();
            this->CalcMetrics(&this->width, &this->height);
            this->invalidate();
            BBP_reconfigure(this);
            break;

        //====================

        case WM_NCRBUTTONUP: // when in BBInterface frame
            this->show_menu(true);
            break;

        //====================
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            if (MK_CONTROL & wParam)
                break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
            goto down;

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            goto down;

        down:

            index = this->MouseEvent(hwnd, message, wParam, lParam, 0);
    #if 1
            if (message == WM_LBUTTONDOWN
                && index == 0
                && this->autoHide
                && false == this->inSlit
                && this->my_Folder.mode != MODE_TASK
                )
            {
                // Let drag the window when clicking into empty space
                PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
                break;
            }
    #endif
            if (this->my_Folder.mode != MODE_TRAY) {
                SetCapture(hwnd);
                capture = true;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
            if (WM_RBUTTONUP == message && MK_CONTROL & wParam) {
                this->show_menu(true);
            } else {
                this->MouseEvent(hwnd, message, wParam, lParam, 2);
            }

            if (capture) {
                ReleaseCapture();
                break;
            }

        case WM_CAPTURECHANGED:
            this->capturedIcon = 0;
            this->capture = false;
            this->dragging = false;
            InvalidateRect(hwnd, NULL, FALSE);
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;

        //====================

        case WM_MOUSEMOVE:
            this->MouseEvent(hwnd, message, wParam, lParam, 1);
            break;

        //====================
        case WM_MOUSELEAVE:
            this->MouseEvent(hwnd, message, wParam, lParam, 3);
            if (this->my_Folder.mode == MODE_TRAY)
                this->capturedIcon = 0;

            this->mouse_over = false;
            if (this->activeIcon && 0 == this->capturedIcon)
            {
                this->activeIcon = 0;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case WM_TIMER:

            if (TASK_RISE_TIMER == wParam)
            {
                KillTimer(hwnd, wParam);
                if (this->mouse_over)
                    handle_task_timer(this->my_Folder.task_over);
                break;
            }

            if (TASK_ACTIVATE_TIMER == wParam)
            {
                KillTimer(hwnd, wParam);
                activeTemp = 0;
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            break;

        case BBIB_DELETE:
            this->m_name[0] = 0;
            show_menu(false);
            delete this;
            write_boxes();
            break;

        case BBIB_UPDATE:
            show_menu(false);
            break;

        case BB_FOLDERCHANGED:
            PostMessage(hwnd, BB_TRAYUPDATE, 0, 0);
            break;

        case WM_KEYDOWN:
            if (VK_F5 == wParam)
            {
                PostMessage(hwnd, BB_TRAYUPDATE, 0, 0);
                break;
            }
            if (VK_ESCAPE == wParam)
            {
                ReleaseCapture();
                break;
            }
            break;

        default:
            return DefWindowProc(hwnd,message,wParam,lParam);

    //====================
    }
    return 0;
}

// ----------------------------------------------
bool new_folder (const char *name, const char *path)
{

    char instance_id[MAX_PATH];
    char box_path[MAX_PATH];
    char *q = instance_id, c; const char *p = name;
    do {
        c = *p++;
        if (' ' == c || '.' == c || ':' == c) *q++ = '+';
        else *q++ = c;
    } while (c);

    if (NULL == path) {
        char rckey[MAX_PATH];
        sprintf(rckey, "%s.%s.path:", szAppName, instance_id);
        path = strcpy(box_path, ReadString(g_rcpath, rckey, "DESKTOP"));
    }

    icon_box *IB;
    if (0 == strcmp(path, "TRAY")) {
        IB = new tray_box();
    } else if (0 == memcmp(path, "TASK", 4)) {
        IB = new task_box(path[4]?path[4]-'0':0);
#ifdef INCLUDE_MODE_PAGER
    } else if (0 == strcmp(path, "PAGER")) {
        IB = new desk_box();
#endif
    } else {
        IB = new folder_box();
    }

    sprintf(IB->m_szInstName, "%s.%s", szAppName, instance_id);

    strcpy(IB->m_name, name);
    strcpy(IB->m_title, name);
    strcpy(IB->my_Folder.path, path);
    strcpy(IB->rcpath, g_rcpath);

    IB->hSlit       = g_hSlit;
    IB->hInstance   = g_hInstance;
    IB->class_name  = szAppName;
    IB->rc_key      = IB->m_szInstName;
    IB->broam_key   = IB->m_szInstName;
    IB->m_index = box_count++;

    if (false == BBP_Init_Plugin(IB))
        return false;

    // Get plugin and style settings...
    IB->GetRCSettings();

    if (NULL == BBP_read_string(IB, NULL, "path", NULL))
        IB->write_rc(NULL);
    else
        BBP_write_string(IB, "path", path);

    IB->GetStyleSettings();
    IB->LoadFolder();
    IB->CalcMetrics(&IB->width, &IB->height);
    BBP_reconfigure(IB);

    return true;
}


int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit)
{
    if (BBhwnd)
    {
        BBP_messagebox(g_PI, MB_OK, "Dont load me twice!");
        return 1;
    }

    BBhwnd = GetBBWnd();
    const char *bbv = GetBBVersion();
    if (0 == memicmp(bbv, "bblean", 6)) BBVersion = BBVERSION_LEAN;
    else
    if (0 == memicmp(bbv, "bb", 2)) BBVersion = BBVERSION_XOB;
    else BBVersion = BBVERSION_09X;

    g_hSlit = hSlit;
    g_hInstance = hPluginInstance;

    InitToolTips(hPluginInstance);
    OleInitialize(NULL);

    BBP_get_rcpath(g_rcpath, g_hInstance, szAppName);

    char rckey[80];
    sprintf(rckey, "%s.id.count:", szAppName);
    int n = ReadInt(g_rcpath, rckey, 0);
    if (n < 1) WriteInt(g_rcpath, rckey, n = 1);

    box_count = 0;
    while (box_count < n)
    {
        char buffer[MAX_PATH];
        sprintf(rckey, "%s.id.%d:", szAppName, box_count+1);
        const char *p = ReadString(g_rcpath, rckey, NULL);
        if (NULL == p)
        {
            if (box_count) break;
            WriteString(g_rcpath, rckey, p = "Desktop");
        }

        if (false == new_folder(strcpy(buffer, p), NULL))
        {
            endPlugin(hPluginInstance);
            return 1;
        }
    }
    return 0;
}

int beginPlugin(HINSTANCE hPluginInstance)
{
    return beginPluginEx(hPluginInstance, NULL);
}

int beginPluginSlit(HINSTANCE hPluginInstance, HWND hw)
{
    return beginPluginEx(hPluginInstance, hw);
}

void endPlugin(HINSTANCE hPluginInstance)
{
    while (g_PI)
        delete g_PI;

    OleUninitialize();

    ExitToolTips();
    free_task_list();
}

// ----------------------------------------------

