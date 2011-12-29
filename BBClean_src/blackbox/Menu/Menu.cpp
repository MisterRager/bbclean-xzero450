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

#include "../BB.h"
#include "../Settings.h"
#include "../Pidl.h"
#include "MenuMaker.h"
#include "Menu.h"
#include <commctrl.h>
#include <Shellapi.h>

// Quick-hack. Drop shadows to menus.
#define CS_DROPSHADOW 0x20000
#define SPI_GETDROPSHADOW 0x1024

//#define CHECKFOCUS

const char MenuClassName[] = "BBMenu";
int g_menu_count;
int Menu::g_MouseWheelAccu;
int Menu::g_DiscardMouseMoves;

MenuList *Menu::g_MenuStructList;   // all menus
MenuList *Menu::g_MenuWindowList;   // menus with a window
MenuList *Menu::g_MenuFocusList;    // menus with focus recently (for fallback)

//===========================================================================
// Menu constructor

Menu::Menu(const char *pszTitle)
{
    ZeroMemory(&this->m_refc, sizeof *this - ((char*)&this->m_refc - (char*)this));

    m_bOnTop        = Settings_menusOnTop;
    //m_bNoTitle      = true;

    // Initialize the item-add-pointer
    m_ppAddItem = &m_pMenuItems;

    // Add TitleItem
    AddMenuItem(new TitleItem(pszTitle));

    // add to the global menu list
    cons_node(&g_MenuStructList, new_node(this));
    ++g_menu_count;

    // start with a refcount of 1, assuming that
    // the menu is either shown or linked as submenu
    m_refc = 1;

	// use default icon size
	m_iconSize = -2;
}

//==============================================
// Menu destructor

Menu::~Menu()
{
    DeleteMenuItems();
    delete m_pMenuItems; // TitleItem
    free_str(&m_IDString);
    delete_assoc(&g_MenuStructList, this);
    --g_menu_count;
}

//===========================================================================
// The menu window class

void register_menuclass(void)
{
	// Noccy: Check if the system has menu shadows enabled in the first plcae
	long lDropShadowEnabled = 0;
	SystemParametersInfo(SPI_GETDROPSHADOW,0,&lDropShadowEnabled,0);
	bool bDropShadows = !(lDropShadowEnabled == 0);

	// Go on with creating the class
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style            = CS_DBLCLKS;
	if ((Settings_menuShadowsEnabled) && (bDropShadows))
		wc.style = wc.style | CS_DROPSHADOW;
    wc.lpfnWndProc      = Menu::WindowProc;
    wc.hInstance        = hMainInstance;
    wc.lpszClassName    = MenuClassName;
    wc.hCursor          = MenuInfo.arrow_cursor = LoadCursor(NULL, IDC_ARROW);
    wc.cbWndExtra       = sizeof (LONG_PTR);
    BBRegisterClass(&wc);
}

void un_register_menuclass(void)
{
	Menu_All_Delete();
    UnregisterClass(MenuClassName, hMainInstance);
}

bool IsMenu(HWND hwnd)
{
    return (LONG_PTR)Menu::WindowProc == GetWindowLongPtr(hwnd, GWLP_WNDPROC);
}

//===========================================================================
// Give a window to the menu, when it's about to be shown

void Menu::make_menu_window(void)
{
    if (NULL == m_hwnd)
    {
        incref();
        m_hwnd =  CreateWindowEx(
            WS_EX_TOOLWINDOW,
            MenuClassName,      // window class name
            NULL,               // window title
            WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 0, 0,
            NULL,               // parent window
            NULL,               // no menu
            hMainInstance,      // hInstance
            this                // creation data - link to the object
            );

        MakeSticky(m_hwnd);
        register_droptarget(true);
        insert_at_last();

        m_bMouseOver = false;
        m_bIconized = false;
        m_keyboard_item_pos = -2;
        m_active_item_pos = -2;
        m_alpha = 255;
    }
}

//===========================================================================
// Remove the window, when the menu is hidden

void Menu::destroy_menu_window(void)
{
    if (m_hwnd)
    {
        register_droptarget(false);
        HWND hwnd = m_hwnd;
        m_hwnd = NULL;
        RemoveSticky(hwnd);
        DestroyWindow(hwnd);
        delete_assoc(&g_MenuWindowList, this);
        delete_assoc(&g_MenuFocusList, this);
        if (m_hBitMap) DeleteObject(m_hBitMap), m_hBitMap = NULL;
        decref();
    }
}

//===========================================================================
void Menu::register_droptarget(bool set)
{
    if (set)
    {
        if (m_bIsDropTarg)
            m_droptarget = init_drop_targ(m_hwnd, NULL);
    }
    else
    {
        if (m_droptarget)
            exit_drop_targ(m_droptarget), m_droptarget = NULL;
    }
}

//===========================================================================
// Delete all menues

void Menu_All_Delete(void)
{
    while (Menu::g_MenuWindowList)
        Menu::g_MenuWindowList->m->destroy_menu_window();
}

//==============================================
// Menu destruction is controlled by reference
// counter. References are:
// - having a Window
// - being a submenu, i.e. bound to m_pSubMenu of an item

int Menu::decref(void)
{
    //dbg_printf("decref %d %s", m_refc-1, m_pMenuItems->m_pszTitle);
    int n; if (0 == (n = --m_refc)) delete this;
    return n;
}

int Menu::incref(void)
{
    //dbg_printf("incref %d %s", m_refc, m_pMenuItems->m_pszTitle);
    return ++m_refc;
}

//==============================================
// When a submenu becomes visible, the m_pParent(Item)
// pointers are set, also the m_pChild pointer of the parent

// When it becomes invisible (Item = NULL), they are reset.

void Menu::LinkToParentItem(MenuItem *Item)
{
    if (m_pParentItem)
    {
        m_pParent->m_pChild = NULL;
        m_pParent = NULL;
    }
    m_pParentItem = Item;
    if (m_pParentItem)
    {
        m_pParent = m_pParentItem->m_pMenu;
        m_pParent->m_pChild = this;
    }
}

//==============================================
// Append item to the list

MenuItem *Menu::AddMenuItem(MenuItem* Item)
{
    // link to the list
    m_ppAddItem = &(*m_ppAddItem=Item)->next;

    // set references
    Item->m_pMenu = this;
    return Item;
}

//==============================================
// clear all Items - except the TitleItem

void Menu::DeleteMenuItems()
{
    MenuItem *thisItem, *Item = m_pMenuItems;
    Item->m_bActive = false;
    Item = *(m_ppAddItem = &Item->next);
    *m_ppAddItem = m_pActiveItem = NULL;
    while (Item) Item = (thisItem=Item)->next, delete thisItem;
}

//==============================================

Menu *Menu::FindNamedMenu(LPCSTR IDString)
{
    MenuList *ml;
    dolist (ml, g_MenuStructList)
        if (ml->m->m_IDString && 0==strcmp(ml->m->m_IDString, IDString))
        {
            return ml->m;
        }
    return NULL;
}

//===========================================================================

// Timer to do the submenu open and close delay.
void Menu::set_timer(bool active, bool set) {

	if ( active == true ) {
		//Native handling for popup
		if (false == set) {
			KillTimer(m_hwnd, MENU_POPUP_TIMER);
		} else if (Settings_menuPopupDelay) {
			SetTimer(m_hwnd, MENU_POPUP_TIMER, Settings_menuPopupDelay, NULL);
		} else {
			UpdateWindow(m_hwnd); // Set priority to updating the hilite bar
			PostMessage(m_hwnd, WM_TIMER, MENU_POPUP_TIMER, 0);
		}
	} else {
		if ( set == false ) {
			KillTimer(m_hwnd, MENU_POPUP_TIMER);
		} else if (Settings_menuCloseDelay) {
			SetTimer(m_hwnd, MENU_POPUP_TIMER, Settings_menuCloseDelay, NULL);
		} else {
			UpdateWindow(m_hwnd);
			PostMessage(m_hwnd, WM_TIMER, MENU_POPUP_TIMER, 0);
		}
	}
}

void Menu::set_focus(void)
{
    // pass the focus to the edit control eventually
    HWND cwnd = GetWindow(m_hwnd, GW_CHILD);
    SetFocus(cwnd ? cwnd : m_hwnd);

    // if there is a submenu, possibly overlapping,
    // then place this behind it again
    if (m_pChild) menu_set_pos(NULL, SWP_NOMOVE|SWP_NOSIZE);
}

//===========================================================================
// scroll helpers

