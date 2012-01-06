/*
 ============================================================================

  This file is part of the bbLean source code.

  Copyright © 2004-2009 grischka
  http://bb4win.sf.net/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2).

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================
*/

#include "BBApi.h"
#include "bblib.h"
#include "bbPlugin.h"

#ifndef GetWindowLongPtr
#define LONG_PTR LONG
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#endif

#ifdef __cplusplus
#define _THIS
#define _THIS_
#else
#define _THIS PI
#define _THIS_ PI,
#endif

int BBP_bbversion(void)
{
    const char *bbv;
    static int BBVersion = -1;
    if (-1 == BBVersion) {
        BBVersion = 0;
        bbv = GetBBVersion();
        if (0 == memcmp(bbv, "bbLean", 6)) {
            int a, b, c = 0;
            if (sscanf(bbv+7, "%d.%d.%d", &a, &b, &c) >= 2)
                BBVersion = a*1000+b*10+c;
            else
                BBVersion = 2;
        }
        else if (0 == memcmp(bbv, "bb", 2)) {
            BBVersion = 1;
        }
    }
    return BBVersion;
}

//===========================================================================
#ifndef BBPLUGIN_NOMENU

struct _menuitem;
struct n_menu;

enum _menuitem_modes
{
    i_nop = 1,
    i_sub ,
    i_cmd ,
    i_bol ,
    i_int ,
    i_str
};

struct n_menu
{
    char *title;
    _menuitem *items;
    _menuitem **pitems;
    _menuitem *lastitem;

    const char *id_string;
    char **broam_key;
    bool popup;

    n_menu(const char *_title)
    {
        title = new_str(_title);
        items = lastitem = NULL;
        pitems = &items;
    }

    ~n_menu()
    {
        free_str(&title);
        delitems();
    }

    void additem(_menuitem *mi);
    void delitems(void);
    const char *addid(const char *cmd);
    Menu *convert(char **broam_key, const char *id_string, bool popup);
};

//-----------------------------------------------------
struct _menuitem
{
    _menuitem *next;
    n_menu *menu;
    char *text;
    int mode;
    bool disabled;

    _menuitem(n_menu *m, const char *_text, int _mode)
    {
        mode = _mode;
        text = new_str(_text);
        next = NULL;
        disabled = false;
        m->additem(this);
    }

    virtual ~_menuitem()
    {
        free_str(&text);
    }

    virtual MenuItem* _make_item(Menu *pMenu)
    {
        return NULL;
    }

};

//-----------------------------------------------------
struct _menuitemnop : _menuitem
{
    _menuitemnop(n_menu *m, const char *_text)
        : _menuitem(m, _text, i_nop)
    {
    }
    MenuItem* _make_item(Menu *pMenu)
    {
        return MakeMenuNOP(pMenu, text);
    }
};

//-----------------------------------------------------
struct _menuitemsub : _menuitem
{
    struct n_menu *sub;
    _menuitemsub(n_menu *m, const char *_text, n_menu* _sub)
        : _menuitem(m, _text, i_sub)
    {
        sub = _sub;
    }

    ~_menuitemsub()
    {
        delete sub;
    }

    MenuItem* _make_item(Menu *pMenu)
    {
        char buffer[200];
        sprintf(buffer, "%s:%s", menu->id_string, sub->title);
        Menu *s = sub->convert(menu->broam_key, buffer, menu->popup);
        return MakeSubmenu(pMenu, s, text);
    }
};

//-----------------------------------------------------
struct _menuitemcmd : _menuitem
{
    char *cmd;
    _menuitemcmd(n_menu *m, const char *_text, const char *_cmd)
        : _menuitem(m, _text, i_cmd)
    {
        cmd = new_str(_cmd);
    }
    ~_menuitemcmd()
    {
        free_str(&cmd);
    }

    MenuItem* _make_item(Menu *pMenu)
    {
        return MakeMenuItem(pMenu, text, menu->addid(cmd), false);
    }
};

//-----------------------------------------------------
struct _menuitembol : _menuitemcmd
{
    bool checked;
    _menuitembol(n_menu *m, const char *_text, const char *_cmd, bool _checked)
        : _menuitemcmd(m, _text, _cmd)
    {
        checked = _checked;
        mode = i_bol;
    }
    MenuItem* _make_item(Menu *pMenu)
    {
        return MakeMenuItem(pMenu, text, menu->addid(cmd), checked);
    }
};

//-----------------------------------------------------
struct _menuitemstr : _menuitem
{
    char *cmd;
    char *init;
    _menuitemstr(n_menu *m, const char *_text, const char *_cmd, const char *_init)
        : _menuitem(m, _text, i_str)
    {
        cmd = new_str(_cmd);
        init = new_str(_init);
    }
    ~_menuitemstr()
    {
        free_str(&cmd);
        free_str(&init);
    }
    MenuItem* _make_item(Menu *pMenu)
    {
        return MakeMenuItemString(pMenu, text, menu->addid(cmd), init);
    }
};

//-----------------------------------------------------
struct _menuitemint : _menuitem
{
    char *cmd;
    int initval;
    int minval;
    int maxval;
    _menuitemint(n_menu *m, const char *_text, const char *_cmd, int _initval, int _minval, int _maxval)
    : _menuitem(m, _text, i_int)
    {
        cmd = new_str(_cmd);
        initval = _initval;
        minval = _minval;
        maxval = _maxval;
    }
    ~_menuitemint()
    {
        free_str(&cmd);
    }
    MenuItem* _make_item(Menu *pMenu)
    {
        if (BBVERSION_09X) {
            char buffer[20]; sprintf(buffer, "%d", initval);
            return MakeMenuItemString(pMenu, text, menu->addid(cmd), buffer);
        } else {
            return MakeMenuItemInt(pMenu, text, menu->addid(cmd), initval, minval, maxval);
        }
    }
};

