/*
 ============================================================================
  This file is part of the bbSlit source code.

  bbSlit is a plugin for BlackBox for Windows
  Copyright © 2003-2009 grischka

  http://bb4win.sourceforge.net/bblean/

  bbSlit is free software, released under the GNU General Public License
  (GPL version 2). See for details:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/ // bbSlit.cpp

#include "BBApi.h"
#include "bblib.h"
#include "bbversion.h"
#include "BBSendData.h"
#include "bbPlugin.h"

const char szVersion      [] = "bbSlit "BBLEAN_VERSION;
const char szAppName      [] = "bbSlit";
const char szInfoVersion  [] = BBLEAN_VERSION;
const char szInfoAuthor   [] = "grischka";
const char szInfoRelDate  [] = BBLEAN_RELDATE;
const char szInfoLink     [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail    [] = "grischka@users.sourceforge.net";
const char szCopyright    [] = "2006-2009";

DLL_EXPORT LPCSTR pluginInfo(int field)
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

//=============================================================================

StyleItem m_style;
int padding;
int margin;
int bblean_version;
void getStyleSettings();

struct plugin_info *g_PI;

//===========================================================================
// structures

struct PluginInfo
{
    // the next window, or NULL
    struct PluginInfo *next;
    HWND hwnd;
    bool visible;

    // the window sizes.
    int width;
    int height;

    // the window position.
    int xpos;
    int ypos;

    // the window sizes to double-check
    int old_width;
    int old_height;
};

struct ModuleInfo
{
    struct ModuleInfo *next;
    HMODULE hMO;
    int (*beginSlitPlugin)(HINSTANCE hMainInstance, HWND hBBSlit);
    int (*beginPluginEx)(HINSTANCE hMainInstance, HWND hBBSlit);
    int (*endPlugin)(HINSTANCE hMainInstance);

    char name[100];
    char args[100];
};

struct slit_info : plugin_info
{
    #define FIRST_ITEM m_szInstName

    char m_szInstName [40];
    int n_inst;

    PluginInfo *m_pInfo;
    ModuleInfo *m_pMO;

    int bmp_width;
    int bmp_height;
    HBITMAP bufbmp;

    int alignment;
    int order;
    int baseWidth;
    bool setMargin;

    slit_info()
    {
        BBP_clear(this, FIRST_ITEM);
        this->next = g_PI;
        g_PI = this;
    }

    ~slit_info()
    {
        unloadPlugins();
        BBP_Exit_Plugin(this);
        struct plugin_info **pp;
        for (pp = &g_PI; *pp; pp = &(*pp)->next)
        {
            if (this == *pp) {
                *pp = this->next;
                break;
            }
        }
    }

    void about_box()
    {
        BBP_messagebox(this, MB_OK, "%s - © %s %s\n", szVersion, szCopyright, szInfoEmail);
    }

    void process_broam(const char *temp, int f);
    LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret);

    void calculate_frame(void);
    void position_windows(void);

    void getRCSettings(void);
    void show_menu (bool popit);

    void loadPlugins(FILE *fp);
    void unloadPlugins(void);
    void pos_changed(void);
};

//===========================================================================
DLL_EXPORT int beginPlugin(HINSTANCE hPluginInstance)
//DLL_EXPORT int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit)
{

    const char *bbv;
    int a,b,c;
    bbv = GetBBVersion();
    c = 0;
    bblean_version = sscanf(bbv, "bbLean %d.%d.%d", &a, &b, &c)
        >= 2 ? a*1000+b*10+c : 0;

    // dbg_printf("bblean_version %d", bblean_version);

    struct plugin_info *p;
    int n_inst;

    for (p = g_PI, n_inst = 0; p; p = p->next, ++n_inst);

    struct slit_info *PI = new slit_info();
    PI->n_inst = n_inst;
    sprintf(PI->m_szInstName, n_inst ? "%s.%d":"%s", szAppName, 1+n_inst);

    //PI->hSlit       = hSlit;
    PI->hInstance   = hPluginInstance;
    PI->class_name  = n_inst ? "BBSlit.X" : szAppName;
    PI->rc_key      = PI->m_szInstName;
    PI->broam_key   = PI->m_szInstName;

    PI->getRCSettings();

    if (false == BBP_Init_Plugin(PI))
    {
        delete PI;
        return BEGINPLUGIN_FAILED;
    }

    if (0 == n_inst)
    {
        getStyleSettings();
#if 0
        FILE *fp = FileOpen(plugrcPath());
        if (fp)
        {
            PI->loadPlugins(fp);
            FileClose(fp);
        }
#endif
    }
    else
    {

        PI->loadPlugins(NULL);
        PI->calculate_frame();
    }
    return BEGINPLUGIN_OK;
}


DLL_EXPORT void endPlugin(HINSTANCE hPluginInstance)
{
    if (g_PI) delete g_PI;
}

//=============================================================================
void pdbg (HWND window, char *msg)
{
    char buffer[256];
    GetClassName(window, buffer, 256);
    dbg_printf("%s %x <%s>", msg, window, buffer);
}

//=============================================================================
void slit_info::show_menu (bool pop)
{
    n_menu *sub, *menu;

    menu = n_makemenu(m_szInstName);
    BBP_n_placementmenu(this, menu);
    sub = n_submenu(menu, "Configuration");
    BBP_n_insertmenu(this, sub);
    if (false == this->inSlit) {
        n_menuitem_nop(sub, NULL);
        n_menuitem_bol(sub, "Set Desktop Margin", "setMargin", setMargin);
    }
    n_menuitem_nop(menu, NULL);

    const char *a1, *a3, *b;
    if (this->orient_vertical)
        a1 = "Left", a3 = "Right", b = "Base Width";
    else
        a1 = "Top", a3 = "Bottom", b = "Base Height";

    BBP_n_orientmenu(this, menu);
    sub = n_submenu(menu, "Order");
    n_menuitem_bol(sub, "Standard", "order 1", 1 == order);
    n_menuitem_bol(sub, "As Fit",   "order 2", 2 == order);
    //n_menuitem_bol(sub, "Puzzle",   "order 3", 3 == order);
    sub = n_submenu(menu, "Alignment");
    n_menuitem_bol(sub, a1,         "alignment 1", 1 == alignment);
    n_menuitem_bol(sub, "Center",   "alignment 2", 2 == alignment);
    n_menuitem_bol(sub, a3,         "alignment 3", 3 == alignment);
    if (1 != order)
        n_menuitem_int(menu, b, "baseWidth", baseWidth, 0, 1000);
    /* if (1 == order)
        n_disable_lastitem(menu); */
