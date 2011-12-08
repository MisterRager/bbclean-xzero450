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
*/

#include "../../blackbox/BBApi.h"
//#include <sbex.h>
#include "../../blackbox/m_alloc.h"
#include "BBPlugin.h"

#define ST static

ST int BBVersion;

//===========================================================================
int imax(int a, int b) {
	return a>b?a:b;
}

int imin(int a, int b) {
	return a<b?a:b;
}

int iminmax(int a, int b, int c) {
	if (a<b) a=b;
	if (a>c) a=c;
	return a;
}

void BitBltRect(HDC hdc_to, HDC hdc_from, RECT *r)
{
	BitBlt(
		hdc_to,
		r->left, r->top, r->right-r->left, r->bottom-r->top,
		hdc_from,
		r->left, r->top,
		SRCCOPY
		);
}

static void free_str(char**p)
{
	if (*p) delete *p, *p = NULL;
}

static char *new_str(const char *s)
{
	return s ? strcpy(new char [strlen(s)+1], s) : NULL;
}

//===========================================================================
// Function: get_string_index
// Purpose:  search for a match in a string array
// In:       searchstring, array
// Out:      index or -1
//===========================================================================

int get_string_index (const char *key, const char **string_list)
{
	int i;
	for (i=0; *string_list; i++, string_list++)
		if (0==stricmp(key, *string_list)) return i;
	return -1;
}

//*****************************************************************************

//*****************************************************************************

int get_place(struct plugin_info *PI)
{
	PI->hMon = GetMonitorRect(PI->hwnd, &PI->mon_rect, GETMON_FROM_WINDOW);

	int sw  = PI->mon_rect.right  - PI->mon_rect.left;
	int sh  = PI->mon_rect.bottom - PI->mon_rect.top;
	int x   = PI->xpos - PI->mon_rect.left;
	int y   = PI->ypos - PI->mon_rect.top;
	int w   = PI->width;
	int h   = PI->height;

	x = iminmax(x, 0, sw - w);
	y = iminmax(y, 0, sh - h);

	bool top        = y == 0;
	bool vcenter    = y == sh/2 - h/2;
	bool bottom     = y == (sh - h);

	bool left       = x == 0;
	bool center     = x == sw/2 - w/2;
	bool right      = x == (sw - w);

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

	return POS_User;
}

//===========================================================================