//-----------------------------------------------------
void n_menu::additem(_menuitem *mi)
{
    *pitems = mi;
    pitems = &mi->next;
    mi->menu = this;
    lastitem = mi;
}

void n_menu::delitems(void)
{
    _menuitem *mi = items;
    while (mi)
    {
        _menuitem *n = mi->next;
        delete mi;
        mi = n;
    }
}

const char *n_menu::addid(const char *cmd)
{
    if ('@' == *cmd) return cmd;
    strcpy(this->broam_key[1], cmd);
    return this->broam_key[0];
}

Menu *n_menu::convert(char **broam_key, const char *_id_string, bool _popup)
{
    this->id_string = _id_string;
    this->popup = _popup;
    this->broam_key = broam_key;
    //dbg_printf("<%s>", id_string);
    Menu *pMenu = MakeNamedMenu(title, id_string, popup);
    _menuitem *mi = items; while (mi)
    {
        MenuItem *pItem = mi->_make_item(pMenu);
        if (mi->disabled)
            MenuItemOption(pItem, BBMENUITEM_DISABLED);
        mi = mi->next;
    }
    return pMenu;
}

//-----------------------------------------------------
n_menu *n_makemenu(const char *title)
{
    return new n_menu(title);
}

n_menu *n_submenu(n_menu *m, const char *text)
{
    n_menu *s = n_makemenu(text);
    new _menuitemsub(m, text, s);
    return s;
}

void n_menuitem_nop(n_menu *m, const char *text)
{
    new _menuitemnop(m, text);
}

void n_menuitem_cmd(n_menu *m, const char *text, const char *cmd)
{
    new _menuitemcmd(m, text, cmd);
}

void n_menuitem_bol(n_menu *m, const char *text, const char *cmd, bool check)
{
    new _menuitembol(m, text, cmd, check);
}

void n_menuitem_int(n_menu *m, const char *text, const char *cmd, int val, int vmin, int vmax)
{
    new _menuitemint(m, text, cmd, val, vmin, vmax);
}

void n_menuitem_str(n_menu *m, const char *text, const char *cmd, const char *init)
{
    new _menuitemstr(m, text, cmd, init);
}

void n_disable_lastitem(n_menu *m)
{
    if (m->lastitem)
        m->lastitem->disabled = true;
}

Menu *n_convertmenu(n_menu *m, const char *broam_key, bool popup)
{
    char broam[200];
    int x = sprintf(broam, "@%s.", broam_key);
    char *b[2] = { broam, broam+x };
    Menu *pMenu = m->convert(b, broam_key, popup);
    delete m;
    return pMenu;
}

void BBP_showmenu(plugin_info *PI, Menu *pMenu, int flags)
{
    if (flags & (int)true) {
        HWND hwnd = PI->inSlit?PI->hSlit:PI->hwnd;
        MenuOption(pMenu, (flags & ~(int)true) | BBMENU_HWND, PI->hwnd);
        PostMessage(hwnd, BB_AUTOHIDE, 1, 1);
    }
    ShowMenu(pMenu);
}

void n_showmenu(plugin_info *PI, n_menu *m, bool popup, int flags, ...)
{
    Menu *pMenu;
    pMenu = n_convertmenu(m, PI->broam_key, popup);
    if (flags) {
        va_list vl;
        va_start(vl, flags);
        void *o1, *o2, *o3;
        o1 = va_arg(vl, void*);
        o2 = va_arg(vl, void*);
        o3 = va_arg(vl, void*);
        MenuOption(pMenu, flags, o1, o2, o3);
    }
    BBP_showmenu(PI, pMenu, popup);
}

#endif // def BBPLUGIN_NOMENU

//*****************************************************************************

//*****************************************************************************

int get_place(plugin_info *PI)
{
    int sw, sh, x, y, w, h;
    bool top, vcenter, bottom, left, center, right;

    PI->hMon = GetMonitorRect(PI->hwnd, &PI->mon_rect, GETMON_FROM_WINDOW);

    sw  = PI->mon_rect.right  - PI->mon_rect.left;
    sh  = PI->mon_rect.bottom - PI->mon_rect.top;
    x   = PI->xpos - PI->mon_rect.left;
    y   = PI->ypos - PI->mon_rect.top;
    w   = PI->width;
    h   = PI->height;

    x = iminmax(x, 0, sw - w);
    y = iminmax(y, 0, sh - h);

    top        = y == 0;
    vcenter    = y == sh/2 - h/2;
    bottom     = y == (sh - h);

    left       = x == 0;
    center     = x == sw/2 - w/2;
    right      = x == (sw - w);

    if (top)
    {
        if (left)   return POS_TopLeft   ;
        if (center) return POS_TopCenter ;
        if (right)  return POS_TopRight  ;
        return POS_Top;
    }

    if (bottom)
    {
        if (left)   return POS_BottomLeft   ;
        if (center) return POS_BottomCenter ;
        if (right)  return POS_BottomRight  ;
        return POS_Bottom;
    }

    if (left)
    {
        if (vcenter) return POS_CenterLeft;
        return POS_Left;
    }

    if (right)
    {
        if (vcenter) return POS_CenterRight;
        return POS_Right;
    }

    if (center)
    {
        if (vcenter) return POS_Center;
        return POS_CenterH;
    }

    if (vcenter)
        return POS_CenterV;
    return POS_User;
}

