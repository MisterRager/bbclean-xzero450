/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#ifndef __MENU_H
#define __MENU_H

#include <windows.h>
#include "MenuMaker.h"
// for VolumeItem Class
#include "../VolumeControl.h"

// global vars
extern int g_menu_count;
extern int g_menu_item_count;

// utility
HWND window_under_mouse(void);

void register_menuclass(void);
void un_register_menuclass(void);

struct MenuList { struct MenuList *next; class Menu *m; };

//===========================================================================
class Menu  
{
protected:
	int         m_refc;         // menu reference count

	Menu*       m_pParent;      // parent menu, if onscreen
	Menu*       m_pChild;       // child menu, if onscreen
	Menu*       m_pLastChild;   // remember child while in menu update

	MenuItem*   m_pMenuItems;   // items, first is title (always present)
	MenuItem*   m_pParentItem;  // parentmenu's folderitem linked to this
	MenuItem*   m_pActiveItem;  // currently hilited item
	MenuItem**  m_ppAddItem;    // helper while adding items

	int         m_MenuID;       // see below
	char*       m_IDString;     // unique ID for plugin menus

	int         m_itemcount;    // total items
	int         m_topindex;     // top item index
	int         m_pagesize;     // visible items
	int         m_firstitem_top;    // in pixel

	int         m_scrollpos;    // scroll button location (pixel from trackstart)
	int         m_captureflg;   // when the mouse is captured, see below
	int         m_keyboard_item_pos;    // item hilited by the keyboard
	int         m_active_item_pos;  // temporarily while in update

	bool        m_bOnTop;       // z-order
	bool        m_bPinned;      // pinned
	bool        m_bNoTitle;     // dont draw title
	bool        m_bKBDInvoked;  // was invoked from kbd
	bool        m_bIsDropTarg;  // is registered as DT
	bool        m_bIconized;    // iconized to titlebar

	bool        m_bMouseOver;   // current states:
	bool        m_bHasFocus;
	bool        m_dblClicked;
	bool        m_bMoving;      // moved by the user
	bool        m_bInDrag;      // in drag&drop operation
	bool        m_bPopup;       // for plugin menus, false when updating

	BYTE        m_alpha;        // transparency

	int         m_iconSize;
	int         m_xpos;         // window position and sizes
	int         m_ypos;
	int         m_width;
	int         m_height;

	HBITMAP     m_hBitMap;      // background bitmap, only while onscreen
	HBITMAP     m_hBmpScroll;   // bitmap to paint scroller
	HWND        m_hwnd;         // window handle, only while onscreen

	class CDropTarget *m_droptarget;

	// ----------------------
	// static vars

	static MenuList *g_MenuWindowList;  // all menus with a window
	static MenuList *g_MenuStructList;  // all menus
	static MenuList *g_MenuFocusList;   // list of menus with focus recently

	static int  g_MouseWheelAccu;
	static int  g_DiscardMouseMoves;

	// ----------------------
	// class methods

	Menu(const char *pszTitle);
	virtual ~Menu();

	int decref(void);
	int incref(void);

	MenuItem *AddMenuItem(MenuItem* m);
	void DeleteMenuItems();

	bool IsPinned() { return m_bPinned; };
	void SetPinned( bool bPinned );

	void LinkToParentItem(MenuItem *Item);
	void Detach(void);

	void Hide(void);
	void HideThis(void);
	void HideChild(void);
	void UnHilite(void);

	void Show(int xpos, int ypos, int flags);

	void SetZPos(void);
	void Validate(void);
	void Paint();
	void MenuTimer(UINT);

	bool Handle_Key(UINT msg, UINT wParam);
	void Handle_Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void UpdateFolder(void);
	virtual void register_droptarget(bool set);

	// ----------------------
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// ----------------------

	// popup/close delay
	//void set_timer(bool set);
	void set_timer(bool active, bool set);

	// init / exit
	void make_menu_window(void);
	void destroy_menu_window(void);

	// scrolling
	int get_y_range(int *py0, int *ps);
	void get_vscroller_rect(RECT* rw);
	int calc_topindex (bool adjust);
	void set_vscroller(int ymouse);
	void scroll_assign_items(int n);
	void scroll_menu(int n);

	// keyboard
	void set_keyboard_active_item(void);
	MenuItem * match_next_shortcut(const char *d);
	void hilite(MenuItem *Item);
	void activate_by_key(MenuItem *Item);
	void hide_on_click(void);

	// retrieve specific things
	Menu *menu_root (void);
	int get_item_index(MenuItem *item);
	int get_active_index (void);
	bool has_in_chain(HWND hwnd);
	bool has_focus_in_chain(void);
	MenuItem * nth_item(int a);