// helper: get the scroll range in pixel (menuheight-title-scrollbutton)
int Menu::get_y_range(int *py0, int *ps)
{
    int y0, s;
	int is = m_iconSize;
	if (-2 == is) is = MenuInfo.nIconSize;
	*ps = s = MenuInfo.nScrollerSize[is];
	*py0 = y0 = m_firstitem_top - MenuInfo.nScrollerTopOffset;
	return m_height - y0 - s - MenuInfo.nScrollerSideOffset;
}

// get the scroll button square from scrollindex
void Menu::get_vscroller_rect(RECT* rw) {
	int y0, s, k, d;
	k = get_y_range(&y0, &s);
	d = MenuInfo.nScrollerSideOffset;
	rw->top    = m_scrollpos + y0;
	rw->bottom = rw->top + s;
	
	//I think this basicly forces the left OR right side of the scroll button to be drawn
	//else it draws all the way accross the menu
	if ( DT_RIGHT == FolderItem::m_nScrollPosition ) {
		rw->left = (rw->right = m_width - d) - s;
	} else if ( FolderItem::m_nScrollPosition == DT_LEFT ) {
		rw->right = (rw->left = d) + s;
	} else {
		rw->right =  rw->left;
	}
}

// helper:  get the topitem-index from scrollindex
int Menu::calc_topindex (bool adjust)
{
    int y0, s, k, d, n;
    k = get_y_range(&y0, &s);
    d = m_itemcount - m_pagesize;
    if (d == 0)
    {
        m_scrollpos = n = 0;
    }
    else
    {
        s = imax(d, m_pagesize);
        n = imin(d, (s * m_scrollpos + k/2) / k);
        m_scrollpos = imin(m_scrollpos, d * k / s);
        if (adjust && n != m_topindex)
            m_scrollpos = m_topindex * k / s;
    }
    return n;
}

// set the scroll button according to mouse position, scroll menu if needed
void Menu::set_vscroller(int ymouse)
{
    int y0, s, k, i;
    k = get_y_range(&y0, &s);
    i = iminmax(ymouse - s/2 - y0, 0, k);

    RECT scroller1, scroller2;
    get_vscroller_rect(&scroller1);
    m_scrollpos = i;
    scroll_assign_items(calc_topindex(false));
    get_vscroller_rect(&scroller2);
    if (scroller1.top != scroller2.top)
    {
        InvalidateRect(m_hwnd, &scroller1, FALSE);
        InvalidateRect(m_hwnd, &scroller2, FALSE);
    }
}

// set the scroll button according to changed topindex, itemcount, pagesize
void Menu::scroll_menu(int n)
{
    scroll_assign_items(n + m_topindex);
    calc_topindex(true);
}

// assign y coordinates to items according to _new topindex
void Menu::scroll_assign_items(int new_top)
{
    new_top = imax(0, imin(new_top, m_itemcount - m_pagesize));
    if (m_hBitMap)
    {
        if (m_topindex == new_top) return;
        if (m_hwnd) InvalidateRect(m_hwnd, NULL, FALSE);
    }

    int c0  = m_topindex = new_top;
    int c1  = c0 + m_pagesize;
    int c   = 0;
    int y   = m_firstitem_top;
    MenuItem *Item = m_pMenuItems;
    while (NULL != (Item=Item->next)) // skip TitleItem
    {
        if (c<c0 || c>=c1) Item->m_nTop = -1000;
        else Item->m_nTop = y, y += Item->m_nHeight;
        c++;
    }
}

//===========================================================================
// some utilities

// When a menu get's the focus, it is inserted at the last position of the
// list, so that it is set to the top of the z-order with "Menu_All_BringOnTop()"
void Menu::insert_at_last (void)
{
    MenuList **mp, *ml, *mn = NULL;
    for (mp = &g_MenuWindowList; NULL != (ml = *mp); )
    {
        if (this == ml->m) *mp = (mn = ml)->next;
        else mp = &ml->next;
    }
    if (NULL == mn) mn = (MenuList*)new_node(this);
    (*mp = mn)->next = NULL;
}

void Menu::cons_focus(void)
{
    MenuList *ml = (MenuList *)assoc(g_MenuFocusList, this);
    if (ml) remove_node(&g_MenuFocusList, ml);
    else ml = (MenuList*)new_node(this);
    cons_node(&g_MenuFocusList, ml);
}

// get the list-index for item or -2, if not found
int Menu::get_item_index (MenuItem *item)
{
    int c = -1; MenuItem *Item;
    dolist (Item, m_pMenuItems)
    {
        if (item == Item) return c;
        c++;
    }
    return -2;
}

// get the active item's index
int Menu::get_active_index (void)
{
    return get_item_index(m_pActiveItem);
}

// get the item from idex
MenuItem * Menu::nth_item(int a)
{
    if (a < -1) return NULL;
    int c = -2; MenuItem *Item;
    dolist (Item, m_pMenuItems) if (++c == a) break;
    return Item;
}

// get the root in a menu chain
Menu *Menu::menu_root (void)
{
    Menu *p, *m = this;
    while (NULL != (p = m->m_pParent)) m = p;
    return m;
}

// is this window part of the menu-chain, which contains Menu *m
bool Menu::has_in_chain(HWND hwnd)
{
    Menu *p;
    for (p = this->menu_root(); p; p = p->m_pChild)
        if (p->m_hwnd == hwnd) return true;
    return false;
}

bool Menu::has_focus_in_chain(void)
{
    Menu *p;
    for (p = this->menu_root(); p; p = p->m_pChild)
        if (p->m_bHasFocus) return true;
    return false;
}

//===========================================================================
// static global functions to access menus

// Get the root of the last active menu
Menu * Menu::last_active_menu_root(void)
{
    MenuList *ml;
    dolist (ml, g_MenuWindowList)
        if (NULL == ml->next) return ml->m->menu_root();
    return NULL;
}

// Set focus to last active menu
bool Menu_Activate_Last(void)
{
    Menu *m = Menu::last_active_menu_root();
    if (m) { m->set_focus(); return true; }
    return false;
}

// Toggle visibility state
void Menu_All_Toggle(bool hide)
{
    if (false == Settings_menuspluginToggle) return;
    MenuList *ml;
    dolist (ml, Menu::g_MenuWindowList)
        ShowWindow(ml->m->m_hwnd, hide ? SW_HIDE : SW_SHOWNA);
}

// update menus after style changed
void Menu_All_Redraw(int flags)
{
    Menu*m; MenuList *ml;
    dolist (ml, Menu::g_MenuWindowList)
        if (NULL == (m=ml->m)->m_pParent)
            PostMessage(m->m_hwnd, BB_REDRAWGUI, flags, 0);
}

// Hide all menus, which are not pinned
void Menu_All_Hide(void)
{
    Menu::Hide_All_But(NULL);
}

// bring all menus on top, restoring the previous z-order
void Menu_All_BringOnTop(void)
{
    Menu *m; MenuList *ml;
    dolist (ml, Menu::g_MenuWindowList)
    {
        SetOnTop((m=ml->m)->m_hwnd);
        m->cons_focus();
    }
}

//===========================================================================
// continue with private functions

// Hide all menus, which are not pinned, but not 'p'
void Menu::Hide_All_But(Menu *p)
{
    Menu*m; MenuList *ml;
    dolist (ml, g_MenuWindowList)
        if (NULL == (m=ml->m)->m_pParent && p != m)
            PostMessage(m->m_hwnd, BB_HIDEMENU, 0, 0);
}

// Hide menu on item clicked
void Menu::hide_on_click(void)
{
    Menu *root = menu_root();
    if (!root->IsPinned()) root->Hide();
    else
    if (m_MenuID & MENU_ID_SHCONTEXT) Hide();
}

// save menu fixed keyboard position
void Menu::write_menu_pos(void)
{
    if (m_bKBDInvoked)
    {
        Settings_menuPositionX = m_xpos;
        Settings_menuPositionY = m_ypos;
        Settings_WriteRCSetting(&Settings_menuPositionX);
        Settings_WriteRCSetting(&Settings_menuPositionY);
    }
}

