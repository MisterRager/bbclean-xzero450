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

// This file is the common header for the files that make up
// the menu implementation.

#ifndef _MENU_H_
#define _MENU_H_

#include "bbshell.h"
#include "MenuMaker.h"

struct MenuList { struct MenuList *next; class Menu *m; };

// global vars
extern int g_menu_count;
extern int g_menu_item_count;

typedef bool (*MENUENUMPROC)(Menu *m, void *ud);

//=======================================
class Menu
{
protected:
    int         m_refc;         // menu reference count (must come first)

    Menu*       m_pParent;      // parent menu, if onscreen
    Menu*       m_pChild;       // child menu, if onscreen

    MenuItem*   m_pMenuItems;   // items, first is title (always present)
    MenuItem*   m_pParentItem;  // parentmenu's folderitem linked to this
    MenuItem*   m_pActiveItem;  // currently hilited item
    MenuItem*   m_pLastItem;    // tail pointer while adding items

    int         m_MenuID;       // see below
    char*       m_IDString;     // unique ID for plugin menus

    int         m_itemcount;    // total items
    int         m_topindex;     // top item index
    int         m_pagesize;     // visible items
    int         m_firstitem_top;// in pixel
    int         m_kbditempos;   // item hilited by the keyboard

    int         m_scrollpos;    // scroll button location (pixel from trackstart)
    int         m_captureflg;   // when the mouse is captured, see below

    Menu*       m_OldChild;     // in update: remember child
    int         m_OldPos;       // in update: remember active item
    bool        m_saved;

    bool        m_bOnTop;       // z-order
    bool        m_bPinned;      // pinned
    bool        m_bNoTitle;     // dont draw title

    bool        m_kbdpos;  // save position to blackbox.rc on changes
    bool        m_bIsDropTarg;  // window should be registered as DT
    bool        m_bMouseOver;   // current states:
    bool        m_bHasFocus;
    bool        m_dblClicked;
    bool        m_bMoving;      // moved by the user
    bool        m_bInDrag;      // in drag&drop operation
    bool        m_bPopup;       // for plugin menus, false when updating

    unsigned char m_alpha;      // transparency

    int         m_sortmode;     // folder sort mode
    int         m_sortrev;      // folder sort order

    int         m_xpos;         // window position and sizes
    int         m_ypos;
    int         m_width;
    int         m_height;

    HBITMAP     m_hBitMap;      // background bitmap, only while onscreen
    HWND        m_hwnd;         // window handle, only while onscreen
    HWND        m_hwndChild;    // edit control of StringItems
    HWND        m_hwndRef;      // hwnd to send notifications to */

	int         m_minwidth; /* BlackboxZero 12.17.2011 */
    int         m_maxwidth;
    int         m_maxheight;

    RECT        m_mon;          // monitor rect where the menu is on
    RECT        m_pos;          // initial display position setting
    int         m_flags;        // initial display (and other) flags

    struct pidl_node *m_pidl_list;
    class CDropTarget *m_droptarget;
    UINT m_notify;

    // ----------------------
    // global variables

    static MenuList *g_MenuWindowList;  // all menus with a window
    static MenuList *g_MenuStructList;  // all menus
    static MenuList *g_MenuRefList;  // menus for deletion
    static void g_incref();
    static void g_decref();

    // ----------------------
    // class methods

    Menu(const char *pszTitle);
    virtual ~Menu();
    virtual void UpdateFolder(void);

    int decref(void);
    int incref(void);

    MenuItem *AddMenuItem(MenuItem* m);
    void DeleteMenuItems(void);

    void LinkToParentItem(MenuItem *pItem);

    void Hide(void);
    void HideNow(void);
    void HideChild(void);
    void UnHilite(void);

    void SetPinned(bool bPinned);
    void SetZPos(void);
    void Validate(void);
    void Show(int xpos, int ypos, bool fShow);
    void Paint(void);

    void MenuTimer(UINT);
    bool Handle_Key(UINT msg, UINT wParam);
    void Handle_Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void register_droptarget(bool set);

    // window callback
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // popup/close delay
    void set_timer(bool active, bool set); /*BlackboxZero 1.3.2012 */
    void set_capture(int flg);

    // init / exit
    void make_menu_window(void);
    void destroy_menu_window(bool force);
    void post_autohide();

    // scrolling
    int get_y_range(int *py0, int *ps);
    void get_vscroller_rect(RECT* rw);
    int calc_topindex (bool adjust);
    void set_vscroller(int ymouse);
    void scroll_assign_items(int n);
    void scroll_menu(int n);

    // keyboard
    MenuItem * kbd_get_next_shortcut(const char *d);
    void kbd_hilite(MenuItem *pItem);