	// mouse
	void mouse_over(bool indrag);
	void mouse_leave(void);
	const _ITEMIDLIST *dragover(POINT* ppt);
	void start_drag(const char *arg, const _ITEMIDLIST *pidl);
	void set_focus(void);

	// other
	void insert_at_last (void);
	void cons_focus(void);
	void bring_menu_ontop(void);
	void menu_set_pos(HWND after, UINT flags);
	void redraw(void);
	void redraw_structure(void);
	void write_menu_pos(void);

	// add special folder items
	void add_folder_contents(MenuItem *pItems, bool join);

	// -------------------------------------
	// overall menu functions
	static Menu *last_active_menu_root(void);
	static void Hide_All_But(Menu *m);
	static Menu *FindNamedMenu(LPCSTR IDString);

	// -------------------------------------

	// init/exit
	friend void register_menuclass(void);
	friend void un_register_menuclass(void);
	friend void Menu_All_Delete(void);

	// PluginMenu API friends
	friend Menu* MakeMenu(LPCSTR HeaderText);
	friend Menu* MakeNamedMenu(LPCSTR HeaderText, LPCSTR IDString, bool popup);
	friend void ShowMenu(Menu *PluginMenu);
	friend bool MenuExists(const char* IDString_start);
	friend MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCSTR Title);
	friend MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCSTR Title, LPCSTR Icon);
	friend MenuItem* MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator);
	friend MenuItem* MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator, LPCSTR Icon);
	friend MenuItem* MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator, char iconMode, void *im_stuff);
	friend MenuItem* make_helper_menu(Menu *PluginMenu, LPCSTR Title, int menuID, MenuItem *Item);
	friend MenuItem* MakeMenuItemInt(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, int val, int minval, int maxval);
	friend MenuItem* MakeMenuItemString(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, LPCSTR init_string);
	friend MenuItem* MakeMenuNOP(Menu *PluginMenu, LPCSTR Title);
	friend MenuItem* MakeMenuGrip(Menu *PluginMenu, LPCSTR Title);
	friend MenuItem* MakeMenuVOL(Menu *PluginMenu, LPCSTR Title, LPCSTR DllName, LPCSTR Icon);
	friend MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd);
	friend MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd, LPCSTR Icon);
	friend void DelMenu(Menu *PluginMenu);
	friend void DisableLastItem(Menu *PluginMenu);


	// general
	friend Menu* ParseMenu(FILE **fp, int *fc, const char *path, const char *title, const char *IDString, bool popup);
	friend Menu* SingleFolderMenu(const char* path);
	friend bool MenuMaker_ShowMenu(int id, LPARAM param);
	friend bool check_menu_toggle(const char* menu_id, bool kbd_invoked);
	friend Menu *MakeRootMenu(const char *menu_id, const char *path, const char *default_menu);
	friend MenuItem *get_real_item(MenuItem *pItem);

	// other
	friend bool Menu_Activate_Last(void);
	friend void Menu_All_Redraw(int flags);
	friend void Menu_Tab_Next(Menu *start);
	friend void Menu_All_Toggle(bool hide);
	friend void Menu_All_BringOnTop(void);
	friend void Menu_All_Hide(void);
	friend void Menu_All_Update(int special);
	friend void Menu_Fire_Timer(void);
	friend void Menu_ShowFirst(Menu *pMenu, bool from_kbd, bool with_xy, int x, int y);
	friend bool IsMenu(HWND);

	// friend classes
	friend class MenuItem;
	friend class TitleItem;
	friend class MenuGrip;
	friend class FolderItem;
	friend class CommandItem;
	friend class CommandItemEx;
	friend class IntegerItem;
	friend class VolumeItem;
	friend class StringItem;
	friend class ContextItem;
	friend class ContextMenu;
	friend class SpecialFolderItem;
	friend class SpecialFolder;
};

//---------------------------------
#define MENU_POPUP_TIMER        1
#define MENU_TRACKMOUSE_TIMER   2
#define MENU_INTITEM_TIMER      3

//---------------------------------
// values for m_MenuID

#define MENU_ID_NORMAL      0
#define MENU_ID_SF          1
#define MENU_ID_SHCONTEXT   2
#define MENU_ID_STRING      4
#define MENU_ID_INT         8

//---------------------------------
// values for m_captureflg

#define MENU_CAPT_SCROLL    1
#define MENU_CAPT_TWEAKINT  2

// flags for void Menu::Show(int xpos, int ypos, int flags);
#define MENUSHOW_LEFT 0
#define MENUSHOW_CENTER 1
#define MENUSHOW_RIGHT 2
#define MENUSHOW_UPDATE 4
#define MENUSHOW_BOTTOM 8