void Menu::menu_set_pos(HWND after, UINT flags)
{
    if (m_pChild) // keep it behind the child menu anyway
        after = m_pChild->m_hwnd;
    else
	if (m_bOnTop || Settings_menusOnTop || false == menu_root()->IsPinned())
        after = HWND_TOPMOST;

    SetWindowPos(m_hwnd, after, m_xpos, m_ypos, m_width, m_height,
        flags|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
}

// set menu on top of the z-order
void Menu::bring_menu_ontop(void)
{
#if 0
    // autoraise focus
    ForceForegroundWindow(m_hwnd);
#else
    if (bbactive)
    {
        set_focus();
    }
    else
    {
        menu_set_pos(HWND_TOP, SWP_NOMOVE|SWP_NOSIZE);
        // update it's position in the global menu-list
        insert_at_last();
    }
#endif
}

//--------------------------------------
// Unspecific utility

HWND window_under_mouse(void)
{
    POINT pt; GetCursorPos(&pt);
	return GetRootWindow(WindowFromPoint(pt));
}

//===========================================================================
// Paint the menu. The Background gradient bitmap is cached. Items are drawn
// only if they intersect with the update rectangle.

void Menu::Paint()
{
    //int x = GetTickCount();

    PAINTSTRUCT ps;
    HDC hdc, hdc_screen = hdc = BeginPaint(m_hwnd, &ps);
    HDC back = CreateCompatibleDC(hdc_screen);
    RECT r; HGDIOBJ S0, F0, B0 = NULL;

    int y1 = ps.rcPaint.top;
    int y2 = ps.rcPaint.bottom;

    if (y1 == 0)
    {
        hdc = CreateCompatibleDC(hdc_screen);
        B0 = SelectObject(hdc, CreateCompatibleBitmap(hdc_screen, m_width, m_height));
    }

    MenuItem *Item = m_pMenuItems;

    if (NULL == m_hBitMap) // create the background
    {
        m_hBitMap = CreateCompatibleBitmap(hdc_screen, m_width, m_height);
        S0 = SelectObject(back, m_hBitMap);

        StyleItem *pSI = &mStyle.MenuFrame;
        int bw = pSI->borderWidth;
        COLORREF bc = pSI->borderColor;

        if (bw){
            _SetRect(&r, 0, 0, m_width, m_height);
            CreateBorder(back, &r, bc, bw);
        }

        _SetRect(&r, bw, bw, m_width - bw, m_height - bw);
        if (false == mStyle.MenuTitle.parentRelative)
            r.top = m_firstitem_top + bw - MenuInfo.nFrameMargin;

        if (r.bottom > r.top)
            MakeStyleGradient(back, &r, pSI, false);

        if (false == m_bNoTitle)
        {
            // Menu Title
            F0 = SelectObject(back, MenuInfo.hTitleFont);
            SetBkMode(back, TRANSPARENT);
            SetTextColor(back, mStyle.MenuTitle.TextColor);
            Item->Paint(back);
            SelectObject(back, F0);
        }
    }
    else
    {
        S0 = SelectObject(back, m_hBitMap);
    }

    // copy (invalid part of) the background
    BitBltRect(hdc, back, &ps.rcPaint);
    SelectObject(back, S0);
    DeleteDC(back);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, mStyle.MenuFrame.TextColor);
    F0 = SelectObject(hdc, MenuInfo.hFrameFont);
    int c1 = m_topindex;
    int c2 = c1 + m_pagesize;
    int c = -1;
    while (c < c1 && Item) Item = Item->next, ++c;
    while (c < c2 && Item)
    {
        int y = Item->m_nTop;
        if (y >= y2) break;
        if (y + Item->m_nHeight > y1) Item->Paint(hdc);
        Item = Item->next, ++c;
    }
    SelectObject(hdc, F0);

    if (m_pagesize < m_itemcount && false == m_bIconized)
	{
		if ( FolderItem::m_nScrollPosition != DT_CENTER ) {
			get_vscroller_rect(&r);
			HDC buf = CreateCompatibleDC(hdc_screen);
		
			if (NULL == m_hBmpScroll)
			{
				RECT rs;
				rs.left = 0; rs.right  = r.right - r.left;
				rs.top  = 0; rs.bottom = r.bottom - r.top;
				m_hBmpScroll = CreateCompatibleBitmap(hdc_screen, rs.right, rs.bottom);
				S0 = SelectObject(buf, m_hBmpScroll);
				StyleItem *pSI = &MenuInfo.Scroller;
				MakeStyleGradient(buf, &rs, pSI, pSI->bordered);
			}
			else
				S0 = SelectObject(buf, m_hBmpScroll);

			// draw the scroll button
				if (Settings_menuScrollHue ) {
					int x,y;
					for (x = r.left; x < r.right; ++x)
						for (y = r.top; y < r.bottom; ++y)
							SetPixel(hdc, x, y, mixcolors(
									GetPixel(hdc, x, y),
									GetPixel(buf, x - r.left, y - r.top),
									Settings_menuScrollHue
								));
				} else
					BitBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, buf, 0, 0, SRCCOPY);
		

			SelectObject(buf, S0);
			DeleteDC(buf);
		}
    }

    if (hdc != hdc_screen)
    {
        BitBltRect(hdc_screen, hdc, &ps.rcPaint);
        DeleteObject(SelectObject(hdc, B0));
        DeleteDC(hdc);
    }

#ifdef CHECKFOCUS
	bool focus = GetRootWindow(GetFocus()) == m_hwnd;
    if (focus || m_bHasFocus)
    {
        RECT r; GetClientRect(m_hwnd, &r);
        COLORREF c = focus ? (m_bHasFocus ? 0x00DDFF : 0x0088FF) : 0x00CC00;
        CreateBorder(hdc_screen, &r, c, 2);
    }
#endif

    EndPaint(m_hwnd, &ps);
    //dbg_printf("ticks %d", GetTickCount() - x);
}

//==============================================

void Menu::SetZPos(void)
{
    menu_set_pos(HWND_NOTOPMOST, SWP_NOMOVE|SWP_NOSIZE);
}


//==============================================

void Menu::Show(int xpos, int ypos, int flags)
{
    bool ontop = m_bOnTop || false == menu_root()->IsPinned();
    RECT screen_rect;

    Menu *p = m_pParent;

    if (p) // a submenu
    {
		int overlap = MenuInfo.nFrameMargin + mStyle.MenuHilite.borderWidth;
        int xleft = p->m_xpos - this->m_width + overlap;
        int xright = p->m_xpos + p->m_width - overlap;
// 
//         if (mStyle.MenuHilite.borderWidth){
//             xleft -= 1;
//             xright += 1;
//         }
// 
        GetMonitorRect(p->m_hwnd, &screen_rect, ontop ? GETMON_FROM_WINDOW : GETMON_FROM_WINDOW|GETMON_WORKAREA );

        // pop to the left or to the right ?
        bool at_left;
        if (xright + m_width > screen_rect.right) at_left = true;
        else
        if (xleft < screen_rect.left) at_left = false;
        else
        if (p->m_pParent)
            at_left = p->m_pParent->m_xpos > p->m_xpos;
        else
        if (BS_EMPTY != FolderItem::m_nBulletStyle)
            at_left = FolderItem::m_nBulletPosition == DT_LEFT;
        else
            at_left = p->m_xpos > (screen_rect.left + screen_rect.right) / 2;

        xpos = at_left ? xleft : xright;
        ypos = p->m_ypos;
        if (m_pParentItem != m_pParent->m_pMenuItems) // not the TitleItem
            ypos += m_pParentItem->m_nTop - p->m_firstitem_top;
    }
    else
    {
        if (m_hwnd)
        {
            GetMonitorRect(m_hwnd, &screen_rect, GETMON_FROM_WINDOW);
        }
        else
        {
            POINT pt = { xpos, ypos };
            GetMonitorRect(&pt, &screen_rect, GETMON_FROM_POINT);
        }

		//This isn't pretty, as every time you adjust a setting, both get adjusted by the value
		//Perhaps removing this from being adjustable in the menu would be best
		//Settings_menuPositionAdjustPlacement
		const char *MenuPlacement = Settings_menuPositionAdjustPlacement;
		if ( !(stricmp(MenuPlacement, "default") == 0) ) {

			if ( stricmp(MenuPlacement, "TopRight") == 0) {
				//We could just pass this on to the code below, but we wont
				xpos -= m_width;
			} else if ( stricmp(MenuPlacement, "TopCenter") == 0) {
				xpos -= m_width/2;
			} else if ( stricmp(MenuPlacement, "TopLeft") == 0) {
				//Do nothing
			} else if ( stricmp(MenuPlacement, "MiddleRight") == 0) {
				//We could just pass this on to the code below, but we wont
				xpos -= m_width;
				ypos -= m_height/2;
			} else if ( stricmp(MenuPlacement, "MiddleCenter") == 0) {
				xpos -= m_width/2;
				ypos -= m_height/2;
			} else if ( stricmp(MenuPlacement, "MiddleLeft") == 0) {
				//xpos -- Do nothing
				ypos -= m_height/2;
			} else if ( stricmp(MenuPlacement, "BottomRight") == 0) {
				//We could just pass this on to the code below, but we wont
				xpos -= m_width;
				ypos -= m_height;
			} else if ( stricmp(MenuPlacement, "BottomCenter") == 0) {
				xpos -= m_width/2;
				ypos -= m_height;
			} else if ( stricmp(MenuPlacement, "BottomLeft") == 0) {
				//xpos -- Do nothing
				ypos -= m_height;
			} else if ( stricmp(MenuPlacement, "custom") == 0 ) {
				xpos += Settings_menuPositionAdjustX;
				ypos += Settings_menuPositionAdjustY;
			}
		} else {

			if (MENUSHOW_CENTER & flags) {
				xpos -= m_width/2;
				ypos -= MenuInfo.nTitleHeight/2;
			} else if (MENUSHOW_BOTTOM & flags) {
				ypos -= m_height;
			}

			if (MENUSHOW_RIGHT & flags) {
				xpos -= m_width;
				if (MENUSHOW_UPDATE & flags) {
					int w = screen_rect.right - screen_rect.left;
					if (xpos < screen_rect.left + w * 2 / 3)
						xpos = m_xpos;
				}
			}
		}
	}

    // make sure the popup is on the screen
    m_xpos = iminmax(xpos, screen_rect.left, screen_rect.right - m_width);
    m_ypos = iminmax(ypos, screen_rect.top, screen_rect.bottom - (IsPinned() ? MenuInfo.nTitleHeight : m_height));

	//if (NULL == ms_hwnd) make_menu_shadow_window();
	if (NULL == m_hwnd) make_menu_window();
	const char *tmp = Settings_menuAlphaMethod_cfg;
	if (m_alpha != Settings_menuAlpha && !(stricmp(tmp, "default") == 0) )
        SetTransparency(m_hwnd, m_alpha = Settings_menuAlpha);

    if (m_MenuID & MENU_ID_STRING)
        ((StringItem*)m_pMenuItems->next)->SetPosition();

    if (flags & MENUSHOW_UPDATE)
        menu_set_pos(HWND_NOTOPMOST, SWP_NOZORDER);
    else
        menu_set_pos(HWND_TOP, SWP_SHOWWINDOW);
}