    // retrieve specific things
    Menu *menu_root (void);
    int get_item_index(MenuItem *item);
    bool has_focus_in_chain(void);
    bool has_hwnd_in_chain(HWND hwnd);
    MenuItem * nth_item(int a);

    // mouse
    void mouse_over(bool indrag);
    void mouse_leave(void);
    LPCITEMIDLIST dragover(POINT* ppt);
    void start_drag(const char *path, LPCITEMIDLIST pidl);

    // other
    void write_menu_pos(void);
    void insert_at_last (void);
    void set_focus(void);
    void bring_ontop(bool force_active);
    void menu_set_pos(HWND after, UINT flags);
    void on_killfocus(HWND newfocus);
    void on_setfocus(HWND oldFocus);
    void hide_on_click(void);

    void Redraw(int mode);
    void RedrawGUI(int flags);
    void RestoreState(void);
    void SaveState(void);
    void ShowMenu(void);

    // for inserting SpecialFolderItems
    int AddFolderContents(const struct pidl_node *pidl_list, const char *extra);

    // -------------------------------------
    // overall menu functions
    static void ignore_mouse_msgs(void);
    static Menu *last_active_menu_root(void);
    static Menu *find_named_menu(const char * IDString, bool fuzzy = false);
    static void Sort(MenuItem **, int(*cmp_fn)(MenuItem **, MenuItem**));

    static bool del_menu(Menu *m, void *ud);
    static bool toggle_menu(Menu *m, void *ud);
    static bool redraw_menu(Menu *m, void *ud);
    static bool hide_menu(Menu *m, void *ud);

    static Menu *find_special_folder(struct pidl_node* p1);

    // -------------------------------------
    // friend functions

    // init/exit
    friend void register_menuclass(void);
    friend void un_register_menuclass(void);

    // PluginMenu API friends
    friend Menu* MakeMenu(const char* HeaderText);
    friend Menu* MakeNamedMenu(const char* HeaderText, const char* IDString, bool popup);
    friend void ShowMenu(Menu *PluginMenu);
    friend void ShowMenuEx(Menu *PluginMenu, int flags, ...);
    friend bool MenuExists(const char* IDString_start);
    friend MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, const char* Title);
    friend MenuItem* MakeMenuItem(Menu *PluginMenu, const char* Title, const char* Cmd, bool ShowIndicator);
    friend MenuItem* MakeMenuItemInt(Menu *PluginMenu, const char* Title, const char* Cmd, int val, int minval, int maxval);
    friend MenuItem* MakeMenuItemString(Menu *PluginMenu, const char* Title, const char* Cmd, const char* init_string);
    friend MenuItem* MakeMenuNOP(Menu *PluginMenu, const char* Title);
    friend MenuItem* MakeMenuItemPath(Menu *ParentMenu, const char* Title, const char* path, const char* Cmd);
    friend Menu* MakeFolderMenu(const char *title, const char* path, const char *cmd);
    friend void MenuOption(Menu *pMenu, int flags, ...);
    friend void MenuItemOption(MenuItem *pItem, int option, ...);

    friend void DelMenu(Menu *PluginMenu);
    friend MenuItem* helper_menu(Menu *PluginMenu, const char* Title, int menuID, MenuItem *pItem);

    // other (also in MenuMaker.h)
    friend void Menu_Init(void);
    friend void Menu_Exit(void);
    friend void Menu_Reconfigure(void);
    friend void Menu_Stats(struct menu_stats *st);
    friend bool Menu_IsA(HWND);

    friend Menu *MenuEnum(MENUENUMPROC fn, void *ud);
    friend void Menu_All_Redraw(int flags);
    friend void Menu_All_Toggle(bool hidden);
    friend void Menu_All_BringOnTop(void);
    friend void Menu_All_Hide(void);
    friend void Menu_All_Hide_But(Menu*);
    friend bool Menu_Exists(bool pinned);
    friend void Menu_Update(int what);
    friend void Menu_Tab_Next(Menu *start);
    friend bool Menu_ToggleCheck(const char* menu_id);

    // friend classes
    friend class MenuItem;
    friend class SeparatorItem;
    friend class TitleItem;
    friend class FolderItem;
    friend class CommandItem;
    friend class CommandItemEx;
    friend class IntegerItem;
    friend class StringItem;
    friend class SpecialFolder;
    friend class SpecialFolderItem;
    friend class SFInsert;
    friend class ContextMenu;
    friend class ContextItem;
};

//---------------------------------
#define MENU_POPUP_TIMER        1
#define MENU_TRACKMOUSE_TIMER   2
#define MENU_INTITEM_TIMER      3

//---------------------------------
// values for m_MenuID