#if 0
    if (false == this->is_first)
    {
        ModuleInfo *p;
        n_menuitem_nop(menu, NULL);
        sub = n_submenu(menu, "Plugins");
        for (p = m_pMO; p; p = p->next)
        {
            n_menuitem_bol(sub, p->name, "plugin.load", false);
        }
    }
#endif
    n_menuitem_nop(menu, NULL);
    n_menuitem_cmd(menu, "Edit Settings", "EditRC");
    n_menuitem_cmd(menu, "About", "About");
    n_showmenu(this, menu, pop, 0);
}

//=============================================================================
bool get_style(StyleItem *si, const char *key)
{
    const char *s, *p;
    COLORREF c; int w;
    char fullkey[80], *r;

    memset(si, 0, sizeof *si);
    r = strchr(strcpy(fullkey, key), 0);
    s = stylePath();

    strcpy(r, ".appearance:");
    p = ReadString(s, fullkey, NULL);
    if (p) {
        si->bordered = IsInString(p, "border");
    } else {
        strcpy(r, ":");
        p = ReadString(s, fullkey, NULL);
        if (NULL == p)
            return false;
        si->bordered = true;
    }
    ParseItem(p, si);

    if (B_SOLID != si->type || si->interlaced)
        strcpy(r, ".color1:");
    else
        strcpy(r, ".backgroundColor:");
    c = ReadColor(s, fullkey, NULL);
    if ((COLORREF)-1 == c) {
        strcpy(r, ".color:");
        c = ReadColor(s, fullkey, NULL);
        if ((COLORREF)-1 == c)
            return false;
    }

    si->Color = si->ColorTo = c;
    if (B_SOLID != si->type || si->interlaced) {
        strcpy(r, ".color2:");
        c = ReadColor(s, fullkey, NULL);
        if ((COLORREF)-1 == c) {
            strcpy(r, ".colorTo:");
            c = ReadColor(s, fullkey, NULL);
        }
        if ((COLORREF)-1 != c)
            si->ColorTo = c;
    }

    if (si->bordered) {
        strcpy(r, ".borderColor:");
        c = ReadColor(s, fullkey, NULL);
        if ((COLORREF)-1 != c)
            si->borderColor = c;
        else
            si->borderColor = ReadColor(s, "borderColor:", "black");

        strcpy(r, ".borderWidth:");
        w = ReadInt(s, fullkey, -100);
        if (-100 != w)
            si->borderWidth = w;
        else
            si->borderWidth = ReadInt(s, "borderWidth:", 1);
    }

    strcpy(r, ".marginWidth:");
    w = ReadInt(s, fullkey, -100);
    if (-100 != w)
        si->marginWidth = w;
    else
        si->marginWidth = ReadInt(s, "bevelWidth:", 2);
    return true;
}