//===========================================================================
// Redraw the menu on changes.

// - keeps the active item if possible
// - adjust sizes and position if neccessary

void Menu::redraw(void)
{
    // save right corner
    int x_right = m_xpos + m_width;
    // recalculate sizes
    Validate();
    // update position and size
    Show(x_right, m_ypos, MENUSHOW_RIGHT|MENUSHOW_UPDATE);
}

//==============================================
// On Timer: Show/Hide submenu

void Menu::MenuTimer(UINT nTimer)
{
    if (MENU_POPUP_TIMER == nTimer)
    {
		//set_timer(false);
		set_timer(true, false);
        if (NULL == m_pActiveItem)
        {
            HideChild();
        /*
            if (false == bbactive && false == has_in_chain(window_under_mouse()))
                Hide_All_But(NULL);
        */
            return;
        }

        if (NULL == m_pParent && NULL == m_pChild)
            bring_menu_ontop();

        Hide_All_But(this->menu_root());
    }

    if (m_pActiveItem) m_pActiveItem->ItemTimer(nTimer);
}

void Menu::UnHilite()
{
    if (m_pActiveItem) m_pActiveItem->Active(0);
}

//==============================================
// File Folders and context menus are destroyed
// after use, whereas custom menus are kept with
// their data

void Menu::Detach()
{
    if (m_pParent)
    {
        if (false == m_pParent->m_bMouseOver)
            m_pParent->UnHilite();

        if ((m_MenuID & (MENU_ID_SF|MENU_ID_SHCONTEXT))
         && !(m_pParent->m_MenuID & MENU_ID_SHCONTEXT))
            m_pParentItem->UnlinkSubmenu();
        else
            LinkToParentItem(NULL);
    }
}

//==============================================
// Hide menus

void Menu::Hide(void)
{
    HideChild();

    if (false == m_bMouseOver)
    {
        UnHilite();
    }

    if (false == IsPinned())
    {
        if (m_bHasFocus)
        {
            delete_assoc(&g_MenuFocusList, this);
            if (m_pParent) m_pParent->set_focus();
            else
            if (g_MenuFocusList) g_MenuFocusList->m->set_focus();
        }

        Detach();

        // the OS does not seem to like a window being destroyed while
        // it is dragsource (may drop just somewhere) or droptarget
        // (wont be released)
        if (false == m_bInDrag && false == in_drop(m_droptarget))
        {
            UnHilite();
            destroy_menu_window();
        }
    }
}

void Menu::HideThis(void)
{
    SetPinned(false); // force
    Hide();
}

void Menu::HideChild(void)
{
	if (m_pChild)
		m_pChild->Hide(); // close submenu
}

//==============================================
// detach menu from it's parent

void Menu::SetPinned(bool bPinned)
{
    m_bPinned = bPinned;
    if (m_bPinned)
    {
        Menu *p = menu_root();
        if (p != this)
        {
            Detach();
            p->Hide();
        }
    }
}

//==============================================
// Virtual: Implemented by SpecialFolder to load
// update contents on folder change

void Menu::UpdateFolder(void)
{
}

//==============================================
// Calculate sizes of the menu-window

void Menu::Validate()
{
    int margin  = MenuInfo.nFrameMargin;
    int border  = mStyle.MenuFrame.borderWidth;
    int tborder = mStyle.MenuTitle.borderWidth;
    int hmax    = MenuInfo.MaxMenuHeight;
	bool workspace_menu = false;

    int w1, w2, c0, c1, h1; SIZE size;
    char current_optional_command[2*MAX_PATH];

    current_optional_command[0] = 0;
    w1 = w2 = c0 = c1 = h1 = 0;

    // ---------------------------------------------------
    // measure text-sizes
    MenuItem *Item = m_pMenuItems;

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ other_font = SelectObject(hDC, MenuInfo.hTitleFont);

    // title item
    Item->Measure(hDC, &size);
    Item->m_nHeight = MenuInfo.nTitleHeight;
    w1 = size.cx;

    SelectObject(hDC, MenuInfo.hFrameFont);

    // frame items
    while (NULL != (Item = Item->next))
    {
        Item->Measure(hDC, &size);
        if (size.cx > w2) w2 = size.cx;
        ++c0;
        int h = Item->m_nHeight = size.cy;
        if (h1 < hmax) { c1 = c0; h1 += h; }

        // set the 'current style selected' checkmark
        if (Item->m_ItemID & MENUITEM_ID_STYLE)
        {
            const char *cmd = Item->m_pszCommand;
            if (0 == current_optional_command[0])
                init_check_optional_command(cmd, current_optional_command);
            Item->m_isChecked = 0 == stricmp(current_optional_command, cmd);
        }

		if ( Item->m_ItemID & MENUITEM_ID_FOLDER && Item->m_pMenu && Item->m_pMenu->m_IDString && !Settings_menuIconSize ) {
			char	tmp[128];
			sprintf(tmp, "%s", Item->m_pMenu->m_IDString);
			if ( tmp[0] == 'I' && stricmp(tmp, "IDroot_workspaces") == 0 ) {
				workspace_menu = true;
			} else {
				workspace_menu = false;
			}
		}


	}

	m_itemcount = c0;
    m_pagesize = c1;

    SelectObject(hDC, other_font);
    DeleteDC(hDC);
    
	// ---------------------------------------------------
	// make sure that the menu is not wider than something

	int is = m_iconSize;
	if (-2 == is) is = MenuInfo.nIconSize;
	m_width = imin( MenuInfo.MaxMenuWidth, imax(w1 + 2 * (MenuInfo.nTitleIndent + border),
			 w2 + 2 * (MenuInfo.nItemIndent[is] + margin) ));

	if ( workspace_menu && !Settings_menuIconSize ) {
		m_width += 4;
	}
//Check and set minimum width
	if ( m_width < MenuInfo.MinMenuWidth && MenuInfo.MinMenuWidth <= MenuInfo.MaxMenuWidth ) {
		m_width = MenuInfo.MinMenuWidth;
	}

	if ( !Settings_menuIconSize ) {
		m_width += 12;//Hack.
	}

	// ---------------------------------------------------
    // assign item positions

    // the title item
    Item = m_pMenuItems;

    if (m_bNoTitle)
    {
        Item->m_nTop = -1000;
        m_firstitem_top = margin;
    }
    else
    {
        Item->m_nTop = border;
        m_firstitem_top = Item->m_nHeight + margin;
    }

    if (c0 && false == m_bIconized) // there are items
        m_height = m_firstitem_top + h1 + margin;
    else
    if (m_bNoTitle)
        m_height = 2*(border + margin + 1);
    else
    if (mStyle.MenuTitle.parentRelative)
        m_height = m_firstitem_top + border;
    else
        m_height = Item->m_nHeight - tborder + 2*border;

    // asign x-coords and width
    w1 = m_width - 2*border;
    w2 = m_width - 2*margin;
    Item->m_nLeft = border;
    Item->m_nWidth = w1;
    while (NULL != (Item = Item->next))
    {
        Item->m_nLeft = margin;
        Item->m_nWidth = w2;
    }

    // need a _new background
    if (m_hBitMap)
    {
        DeleteObject(m_hBitMap), m_hBitMap = NULL;
        InvalidateRect(m_hwnd, NULL, FALSE);
	}

	// remove the scroller bitmap in cache
	if (m_hBmpScroll)
		DeleteObject(m_hBmpScroll), m_hBmpScroll = NULL;

	// asign y-coords
    scroll_menu(0);
}

