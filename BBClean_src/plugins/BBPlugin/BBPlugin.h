/*
 ============================================================================

  This file is part of the bbLean source code.

  Copyright © 2004 grischka
  http://bb4win.sf.net/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2 or later).

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 ============================================================================
*/  // BBPlugin.h

struct plugin_info
{
	HINSTANCE hInstance;
	struct plugin_info *next;

	const char *class_name;
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
	bool transparency;
	BYTE alpha;

	bool clickRaise;
	bool snapWindow;
	bool pluginToggle;

	bool orient_vertical;

	// state vars
	bool inSlit;
	bool hidden;
	bool toggled_hidden;
	bool auto_hidden;
	bool auto_shown;
	bool is_moving;
	bool is_sizing;

	bool is_visible;
	BYTE has_alpha;

	// misc
	char rcpath[MAX_PATH];
	const char *rc_key;
	const char *broam_key;
	int broam_key_len;

	bool is_bar;

	// virtual functions
	virtual void process_broam(const char *temp, int f) {}
	virtual void show_menu(bool f) {}
	virtual void pos_changed(void) {}
	virtual LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret) = 0;
	virtual ~plugin_info() {}
};

enum
{
	POS_User          ,

	POS_TopLeft       ,
	POS_TopCenter     ,
	POS_TopRight      ,

	POS_BottomLeft    ,
	POS_BottomCenter  ,
	POS_BottomRight   ,

	POS_CenterLeft    ,
	POS_CenterRight   ,

	POS_Top           ,
	POS_Bottom        ,
	POS_Left          ,
	POS_Right         ,

	POS_LAST
};

//===========================================================================
#ifdef BBP_LIB
#define BBP_DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define BBP_DLL_EXPORT extern "C"
#endif

#define BBVERSION_LEAN 2
#define BBVERSION_XOB 1
#define BBVERSION_09X 0

#define AUTOHIDE_TIMER 1

#define BBP_clear(_P) ZeroMemory(&_P->hInstance,\
	sizeof *_P - ((int)&_P->hInstance - (int)_P));


BBP_DLL_EXPORT int  BBP_Init_Plugin(struct plugin_info *PI);
BBP_DLL_EXPORT void BBP_Exit_Plugin(struct plugin_info *PI);
BBP_DLL_EXPORT bool BBP_handle_message(struct plugin_info *PI, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

BBP_DLL_EXPORT bool BBP_get_rcpath(char *rcpath, HINSTANCE hInstance, const char *rcfile);
BBP_DLL_EXPORT bool BBP_read_window_modes(struct plugin_info *PI, const char *rcfile);
BBP_DLL_EXPORT void BBP_write_window_modes(struct plugin_info *PI);

BBP_DLL_EXPORT void BBP_set_window_modes(struct plugin_info *PI);
BBP_DLL_EXPORT void BBP_set_place(struct plugin_info *PI, int place);
BBP_DLL_EXPORT void BBP_set_size(struct plugin_info *PI, int w, int h);
BBP_DLL_EXPORT void BBP_set_hidden(struct plugin_info *PI, bool hidden);
BBP_DLL_EXPORT void BBP_set_autoHide(struct plugin_info *PI, bool set);
BBP_DLL_EXPORT void BBP_reconfigure(struct plugin_info *PI);

BBP_DLL_EXPORT const char *BBP_placement_string(int pos);

BBP_DLL_EXPORT void BBP_write_string(struct plugin_info *PI, const char *rcs, const char *val);
BBP_DLL_EXPORT void BBP_write_int(struct plugin_info *PI, const char *rcs, int val);
BBP_DLL_EXPORT void BBP_write_bool(struct plugin_info *PI, const char *rcs, bool val);

BBP_DLL_EXPORT const char* BBP_read_string(struct plugin_info *PI, char *dest, const char *rcs, const char *def);
BBP_DLL_EXPORT int  BBP_read_int(struct plugin_info *PI, const char *rcs, int def);
BBP_DLL_EXPORT bool BBP_read_bool(struct plugin_info *PI, const char *rcs, bool def);

//===========================================================================
struct n_menu;

BBP_DLL_EXPORT n_menu *n_makemenu(const char *title);
BBP_DLL_EXPORT n_menu *n_submenu(n_menu *m, const char *text);
BBP_DLL_EXPORT void n_showmenu(n_menu *m, const char *broam_key, bool popup);
BBP_DLL_EXPORT void n_menuitem_nop(n_menu *m, const char *text = NULL);
BBP_DLL_EXPORT void n_menuitem_cmd(n_menu *m, const char *text, const char *cmd);
BBP_DLL_EXPORT void n_menuitem_bol(n_menu *m, const char *text, const char *cmd, bool check);
BBP_DLL_EXPORT void n_menuitem_int(n_menu *m, const char *text, const char *cmd, int val, int vmin, int vmax);
BBP_DLL_EXPORT void n_menuitem_str(n_menu *m, const char *text, const char *cmd, const char *init);
BBP_DLL_EXPORT void n_disable_lastitem(n_menu *m);

BBP_DLL_EXPORT void BBP_n_insertmenu(struct plugin_info *PI, n_menu *m);
BBP_DLL_EXPORT void BBP_n_placementmenu(struct plugin_info *PI, n_menu *m);
BBP_DLL_EXPORT void BBP_n_orientmenu(struct plugin_info *PI, n_menu *m);

//===========================================================================

BBP_DLL_EXPORT int imax(int a, int b);
BBP_DLL_EXPORT int imin(int a, int b);
BBP_DLL_EXPORT int iminmax(int a, int b, int c);
BBP_DLL_EXPORT int get_string_index (const char *key, const char **string_list);
BBP_DLL_EXPORT void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r);
BBP_DLL_EXPORT bool check_mouse(HWND hwnd);

//===========================================================================