void getStyleSettings()
{
    StyleItem si, *S;
    S = NULL;

    if (bblean_version >= 1170)
        S = (StyleItem*)GetSettingPtr(SN_SLIT);

    if (NULL == S) {
        S = &si;
        if (false == get_style(S, "slit"))
            S = (StyleItem*)GetSettingPtr(SN_TOOLBAR);
    }

    m_style = *S;
    m_style.parentRelative = false;
    padding = m_style.marginWidth;
    margin = m_style.marginWidth + m_style.borderWidth;
}

//=============================================================================
void slit_info::getRCSettings()
{
    this->place = POS_CenterRight; /* default placement */
    BBP_read_window_modes(this, szAppName);

    this->baseWidth = BBP_read_int(this, "baseWidth", 0);
    this->order = BBP_read_int(this, "order", 2);
    this->setMargin = BBP_read_bool(this, "setMargin", false);
    this->alignment = iminmax (BBP_read_int(this, "alignment", 2), 1, 3);
}

//=============================================================================
void slit_info::process_broam(const char *temp, int f)
{
    if (f & BBP_BROAM_HANDLED)
    {
        if (f & BBP_BROAM_METRICS)
            calculate_frame();
        show_menu(false);
        return;
    }

    if (BBP_broam_int(this, temp, "baseWidth", &baseWidth))
    {
        calculate_frame();
        show_menu(false);
        return;
    }

    if (BBP_broam_int(this, temp, "alignment", &alignment))
    {
        calculate_frame();
        show_menu(false);
        return;
    }

    if (BBP_broam_int(this, temp, "order", &order))
    {
        calculate_frame();
        show_menu(false);
        return;
    }

    if (BBP_broam_bool(this, temp, "setMargin", &setMargin))
    {
        pos_changed();
        show_menu(false);
        return;
    }
}

//=============================================================================

bool get_size(PluginInfo *p)
{
    RECT r;
    if (FALSE == GetWindowRect(p->hwnd, &r))
        return false;
    p->width  = r.right - r.left;
    p->height = r.bottom - r.top;
    p->visible = FALSE != IsWindowVisible(p->hwnd);
    return true;
}

//=============================================================================
void slit_info::position_windows(void)
{
    PluginInfo *p; int n; HDWP dwp;

    for (n = 0, p = m_pInfo; p; p = p->next, ++n);
    dwp = BeginDeferWindowPos(n);

    for (p = m_pInfo; p; p = p->next) {
        dwp = DeferWindowPos(
            dwp,
            p->hwnd, NULL,
            p->xpos, p->ypos, p->width, p->height,
            SWP_NOACTIVATE
            | SWP_NOZORDER
            | SWP_NOSIZE
            );
    }
    EndDeferWindowPos(dwp);
}

//=============================================================================