//===========================================================================

void set_place(plugin_info *PI)
{
    int sw, sh, x, y, w, h, place;

    sw  = PI->mon_rect.right  - PI->mon_rect.left;
    sh  = PI->mon_rect.bottom - PI->mon_rect.top;
    x   = PI->xpos - PI->mon_rect.left;
    y   = PI->ypos - PI->mon_rect.top;
    w   = PI->width;
    h   = PI->height;
    place = PI->place;

    switch (place)
    {

        case POS_Top:
        case POS_TopLeft:
        case POS_TopCenter:
        case POS_TopRight:
            y = 0;
            break;

        case POS_CenterLeft:
        case POS_CenterRight:
        case POS_CenterV:
        case POS_Center:
            y = sh/2 - h/2;
            break;

        case POS_Bottom:
        case POS_BottomLeft:
        case POS_BottomCenter:
        case POS_BottomRight:
            y = sh - h;
            break;
    }

    switch (place)
    {
        case POS_Left:
        case POS_TopLeft:
        case POS_CenterLeft:
        case POS_BottomLeft:
            x = 0;
            break;

        case POS_TopCenter:
        case POS_BottomCenter:
        case POS_CenterH:
        case POS_Center:
        x = sw/2 - w/2;
            break;

        case POS_Right:
        case POS_TopRight:
        case POS_CenterRight:
        case POS_BottomRight:
            x = sw - w;
            break;
    }

    PI->xpos = PI->mon_rect.left + iminmax(x, 0, sw - w);
    PI->ypos = PI->mon_rect.top  + iminmax(y, 0, sh - h);
}

//===========================================================================

void BBP_set_window_modes(plugin_info *PI)
{
    bool useslit = PI->useSlit && PI->hSlit;
    bool visible = PI->visible && (false == PI->toggled_hidden || useslit);
    bool updateslit = false;
    BYTE trans = 255;

    PI->auto_hidden = false;

    if (visible != PI->is_visible)
    {
        ShowWindow(PI->hwnd, visible ? SW_SHOWNA : SW_HIDE);
        PI->is_visible = visible;
        updateslit = true;
    }

    if (useslit)// && false == hidden)
    {
        RECT r;
        int w, h;
        bool update_size;

        GetWindowRect(PI->hwnd, &r);
        w = r.right - r.left;
        h = r.bottom - r.top;
        update_size = (w != PI->width || h != PI->height);

        if (update_size)
        {
            SetWindowPos(
                PI->hwnd,
                NULL,
                0,
                0,
                PI->width,
                PI->height,
                SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER
                );
            updateslit = true;
        }

        if (false == PI->inSlit)
        {
            if (PI->is_alpha != trans)
            {
                SetTransparency(PI->hwnd, trans);
                PI->is_alpha = trans;
            }
            PI->inSlit = true;
            SendMessage(PI->hSlit, SLIT_ADD, (WPARAM)PI->broam_key, (LPARAM)PI->hwnd);
        }
        else
        if (updateslit)
        {
            SendMessage(PI->hSlit, SLIT_UPDATE, 0, (LPARAM)PI->hwnd);
        }
    }
    else
    {
        int x, y, w, h;
        HWND hwnd_after;
        UINT flags = SWP_NOACTIVATE;

        if (PI->inSlit)
        {
            SendMessage(PI->hSlit, SLIT_REMOVE, 0, (LPARAM)PI->hwnd);
            PI->inSlit = false;
        }

        set_place(PI);
        x = PI->xpos;
        y = PI->ypos;
        w = PI->width;
        h = PI->height;

        if (PI->autoHide && false == PI->auto_shown)
        {
            int hangout = 1;
            int place = PI->place;
            if ((false == PI->orient_vertical && (place == POS_TopLeft || place == POS_BottomLeft))
                || place == POS_CenterLeft || place == POS_Left)
            {
                x = PI->mon_rect.left, w = hangout;
                PI->auto_hidden = true;
            }
            else
            if ((false == PI->orient_vertical && (place == POS_TopRight || place == POS_BottomRight))
                || place == POS_CenterRight || place == POS_Right)
            {
                x = PI->mon_rect.right - hangout, w = hangout;
                PI->auto_hidden = true;
            }
            else
            if (place == POS_TopLeft || place == POS_TopCenter || place == POS_TopRight || place == POS_Top)
            {
                y = PI->mon_rect.top, h = hangout;
                PI->auto_hidden = true;
            }
            else
            if (place == POS_BottomLeft || place == POS_BottomCenter || place == POS_BottomRight || place == POS_Bottom)
            {
                y = PI->mon_rect.bottom - hangout, h = hangout;
                PI->auto_hidden = true;
            }
        }

        if (WS_CHILD & GetWindowLong(PI->hwnd, GWL_STYLE))
        {
            flags |= SWP_NOZORDER;
            hwnd_after = NULL;
        }
        else
        {
            if (PI->alwaysOnTop || PI->auto_shown || PI->auto_hidden)
                hwnd_after = HWND_TOPMOST;
            else
                hwnd_after = HWND_NOTOPMOST;
        }

        SetWindowPos(PI->hwnd, hwnd_after, x, y, w, h, flags);

        if (PI->auto_hidden)
            trans = 16;
        else
        if (PI->alphaEnabled)
            trans = PI->alphaValue;

        if (PI->is_alpha != trans)
        {
            SetTransparency(PI->hwnd, trans);
            PI->is_alpha = trans;
        }
    }

    PI->pos_changed(_THIS);
}

//===========================================================================