#define MENU_ID_NORMAL      0
#define MENU_ID_SF          1   // SpecialFolder
#define MENU_ID_SHCONTEXT   2   // ContextMenu
#define MENU_ID_INT         4   // IntegerItem's ParentMenu
#define MENU_ID_STRING      8   // StringItem's ParentMenu
#define MENU_ID_RMENU      16   // Custom Rightclick Menu

//---------------------------------
// flags for set_capture(int flg)

#define MENU_CAPT_SCROLL 1
#define MENU_CAPT_ITEM 2

//=======================================
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
    virtual void ShowSubmenu(void);

    void UnlinkSubmenu(void);
    void LinkSubmenu(Menu *pSubMenu);
    void Active(int bActive);
    void ShowRightMenu(Menu *pSub);
    void ShowContextMenu(const char *path, LPCITEMIDLIST pidl);

    void GetItemRect(RECT* r);
    void GetTextRect(RECT* r);
    const char* GetDisplayString(void);
    LPCITEMIDLIST GetPidl(void);

    MenuItem *get_real_item(void);

    // mouse over check
    inline bool isover(int y) { return y >= m_nTop && y < m_nTop + m_nHeight; }

    // ----------------------
    char *m_pszTitle;
    char *m_pszCommand;
    char *m_pszRightCommand;
    struct pidl_node *m_pidl_list;

    Menu* m_pMenu;          // the menu where the item is on
    Menu* m_pSubmenu;       // for folder items, also context menus
    Menu* m_pRightmenu;     // optional rightclick menu

    int m_nTop;             // metrics
    int m_nLeft;
    int m_nWidth;
    int m_nHeight;

    int m_Justify;          // alignment
    int m_ItemID;           // see below
    int m_nSortPriority;    // see below

    bool m_bActive;         // hilite
    bool m_bNOP;            // just text
    bool m_bDisabled;       // draw with disabledColor
    bool m_bChecked;        // draw check mark

//#ifdef BBOPT_MENUICONS
    HICON m_hIcon;
    char *m_pszIcon;
    void DrawIcon(HDC hDC);
//#endif
};

//---------------------------------
// values/bitflags for m_ItemID

#define MENUITEM_ID_NORMAL  0
#define MENUITEM_ID_CMD     4
#define MENUITEM_ID_INT     (MENUITEM_ID_CMD|1)
#define MENUITEM_ID_STR     (MENUITEM_ID_CMD|2)
#define MENUITEM_ID_FOLDER  8
#define MENUITEM_ID_SF      (MENUITEM_ID_FOLDER|1)
#define MENUITEM_ID_INSSF   16
#define MENUITEM_UPDCHECK   128 // refresh checkmark everytime

// values for m_nSortPriority
#define M_SORT_NORMAL 0
#define M_SORT_NAME   1 // ignore extensions when sorting
#define M_SORT_FOLDER 2 // insert above other items

// button values for void Invoke(int button);
#define INVOKE_DBL      1
#define INVOKE_LEFT     2
#define INVOKE_RIGHT    4
#define INVOKE_MID      8
#define INVOKE_DRAG    16
#define INVOKE_PROP    32
#define INVOKE_RET     64

// FolderItem bullet positions
#define FOLDER_RIGHT    0
#define FOLDER_LEFT     1

// DrawText flags
#define DT_MENU_STANDARD (DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP | DT_EXPANDTABS)
#define DT_MENU_MEASURE_STANDARD (DT_LEFT | DT_EXPANDTABS | DT_CALCRECT | DT_EXPANDTABS)

#define MENUITEM_STANDARD_JUSTIFY -1
#define MENUITEM_CUSTOMTEXT -2

//=======================================
// A structure with precalculated menu metrics,
// as filled in by 'Menu_Reconfigure()'.

struct MenuInfo
{
    HFONT hFrameFont; // fonts...
    HFONT hTitleFont;

    int nTitleHeight; // total height of title
    int nTitleIndent; // left/right text indent

    int nItemHeight; // height of normal item
    int nItemLeftIndent; // text indent
    int nItemRightIndent;

    int nFrameMargin; // outer margin of menu frame, including border
    int nTitleMargin; // frame margin around title (normally 0)

    int nSubmenuOverlap;
    int MinWidth;     // as configured /* BlackboxZero 12.17.2011 */
    int MaxWidth;     // as configured

    COLORREF separatorColor;
    int separatorWidth; // in pixel
    bool check_is_pr; // whether checkmarks cant use the hilite style
    bool openLeft;

    // presets for possible scrollbuttons
    int nScrollerSize;
    int nScrollerSideOffset;
    int nScrollerTopOffset;
    StyleItem Scroller;

    int nBulletPosition;
    int nBulletStyle;
    int nIconSize;
};

extern struct MenuInfo MenuInfo;