LRESULT slit_info::wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret)
{
    static unsigned int msgs[] = { BB_REDRAWGUI, 0 };
    PluginInfo *p, **pp;

    if (ret)
        return *ret;

    switch(message)
    {
        //=============================================

        case WM_CREATE:
            SendMessage(GetBBWnd(), BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            break;

        case WM_DESTROY:
            SendMessage(GetBBWnd(), BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            if (this->bufbmp)
                DeleteObject(this->bufbmp), this->bufbmp = NULL;
            SetDesktopMargin(hwnd, 0, 0);
            for (pp = &this->m_pInfo; NULL != (p = *pp); *pp = p->next, m_free(p));
            break;

        //=============================================

        case SLIT_ADD:
            //pdbg ((HWND) lParam, "add");
            for (pp = &this->m_pInfo; NULL != (p = *pp); pp = &p->next);
            *pp = p = (PluginInfo*)m_alloc(sizeof(struct PluginInfo));
            memset(p, 0, sizeof *p);
            /* if (!IsBadStringPtr((const char*)wParam, 80))
                ... */
            p->hwnd = (HWND)lParam;
            SetWindowLong(p->hwnd, GWL_STYLE,
                (GetWindowLong(p->hwnd, GWL_STYLE) & ~WS_POPUP) | WS_CHILD);
            SetParent(p->hwnd, hwnd);
            SetTimer(hwnd, 2, 20, NULL);
            break;

        case SLIT_REMOVE:
            //pdbg ((HWND) lParam, "remove");
            for (pp = &this->m_pInfo; NULL != (p = *pp) && p->hwnd != (HWND)lParam; pp = &p->next);
            if (p) {
                if (IsWindow(p->hwnd)) {
                    SetParent(p->hwnd, NULL);
                    SetWindowLong(p->hwnd, GWL_STYLE,
                        (GetWindowLong(p->hwnd, GWL_STYLE) & ~WS_CHILD) | WS_POPUP);
                }
                *pp = p->next;
                m_free(p);
            }
            SetTimer(hwnd, 2, 20, NULL);
            this->suspend_autohide = 0;
            break;

        case SLIT_UPDATE:
            //pdbg ((HWND) lParam, "update");
            SetTimer(hwnd, 2, 20, NULL);
            break;

        case WM_TIMER:
            if (2 == wParam) {
                KillTimer(hwnd, wParam);
                this->calculate_frame();
            }
            break;

        //=============================================
        // support bbStyleMaker under bbLean 1.16
        case WM_COPYDATA:
            return BBReceiveData(hwnd, lParam, NULL);

        case BB_SETSTYLESTRUCT:
            if (SN_SLIT == wParam)
                memcpy(&m_style, (void*)lParam, sizeof m_style);
            break;

        case BB_REDRAWGUI:
            if (wParam & BBRG_SLIT)
            {
                int m = margin;
                int p = padding;
                if (this->bufbmp)
                    DeleteObject(this->bufbmp), this->bufbmp = NULL;

                if (bblean_version >= 1170) {
                    getStyleSettings();
                } else {
                    // Workaround under bbLean 1.16
                    padding = m_style.marginWidth;
                    margin = m_style.marginWidth + m_style.borderWidth;
                }

                if (m != margin || p != padding)
                    this->calculate_frame();
                else
                    InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case BB_RECONFIGURE:
            if (this->bufbmp)
                DeleteObject(this->bufbmp), this->bufbmp = NULL;
            getStyleSettings();
            this->getRCSettings();
            SetTimer(hwnd, 2, 20, NULL);
            break;

        //=============================================

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC buf = CreateCompatibleDC(NULL);
            HGDIOBJ otherbmp;

            if (this->bufbmp
             && (this->bmp_width != this->width || this->bmp_height != this->height))
                DeleteObject(this->bufbmp), this->bufbmp = NULL;

            if (NULL == this->bufbmp) {
                this->bufbmp = CreateCompatibleBitmap(hdc,
                    this->bmp_width = this->width,
                    this->bmp_height = this->height
                    );
                otherbmp = SelectObject(buf, this->bufbmp);
                RECT r = { 0, 0, this->bmp_width, this->bmp_height };
                MakeStyleGradient(buf, &r, &m_style, m_style.bordered);
            } else {
                otherbmp = SelectObject(buf, this->bufbmp);
            }

            BitBltRect(hdc, buf, &ps.rcPaint);
            SelectObject(buf, otherbmp);
            DeleteDC(buf);
            EndPaint(hwnd, &ps);
            break;
        }

        //=============================================

        // Right mouse button clicked?
        case WM_RBUTTONUP:
            this->show_menu (true);
            break;

        case WM_LBUTTONDBLCLK:
            BBP_set_autoHide(this, false == this->autoHide);
            this->show_menu (false);
            break;
    
        //=============================================
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

//============================================================================
#if 1
//============================================================================
void slit_info::loadPlugins(FILE *fp)
{
    char rc_key[256];
    LONG L = 0;

    sprintf(rc_key, "%s.plugin:", this->rc_key);
    for (;;)
    {
        const char *plug_path;
        char line[MAX_PATH];
        if (fp)
        {
            if (false == ReadNextCommand(fp, line, sizeof line))
                break;
            if (line[0] != '&')
                continue;
            plug_path = line;
            while (' ' == *++plug_path);
        }
        else
        {
            plug_path = ReadValue(rcpath, rc_key, &L);
            if (NULL == plug_path)
                break;
        }

        //dbg_printf("%d loading %s", L, plug_path);

        ModuleInfo *m = (ModuleInfo*)m_alloc(sizeof(struct ModuleInfo));
        memset(m, 0, sizeof *m);

        const char *name = plug_path;
        const char *p1 = strrchr (plug_path, '\\');
        const char *p2 = strrchr (plug_path, '/');
        if (p2 > p1) p1 = p2;
        if (p1) name = p1 + 1;

        strcpy(m->name, name);

        char *p3 = strrchr(m->name, '.');
        if (p3) *p3 = 0;
        m->args[0] = 0;

        const char *errormsg = NULL;
        HMODULE hMO = LoadLibrary(plug_path);

        if (NULL == hMO) {
            errormsg = "This plugin you are trying to load"
            " does not exist or cannot be loaded.";
        } else {
            *(FARPROC*)&m->beginPluginEx   = GetProcAddress(hMO, "beginPluginEx");
            *(FARPROC*)&m->beginSlitPlugin = GetProcAddress(hMO, "beginSlitPlugin");
            *(FARPROC*)&m->endPlugin       = GetProcAddress(hMO, "endPlugin");

            if (NULL == m->beginSlitPlugin && NULL == m->beginPluginEx) {
                errormsg = "This plugin doesn't have an 'beginSlitPlugin'."
                "\nProbably it is not designed for the slit.";
            }
            else
            if (NULL == m->endPlugin) {
                errormsg = "This plugin doesn't have an 'endPlugin'."
                "\nProbably it is not a plugin designed for bb4win.";
            } else {
#ifdef _MSC_VER
                _try {
#endif
                    int result;
                    if (m->beginPluginEx)
                        result = m->beginPluginEx(hMO, this->hwnd);
                    else
                        result = m->beginSlitPlugin(hMO, this->hwnd);
                    if (BEGINPLUGIN_OK != result) {
                        errormsg = "This plugin you are trying to load"
                        " failed to initialize.";
                    }
#ifdef _MSC_VER
                } _except(EXCEPTION_EXECUTE_HANDLER) {
                    errormsg = "This plugin caused a general protection fault on loading.";
                }
#endif
            }
        }

        if (NULL == errormsg) {
            m->hMO = hMO;
            m->next = m_pMO;
            m_pMO = m;
            continue;
        }

        if (hMO)
            FreeLibrary(hMO);
        m_free(m);

        BBP_messagebox(this, MB_OK, "%s\n%s", plug_path, errormsg);
    }
}

//============================================================================
#endif
//============================================================================

void slit_info::unloadPlugins(void)
{
    ModuleInfo *m;
    for (m = m_pMO; m; m = m->next) {
        m->endPlugin(m->hMO);
        FreeLibrary(m->hMO);

    }
    while (m_pMO) {
        m = m_pMO ->next;
        m_free(m_pMO);
        m_pMO = m;
    }
}

//=============================================================================
void slit_info::pos_changed(void)
{
    int pos, margin;
    pos = margin = 0;
    if (this->setMargin && false == this->autoHide && this->is_visible)
    {
        switch (place)
        {
            case POS_TopLeft        :
                if (this->orient_vertical)
                    goto case_POS_Left;
                else
                    goto case_POS_Top;
            case POS_TopRight       :
                if (this->orient_vertical)
                    goto case_POS_Right;
                else
                    goto case_POS_Top;

            case POS_BottomLeft     :
                if (this->orient_vertical)
                    goto case_POS_Left;
                else
                    goto case_POS_Bottom;
            case POS_BottomRight    :
                if (this->orient_vertical)
                    goto case_POS_Right;
                else
                    goto case_POS_Bottom;

            case POS_Top            :
            case POS_TopCenter      :
            case_POS_Top            : pos = BB_DM_TOP; margin = height;
                break;
            case POS_Bottom         :
            case POS_BottomCenter   :
            case_POS_Bottom         : pos = BB_DM_BOTTOM; margin = height;
                break;
            case POS_Left           :
            case POS_CenterLeft     :
            case_POS_Left           : pos = BB_DM_LEFT; margin = width;
                break;
            case POS_Right          :
            case POS_CenterRight    :
            case_POS_Right          : pos = BB_DM_RIGHT; margin = width;
                break;
        }
        if (margin && false == this->alwaysOnTop)
            margin += 4;
    }

    SetDesktopMargin(this->hwnd, pos, margin);

}

//=============================================================================

//=============================================================================

#define ALN_LEFT 1
#define ALN_CENTER 2
#define ALN_RIGHT 3

struct options {
    int alignment;
    int padding;
    int basesize;
};

struct obj {
    int x, y;   // output
    int w, h;   // input
    int f;      // user flag
};

struct nobjs {
    int n;      // actual member count of p
    struct obj p[1];
};

//=============================================================================

//=============================================================================
// standard order algorithm, based on related code in BBSlit by Tres'ni

void reorder_standard (struct nobjs *pv, struct options *o)
{
    int w, h, n; struct obj *p;
    for (p = pv->p, n = w = h = 0; n < pv->n; ++n, ++p) {
        p->x = 0;
        p->y = h;
        w = imax(w, p->w);
        h += p->h + o->padding;
    }

    if (o->alignment != ALN_LEFT)
        for (p = pv->p, n = 0; n < pv->n; ++n, ++p)
            if (o->alignment == ALN_RIGHT)
                p->x = (w - p->w);
            else
                p->x = (w - p->w) / 2;
}

//=============================================================================

//=============================================================================
/* As Fit - algorithm */

void reorder_fit (struct nobjs *pv, struct options *o);

struct vi { int n; int i[1]; };

void vi_insert(struct vi *vi, int x)
{
    int m, n, *i;
    for (m = vi->n, i = vi->i, n = 0; n < m; ++n) {
        if (i[n] < x)
            continue;
        if (i[n] == x)
            return;
        memmove(i+n+1, i+n, (m - n) * sizeof *i);
        break;
    }
    i[n] = x, ++vi->n;
}

bool overlap(struct obj *l, struct obj *p, int pad)
{
    int ox, oy;
    oy = imin(l->y + l->h, p->y + p->h) - imax(l->y, p->y);
    if (oy <= -pad)
        return false;
    ox = imin(l->x + l->w, p->x + p->w) - imax(l->x, p->x);
    if (ox <= -pad)
        return false;
    return true;
}

bool overlap_any(struct nobjs *pv, struct obj *p, int pad)
{
    int n, m; struct obj *l;
    for (l = pv->p, m = pv->n, n = 0; n < m; ++n, ++l)
        if (overlap(l, p, pad))
            return true;
    return false;
}

int right_next(struct nobjs *pv, struct obj *p, int x_right, int pad)
{
    int n, m, oy; struct obj *l;
    for (l = pv->p, m = pv->n, n = 0; n < m; ++n, ++l) {
        if (l == p)
            continue;
        if (l->x < p->x + p->w)
            continue;
        oy = imin(l->y + l->h, p->y + p->h) - imax(l->y, p->y);
        if (oy <= -pad)
            continue;
        x_right = imin(x_right, l->x - pad);
    }
    return x_right;
}

void reorder_fit (struct nobjs *pv, struct options *o)
{
    struct obj *p;
    int n, m, b0, w0, h0;
    struct vi *vx, *vy;

    m = pv->n;

    /* get max width / height */
    b0 = o->basesize;
    for (p = pv->p, n = 0; n < m; ++p, ++n)
        b0 = imax(b0, p->w);

    /* allocate scratch buffers */
    vx = (struct vi *)m_alloc(sizeof *vx + m * sizeof vx->i);
    vy = (struct vi *)m_alloc(sizeof *vy + m * sizeof vy->i);
    vx->n = vy->n = 1; /* start with one edge each */
    vx->i[0] = vy->i[0] = 0; /* at x,y:0,0 */

    for (p = pv->p, w0 = h0 = n = 0; (pv->n = n) < m; ++p, ++n)
    {
        int xn, yn;
        yn = 0;
        do {
            p->y = vy->i[yn]; xn = 0;
            do {
                p->x = vx->i[xn];
                if (p->x + p->w > b0) /* beyond max width, forget it */
                    break;
                if (false == overlap_any(pv, p, o->padding))
                    goto bbreak;
            } while (++xn < vx->n);
        } while (++yn < vy->n);

    bbreak:
        /* insert new edges into sorted array */
        vi_insert(vx, p->x + p->w + o->padding);
        vi_insert(vy, p->y + p->h + o->padding);
        /* extend right frame edge */
        w0 = imax(w0, p->x + p->w);
        h0 = imax(h0, p->y + p->h);
    }

    if (o->alignment != ALN_LEFT)
    {
        /* Adjust windows for alignment, when possible.
           Repeat until nothing changed */
        int w, f;
        for (p = pv->p, n = 0; n < m; p->f = 0, ++p, ++n);
        do {
            f = 0;
            for (p = pv->p, n = 0; n < m; ++p, ++n) {
                if (p->f)
                    continue;
                w = right_next(pv, p, w0, o->padding);
                if (w <= p->x + p->w)
                    continue;
                if (w == w0 && o->alignment == ALN_CENTER) {
                    p->x = (w - p->w + p->x) / 2;
                } else {
                    p->x = w - p->w;
                }
                p->f = f = 1;
            }
        } while (f);
    }
    m_free(vx);
    m_free(vy);
}

//=============================================================================

//=============================================================================
void slit_info::calculate_frame(void)
{
    PluginInfo *pi, **pp;
    int w0, h0, n, x, y;

    if (false == this->is_visible)
        return;

    n = 0;
    for (pp = &m_pInfo; NULL != (pi = *pp); ) {
        if (get_size(pi)) {
            if (pi->visible)
                ++n;
            pp = &pi->next;
        } else {
            *pp = pi->next;
            m_free(pi);
        }
    }

    w0 = h0 = 0;
    if (0 == n) {

        if (this->n_inst)
            w0 = h0 = 24;

    } else {

        //DWORD t0 = GetTickCount(); for (int tc = 0; tc < 1000; ++tc) {

        struct options o;
        struct nobjs *pv;
        struct obj *p;
        int vertical = this->orient_vertical;

        pv = (struct nobjs*)m_alloc(sizeof *pv + (n-1) * sizeof pv->p);
        pv->n = n;

        /* Fill in the nobjs */
        p = pv->p;
        for (pi = m_pInfo; pi; pi = pi->next) {
            if (false == pi->visible)
                continue;
            p->x = p->y = p->f = 0;
            if (vertical) {
                p->w = pi->width;
                p->h = pi->height;
            } else {
                p->w = pi->height;
                p->h = pi->width;
            }
            ++p;
        }

        /* Fill in the options */
        o.basesize = this->baseWidth;
        o.alignment = this->alignment;
        o.padding = padding;

        /* Call the order-algorithm */
        switch (this->order) {
            case 2:
                reorder_fit(pv, &o);
                break;
            default:
                reorder_standard(pv, &o);
                break;
        }

        /* Assign xy coords to plugins and calculate frame size */
        p = pv->p;
        for (pi = m_pInfo; pi; pi = pi->next) {
            if (false == pi->visible)
                continue;
            if (vertical) {
                x = p->x;
                y = p->y;
            } else {
                x = p->y;
                y = p->x;
            }
            w0 = imax(w0, (pi->xpos = x + margin) + pi->width + margin);
            h0 = imax(h0, (pi->ypos = y + margin) + pi->height + margin);
            //dbg_printf("%x (%d) %d %d %d %d", pi->hwnd, n, pi->xpos, pi->ypos, pi->width, pi->height);
            ++p;
        }
        m_free(pv);

        //} dbg_printf("reorder: %d ms", GetTickCount() - t0);
    }

    this->width  = imin(w0, this->mon_rect.right - this->mon_rect.left);
    this->height = imin(h0, this->mon_rect.bottom - this->mon_rect.top);
    BBP_reconfigure(this);
    position_windows();
}

//=============================================================================