//===========================================================================
void track_mouse_event(HWND hwnd, bool indrag)
{
	if (false == indrag && pTrackMouseEvent) {
		TRACKMOUSEEVENT tme;
		tme.cbSize  = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hwnd;
		tme.dwHoverTime = HOVER_DEFAULT;
		pTrackMouseEvent(&tme);
	} else {
		// TrackMouseEvent does not seem to work in drag operation (and on win95)
		SetTimer(hwnd, MENU_TRACKMOUSE_TIMER, iminmax(Settings_menuPopupDelay-10, 20, 100), NULL);
	}
}

//===========================================================================
void Menu::mouse_over(bool indrag)
{
    if (m_bMouseOver) return;

    // hide grandchilds
    if (m_pChild) m_pChild->HideChild();

    //hilite my parentItem
    if (m_pParent)
    {
		//m_pParent->set_timer(false);
		m_pParent->set_timer(true, false);
        m_pParentItem->Active(2);
    }

    //set focus to this one immediately
    if (has_focus_in_chain()) set_focus();

    m_bMouseOver=true;
    track_mouse_event(m_hwnd, indrag);
}

//==============================================

void Menu::mouse_leave(void)
{
    if (false == Settings_menuKeepHilite
        && (NULL == m_pChild
            || (false == m_pChild->m_bMouseOver
                && NULL == m_pChild->m_pChild))
       )
    {
        UnHilite();
		//if (m_pChild) set_timer(true); // to close submenu
		if (m_pChild) set_timer(false, true); // to close submenu
    }
}

//==============================================

void Menu::Handle_Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    if (hwnd == GetCapture())
    {
        // ------------------------------------------------
        if (m_captureflg & MENU_CAPT_SCROLL) // the scroll button
        {
            if (WM_MOUSEMOVE == uMsg)
            {
                set_vscroller(pt.y);
            }
        }
        else
        if (m_captureflg & MENU_CAPT_TWEAKINT) // integer item
        {
            if (m_pActiveItem)
                m_pActiveItem->Mouse(hwnd, uMsg, wParam, lParam);
        }
        else
        if (WM_MOUSEMOVE == uMsg && m_pActiveItem && hwnd != window_under_mouse())
        {
            // start drag operation on mouseleave
            ReleaseCapture();
            m_captureflg = 0;
            m_pActiveItem->Invoke(INVOKE_DRAG);
        }

        if (WM_MOUSEMOVE == uMsg)
            return;
    }
    else // not captured
    {
        // Set Focus, track mouse, etc...
        mouse_over(false);

        // ------------------------------------------------
        // check ScrollButton, if there
        if (m_pagesize < m_itemcount)
        {
            RECT scroller; get_vscroller_rect(&scroller);
            if (pt.x >= scroller.left && pt.x < scroller.right && pt.y >= MenuInfo.nTitleHeight)
            {
                if (WM_LBUTTONDOWN == uMsg)
                {
                    set_vscroller(pt.y);
                    m_captureflg |= MENU_CAPT_SCROLL;
                    SetCapture(hwnd);
                }
                else
                if (m_pChild)
                {
					if (m_pChild->m_pParentItem->within(pt.y)) {
						// ignore scroller, if an folder-item is at the same position
						bool at_right = m_pChild->m_xpos > m_xpos;
						bool scroll_right = DT_RIGHT == FolderItem::m_nScrollPosition;
						if (at_right == scroll_right) return;
					}
                }
                mouse_leave();
                return;
            }
        }
    }

    // ------------------------------------------------
    // enable moving menus without title
    if (m_bNoTitle && (wParam & MK_CONTROL))
    {
        m_pMenuItems->Mouse(hwnd, uMsg, wParam, lParam);
        return;
    }

    // check through items, where the mouse is over
    MenuItem* item;
    dolist(item, m_pMenuItems)
        if (item->within(pt.y))
        {
            item->Mouse(hwnd, uMsg, wParam, lParam);
            break;
        }
}

//==============================================
// simulate mouse movement in drag operation

const _ITEMIDLIST *Menu::dragover(POINT* ppt)
{
    POINT pt = *ppt;
    ScreenToClient(m_hwnd, &pt);
    mouse_over(true);
    MenuItem* Item; dolist(Item, m_pMenuItems)
        if (Item->within(pt.y))
        {
            Item->Active(1);
            if (Item->m_ItemID == MENUITEM_ID_SF)
                return ((SpecialFolderItem*)Item)->check_pidl();
            return Item->m_pidl;
        }
    return NULL;
}

void Menu::start_drag(const char *arg, const _ITEMIDLIST *c_pidl)
{
    _ITEMIDLIST *pidl = NULL;
    if (false == m_bInDrag)
    {
        if (c_pidl) pidl = duplicateIDlist(c_pidl);
        else
        if (arg) pidl = sh_getpidl(NULL, arg);
        if (pidl)
        {
            incref();
            m_bInDrag = true;
            drag_pidl(pidl);
            m_bInDrag = false;
            decref();
            m_free(pidl);
        }
    }
}

//===========================================================================
// Menu Keystroke handling
//===========================================================================

void Menu::hilite(MenuItem *Item)
{
    HideChild();
    Item->Active(2);
    int a = get_active_index();
    m_keyboard_item_pos = a;
    if ((a -= m_topindex) < 0 || (a -= m_pagesize-1) > 0)
        scroll_menu(a);
}

//--------------------------------------
void Menu::activate_by_key(MenuItem *Item)
{
    hilite(Item);
    if (Item->m_pSubMenu && Item->m_pSubMenu != m_pChild)
    {
        if (Item->m_pSubMenu->m_MenuID & (MENU_ID_INT | MENU_ID_STRING))
            Item->ShowSubMenu();
    }
}

//===========================================================================
MenuItem * Menu::match_next_shortcut(const char *d)
{
	MenuItem *Item;
	const char *s, *t, *e;
	unsigned n = 2, dl = (int)strlen(d);
	for (Item = m_pActiveItem; ; Item = m_pMenuItems)
	{
		if (Item) while (NULL != (Item = Item->next))
		{
			s = strchr(t = Item->m_pszTitle, '&');
			e = s && s[1] != s[0] ? &s[1] : &t[0];
			if (strlen(e) >= dl
			 && 2 == CompareString(
					LOCALE_USER_DEFAULT,
					NORM_IGNORECASE|NORM_IGNOREWIDTH|NORM_IGNOREKANATYPE,
					d, dl, e, dl))
				break;
		}
		if (Item || 0 == --n) return Item;
	}
}

//--------------------------------------
void Menu::set_keyboard_active_item(void)
{
	MenuItem *Item;
	int a = m_keyboard_item_pos;
	if (a < 0) a = m_topindex;
	Item = nth_item(a);
	if (Item) hilite(Item);
}

//--------------------------------------
void Menu_Tab_Next(Menu *p)
{
	//bool backwards = 0x8000 & GetAsyncKeyState(VK_SHIFT);
	Menu *m; MenuList *ml;
	dolist (ml, Menu::g_MenuWindowList)
		if (NULL == (m=ml->m)->m_pParent && p != m)
		{
			m->bring_menu_ontop();
			Menu::Hide_All_But(m);
			m->set_keyboard_active_item();
			break;
		}
}