const char *placement_strings[] = {
    "User"          ,

    "TopLeft"       ,
    "TopCenter"     ,
    "TopRight"      ,

    "BottomLeft"    ,
    "BottomCenter"  ,
    "BottomRight"   ,

    "CenterLeft"    ,
    "CenterRight"   ,
    "Center"        ,

    "Top"           ,
    "Bottom"        ,
    "Left"          ,
    "Right"         ,
    "CenterH" ,
    "CenterV" ,
    NULL
};

const char *menu_placement_strings[] = {
    "User"          ,

    "Top Left"       ,
    "Top Center"     ,
    "Top Right"      ,

    "Bottom Left"    ,
    "Bottom Center"  ,
    "Bottom Right"   ,

    "Center Left"    ,
    "Center Right"   ,
    "Center Screen"  ,

    "Top"           ,
    "Bottom"        ,
    "Left"          ,
    "Right"         ,
    NULL
};

//===========================================================================
const char *BBP_placement_string(int pos)
{
    if (pos < 0 || pos >= POS_LAST) pos = 0;
    return placement_strings[pos];
}

int BBP_get_placement(const char* place_string)
{
    return get_string_index(place_string, placement_strings);
}

//===========================================================================
#ifndef BBPLUGIN_NOMENU

static char *make_key(char *buffer, struct plugin_info *PI, const char *rcs)
{
    const char *s;
    char *d = buffer;
    s = PI->rc_key; while (0 != (*d = *s)) ++s, ++d;
    *d++ = '.';
    s = rcs; while (0 != (*d = *s)) ++s, ++d;
    *d++ = ':'; *d = 0;
    return buffer;
}

void BBP_write_string(struct plugin_info *PI, const char *rcs, const char *val)
{
    char buffer[256];
    if (PI->rc_key)
        WriteString(PI->rcpath, make_key(buffer, PI, rcs), val);
}

void BBP_write_int(struct plugin_info *PI, const char *rcs, int val)
{
    char buffer[256];
    if (PI->rc_key)
        WriteInt(PI->rcpath, make_key(buffer, PI, rcs), val);
}

void BBP_write_bool(struct plugin_info *PI, const char *rcs, bool val)
{
    char buffer[256];
    if (PI->rc_key)
        WriteBool(PI->rcpath, make_key(buffer, PI, rcs), val);
}

const char* BBP_read_string(struct plugin_info *PI, char *dest, const char *rcs, const char *def)
{
    char buffer[256];
    const char *p = ReadString(PI->rcpath, make_key(buffer, PI, rcs), def);
    if (p && dest) return strcpy(dest, p);
    return p;
}

int BBP_read_int(struct plugin_info *PI, const char *rcs, int def)
{
    char buffer[256];
    return ReadInt(PI->rcpath, make_key(buffer, PI, rcs), def);
}

bool BBP_read_bool(struct plugin_info *PI, const char *rcs, bool def)
{
    char buffer[256];
    return ReadBool(PI->rcpath, make_key(buffer, PI, rcs), def);
}


const char * BBP_read_value(struct plugin_info *PI, const char *rcs, LONG *pPos)
{
    char buffer[256];
    return ReadValue(PI->rcpath, make_key(buffer, PI, rcs), pPos);
}

void BBP_rename_setting(struct plugin_info *PI, const char *rcs, const char *rcnew)
{
    char buffer1[256], buffer2[256];
    RenameSetting(PI->rcpath, make_key(buffer1, PI, rcs),
        rcnew ? make_key(buffer2, PI, rcnew) : NULL);
}

void write_rc(struct plugin_info *PI, void *v)
{
    if (NULL == PI->rc_key)
        return;

    if (v == &PI->xpos)
    {
        BBP_write_string    (PI, "placement", BBP_placement_string(PI->place));
        BBP_write_int       (PI, "position.x", PI->xpos);
        BBP_write_int       (PI, "position.y", PI->ypos);
    }
    else
    if (v == &PI->width)
    {
        BBP_write_int       (PI, "width",  PI->width);
        BBP_write_int       (PI, "height", PI->height);
    }
    else
    if (v == &PI->useSlit)
        BBP_write_bool      (PI, "useSlit", PI->useSlit);
    else
    if (v == &PI->alwaysOnTop)
        BBP_write_bool      (PI, "alwaysOnTop", PI->alwaysOnTop);
    else
    if (v == &PI->autoHide)
        BBP_write_bool      (PI, "autoHide", PI->autoHide);
    else
    if (v == &PI->clickRaise)
        BBP_write_bool      (PI, "clickRaise", PI->clickRaise);
    else
    if (v == &PI->snapWindow)
        BBP_write_bool      (PI, "snapWindow", PI->snapWindow);
    else
    if (v == &PI->pluginToggle)
        BBP_write_bool      (PI, "pluginToggle", PI->pluginToggle);
    else
    if (v == &PI->alphaEnabled)
        BBP_write_bool      (PI, "alpha.enabled", PI->alphaEnabled);
    else
    if (v == &PI->alphaValue)
        BBP_write_int       (PI, "alpha.value", PI->alphaValue);
    else
    if (v == &PI->orient_vertical)
        BBP_write_string    (PI, "orientation", PI->orient_vertical ? "vertical" : "horizontal");
}

//===========================================================================