void set_place(struct plugin_info *PI)
{
	int sw  = PI->mon_rect.right  - PI->mon_rect.left;
	int sh  = PI->mon_rect.bottom - PI->mon_rect.top;
	int x   = PI->xpos - PI->mon_rect.left;
	int y   = PI->ypos - PI->mon_rect.top;
	int w   = PI->width;
	int h   = PI->height;

	int place = PI->place;

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

void BBP_set_window_modes(struct plugin_info *PI)
{
	if (PI->hidden)
	{
		if (PI->is_visible)
		{
			ShowWindow(PI->hwnd, SW_HIDE);
			PI->is_visible = false;
			PI->pos_changed();
		}
		return;
	}

	bool useslit = PI->useSlit && PI->hSlit;
	PI->auto_hidden = false;

	if (PI->inSlit && false == useslit)
	{
		SendMessage(PI->hSlit, SLIT_REMOVE, 0, (LPARAM)PI->hwnd);
		PI->inSlit = false;
	}

	if (useslit)
	{
		RECT r; GetWindowRect(PI->hwnd, &r);
		int w = r.right - r.left;
		int h = r.bottom - r.top;
		bool update_size = (w != PI->width || h != PI->height);

		if (update_size)
			SetWindowPos(
				PI->hwnd,
				NULL,
				0,
				0,
				PI->width,
				PI->height,
				SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER
				);

		if (false == PI->inSlit)
		{
			BYTE trans = 255;
			if (PI->has_alpha != trans)
			{
				SetTransparency(PI->hwnd, trans);
				PI->has_alpha = trans;
			}
			SendMessage(PI->hSlit, SLIT_ADD, (WPARAM)PI->broam_key, (LPARAM)PI->hwnd);
			PI->inSlit = true;
		}
		else
		if (update_size)
		{
			SendMessage(PI->hSlit, SLIT_UPDATE, 0, (LPARAM)PI->hwnd);
		}
	}
	else
	{
		set_place(PI);
		int x = PI->xpos;
		int y = PI->ypos;

		if (PI->autoHide && false == PI->auto_shown)
		{
			int hangout = 1;
			int place = PI->place;
			if ((PI->orient_vertical && (place == POS_TopLeft || place == POS_BottomLeft))
				|| place == POS_CenterLeft || place == POS_Left)
			{
				x = PI->mon_rect.left + hangout - PI->width;
				PI->auto_hidden = true;
			}
			else
			if ((PI->orient_vertical && (place == POS_TopRight || place == POS_BottomRight))
				|| place == POS_CenterRight || place == POS_Right)
			{
				x = PI->mon_rect.right - hangout;
				PI->auto_hidden = true;
			}
			else
			if (place == POS_TopLeft || place == POS_TopCenter || place == POS_TopRight || place == POS_Top)
			{
				y = PI->mon_rect.top + hangout - PI->height;
				PI->auto_hidden = true;
			}
			else
			if (place == POS_BottomLeft || place == POS_BottomCenter || place == POS_BottomRight || place == POS_Bottom)
			{
				y = PI->mon_rect.bottom - hangout;
				PI->auto_hidden = true;
			}
		}

		HWND hwnd_after; UINT flags;

		if (WS_CHILD & GetWindowLong(PI->hwnd, GWL_STYLE))
		{
			flags = SWP_NOACTIVATE|SWP_NOZORDER;
			hwnd_after = NULL;
		}
		else
		{
			flags = SWP_NOACTIVATE;
			if (PI->alwaysOnTop || PI->auto_shown || PI->auto_hidden)
				hwnd_after = HWND_TOPMOST;
			else
				hwnd_after = HWND_NOTOPMOST;
		}

		SetWindowPos(
			PI->hwnd,
			hwnd_after,
			x,
			y,
			PI->width,
			PI->height,
			flags
			);

		BYTE trans;
		if (PI->auto_hidden) trans = 8;
		else
		if (PI->transparency) trans = PI->alpha;
		else trans = 255;

		if (PI->has_alpha != trans)
		{
			SetTransparency(PI->hwnd, trans);
			PI->has_alpha = trans;
		}
	}

	if (false == PI->is_visible)
	{
		ShowWindow(PI->hwnd, SW_SHOWNA);
		PI->is_visible = true;
	}

	PI->pos_changed();
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

	"Top"           ,
	"Bottom"        ,
	"Left"          ,
	"Right"         ,
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

//===========================================================================

void BBP_write_string(struct plugin_info *PI, const char *rcs, const char *val)
{
	if (0 == PI->rcpath[0]) return;
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	WriteString(PI->rcpath, buffer, val);
}

void BBP_write_int(struct plugin_info *PI, const char *rcs, int val)
{
	if (0 == PI->rcpath[0]) return;
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	WriteInt(PI->rcpath, buffer, val);
}

void BBP_write_bool(struct plugin_info *PI, const char *rcs, bool val)
{
	if (0 == PI->rcpath[0]) return;
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	WriteBool(PI->rcpath, buffer, val);
}

const char* BBP_read_string(struct plugin_info *PI, char *dest, const char *rcs, const char *def)
{
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	const char *p = ReadString(PI->rcpath, buffer, def);
	if (p && dest) return strcpy(dest, p);
	return p;
}

int BBP_read_int(struct plugin_info *PI, const char *rcs, int def)
{
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	return ReadInt(PI->rcpath, buffer, def);
}

bool BBP_read_bool(struct plugin_info *PI, const char *rcs, bool def)
{
	char buffer[256];
	sprintf(buffer, "%s.%s:", PI->rc_key, rcs);
	return ReadBool(PI->rcpath, buffer, def);
}

void write_rc(struct plugin_info *PI, void *v)
{
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
		BBP_write_bool      (PI, "OnTop", PI->alwaysOnTop);
	else
	if (v == &PI->autoHide)
		BBP_write_bool      (PI, "autoHide", PI->autoHide);
	else
	if (v == &PI->clickRaise)
		BBP_write_bool      (PI, "clickRaise", PI->clickRaise);
	else
	if (v == &PI->snapWindow)
		BBP_write_bool      (PI, "snapToEdge", PI->snapWindow);
	else
	if (v == &PI->pluginToggle)
		BBP_write_bool      (PI, "pluginToggle", PI->pluginToggle);
	else
	if (v == &PI->transparency)
		BBP_write_bool      (PI, "alpha.enabled", PI->transparency);
	else
	if (v == &PI->alpha)
		BBP_write_int       (PI, "alpha.value", PI->alpha);
	else
	if (v == &PI->orient_vertical)
		BBP_write_string    (PI, "orientation", PI->orient_vertical ? "vertical" : "horizontal");
}

//===========================================================================

bool BBP_get_rcpath(char *rcpath, HINSTANCE hInstance, const char *rcfile)
{
#if 1
	int i = 0;
	for (;;)
	{
		// First and third, we look for the config file
		// in the same folder as the plugin...
		HINSTANCE hInst = hInstance;
		// second we check the blackbox directory
		if (1 == i) hInst = NULL;

		GetModuleFileName(hInst, rcpath, MAX_PATH);
		char *file_name_start = strrchr(rcpath, '\\');
		if (file_name_start) ++file_name_start;
		else file_name_start = strchr(rcpath, 0);

		sprintf(file_name_start, "%s.rc", rcfile);
		if (3 == ++i) return false;
		if (FileExists(rcpath)) break;
		sprintf(file_name_start, "%src", rcfile);
		if (FileExists(rcpath)) break;
	}
	return true;

#else
	char modulepath[MAX_PATH];
	char filename[MAX_PATH];
	sprintf(filename, "%s.rc", rcfile);
	GetModuleFileName(hInstance, modulepath, MAX_PATH);
	return FindConfigFile(rcpath, filename, modulepath);
#endif
}

bool BBP_read_window_modes(struct plugin_info *PI, const char *rcfile)
{
	if (0 == *PI->rcpath)
		BBP_get_rcpath(PI->rcpath, PI->hInstance, rcfile);

	PI->xpos = BBP_read_int(PI,  "position.x", 20);
	PI->ypos = BBP_read_int(PI,  "position.y", 20);

	const char *place_string = BBP_read_string(PI, NULL, "placement", NULL);
	PI->place = place_string
		? get_string_index(place_string, placement_strings)
		: POS_User
		;

	PI->hMon = GetMonitorRect(&PI->xpos, &PI->mon_rect, GETMON_FROM_POINT);
	set_place(PI);

	PI->useSlit         = BBP_read_bool(PI, "useSlit", false);
	PI->alwaysOnTop     = BBP_read_bool(PI, "OnTop", true);
	PI->autoHide        = BBP_read_bool(PI, "autoHide", false);
	PI->snapWindow      = BBP_read_bool(PI, "snapToEdge", true);
	PI->pluginToggle    = BBP_read_bool(PI, "pluginToggle", true);
	PI->clickRaise      = BBP_read_bool(PI, "clickRaise", true);
	PI->transparency    = BBP_read_bool(PI, "alpha.enabled", false);
	PI->alpha           = (BYTE)BBP_read_int(PI,  "alpha.value",  192);
	PI->orient_vertical = false == PI->is_bar && 0 == stricmp("vertical", BBP_read_string(PI, NULL, "orientation", "vertical"));
	if (NULL == place_string)
	{
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
	write_rc(PI, &PI->transparency);
	write_rc(PI, &PI->alpha);
	if (false == PI->is_bar) write_rc(PI, &PI->orient_vertical);
}

//===========================================================================

void BBP_set_hidden(struct plugin_info *PI, bool hidden)
{
	if (PI->hidden != hidden)
	{
		PI->hidden = hidden;
		BBP_set_window_modes(PI);
	}
}

void set_pluginToggle(struct plugin_info *PI, bool pluginToggle)
{
	PI->pluginToggle = pluginToggle;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->pluginToggle);
}

void set_useSlit(struct plugin_info *PI, bool useSlit)
{
	PI->useSlit = useSlit;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->useSlit);
}

void set_alwaysOnTop(struct plugin_info *PI, bool alwaysOnTop)
{
	PI->alwaysOnTop = alwaysOnTop;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->alwaysOnTop);
}