//===========================================================================

//===========================================================================

class MenuItem
{
public:
	MenuItem *next;

	MenuItem(const char* pszTitle);
	virtual ~MenuItem();

	virtual void Measure(HDC hDC, SIZE *size);
	virtual void Paint(HDC hDC);
	virtual void Invoke(int button);
	virtual void Mouse(HWND hw, UINT nMsg, DWORD, DWORD);
	virtual void Key(UINT nMsg, WPARAM wParam);
	virtual void ItemTimer(UINT nTimer);
	virtual void ShowSubMenu();

	void UnlinkSubmenu(void);
	void LinkSubmenu(Menu *pSubMenu);
	void Active(int bActive);
	void ShowContextMenu(const char *path, const struct _ITEMIDLIST *pidl);

	void GetItemRect(RECT* r);
	void GetTextRect(RECT* r, int iconSize);
	const char*  GetDisplayString(void);
	inline bool within(int y) { return y >= m_nTop && y < m_nTop + m_nHeight; }
	MenuItem *Sort(int(*cmp_fn)(MenuItem**, MenuItem**));

	// ----------------------
	char* m_pszTitle;
	int m_nSortPriority;
	int m_ItemID;

	int m_nTop;
	int m_nLeft;
	int m_nWidth;
	int m_nHeight;

	bool m_bActive;
	bool m_isChecked;
	char m_isNOP;

	Menu* m_pMenu;
	Menu* m_pSubMenu;

	_ITEMIDLIST *m_pidl;
	char *m_pszCommand;
	char *m_pszIcon;
	HICON m_hIcon;
	//int m_bSmallIcon;//was bool in this base
	bool m_bSmallIcon;
	char m_iconMode;
	void *m_im_stuff;

	// ----------------------
	static int center_justify;
	static int left_justify;
};

//---------------------------------
// values/bitflags for m_ItemID

#define MENUITEM_ID_NORMAL  0
#define MENUITEM_ID_FOLDER  1
#define MENUITEM_ID_CI      2
#define MENUITEM_ID_SF      (4 | MENUITEM_ID_FOLDER)
#define MENUITEM_ID_CIInt   (8 | MENUITEM_ID_CI)
#define MENUITEM_ID_CIStr   (16| MENUITEM_ID_CI)
#define MENUITEM_ID_STYLE   32
#define MENUITEM_ID_CONTEXT 64
#define MENUITEM_ID_VI      128

// values for m_isNOP
#define MI_NOP_TEXT 1
#define MI_NOP_SEP 2
#define MI_NOP_DISABLED 4
#define MI_NOP_LINE 8

// values for m_nSortPriority
#define M_SORT_NORMAL 1
#define M_SORT_NAME   3
#define M_SORT_SEP    4
#define M_SORT_FOLDER 5

// button values for void Invoke(int button);
#define INVOKE_DBL      1
#define INVOKE_LEFT     2
#define INVOKE_RIGHT    4
#define INVOKE_MID      8
#define INVOKE_DRAG    16

// FolderItem bullet style / position
#define BS_EMPTY        0
#define BS_DIAMOND      1
#define BS_SQUARE       2
#define BS_TRIANGLE     3
#define BS_CIRCLE       4
#define BS_SATURN       5
#define BS_JUPITER      6
#define BS_MARS         7
#define BS_VENUS        8
#define BS_BMP          9

#define FOLDER_LEFT     0
#define FOLDER_RIGHT    1

// Mode to load icon
#define IM_NONE         0
#define IM_PIDL         1
#define IM_TASK         2
#define IM_PATH         3

// DrawText flags
#define DT_MENU_STANDARD (DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP | DT_EXPANDTABS)
#define DT_MENU_MEASURE_STANDARD (DT_LEFT | DT_EXPANDTABS | DT_CALCRECT)

//===========================================================================

//===========================================================================

class TitleItem : public MenuItem
{
public:
	TitleItem(const char* pszTitle) : MenuItem(pszTitle) {}
	void Paint(HDC hDC);
	void Mouse(HWND hw, UINT nMsg, DWORD wP, DWORD lP);
};

//===========================================================================

//===========================================================================

class MenuGrip : public MenuItem
{
public:
	MenuGrip(const char* pszTitle) : MenuItem(pszTitle) {}
	void Paint(HDC hDC);
	void Mouse(HWND hw, UINT nMsg, DWORD wP, DWORD lP);
};

//===========================================================================

//===========================================================================
// An menuitem that is a pointer to a sub menu, theese folder items
// typically contain a |> icon at their right side.

