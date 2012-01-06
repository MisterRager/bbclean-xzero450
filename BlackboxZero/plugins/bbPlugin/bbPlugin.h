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

#ifndef _BBPLUGIN_H_
#define _BBPLUGIN_H_

typedef struct plugin_info plugin_info;

struct plugin_info
{
    struct plugin_info *next;
    const char *class_name;
    HINSTANCE hInstance;
    HWND hwnd;

    HWND hSlit;
    HMONITOR hMon;
    RECT mon_rect;

    // config vars
    int place;

    int xpos;
    int ypos;
    int width;
    int height;

    bool useSlit;
    bool alwaysOnTop;
    bool autoHide;
    bool alphaEnabled;
    BYTE alphaValue;

    bool clickRaise;
    bool snapWindow;
    bool pluginToggle;
    bool visible;

    bool orient_vertical;
    bool is_bar;

    // state vars
    bool inSlit;
    bool toggled_hidden;
    bool auto_hidden;
    bool auto_shown;
    bool mouse_over;
    char suspend_autohide;

    bool is_moving;
    bool is_sizing;
    bool is_visible;
    BYTE is_alpha;

    // misc
    char rcpath[MAX_PATH];
    const char *rc_key;
    const char *broam_key;
    int broam_key_len;
    int broam_key_len_common;

#ifdef __cplusplus
    plugin_info()
    {
        // clear everything but vtable
        memset(&next, 0, sizeof(plugin_info) - sizeof(void*));
    }
    virtual ~plugin_info() {}
#define _THIS
#define _THIS_
#define _VIRTUAL(t,f) virtual t f
#define _BODY {}
#define _PURE = 0
#define _FN(f) f
#else
    void *p_data;
#define _THIS plugin_info*
#define _THIS_ _THIS,
#define _VIRTUAL(t,f) t (*f)
#define _BODY
#define _PURE
#endif

    _VIRTUAL(void, process_broam)(_THIS_ const char *temp, int f) _BODY;
    _VIRTUAL(void, pos_changed)(_THIS) _BODY;
    _VIRTUAL(void, about_box)(_THIS) _BODY;
    _VIRTUAL(LRESULT, wnd_proc)(_THIS_ HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret) _PURE;

#undef _THIS
#undef _THIS_
#undef _VIRTUAL
#undef _BODY
#undef _PURE
};

enum Plugin_Positions
{
    POS_User        = 0,

    POS_TopLeft       ,
    POS_TopCenter     ,
    POS_TopRight      ,

    POS_BottomLeft    ,
    POS_BottomCenter  ,
    POS_BottomRight   ,

    POS_CenterLeft    ,
    POS_CenterRight   ,
    POS_Center        ,

    POS_Top           ,
    POS_Bottom        ,
    POS_Left          ,
    POS_Right         ,
    POS_CenterH ,
    POS_CenterV ,

    POS_LAST
};