bool BBP_read_window_modes(struct plugin_info *PI, const char *rcfile)
{
    if (0 == PI->rcpath[0])
        BBP_get_rcpath(PI->rcpath, PI->hInstance, rcfile);

    PI->xpos = BBP_read_int(PI,  "position.x", 20);
    PI->ypos = BBP_read_int(PI,  "position.y", 20);

    const char *place_string = BBP_read_string(PI, NULL, "placement", NULL);
    if (place_string)
        PI->place = BBP_get_placement(place_string);

    PI->hMon = GetMonitorRect(&PI->xpos, &PI->mon_rect, GETMON_FROM_POINT);
    set_place(PI);

    PI->useSlit         = BBP_read_bool(PI, "useSlit", false);
    PI->alwaysOnTop     = BBP_read_bool(PI, "alwaysOnTop", false);
    PI->autoHide        = BBP_read_bool(PI, "autoHide", false);
    PI->snapWindow      = BBP_read_bool(PI, "snapWindow", true);
    PI->pluginToggle    = BBP_read_bool(PI, "pluginToggle", true);
    PI->clickRaise      = BBP_read_bool(PI, "clickRaise", true);
    PI->alphaEnabled    = BBP_read_bool(PI, "alpha.enabled", false);
    PI->alphaValue           = (BYTE)BBP_read_int(PI,  "alpha.value",  192);
    PI->orient_vertical = PI->is_bar || 0 == stricmp("vertical", BBP_read_string(PI, NULL, "orientation", "vertical"));
    if (NULL == place_string) {
        BBP_write_window_modes(PI);
        return false;
    }
    return true;
}

void BBP_write_window_modes(struct plugin_info *PI)
{
    write_rc(PI, &PI->xpos);
    write_rc(PI, &PI->useSlit);
    write_rc(PI, &PI->alwaysOnTop);
    write_rc(PI, &PI->autoHide);
    write_rc(PI, &PI->clickRaise);
    write_rc(PI, &PI->snapWindow);
    write_rc(PI, &PI->pluginToggle);
    write_rc(PI, &PI->alphaEnabled);
    write_rc(PI, &PI->alphaValue);
    if (false == PI->is_bar)
        write_rc(PI, &PI->orient_vertical);
}

//===========================================================================

//===========================================================================

n_menu * BBP_n_placementmenu(struct plugin_info *PI, n_menu *m)
{
    n_menu *P; int n, last;
    P = n_submenu(m, "Placement");
    last = PI->is_bar ? POS_BottomRight : POS_Center;
    for (n = 1; n <= last; n++)
    {
        if (POS_BottomLeft == n || POS_CenterLeft == n)
            n_menuitem_nop(P, NULL);

        char b2[80];
        sprintf(b2, "placement %s", placement_strings[n]);
        n_menuitem_bol(P, menu_placement_strings[n], b2, PI->place == n);
    }
    return P;
}

n_menu * BBP_n_orientmenu(struct plugin_info *PI, n_menu *m)
{
    n_menu *o = n_submenu(m, "Orientation");
    n_menuitem_bol(o, "Vertical", "orientation vertical",  false != PI->orient_vertical);
    n_menuitem_bol(o, "Horizontal", "orientation horizontal",  false == PI->orient_vertical);
    return o;
}

void BBP_n_insertmenu(struct plugin_info *PI, n_menu *m)
{
    if (PI->hSlit)
    {
        n_menuitem_bol(m, "Use Slit", "useSlit", PI->useSlit);
    }

    if (false == PI->useSlit || NULL == PI->hSlit)
    {
        n_menuitem_bol(m, "Auto Hide", "autoHide",  PI->autoHide);
        n_menuitem_bol(m, "Always On Top", "alwaysOnTop",  PI->alwaysOnTop);
        if (false == PI->alwaysOnTop)
            n_menuitem_bol(m, "Raise on DeskClick", "clickRaise", PI->clickRaise);
        n_menuitem_bol(m, "Snap To Edges", "snapWindow",  PI->snapWindow);
        n_menuitem_bol(m, "Toggle With Plugins", "pluginToggle",  PI->pluginToggle);
        n_menuitem_nop(m, NULL);
        n_menuitem_bol(m, "Transparency", "alpha.enabled",  PI->alphaEnabled);
        n_menuitem_int(m, "Alpha Value", "alpha.value",  PI->alphaValue, 0, 255);
    }
    // n_menuitem_bol(m, "Visible", "visible",  PI->visible);
}

//===========================================================================
#else // BBPLUGIN_NOMENU
//===========================================================================

#define write_rc(a,b)
#define BBP_handle_broam(a,b) 0
static void pos_changed(plugin_info* PI) {}
static void process_broam(plugin_info* PI, const char *broam, int f) {}
plugin_info *BBP_create_info(void)
{
    plugin_info *PI;
    PI = (plugin_info *)m_alloc(sizeof (plugin_info));
    memset(PI, 0, sizeof(plugin_info));
    PI->pos_changed = pos_changed;
    PI->process_broam = process_broam;
    return PI;
}

#endif // def BBPLUGIN_NOMENU

//===========================================================================

void BBP_set_visible(plugin_info *PI, bool visible)
{
    if (PI->visible != visible)
    {
        PI->visible = visible;
        BBP_set_window_modes(PI);
    }
}

void BBP_exit_moving(plugin_info *PI)
{
    if (false == PI->inSlit && (PI->is_moving || PI->is_sizing))
    {
        RECT r;
        GetWindowRect(PI->hwnd, &r);

        if (PI->is_sizing) {
            PI->width = r.right - r.left;
            PI->height = r.bottom - r.top;
            write_rc(PI, &PI->width);
        } else {
            PI->xpos = r.left;
            PI->ypos = r.top;
            PI->place = get_place(PI);
            write_rc(PI, &PI->xpos);
        }
    }

    PI->is_moving = false;
    PI->is_sizing = false;
}