void set_transparency(struct plugin_info *PI, bool transparency)
{
	PI->transparency = transparency;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->transparency);
}

void set_snapWindow(struct plugin_info *PI, bool snapWindow)
{
	PI->snapWindow = snapWindow;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->snapWindow);
}

void set_alpha(struct plugin_info *PI, int alpha)
{
	PI->alpha = (BYTE)alpha;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->alpha);
}

void BBP_exit_moving(struct plugin_info *PI)
{
	if (false == PI->inSlit && PI->is_moving)
	{
		RECT r; GetWindowRect(PI->hwnd, &r);
		PI->xpos = r.left;
		PI->ypos = r.top;
		PI->width = r.right - r.left;
		PI->height = r.bottom - r.top;
		PI->place = get_place(PI);
		write_rc(PI, &PI->xpos);
	}
	PI->is_moving = false;
	PI->is_sizing = false;
}

void BBP_set_place(struct plugin_info *PI, int n)
{
	PI->place = n;
	BBP_set_window_modes(PI);
	write_rc(PI, &PI->xpos);
}

void BBP_set_size(struct plugin_info *PI, int w, int h)
{
	if (PI->width != w || PI->height != h)
	{
		PI->width = w, PI->height = h;
		BBP_set_window_modes(PI);
	}
}