class FolderItem : public MenuItem
{
public:
	FolderItem(Menu* pSubMenu, const char* pszTitle);
	void Paint(HDC hDC);
	void Invoke(int button);
	static int m_nBulletStyle;
	static int m_nBulletPosition;
	static int m_nBulletBmpSize;
	static BYTE **m_byBulletBmp;
	static int m_nScrollPosition;
};

//===========================================================================

//===========================================================================

class CommandItem : public MenuItem
{
public:
	CommandItem(const char* pszCommand, const char* pszTitle, bool isChecked);
	void Invoke(int button);
};

//=======================================
class CommandItemEx : public CommandItem
{
	CommandItemEx(const char *pszCommand, const char *fmt);
	void next_item (UINT wParam);
	friend class IntegerItem;
	friend class StringItem;
};

//=======================================
class IntegerItem : public CommandItemEx
{
public:
	IntegerItem(const char* pszCommand, int value, int minval, int maxval);

	void Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam);
	void Invoke(int button);
	void ItemTimer(UINT nTimer);
	void Measure(HDC hDC, SIZE *size);
	void Key(UINT nMsg, WPARAM wParam);
	void set_next_value();

	int m_value;
	int m_min;
	int m_max;
	int m_count;
	int direction;
	int oldsize;
	int offvalue;
	const char *offstring;
};

//=======================================
class VolumeItem : public CommandItem, public VolumeControl
{
public:
	VolumeItem(const char *pszTitle, const char *pszDllName, const char *pszIcon);
	void Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam);
	void Measure(HDC hDC, SIZE *size);
	void Invoke(int button);
	void Paint(HDC hDC);
	void Key(UINT nMsg, WPARAM wParam);
	void Active(int active);
private:
	int m_nVol;
	bool m_bMute;
};

//===========================================================================

class StringItem : public CommandItemEx
{
public:
	StringItem(const char* pszCommand, const char *init_string);
	~StringItem();

	void Paint(HDC hDC);
	void Measure(HDC hDC, SIZE *size);
	void SetPosition(void);
	void Key(UINT nMsg, WPARAM wParam);
	void Invoke(int button);

	static LRESULT CALLBACK EditProc(HWND hText, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND hText;
	WNDPROC wpEditProc;
};

//===========================================================================

//===========================================================================

class SpecialFolderItem : public FolderItem
{
public:
	SpecialFolderItem(LPCSTR pszTitle, const char *path, struct pidl_node* pidl_list, const char  *optional_command);
	~SpecialFolderItem();
	void ShowSubMenu(); 
	void Invoke(int button);
	friend class SpecialFolder;
	friend void join_folders(SpecialFolderItem *);
	const struct _ITEMIDLIST *check_pidl(void);
	int m_iconSize;

private:
	struct pidl_node *m_pidl_list;
	char *m_pszPath;
	char *m_pszExtra;
};

//===========================================================================
class SpecialFolder : public Menu
{
public:
	SpecialFolder(const char *pszTitle, const struct pidl_node *pidl_list, const char  *optional_command);
	~SpecialFolder();
	void UpdateFolder(void);
	void register_droptarget(bool set);

private:
	struct pidl_node *m_pidl_list;
	UINT m_notify;
	char *m_pszExtra;
};


//===========================================================================

//===========================================================================

class ContextMenu : public Menu
{
public:
	ContextMenu (LPCSTR title, class ShellContext* w, HMENU hm, int m, int id = 0, DWORD data = 0);
	~ContextMenu();
private:
	void Copymenu (HMENU hm);
	ShellContext *wc;
	friend class ContextItem;
	friend class ODTitleItem;
};

class ContextItem : public FolderItem
{
public:
	ContextItem(Menu *m, LPSTR pszTitle, int id, DWORD data, UINT type);
	~ContextItem();
	void Paint(HDC hDC);
	void Measure(HDC hDC, SIZE *size);
	void Invoke(int button);
private:
	void DrawItem(HDC hDC, ContextMenu* Ctxt, DRAWITEMSTRUCT *dis, COLORREF cr_back, COLORREF cr_txt);
	void DrawItemToBitmap(HDC hDC, int w, int h, bool active);
	void DrawTextFromBitmap(HDC buf, HDC hDC, COLORREF cr_txt, int left, int top, int tw, int h);
	void DrawIconFromBitmap(HDC buf, HDC hDC, int left, int top, int h);
	int   m_id;
	DWORD m_data;
	UINT m_type;
	int m_text_offset;
	RECT m_rIcon;
	HBITMAP m_bmp;
	int m_bmp_width;
};

//===========================================================================
#endif /* __MENU_H */