//===========================================================================
#ifdef __cplusplus
extern "C" {
#endif

#ifdef BBP_LIB
#define BBP_DLL_EXPORT __declspec(dllexport)
#else
#define BBP_DLL_EXPORT
#endif

#define BBVERSION_LEAN (BBP_bbversion()>=2)
#define BBVERSION_XOB (BBP_bbversion()==1)
#define BBVERSION_09X (BBP_bbversion()==0)

#define AUTOHIDE_TIMER 1

BBP_DLL_EXPORT int  BBP_Init_Plugin(plugin_info *PI);
BBP_DLL_EXPORT void BBP_Exit_Plugin(plugin_info *PI);
BBP_DLL_EXPORT bool BBP_handle_message(plugin_info *PI, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
BBP_DLL_EXPORT int BBP_messagebox(plugin_info *PI, int flags, const char *fmt, ...);

BBP_DLL_EXPORT bool BBP_get_rcpath(char *rcpath, HINSTANCE hInstance, const char *rcfile);
BBP_DLL_EXPORT bool BBP_read_window_modes(plugin_info *PI, const char *rcfile);
BBP_DLL_EXPORT void BBP_write_window_modes(plugin_info *PI);

BBP_DLL_EXPORT void BBP_set_window_modes(plugin_info *PI);
BBP_DLL_EXPORT void BBP_set_place(plugin_info *PI, int place);
BBP_DLL_EXPORT void BBP_set_size(plugin_info *PI, int w, int h);
BBP_DLL_EXPORT void BBP_set_visible(plugin_info *PI, bool hidden);
BBP_DLL_EXPORT void BBP_set_autoHide(plugin_info *PI, bool set);

BBP_DLL_EXPORT void BBP_reconfigure(plugin_info *PI);

BBP_DLL_EXPORT const char *BBP_placement_string(int pos);
BBP_DLL_EXPORT int BBP_get_placement(const char*place_string);

BBP_DLL_EXPORT void BBP_write_string(plugin_info *PI, const char *rcs, const char *val);
BBP_DLL_EXPORT void BBP_write_int(plugin_info *PI, const char *rcs, int val);
BBP_DLL_EXPORT void BBP_write_bool(plugin_info *PI, const char *rcs, bool val);

BBP_DLL_EXPORT const char* BBP_read_string(plugin_info *PI, char *dest, const char *rcs, const char *def);
BBP_DLL_EXPORT int  BBP_read_int(plugin_info *PI, const char *rcs, int def);
BBP_DLL_EXPORT bool BBP_read_bool(plugin_info *PI, const char *rcs, bool def);
BBP_DLL_EXPORT void BBP_rename_setting(plugin_info *PI, const char *rcs, const char *rcnew);
BBP_DLL_EXPORT const char * BBP_read_value(plugin_info *PI, const char *rcs, LONG *pPos);

BBP_DLL_EXPORT void BBP_edit_file(const char *path);
BBP_DLL_EXPORT int BBP_bbversion(void);

#define BBP_clear(_struct,_first)\
    memset(&_first, 0, sizeof(*_struct)-((char*)&_struct->_first-(char*)_struct))

//===========================================================================
typedef struct n_menu n_menu;

BBP_DLL_EXPORT n_menu *n_makemenu(const char *title);
BBP_DLL_EXPORT n_menu *n_submenu(n_menu *m, const char *text);

BBP_DLL_EXPORT Menu *n_convertmenu(n_menu *m, const char *broam_key, bool popup);
BBP_DLL_EXPORT void n_menuitem_nop(n_menu *m, const char *text ISNULL);
BBP_DLL_EXPORT void n_menuitem_cmd(n_menu *m, const char *text, const char *cmd);
BBP_DLL_EXPORT void n_menuitem_bol(n_menu *m, const char *text, const char *cmd, bool check);
BBP_DLL_EXPORT void n_menuitem_int(n_menu *m, const char *text, const char *cmd, int val, int vmin, int vmax);
BBP_DLL_EXPORT void n_menuitem_str(n_menu *m, const char *text, const char *cmd, const char *init);
BBP_DLL_EXPORT void n_disable_lastitem(n_menu *m);

BBP_DLL_EXPORT void BBP_n_insertmenu(plugin_info *PI, n_menu *m);
BBP_DLL_EXPORT n_menu *BBP_n_placementmenu(plugin_info *PI, n_menu *m);
BBP_DLL_EXPORT n_menu *BBP_n_orientmenu(plugin_info *PI, n_menu *m);
BBP_DLL_EXPORT bool BBP_broam_int(plugin_info *PI, const char *temp, const char *key, int *ip);
BBP_DLL_EXPORT bool BBP_broam_string(plugin_info *PI, const char *temp, const char *key, const char **ps);
BBP_DLL_EXPORT bool BBP_broam_bool(plugin_info *PI, const char *temp, const char *key, bool *ip);

BBP_DLL_EXPORT void n_showmenu(plugin_info *PI, n_menu *m, bool popup, int flags, ...);
BBP_DLL_EXPORT void BBP_showmenu(plugin_info *PI, Menu *pMenu, int flags);

//===========================================================================

BBP_DLL_EXPORT bool check_mouse(HWND hwnd);

#ifndef __cplusplus
BBP_DLL_EXPORT plugin_info *BBP_create_info(void);
#endif

#define BBP_BROAM_HANDLED 1
#define BBP_BROAM_COMMON 4
#define BBP_BROAM_METRICS 2

#ifdef __cplusplus
}
#endif
//===========================================================================
#endif