//===========================================================================
// autohide

bool check_mouse(HWND hwnd)
{
	POINT pt; RECT rct;
	GetCursorPos(&pt);
	GetWindowRect(hwnd, &rct);
	return PtInRect(&rct, pt);
}

bool wake_autohide(struct plugin_info *PI)
{
	if (PI->auto_hidden)
	{
		SetTimer(PI->hwnd, AUTOHIDE_TIMER, 250, NULL);
		PI->auto_shown = true;
		BBP_set_window_modes(PI);
		return true;
	}
	return false;
}

void BBP_check_autohide(struct plugin_info *PI)
{
	if (PI->auto_shown)
	{
		if (check_mouse(PI->hwnd))
			return;

		PI->auto_shown = false;
	}
	KillTimer(PI->hwnd, AUTOHIDE_TIMER);
	BBP_set_window_modes(PI);
}

void BBP_set_autoHide(struct plugin_info *PI, bool set)
{
	if (set != PI->autoHide)
	{
		PI->autoHide = set;
		write_rc(PI, &PI->autoHide);
	}
	PI->auto_hidden = PI->autoHide;
	wake_autohide(PI);
	BBP_check_autohide(PI);
}

//===========================================================================

int BBP_handle_broam(struct plugin_info *PI, const char *temp)
{

	if (!stricmp(temp, "useSlit"))
	{
		set_useSlit(PI, false == PI->useSlit);
		return 1;
	}

	if (!stricmp(temp, "pluginToggle"))
	{
		set_pluginToggle(PI, false == PI->pluginToggle);
		return 1;
	}

	if (!stricmp(temp, "alwaysOnTop"))
	{
		set_alwaysOnTop(PI, false == PI->alwaysOnTop);
		return 1;
	}

	if (!stricmp(temp, "clickRaise"))
	{
		PI->clickRaise = false == PI->clickRaise;
		write_rc(PI, &PI->clickRaise);
		return 1;
	}

	if (!stricmp(temp, "snapWindow"))
	{
		set_snapWindow(PI, false == PI->snapWindow);
		return 1;
	}

	if (!stricmp(temp, "alphaEnabled"))
	{
		set_transparency(PI, false == PI->transparency);
		return 1;
	}

	if (!memicmp(temp, "alphaValue ", 11))
	{
		set_alpha(PI, atoi(temp + 11));
		return 1;
	}

	if (!memicmp(temp, "Placement.", 10))
	{
		int n = get_string_index(temp + 10, placement_strings);
		if (-1 != n)
		{
			BBP_set_place(PI, n);
		}
		return 1;
	}

	if (!stricmp(temp, "AutoHide"))
	{
		BBP_set_autoHide(PI, false == PI->autoHide);
		return 1;
	}

	if (!stricmp(temp, "EditRC"))
	{
		char szTemp[MAX_PATH];
		GetBlackboxEditor(szTemp);
		BBExecute(NULL, NULL, szTemp, PI->rcpath, NULL, SW_SHOWNORMAL, false);
		return 1;
	}

	static const char Orientation_id[] = "Orientation.";
	if (!memicmp(temp, Orientation_id, sizeof Orientation_id - 1))
	{
		temp += sizeof Orientation_id - 1;
		if (!stricmp(temp, "vertical"))
			PI->orient_vertical = true;
		else
		if (!stricmp(temp, "horizontal"))
			PI->orient_vertical = false;
		else
			PI->orient_vertical = false == PI->orient_vertical;

		write_rc(PI, &PI->orient_vertical);
		return 2;
	}
	return 0;
}

void BBP_reconfigure(struct plugin_info *PI)
{
	if (false == PI->inSlit)
		GetMonitorRect(PI->hMon, &PI->mon_rect, GETMON_FROM_MONITOR);

	InvalidateRect(PI->hwnd, NULL, FALSE);
	BBP_set_window_modes(PI);
}