//--------------------------------------
bool Menu::Handle_Key(UINT msg, UINT wParam)
{
	int n, a, c, d, e, i;
	MenuItem *Item; bool ctrl; const int c_steps = 1;

	//--------------------------------------
	if (MENU_ID_INT & m_MenuID)
	{
		m_pMenuItems->next->Key(msg, wParam);
	}
	else
	if (MENU_ID_STRING & m_MenuID)
	{
	}
	else
	if (WM_CHAR == msg)
	{
		char chars[80]; MSG msg; i = 0;
		chars[i++] = wParam;
		while(PeekMessage(&msg, m_hwnd, WM_CHAR, WM_CHAR, PM_REMOVE) && (unsigned)i < sizeof chars - 1)
			chars[i++] = msg.wParam;
		chars[i] = 0;

		//dbg_printf("chars <%s>", chars);

		// Shortcuts
		Item = match_next_shortcut(chars);
		if (Item)
		{
			hilite(Item);
			if (Item == match_next_shortcut(chars))
			{
				// popup folder if only one choice
				if (MENUITEM_ID_FOLDER & Item->m_ItemID)
					goto k_right;
			}
		}
	}
	else
	if (WM_KEYDOWN == msg)
	{
		a = get_active_index();
		e = m_itemcount - 1;
		d = m_pagesize - 1;
		n = 0;
		ctrl = 0x8000 & GetKeyState(VK_CONTROL);
one_more:
		i = 1;
		switch(wParam)
		{
			set:
				Item = a < 0 ? NULL : nth_item(a);
				if (NULL == Item)
					break;

				if (Item->m_isNOP & (MI_NOP_TEXT | MI_NOP_SEP))
				{
					if (n++ < e) goto one_more;
					m_keyboard_item_pos = -2;
					break;
				}

				activate_by_key(Item);
				break;

			//--------------------------------------
			case VK_DOWN:
				if (ctrl)
				{
					i = imin(c_steps, e - d - m_topindex);
					if (i >= 0)
						scroll_menu(i);
					else
						break;
				}
				if (a < 0) a = m_topindex;
				else
				if (a >= e) a = (d < e) ? e : 0;
				else a = imin(a+i, e);
				goto set;

			//--------------------------------------
			case VK_UP:
				if (ctrl)
				{
					i = imin(c_steps, m_topindex);
					if (i >= 0) scroll_menu(-i);
					else break;
				}
				if (a < 0) a = e;
				else
				if (0 == a) a = (d < e) ? 0 : e;
				else a = imax(a-i, 0);
				goto set;

			//--------------------------------------
			case VK_NEXT:
				if (a < 0) a = m_topindex;
				c = imin (e - d - m_topindex, d);
				if (c <= 0) a = e;
				else a += c;
				scroll_menu(c);
				wParam = VK_DOWN;
				goto set;

			//--------------------------------------
			case VK_PRIOR:
				if (a < 0) a = m_topindex;
				c = imin(m_topindex, d);
				if (c <= 0) a = 0;
				else a -= c;
				scroll_menu(-c);
				wParam = VK_UP;
				goto set;

			//--------------------------------------
			case VK_HOME:
				a = 0;
				goto set;

			//--------------------------------------
			case VK_END:
				a = e;
				goto set;

			//--------------------------------------
			case VK_RIGHT:
				if (FolderItem::m_nBulletPosition == DT_LEFT) goto k_left;
			k_right:
				if (m_pActiveItem)
				{
					m_pActiveItem->ShowSubMenu();
				}
			hilite_child:
				if (m_pChild)
				{
					m_pChild->set_focus();
					Item = m_pChild->m_pMenuItems->next;
					if (Item) m_pChild->activate_by_key(Item);
				}
				break;

			//--------------------------------------
			case VK_LEFT:
				if (FolderItem::m_nBulletPosition == DT_LEFT) goto k_right;
			k_left:
				if (m_pParent)
					m_pParent->hilite(m_pParentItem);
				break;

			//--------------------------------------
			case VK_RETURN: // invoke command
			case VK_SPACE:
				if (m_pActiveItem)
				{
					m_pActiveItem->Invoke(INVOKE_LEFT);
					goto hilite_child;
				}
				break;

			//--------------------------------------
			case VK_APPS: // context menu
				if (m_pActiveItem)
				{
					m_pActiveItem->Invoke(INVOKE_RIGHT);
					goto hilite_child;
				}
				break;

			//--------------------------------------
			case VK_F5: // update folder
				PostMessage(m_hwnd, BB_FOLDERCHANGED, 0, 0);
				break;

			//--------------------------------------
			case VK_TAB: // cycle through menues
				Menu_Tab_Next(this);
				break;

			//--------------------------------------
			case VK_INSERT:
				if (false == IsPinned())
					SetPinned(true);
				else
					m_bOnTop = false == m_bOnTop;

				SetZPos();
				break;

			//--------------------------------------
			case VK_ESCAPE:
				Menu_All_Hide();
				focus_top_window();
				break;

			case VK_DELETE:
				HideThis();
				break;

			//--------------------------------------
			default:
				return false;
		}
	}
	else
	if (WM_SYSKEYDOWN == msg)
	{
		int d = 12;
		//if (0x8000 & GetAsyncKeyState(VK_CONTROL)) d = 1;
		int x = m_xpos;
		int y = m_ypos;
		switch(wParam)
		{
			case VK_LEFT:  x -= d; break;
			case VK_RIGHT: x += d; break;
			case VK_UP:    y -= d; break;
			case VK_DOWN:  y += d; break;
			default: return false;
        }
        Show(x, y, MENUSHOW_LEFT|MENUSHOW_UPDATE);
        write_menu_pos();
        if (false == IsPinned())
            SetPinned(true), SetZPos();
    }
    else
    {
        return false;
    }
    Menu::g_DiscardMouseMoves = 5; // discard next 5 mouse messages
    return true;
}

//===========================================================================
// Window proc for the popup menues