void BBP_set_place(plugin_info *PI, int n)
{
    PI->place = n;
    BBP_set_window_modes(PI);
    write_rc(PI, &PI->xpos);
}

void BBP_set_size(plugin_info *PI, int w, int h)
{
    if (PI->width != w || PI->height != h)
    {
        PI->width = w, PI->height = h;
        BBP_set_window_modes(PI);
    }
}

void BBP_reconfigure(plugin_info *PI)
{
    if (false == PI->inSlit)
        GetMonitorRect(PI->hMon, &PI->mon_rect, GETMON_FROM_MONITOR);

    InvalidateRect(PI->hwnd, NULL, FALSE);
    BBP_set_window_modes(PI);
}

//===========================================================================
bool BBP_get_rcpath(char *rcpath, HINSTANCE hInstance, const char *rcfile)
{
    return FindRCFile(rcpath, rcfile, hInstance);
}

void BBP_edit_file(const char *path)
{
    if (BBVERSION_LEAN) {
        SendMessage(GetBBWnd(), BB_EDITFILE, (WPARAM)-1, (LPARAM)path);
    } else {
        char szTemp[MAX_PATH];
        GetBlackboxEditor(szTemp);
        BBExecute(NULL, NULL, szTemp, path, NULL, SW_SHOWNORMAL, false);
    }
}

//===========================================================================
#ifndef BBPLUGIN_NOMENU
//===========================================================================

bool BBP_broam_bool(struct plugin_info *PI, const char *temp, const char *key, bool *ip)
{
    int n = strlen(key);
    const char *s;
    if (memicmp(temp, key, n))
        return false;
    s = temp + n;
    while (' ' == *s) ++s;
    if (0 == *s || 0 == stricmp(s, "toggle"))
        *ip = false == *ip;
    else
    if (0 == stricmp(s, "false"))
        *ip = false;
    else
    if (0 == stricmp(s, "true"))
        *ip = true;
    else
        return false;
    if (PI) BBP_write_bool(PI, key, *ip);
    return true;
}

bool BBP_broam_int(struct plugin_info *PI, const char *temp, const char *key, int *ip)
{
    int n = strlen(key);
    const char *s;
    if (memicmp(temp, key, n))
        return false;
    s = temp + n;
    if (' ' != *s)
        return false;
    while (' ' == *s)
        ++s;
    if (0 == *s)
        return false;
    *ip = atoi(s);
    if (PI) BBP_write_int(PI, key, *ip);
    return true;
}

bool BBP_broam_string(struct plugin_info *PI, const char *temp, const char *key, const char **ps)
{
    int n = strlen(key);
    const char *s;
    if (memicmp(temp, key, n))
        return false;
    s = temp + n;
    if (' ' != *s)
        return false;
    while (' ' == *s)
        ++s;
    if (0 == *s)
        return false;
    *ps = s;
    if (PI) BBP_write_string(PI, key, s);
    return true;
}

int BBP_handle_broam(struct plugin_info *PI, const char *temp)
{
    int v;
    const char *s;

    if (BBP_broam_bool(PI, temp, "useSlit", &PI->useSlit))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_bool(PI, temp, "pluginToggle", &PI->pluginToggle))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }
/*
    if (BBP_broam_bool(PI, temp, "visible", &PI->visible))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }
*/
    if (BBP_broam_bool(PI, temp, "alwaysOnTop", &PI->alwaysOnTop))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_bool(PI, temp, "clickRaise", &PI->clickRaise))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_bool(PI, temp, "AutoHide", &PI->autoHide))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_bool(PI, temp, "snapWindow", &PI->snapWindow))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_bool(PI, temp, "alpha.enabled", &PI->alphaEnabled))
    {
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_int(PI, temp, "alpha.value", &v))
    {
        PI->alphaValue = (BYTE)v;
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_string(PI, temp, "placement", &s))
    {
        int n = get_string_index(s, placement_strings);
        if (-1 != n) {
            BBP_set_place(PI, n);
        }
        return BBP_BROAM_HANDLED;
    }

    if (!stricmp(temp, "about"))
    {
        PI->about_box(_THIS);
        return BBP_BROAM_HANDLED;
    }

    if (!stricmp(temp, "editRC"))
    {
        BBP_edit_file(PI->rcpath);
        return BBP_BROAM_HANDLED;
    }

    if (0 == stricmp(temp, "readme"))
    {
        char temp[MAX_PATH];
        BBP_edit_file(set_my_path(PI->hInstance, temp, "readme.txt"));
        return BBP_BROAM_HANDLED;
    }

    if (BBP_broam_string(PI, temp, "orientation", &s))
    {
        if (!stricmp(s, "vertical"))
            PI->orient_vertical = true;
        else
        if (!stricmp(s, "horizontal"))
            PI->orient_vertical = false;
        BBP_set_window_modes(PI);
        return BBP_BROAM_HANDLED | BBP_BROAM_METRICS;
    }

    return 0;
}

//===========================================================================
#endif // def BBPLUGIN_NOMENU
//===========================================================================
// autohide

bool check_mouse(HWND hwnd)
{
    POINT pt;
    RECT rct;
    if (GetCapture() == hwnd)
        return 1;
    GetCursorPos(&pt);
    GetWindowRect(hwnd, &rct);
    if (PtInRect(&rct, pt))
        return 1;
    return 0;
}

void set_autohide_timer(plugin_info *PI, bool set)
{
    if (set)
        SetTimer(PI->hwnd, AUTOHIDE_TIMER, 100, NULL);
    else
        KillTimer(PI->hwnd, AUTOHIDE_TIMER);
}