//===========================================================================
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
		if (BBVERSION_09X == BBVersion)
		{
			char buffer[20]; sprintf(buffer, "%d", initval);
			return MakeMenuItemString(pMenu, text, menu->addid(cmd), buffer);
		}
		else
		{
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
		mi->_make_item(pMenu);
		if (mi->disabled) DisableLastItem(pMenu);
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
	if (m->lastitem) m->lastitem->disabled = true;
}

//===========================================================================

//===========================================================================

void n_showmenu(n_menu *m, const char *broam_key, bool popup)
{
	char broam[200];
	int x = sprintf(broam, "@%s.", broam_key);
	char *b[2] = { broam, broam+x };

	Menu *pMenu = m->convert(b, broam_key, popup);
	delete m;
	ShowMenu(pMenu);
}

//===========================================================================

//===========================================================================

void BBP_n_placementmenu(struct plugin_info *PI, n_menu *m)
{
	n_menu *P = n_submenu(m, "Placement");
	int n = 1, o = PI->is_bar ? POS_CenterLeft : POS_Top;
	for (; n < o; n++)
	{
		if (POS_BottomLeft == n || POS_CenterLeft == n)
			n_menuitem_nop(P, NULL);

		char b2[80];
		sprintf(b2, "Placement.%s", placement_strings[n]);
		n_menuitem_bol(P, menu_placement_strings[n], b2, PI->place == n);
	}
}

void BBP_n_orientmenu(struct plugin_info *PI, n_menu *m)
{
	n_menu *o = n_submenu(m, "Orientation");
	n_menuitem_bol(o, "Vertical", "Orientation.vertical",  false != PI->orient_vertical);
	n_menuitem_bol(o, "Horizontal", "Orientation.horizontal",  false == PI->orient_vertical);
}