LRESULT CALLBACK Menu::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    Menu *p = (Menu*)GetWindowLongPtr(hwnd, 0);

    if (NULL == p)
    {
        if (WM_NCCREATE == uMsg)
            // bind window to the c++ structure
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    //dbg_printf("hwnd %x  msg %x  wp %08x  lp %08x", hwnd, uMsg, wParam, lParam);

    p->incref();
    switch(uMsg)
    {
        //====================
        case WM_DESTROY:
            break;

        //====================
        case WM_KILLFOCUS:
            p->m_bHasFocus = false;
            if (NULL == p->m_pChild) p->UnHilite();

#ifdef CHECKFOCUS
            dbg_printf("WM_KILLFOCUS %s", p->m_pMenuItems->m_pszTitle);
            InvalidateRect(hwnd, NULL, FALSE);
#endif
            break;

        //====================
        case WM_SETFOCUS:
            p->m_bHasFocus = true;
            g_MouseWheelAccu = 0;

#ifdef CHECKFOCUS
            dbg_printf("WM_SETFOCUS %s", p->m_pMenuItems->m_pszTitle);
            InvalidateRect(hwnd, NULL, FALSE);
#endif
            break;

        //====================
        case WM_ACTIVATEAPP:
            if (false == wParam)
                freeall(&g_MenuFocusList);
            break;

        case WM_ACTIVATE:
            if (WA_INACTIVE != LOWORD(wParam))
            {
                p->insert_at_last();
                p->cons_focus();
            }
            break;

        //====================
        case BB_HIDEMENU:
            p->Hide();
            break;

        case WM_CLOSE:
            p->HideThis();
            break;

        case BB_REDRAWGUI:
            if (wParam & BBRG_FOLDER) p->UpdateFolder();
            p->redraw_structure();
            break;

        case BB_FOLDERCHANGED:
            PostMessage(hwnd, BB_REDRAWGUI, BBRG_FOLDER, 0);
            break;

        case BB_DRAGOVER:
            result = (LRESULT)p->dragover((POINT *)lParam);
            break;

        //====================
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            if (false == p->Handle_Key(uMsg, wParam))
                goto _default;
            break;

        //====================
        case WM_MOUSEWHEEL:
        {
            int i = g_MouseWheelAccu + Settings_menuMousewheelfac * 5 * (short)HIWORD(wParam);
            int n = 0;
            while (i < -300) n++, i+=600;
            while (i >  300) n--, i-=600;
            g_MouseWheelAccu=i;
            p->scroll_menu(n);

            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ScreenToClient(hwnd, &pt);
            PostMessage(hwnd, WM_MOUSEMOVE, LOWORD(wParam), MAKELPARAM(pt.x, pt.y));
            break;
        }

        //====================
        case WM_MOUSELEAVE:
            //dbg_printf("mouseleave %x", hwnd);
            if (p->m_bMouseOver)
            {
                p->m_bMouseOver = false;
                p->mouse_leave();
            }
            break;

        //====================
        case WM_MOUSEMOVE:
            if (g_DiscardMouseMoves)
                --Menu::g_DiscardMouseMoves;
            else
                p->Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            p->m_dblClicked = true;
            p->Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            p->m_dblClicked = false;
            p->Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            p->Handle_Mouse(hwnd, uMsg, wParam, lParam);
            ReleaseCapture();
            p->m_captureflg = 0;
            break;

        //====================
        case WM_TIMER:
            if (MENU_TRACKMOUSE_TIMER == wParam)
            {
                if (hwnd != window_under_mouse())
                {
                    KillTimer(hwnd, wParam);
                    PostMessage(hwnd, WM_MOUSELEAVE, 0, 0);
                }
                break;
            }
            //dbg_printf("Timer %x %d", hwnd, wParam);
            p->MenuTimer(wParam);
            break;

        //====================
        case WM_WINDOWPOSCHANGING:
            if (p->m_bMoving && Settings_menusSnapWindow)
            {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                SnapWindowToEdge(wp, 0, SNAP_NOPLUGINS|SNAP_FULLSCREEN);
            }
            break;

        case WM_WINDOWPOSCHANGED:
            p->m_xpos = ((LPWINDOWPOS)lParam)->x;
            p->m_ypos = ((LPWINDOWPOS)lParam)->y;
            if (p->m_pChild)
            {
                if (p->menu_root()->m_bMoving)
                    p->m_pChild->Show(0, 0, MENUSHOW_UPDATE);

                // put behind child again
                p->menu_set_pos(NULL, SWP_NOMOVE|SWP_NOSIZE);
            }
            break;

        //====================
        case WM_ENTERSIZEMOVE:
            p->m_bMoving = true;
            break;

        //====================
        case WM_EXITSIZEMOVE:
            p->m_bMoving = false;
            p->write_menu_pos();
            break;

        //====================
        case WM_PAINT:
            p->Paint();
            break;

        //====================
        case WM_ERASEBKGND:
            result = TRUE;
            break;

        //===============================================
        // String Item - parent window notifications
        case WM_COMMAND:
            if (1234 == LOWORD(wParam))
                if (EN_UPDATE == HIWORD(wParam)
                ||  EN_HSCROLL == HIWORD(wParam))
                {
                    InvalidateRect((HWND)lParam, NULL, FALSE);
                }
            break;

        case WM_CTLCOLOREDIT:
            SetTextColor((HDC)wParam, mStyle.MenuFrame.TextColor);
            SetBkMode((HDC)wParam, TRANSPARENT);
            result = (LRESULT)GetStockObject(NULL_BRUSH);
            break;

        case WM_CAPTURECHANGED:
            if (p->m_MenuID & MENU_ID_STRING)
                InvalidateRect(hwnd, NULL, FALSE);

            p->m_captureflg = 0;
            break;

        //===============================================

        default:
        _default:
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;

        //====================
    }
    p->decref();
    return result;
}

//===========================================================================

// Set a left click command for a folderitem, used with the workspace menu
void SetFolderItemCommand(MenuItem *Item, const char *buf)
{
    if (Item) replace_str(&((FolderItem *)Item)->m_pszCommand, buf);
}

// Set checkmark, used with the workspace menu also
void CheckMenuItem(MenuItem *Item, bool checked)
{
    if (Item) Item->m_isChecked = checked;
}

MenuItem *get_real_item(MenuItem *pItem)
{
    if (pItem && pItem->m_pSubMenu && (pItem->m_pSubMenu->m_MenuID & (MENU_ID_STRING | MENU_ID_INT)))
        return pItem->m_pSubMenu->m_pMenuItems->next;
    return NULL;
}

// Set the 'off' value and string for integer items
void MenuItemInt_SetOffValue(class MenuItem *Item, int n, const char *p)
{
    Item = get_real_item(Item);
    if (Item)
    {
        ((IntegerItem*)Item)->offvalue = n;
        ((IntegerItem*)Item)->offstring = p ? p : NLS0("off");
    }
}

//===========================================================================

//===========================================================================
// update and relink all visible menus within an updated menu structure

void Menu::redraw_structure(void)
{
    // show something at least, maybe
    //if (NULL == m_pMenuItems->next) MakeMenuNOP(this, "---");

    MenuItem *ActiveItem = NULL;
    if (m_hwnd)
    {
        redraw();
        ActiveItem = nth_item(m_active_item_pos);
        m_active_item_pos = -2;
    }

    MenuItem *Item;
    dolist (Item, m_pMenuItems)
    {
        Menu *sub;
        if (NULL != (sub = Item->m_pSubMenu))
        {
            if (sub == m_pLastChild)
            {
                sub->LinkToParentItem(Item);
                m_pLastChild = NULL;
                ActiveItem = Item;
            }
            sub->redraw_structure();
        }
    }

    if (m_pLastChild)
    {
        // Lost child - the menu was updated such that the
        // child's parentitem doesn't exist anymore.

        if (m_pLastChild->has_focus_in_chain()) set_focus();
        m_pLastChild->Hide();
        m_pLastChild = NULL;
    }

	if (ActiveItem) ActiveItem->Active(2);
}

//===========================================================================
// The menu is invoked, either with the mouse or with a keystroke

void Menu_ShowFirst(Menu *p, bool from_kbd, bool with_xy, int x, int y)
{
	p->m_bIconized = false;
	p->redraw_structure();
	if (NULL == p->m_hwnd)
	{
		p->Validate();
	}
	else
	{
		p->HideChild();
		p->UnHilite();
		p->Detach();
	}

	if (with_xy)
	{
		int menushow_pos = 0;
		Menu::g_DiscardMouseMoves = 5;
		p->m_bKBDInvoked = false;
		if (x < 0) x += GetSystemMetrics(SM_CXSCREEN) + 1, menushow_pos |= MENUSHOW_RIGHT;
		if (y < 0) y += GetSystemMetrics(SM_CYSCREEN) + 1, menushow_pos |= MENUSHOW_BOTTOM;
		p->Show(x, y, menushow_pos);
	}
	else
	if (from_kbd)
    {
        // some mouse-moves are ignored just in case the mouse is over the menu
        Menu::g_DiscardMouseMoves = 5;
        p->m_bKBDInvoked = true;
        p->Show(Settings_menuPositionX, Settings_menuPositionY, MENUSHOW_LEFT);
    }
    else
    {
        p->SetPinned(false);
        Menu::g_DiscardMouseMoves = 2;
        p->m_bKBDInvoked = false;
        POINT pt; GetCursorPos(&pt);
        p->Show(pt.x, pt.y, MENUSHOW_CENTER);
    }

    p->decref();
    ForceForegroundWindow(p->m_hwnd);   // force focus, if it was invoked by a keystroke
    p->set_focus();
    Menu::Hide_All_But(p);              // hide all other menus but this one
}

//===========================================================================

//===========================================================================
//
//                          PluginMenu API
//
//===========================================================================
// API: MakeNamedMenu
//===========================================================================

Menu *MakeNamedMenu(LPCSTR HeaderText, LPCSTR IDString, bool popup)
{
    Menu *pMenu = NULL;
    if (IDString) pMenu = Menu::FindNamedMenu(IDString);

    if (NULL == pMenu)
    {
        pMenu = new Menu(NLS1(HeaderText));
        pMenu->m_IDString = new_str(IDString);
    }
    else
    {
        pMenu->incref();
        pMenu->m_pLastChild = pMenu->m_pChild;
        pMenu->m_active_item_pos = pMenu->get_active_index();
        pMenu->DeleteMenuItems();
        if (HeaderText)
            replace_str(&pMenu->m_pMenuItems->m_pszTitle, NLS1(HeaderText));
    }
    pMenu->m_bPopup = popup;
    //dbg_printf("MakeNamedMenu %x %s <%s>", pMenu, HeaderText, IDString);
    return pMenu;
}

//===========================================================================
// API: MakeMenu
//===========================================================================