void BBP_set_autoHide(plugin_info *PI, bool set)
{
    if (set != PI->autoHide)
    {
        PI->autoHide = set;
        write_rc(PI, &PI->autoHide);
    }

    PI->suspend_autohide = false;
    PI->auto_shown = set && check_mouse(PI->hwnd);
    if (PI->auto_shown)
        set_autohide_timer(PI, true);
    BBP_set_window_modes(PI);
}

//===========================================================================

//===========================================================================
/*
    // handled messages

    WM_NCCREATE:
    BB_BROADCAST:

    WM_ENTERSIZEMOVE:
    WM_EXITSIZEMOVE:
    WM_WINDOWPOSCHANGING:

    BB_AUTORAISE:
    WM_MOUSEMOVE:
    WM_NCLBUTTONDOWN:
    WM_NCHITTEST:

    WM_TIMER:   // handle autohide
    WM_CLOSE:   // return 0;
*/

//===========================================================================

LRESULT CALLBACK BBP_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static UINT msgs[] = { BB_RECONFIGURE, BB_BROADCAST, BB_DESKCLICK, 0};

    LRESULT Result = 0;
    plugin_info *PI  = (plugin_info *)GetWindowLongPtr(hwnd, 0);

    //dbg_printf("message %x", message);

    if (NULL == PI)
    {
        if (WM_NCCREATE == message)
        {
            // bind the window to the structure
            PI = (plugin_info *)((CREATESTRUCT*)lParam)->lpCreateParams;
            PI->hwnd = hwnd;
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)PI);
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    switch (message)
    {
        case WM_CREATE:
            SendMessage(GetBBWnd(), BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            MakeSticky(hwnd);
            goto pass_nothing;

        case WM_DESTROY:
            SendMessage(GetBBWnd(), BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
            RemoveSticky(hwnd);
            goto pass_nothing;

        // ==========
        case BB_BROADCAST:
        {
            const char *temp = (LPCSTR)lParam;
            int f, len;

            if (0 == stricmp(temp, "@BBShowPlugins")) {
                PI->toggled_hidden = false;
                BBP_set_window_modes(PI);
                goto pass_result;
            }
            if (0 == stricmp(temp, "@BBHidePlugins")) {
                if (PI->pluginToggle) {
                    PI->toggled_hidden = true;
                    BBP_set_window_modes(PI);
                }
                goto pass_result;
            }

            if ('@' != *temp++)
                goto pass_nothing;

            len = PI->broam_key_len;
            if (len && 0 == memicmp(temp, PI->broam_key, len) && '.' == temp[len]) {
                f = 0;
                temp += len + 1;
                goto do_broam;
            }

            if (PI->next)
                goto pass_nothing;

            len = PI->broam_key_len_common;
            if (len && 0 == memicmp(temp, PI->broam_key, len)) {
                f = BBP_BROAM_COMMON;
                temp += len;
                goto do_broam;
            }

            goto pass_nothing;

        do_broam:
            f |= BBP_handle_broam(PI, temp);
            PI->process_broam(_THIS_ temp, f);
            goto pass_result;
        }

        // ==========

        case BB_DESKCLICK:
            if (lParam == 0
             && PI->clickRaise
             && false == PI->alwaysOnTop
             && false == PI->inSlit)
                SetWindowPos(hwnd, HWND_TOP,
                    0,0,0,0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
            goto pass_nothing;

        // ==========

        case WM_WINDOWPOSCHANGING:
            if (PI->is_moving) {
                if (PI->snapWindow
                 && false == PI->inSlit
                 && 0 == (0x8000 & GetAsyncKeyState(VK_SHIFT)))
                    SnapWindowToEdge((WINDOWPOS*)lParam, 10,
                        PI->is_sizing ? SNAP_FULLSCREEN|SNAP_SIZING
                        : SNAP_FULLSCREEN
                        );
                if (PI->is_sizing) {
                    WINDOWPOS* wp = (WINDOWPOS*)lParam;
                    if (wp->cx < 12) wp->cx = 12;
                    if (wp->cy < 12) wp->cy = 12;
                }
            }
            goto pass_nothing;

        case WM_WINDOWPOSCHANGED:
            if (PI->is_sizing) {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                PI->width = wp->cx;
                PI->height = wp->cy;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            goto pass_nothing;

        case WM_ENTERSIZEMOVE:
            PI->is_moving = true;
            goto pass_nothing;

        case WM_EXITSIZEMOVE:
            BBP_exit_moving(PI);
            BBP_set_autoHide(PI, PI->autoHide);
            if (PI->inSlit)
                SendMessage(PI->hSlit, SLIT_UPDATE, 0, (LPARAM)PI->hwnd);
            goto pass_nothing;

        // ==========
        case WM_LBUTTONDOWN:
            SetFocus(hwnd);
            UpdateWindow(hwnd);
            if (false == PI->inSlit && (MK_CONTROL & wParam)) {
                // start moving, when control-key is held down
                PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
                goto pass_result;
            }
            goto pass_nothing;

        case WM_MOUSEMOVE:
            if (false == PI->mouse_over)
            {
                PI->mouse_over = true;
                set_autohide_timer(PI, true);
            }

            if (PI->auto_hidden)
            {
                PI->auto_shown = true;
                BBP_set_window_modes(PI);
                goto pass_result;
            }

            goto pass_nothing;

        case WM_TIMER:
            if (AUTOHIDE_TIMER != wParam)
                goto pass_nothing;

            if (check_mouse(hwnd))
                goto pass_result;
#if 0
            {
                POINT pt;
                GetCursorPos(&pt);
                if (PI->hMon != GetMonitorRect(&pt, NULL, GETMON_FROM_POINT))
                    goto pass_result;
            }
#endif
            if (PI->mouse_over) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                PostMessage(hwnd, WM_MOUSELEAVE, 0, MAKELPARAM(pt.x, pt.y));
                PI->mouse_over = false;
            }

            if (PI->auto_shown) {
                if (PI->suspend_autohide && BBVERSION_LEAN)
                    goto pass_result;
                PI->auto_shown = false;
                BBP_set_window_modes(PI);
            }

            set_autohide_timer(PI, false);
            goto pass_result;


        case BB_AUTOHIDE:
            if (PI->inSlit)
                PostMessage(PI->hSlit, message, wParam, lParam);

            if (wParam)
                PI->suspend_autohide |= lParam;
            else
                PI->suspend_autohide &= ~lParam;

            if (PI->suspend_autohide && PI->auto_hidden) {
                PI->auto_shown = true;
                BBP_set_window_modes(PI);
            }

            set_autohide_timer(PI, true);
            goto pass_result;

        case WM_CLOSE:
            goto pass_result;

        case WM_ERASEBKGND:
            Result = TRUE;
            goto pass_result;

        default:
        pass_nothing:
            return PI->wnd_proc(_THIS_ hwnd, message, wParam, lParam, NULL);
    }
pass_result:
    return PI->wnd_proc(_THIS_ hwnd, message, wParam, lParam, &Result);
}

//===========================================================================

//===========================================================================
struct class_info
{
    struct class_info *next;
    char name[48];
    HINSTANCE hInstance;
    int refc;
};


static struct class_info *CI;

static struct class_info **find_class(const char *name)
{
    struct class_info **pp;
    for (pp = &CI; *pp; pp = &(*pp)->next)
        if (0 == stricmp((*pp)->name, name)) break;
    return pp;
}

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

static int class_info_register(const char *class_name, HINSTANCE hInstance)
{
    struct class_info *p = *find_class(class_name);
    if (NULL == p)
    {
        WNDCLASS wc;
        ZeroMemory(&wc,sizeof(wc));

        wc.lpfnWndProc  = BBP_WndProc;  // our window procedure
        wc.hInstance    = hInstance;    // hInstance of .dll
        wc.lpszClassName = class_name;  // our window class name
        wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;// | CS_DROPSHADOW;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.cbWndExtra = sizeof (void*);

        if (FALSE == RegisterClass(&wc))
        {
            //dbg_printf("failed to register %s", wc.lpszClassName);
            return 0;
        }

        p = (struct class_info*)m_alloc(sizeof(struct class_info));
        p->next = CI;
        CI = p;
        p->refc = 0;
        strcpy(p->name, class_name);
        p->hInstance = hInstance;
        //dbg_printf("registered class <%s> %x", wc.lpszClassName, wc.hInstance);
    }

    p->refc ++;
    return 1;
}

static void class_info_decref(const char *name)
{
    struct class_info *p, **pp = find_class(name);
    if (NULL != (p = *pp) && --p->refc <= 0)
    {
        UnregisterClass(p->name, p->hInstance);
        //dbg_printf("unregistered class <%s> %x", p->name, p->hInstance);
        *pp = p->next;
        m_free(p);
    }
}

int BBP_Init_Plugin(plugin_info *PI)
{
    if (PI->broam_key) {
        const char *s;
        PI->broam_key_len = strlen(PI->broam_key);
        s = strchr(PI->broam_key, '.');
        if (s) PI->broam_key_len_common = s - PI->broam_key + 1;
    }

    PI->is_alpha = 255;
    PI->visible = true;

    if (0 == class_info_register(PI->class_name, PI->hInstance))
        return 0;

    //dbg_printf("creating window <%s>", PI->class_name);
    PI->hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        PI->class_name,
        NULL,
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        PI->xpos,
        PI->ypos,
        PI->width,
        PI->height,
        NULL,           // parent window
        NULL,           // no menu
        PI->hInstance,  // hInstance of .dll
        PI              // init_data
        );

    if (NULL == PI->hwnd)
    {
        class_info_decref(PI->class_name);
        return 0;
    }

    BBP_set_window_modes(PI);
    return 1;
}

void BBP_Exit_Plugin(plugin_info *PI)
{
    //dbg_printf("window destroying <%s>", PI->class_name);
    if (PI->hwnd)
    {
        if (PI->inSlit)
            SendMessage(PI->hSlit, SLIT_REMOVE, 0, (LPARAM)PI->hwnd);
        DestroyWindow(PI->hwnd);
        class_info_decref(PI->class_name);
    }
}

static DWORD WINAPI QuitThread (void *pv)
{
    FreeLibraryAndExitThread((HMODULE)pv, 0);
#if _MSC_VER < 1400
    return 0; // never returns
#endif
}

int BBP_messagebox(
    plugin_info *PI,
    int flags,
    const char *fmt, ...)
{
    va_list args;
    char buffer[4000];
    DWORD threadId;
    HMODULE hLib;

    va_start(args, fmt);
    GetModuleFileName(PI->hInstance, buffer, sizeof buffer);
    hLib = LoadLibrary(buffer);
    vsprintf(buffer, fmt, args);
    flags = MessageBox(NULL, buffer, PI->class_name, flags|MB_TOPMOST|MB_SETFOREGROUND);
    CloseHandle(CreateThread(NULL, 0, QuitThread, hLib, 0, &threadId));

    return flags;
}

//===========================================================================