//=======================================
class SeparatorItem : public MenuItem
{
public:
    SeparatorItem() : MenuItem("") {}
    void Measure(HDC hDC, SIZE *size);
    void Paint(HDC hDC);
};

//=======================================
class TitleItem : public MenuItem
{
public:
    TitleItem(const char* pszTitle) : MenuItem(pszTitle) {}
    void Paint(HDC hDC);
    void Mouse(HWND hw, UINT nMsg, DWORD wP, DWORD lP);
};

//=======================================
// An menuitem that is a pointer to a sub menu, these folder items
// typically contain a |> icon at their right side.

class FolderItem : public MenuItem
{
public:
    FolderItem(Menu* pSubMenu, const char* pszTitle);
    void Paint(HDC hDC);
    void Invoke(int button);
};

//=======================================
class CommandItem : public MenuItem
{
public:
    CommandItem(const char* pszCommand, const char* pszTitle, bool bChecked);
    void Invoke(int button);
    void next_item (WPARAM wParam);
};

//=======================================
class IntegerItem : public CommandItem
{
public:
    IntegerItem(const char* pszCommand, int value, int minval, int maxval);

    void Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam);
    void Invoke(int button);
    void ItemTimer(UINT nTimer);
    void Measure(HDC hDC, SIZE *size);
    void Key(UINT nMsg, WPARAM wParam);
    void set_next_value(void);

    int m_value;
    int m_min;
    int m_max;
    int m_count;
    int m_direction;
    int m_oldsize;
    int m_offvalue;
    const char *m_offstring;
};

//=======================================
class StringItem : public CommandItem
{
public:
    StringItem(const char* pszCommand, const char *init_string);
    ~StringItem();

    void Paint(HDC hDC);
    void Measure(HDC hDC, SIZE *size);
    void Invoke(int button);

    static LRESULT CALLBACK EditProc(HWND hText, UINT msg, WPARAM wParam, LPARAM lParam);
    HWND hText;
    WNDPROC wpEditProc;
    RECT m_textrect;
};

//=======================================
// a menu containing items from a folder
class SpecialFolder : public Menu
{
public:
    SpecialFolder(const char *pszTitle, const struct pidl_node *pidl_list, const char  *pszExtra);
    ~SpecialFolder();
    void UpdateFolder(void);
    friend class SpecialFolderItem;
private:
    char *m_pszExtra;
};

//=======================================
// an item that opens a SpecialFolder - unlike with
// other submenus these are not built until needed
class SpecialFolderItem : public FolderItem
{
public:
    SpecialFolderItem(const char* pszTitle, const char *path, struct pidl_node* pidl_list, const char *pszExtra);
    ~SpecialFolderItem();
    void ShowSubmenu(void);
    void Invoke(int button);
    friend class SpecialFolder;
private:
    char *m_pszExtra;
};

//=======================================
// an invisble item that expands into a folder listing when validated
class SFInsert : public MenuItem
{
public:
    SFInsert(const char *pszPath, const char *pszExtra);
    ~SFInsert();
    void Paint(HDC hDC);
    void Measure(HDC hDC, SIZE *size);
    void RemoveStuff(void);
private:
    char *m_pszExtra;
    MenuItem *m_pLast;
};

//=======================================
/* ContextMenu.cpp */
class ContextMenu : public Menu
{
public:
    ContextMenu (const char* title, class ShellContext* w, HMENU hm, int m);
    ~ContextMenu();
private:
    void Copymenu (HMENU hm);
    class ShellContext *wc;
    friend class ContextItem;
};

class ContextItem : public FolderItem
{
public:
    ContextItem(Menu *m, char* pszTitle, int id, DWORD data, UINT type);
    ~ContextItem();
    void Paint(HDC hDC);
    void Measure(HDC hDC, SIZE *size);
    void Invoke(int button);
    void DrawItem(HDC hdc, int w, int h, bool active);
private:
    int   m_id;
    DWORD m_data;
    UINT  m_type;

    int m_icon_offset;
    HBITMAP m_bmp;
    int m_bmp_width;
    COLORREF cr_back;
};

// ==============================================================
/* droptarget.cpp */
class CDropTarget *init_drop_target(HWND hwnd, LPCITEMIDLIST pidl);
void exit_drop_target(class CDropTarget  *);
bool in_drop(class CDropTarget *dt);

/* dropsource.cpp */
void init_drop(HWND hwnd);
void exit_drop(HWND hwnd);
void drag_pidl(LPCITEMIDLIST pidl);

// SpecialFolder.cpp
int LoadFolder(MenuItem **, LPCITEMIDLIST pIDFolder, const char  *pszExtra, int options);
enum { LF_join = 1, LF_norecurse = 2 };
void show_props(LPCITEMIDLIST pidl);

//===========================================================================
#endif /*ndef _MENU_H_ */