Menu *MakeMenu(LPCSTR HeaderText)
{
    return MakeNamedMenu(HeaderText, NULL, true);
}

//===========================================================================
// API: ShowMenu
//===========================================================================

void ShowMenu(Menu *PluginMenu)
{
	//dbg_printf("ShowMenu %x %s", PluginMenu, PluginMenu->m_pMenuItems->m_pszTitle);
	if (PluginMenu->m_bPopup) {
		SendMessage(BBhwnd, BB_MENU, BB_MENU_PLUGIN, (LPARAM)PluginMenu);
	} else {
		PluginMenu->redraw_structure();
		PluginMenu->decref();
	}
}

//===========================================================================
// API: DelMenu
//===========================================================================

void DelMenu(Menu *PluginMenu) { } // not required anymore.

//===========================================================================
// API: MakeSubmenu
//===========================================================================

MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCSTR Title)
{
    //dbg_printf("MakeSubmenu %x %s - %x %s", ParentMenu, ParentMenu->m_pMenuItems->m_pszTitle, ChildMenu, Title);
	return ParentMenu->AddMenuItem(new FolderItem(ChildMenu, NLS1(Title)));
}

//===========================================================================
// API: MakeSubmenu with Icon
//===========================================================================
MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, LPCSTR Title, LPCSTR Icon)
{
	char *icon = (char *)strchr(Icon, '|');
	if (icon) {
		*icon++ = 0;
		ChildMenu->m_iconSize = iminmax(atoi(Icon), -2, 32);
	} else {
		icon = (char *)Icon;
	}

	MenuItem *mi = new FolderItem(ChildMenu, NLS1(Title));
	if (icon && *icon) {
		mi->m_iconMode = IM_PATH;
		mi->m_im_stuff = (void*)new_str(icon);
	}
	return ParentMenu->AddMenuItem(mi);
}

//===========================================================================
// API: MakeMenuItem
//===========================================================================

MenuItem *MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator)
{
    //dbg_printf("MakeMenuItem %x %s", PluginMenu, Title);
	return PluginMenu->AddMenuItem(new CommandItem(Cmd, NLS1(Title), ShowIndicator));
}

//===========================================================================
// API: MakeMenuItem with Icon
//===========================================================================
MenuItem *MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator, LPCSTR Icon)
{
	MenuItem *mi = new CommandItem(Cmd, NLS1(Title), ShowIndicator);
	if (Icon && *Icon)
	{
		if ('*' == *Icon)
		{
			if (0 == strnicmp("@BBCore.exec", Cmd, 12) || 0 == strnicmp("@BBCore.edit", Cmd, 12))
			{
				char path[MAX_PATH], bbpath[MAX_PATH];
				replace_shellfolders(path, Cmd + 13, true);
				GetBlackboxPath(bbpath, MAX_PATH);
				if (strcmp(bbpath, path))
				{
					mi->m_iconMode = IM_PIDL;
					mi->m_pidl = get_folder_pidl(path);
				}
			}
		}
		else
		{
			mi->m_iconMode = IM_PATH;
			mi->m_im_stuff = (void*)new_str(Icon);
		}
	}
	return PluginMenu->AddMenuItem(mi);
}

MenuItem *MakeMenuItem(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, bool ShowIndicator, char iconMode, void *im_stuff)
{
	MenuItem *mi = new CommandItem(Cmd, NLS1(Title), ShowIndicator);
	mi->m_iconMode = iconMode;
	mi->m_im_stuff = im_stuff;
	return PluginMenu->AddMenuItem(mi);
}

//===========================================================================
// Create the one item submenu for MakeMenuItemInt and MakeMenuItemString

MenuItem * make_helper_menu(Menu *PluginMenu, LPCSTR Title, int menuID, MenuItem *Item)
{
    Menu *sub;
    if (PluginMenu->m_IDString)
    {
        char buff[256];
        sprintf (buff, "%s.%s", PluginMenu->m_IDString, Title);
        sub = MakeNamedMenu(Title, buff, PluginMenu->m_bPopup);
    }
    else
    {
        sub = MakeMenu(Title);
    }
    sub->AddMenuItem(Item);
    sub->m_MenuID = menuID;
	//Grip: Add the grip to all configurative options in the menu
	MakeMenuGrip(sub, Title);
	return MakeSubmenu(PluginMenu, sub, Title);
}

//===========================================================================
// API: MakeMenuItemInt
//===========================================================================

MenuItem *MakeMenuItemInt(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, int val, int minval, int maxval)
{
    return make_helper_menu(PluginMenu, Title, MENU_ID_INT,
        new IntegerItem(Cmd, val, minval, maxval));
}

//===========================================================================
// API: MakeMenuItemString
//===========================================================================

MenuItem *MakeMenuItemString(Menu *PluginMenu, LPCSTR Title, LPCSTR Cmd, LPCSTR init_string)
{
    return make_helper_menu(PluginMenu, Title, MENU_ID_STRING,
        new StringItem(Cmd, init_string));
}

//===========================================================================
// API: MakeMenuNOP
//===========================================================================

MenuItem* MakeMenuNOP(Menu *PluginMenu, LPCSTR Title)
{
    MenuItem *pItem = PluginMenu->AddMenuItem(new MenuItem(Title ? NLS1(Title) : ""));
    pItem->m_isNOP = MI_NOP_TEXT;
    return pItem;
}

//===========================================================================
// API: MakeMenuVOL
//===========================================================================

MenuItem* MakeMenuVOL(Menu *PluginMenu, LPCSTR Title, LPCSTR DllName, LPCSTR Icon)
{
    return PluginMenu->AddMenuItem(new VolumeItem(Title, DllName, Icon));
}

//===========================================================================
// API: MakeMenuGrip
//===========================================================================

MenuItem* MakeMenuGrip(Menu *PluginMenu, LPCSTR Title)
{
	if ( Settings_menusGripEnabled ) {
		return PluginMenu->AddMenuItem(new MenuGrip(Title));
	}
}

//===========================================================================
// API: MakePathMenu
//===========================================================================

MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd)
{
	return ParentMenu->AddMenuItem(new SpecialFolderItem(NLS1(Title), path, NULL, Cmd));
}

MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd, LPCSTR Icon)
{
	MenuItem *mi = new SpecialFolderItem(NLS1(Title), path, NULL, Cmd);
	char *icon = (char *)strchr(Icon, '|');
	if (icon)
	{
		*icon++ = 0;
		((SpecialFolderItem*)mi)->m_iconSize = iminmax(atoi(Icon), -2, 32);
	}
	else
		icon = (char *)Icon;
	
	if (icon && *icon)
	{
		if ('*' == *icon)
		{
			mi->m_iconMode = IM_PIDL;

			// Conserve first path
			char *path_end = (char *)path, c = *path_end;
			while(c && c != '|') c = *(++path_end);
			*path_end = 0;

			mi->m_pidl = get_folder_pidl(path);

			// Restore other path
			*path_end = c;
		}
		else
		{
			mi->m_iconMode = IM_PATH;
			mi->m_im_stuff = (void*)new_str(icon);
		}
	}
	return ParentMenu->AddMenuItem(mi);
}


//===========================================================================
// API: MakePathMenu with Icon
//===========================================================================
/*
MenuItem* MakePathMenu(Menu *ParentMenu, LPCSTR Title, LPCSTR path, LPCSTR Cmd, LPCSTR Icon)
{
    return ParentMenu->AddMenuItem(new SpecialFolderItem(NLS1(Title), path, NULL, Cmd, Icon));
}
*/
//===========================================================================
// API: MenuExists
//===========================================================================

bool MenuExists(const char* IDString_part)
{
    int l = strlen(IDString_part);
    MenuList *ml;
    dolist (ml, Menu::g_MenuStructList)
        if (ml->m->m_IDString && 0 == memcmp(ml->m->m_IDString, IDString_part, l))
        {
            return true;
        }
    return false;
}

//===========================================================================
// API: DisableLastItem - set 'menu.frame.disabledColor' for item
//===========================================================================

void DisableLastItem(Menu *PluginMenu)
{
    if (PluginMenu)
    {
        MenuItem *I1 = NULL;
        // dirty hack: calculate the Item* from &(*Item).next;
        I1 = (MenuItem*)((char*)PluginMenu->m_ppAddItem - ((char*)&I1->next - (char*)I1));
        I1->m_isNOP = MI_NOP_DISABLED;
        MenuItem *I2 = get_real_item(I1);
        if (I2) I2->m_isNOP = I1->m_isNOP;
    }
}

//===========================================================================