void BBP_n_insertmenu(struct plugin_info *PI, n_menu *m)
{
	if (PI->hSlit)
	{
		n_menuitem_bol(m, "Use Slit", "useSlit", PI->useSlit);
	}

	if (false == PI->inSlit)
	{
		n_menuitem_bol(m, "Auto Hide", "autoHide",  PI->autoHide);
		n_menuitem_bol(m, "Always On Top", "alwaysOnTop",  PI->alwaysOnTop);
		if (false == PI->alwaysOnTop)
			n_menuitem_bol(m, "Click Raise", "clickRaise", PI->clickRaise);
		n_menuitem_bol(m, "Snap To Edge", "snapWindow",  PI->snapWindow);
		n_menuitem_bol(m, "Toggle With Plugins", "pluginToggle",  PI->pluginToggle);
		n_menuitem_nop(m, NULL);
		n_menuitem_bol(m, "Transparency", "alphaEnabled",  PI->transparency);
		n_menuitem_int(m, "Alpha Value", "alphaValue",  PI->alpha, 0, 255);
	}
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
	struct plugin_info *PI  = (struct plugin_info *)GetWindowLong(hwnd, 0);

	//dbg_printf("message %x", message);

	if (NULL == PI)
	{
		if (WM_NCCREATE == message)
		{
			// bind the window to the structure
			PI = (plugin_info *)((CREATESTRUCT*)lParam)->lpCreateParams;
			PI->hwnd = hwnd;
			SetWindowLong(hwnd, 0, (LONG)PI);
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
			if (!stricmp((LPCSTR)lParam, "@BBShowPlugins"))
			{
				BBP_set_hidden(PI, PI->toggled_hidden = false);
				goto pass_result;
			}

			if (!stricmp((LPCSTR)lParam, "@BBHidePlugins"))
			{
				if (PI->pluginToggle && false == PI->inSlit)
					BBP_set_hidden(PI, PI->toggled_hidden = true);

				goto pass_result;
			}

			const char *temp = (LPCSTR)lParam;
			if ('@' == *temp
				&& 0 == memicmp(++temp, PI->broam_key, PI->broam_key_len)
				&& '.' == temp[PI->broam_key_len]
				)
			{
				temp += PI->broam_key_len+1;
				PI->process_broam(temp, BBP_handle_broam(PI, temp));
				goto pass_result;
			}

			goto pass_nothing;
		}

		// ==========

		case BB_DESKCLICK:
			if (lParam == 0
			 && PI->clickRaise
			 && false == PI->alwaysOnTop
			 && false == PI->inSlit)
				SetWindowPos(hwnd, HWND_TOP, 0,0,0,0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
			goto pass_result;

		// ==========
		case WM_WINDOWPOSCHANGING:
			if (PI->is_moving
			 && false == PI->inSlit
			 && PI->snapWindow
			 && 0 == (0x8000 & GetAsyncKeyState(VK_SHIFT))
			 )
				SnapWindowToEdge((WINDOWPOS*)lParam, 10,
					//PI->is_sizing ? SNAP_FULLSCREEN|SNAP_SIZING :
					SNAP_FULLSCREEN
					);
			goto pass_result;

		case WM_ENTERSIZEMOVE:
			PI->is_moving = true;
			goto pass_result;

		case WM_EXITSIZEMOVE:
			BBP_exit_moving(PI);
			BBP_set_autoHide(PI, PI->autoHide);
			if (false == PI->auto_hidden)
				PI->pos_changed();
			PI->show_menu(false);
			if (PI->inSlit) SendMessage(PI->hSlit, SLIT_UPDATE, 0, (LPARAM)PI->hwnd);
			goto pass_result;

		// ==========
		case WM_LBUTTONDOWN:
			if (false == PI->inSlit && (MK_CONTROL & wParam))
			{
				// start moving, when control-key is held down
				SetActiveWindow(hwnd);
				UpdateWindow(hwnd);
				PostMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
				goto pass_result;
			}
			goto pass_nothing;

		case WM_NCHITTEST:
			Result = HTCLIENT;
			if (PI->inSlit)
			{
				if (0x8000 & GetAsyncKeyState(VK_SHIFT))
					Result = HTTRANSPARENT;
			}
			goto pass_result;

		case WM_TIMER:
			if (AUTOHIDE_TIMER == wParam)
			{
				if (PI->auto_shown)
				{
					if (PI->wnd_proc(hwnd, message, wParam, lParam, &Result))
						return 0;
				}
				BBP_check_autohide(PI);
				return 0;
			}
			goto pass_nothing;

		case WM_MOUSEMOVE:
			if (wake_autohide(PI))
				goto pass_result;

			goto pass_nothing;

		case WM_CLOSE:
			goto pass_result;

		case WM_ERASEBKGND:
			Result = TRUE;
			goto pass_result;

		default:
		pass_nothing:
			return PI->wnd_proc(hwnd, message, wParam, lParam, NULL);
	}
pass_result:
	return PI->wnd_proc(hwnd, message, wParam, lParam, &Result);
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


ST struct class_info *CI;

ST struct class_info **find_class(const char *name)
{
	struct class_info **pp;
	for (pp = &CI; *pp; pp = &(*pp)->next)
		if (0 == stricmp((*pp)->name, name)) break;
	return pp;
}

ST int class_info_register(const char *class_name, HINSTANCE hInstance)
{
	struct class_info *p = *find_class(class_name);
	if (NULL == p)
	{
		WNDCLASS wc;
		ZeroMemory(&wc,sizeof(wc));

		wc.lpfnWndProc  = BBP_WndProc;  // our window procedure
		wc.hInstance    = hInstance;    // hInstance of .dll
		wc.lpszClassName = class_name;  // our window class name
		wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.cbWndExtra = sizeof (void*);

		if (FALSE == RegisterClass(&wc))
		{
			//dbg_printf("failed to register %s", wc.lpszClassName);
			return 0;
		}

		p = new struct class_info;
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

ST void class_info_decref(const char *name)
{
	struct class_info *p, **pp = find_class(name);
	if (NULL != (p = *pp) && --p->refc <= 0)
	{
		UnregisterClass(p->name, p->hInstance);
		//dbg_printf("unregistered class <%s> %x", p->name, p->hInstance);
		*pp = p->next;
		delete p;
	}
}

int BBP_Init_Plugin(struct plugin_info *PI)
{
	if (NULL == CI)
	{
		const char *bbv = GetBBVersion();
		if (0 == memicmp(bbv, "bblean", 6)) BBVersion = BBVERSION_LEAN;
		else
		if (0 == memicmp(bbv, "bb", 2)) BBVersion = BBVERSION_XOB;
		else BBVersion = BBVERSION_09X;
	}

	PI->broam_key_len = strlen(PI->broam_key);
	PI->has_alpha = 255;

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

void BBP_Exit_Plugin(struct plugin_info *PI)
{
	//dbg_printf("window destroying <%s>", PI->class_name);
	if (PI->hwnd)
	{
		if (PI->inSlit) SendMessage(PI->hSlit, SLIT_REMOVE, 0, (LPARAM)PI->hwnd);
		DestroyWindow(PI->hwnd);
		class_info_decref(PI->class_name);
	}
}

//===========================================================================
