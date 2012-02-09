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

#include "../BB.h"
#include "../Settings.h"
#include "../Workspaces.h"
#include "Menu.h"

// #define CHECKFOCUS

struct MenuInfo MenuInfo;
const char MenuClassName[] = "BBMenu";
static int g_menu_ref;
int g_menu_count;
int g_menu_item_count;
// When the menu is invoked with a keystroke, some mouse moves are
// discarded to avoid unwanted submenu popups.
static int g_ignore_mouse_msgs;

MenuList *Menu::g_MenuStructList;   // all instances of 'class Menu'
MenuList *Menu::g_MenuWindowList;   // all actually visible menus
MenuList *Menu::g_MenuRefList;      // menus for deletion

//===========================================================================
// The menu window class

void register_menuclass(void)
{
    int style = BBCS_VISIBLE|BBCS_EXTRA;
    if (Settings_menu.dropShadows)
        style |= BBCS_DROPSHADOW;
    BBRegisterClass(MenuClassName, Menu::WindowProc, style);
}

void un_register_menuclass(void)
{
    UnregisterClass(MenuClassName, hMainInstance);
}

//===========================================================================
// Menu constructor

Menu::Menu(const char *pszTitle)
{
    // clear everything (except vtable* :-P)
    memset(&m_refc, 0, sizeof *this - sizeof(void*));

    m_bOnTop = Settings_menu.onTop;
    m_maxwidth = MenuInfo.MaxWidth;
	m_minwidth = MenuInfo.MinWidth; /* BlackboxZero 12.17.2011 */
    m_bPopup = true;

    // Add TitleItem. This will remain until the menu is
    // destroyed, while other items may change on updating.
    m_pMenuItems = m_pLastItem = new TitleItem(pszTitle);
    m_pLastItem -> m_pMenu = this;

    // add to the global menu list. It is used to see
    // if a specific menu exists (with 'find_named_menu')
    cons_node(&g_MenuStructList, new_node(this));
    ++g_menu_count;

    // start with a refcount of 1, assuming that
    // the menu is either shown or linked as submenu.
    m_refc = 1;
}

//==============================================
// Menu destructor

Menu::~Menu()
{
    DeleteMenuItems();
    delete m_pMenuItems; // TitleItem
    free_str(&m_IDString);
    delete_pidl_list(&m_pidl_list);

    // remove from the list
    remove_assoc(&g_MenuStructList, this);
    --g_menu_count;
}

//==============================================
// Menu destruction is controlled by reference counter.
// References are:
// - having a Window
// - being a submenu, i.e. bound to m_pSubmenu of an item

int Menu::decref(void)
{
    //dbg_printf("decref %d %s", m_refc-1, m_pMenuItems->m_pszTitle);
    int n = m_refc - 1;
    if (n)
        return m_refc = n;
    if (g_menu_ref) {
        // delay deletion
        cons_node(&g_MenuRefList, new_node(this));
        return n+1;
    }
    delete this;
    return n;
}

int Menu::incref(void)
{
    //dbg_printf("incref %d %s", m_refc, m_pMenuItems->m_pszTitle);
    return ++m_refc;
}

//==============================================
// Global reference count - controls access of ANY menu through either:
// - the menu window_proc or
// - one of the global menu functions, i.e. Menu_All_Hide()

void Menu::g_incref()
{
    ++g_menu_ref;
}

void Menu::g_decref()
{
    MenuList *ml;
    if (--g_menu_ref || NULL == g_MenuRefList)
        return;
    // dbg_printf("MenuDelList %d", listlen(g_MenuRefList));
    dolist (ml, g_MenuRefList)
        ml->m->decref();
    freeall(&g_MenuRefList);
}

//===========================================================================
// Give a window to the menu, when it's about to be shown

void Menu::make_menu_window(void)
{
    if (m_hwnd)
        return;

    m_bMouseOver = false;
    m_kbditempos = -1;
    m_alpha = 255;
    incref();

    m_hwnd = CreateWindowEx(
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

    register_droptarget(true);
    insert_at_last();
}

//===========================================================================
// Remove the window, when the menu is hidden

void Menu::destroy_menu_window(bool force)
{
    HWND hwnd = m_hwnd;
    if (NULL == hwnd)
        return;

    post_autohide();

    if (m_pChild)
        m_pChild->LinkToParentItem(NULL);

    if (m_pParent) {
        if (m_MenuID & (MENU_ID_SF|MENU_ID_SHCONTEXT|MENU_ID_RMENU)
            && 0 == (m_pParent->m_MenuID & MENU_ID_SHCONTEXT)) {
            // unlink folders and context menus since these can
            // be remade any time
            m_pParentItem->UnlinkSubmenu();
        } else {
            LinkToParentItem(NULL);
        }
    }

    // the OS does not seem to like that a window is destroyed while
    // it is dragsource (may drop just somewhere) or droptarget
    // (wont be released)
    if (false == force && (m_bInDrag || in_drop(m_droptarget)))
        return;

    register_droptarget(false);
    remove_assoc(&g_MenuWindowList, this);
    m_hwnd = m_hwndRef = NULL;
    DestroyWindow(hwnd);
    if (m_hBitMap)
        DeleteObject(m_hBitMap), m_hBitMap = NULL;
    decref();
}

//===========================================================================
void Menu::post_autohide()
{
    if (m_flags & BBMENU_HWND) {
        PostMessage(m_hwndRef, BB_AUTOHIDE, 0, 1);
        m_flags &= ~BBMENU_HWND;
    }
}

//===========================================================================
LRESULT CALLBACK Menu::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Menu *p = (Menu*)GetWindowLongPtr(hwnd, 0);
    if (p)
    {
        LRESULT result;
        p->incref(), g_incref();
        result = p->wnd_proc(hwnd, uMsg, wParam, lParam);
        g_decref(), p->decref();
        return result;
    }

    if (WM_NCCREATE == uMsg)
    {
        // bind window to the c++ structure
        SetWindowLongPtr(hwnd, 0,
            (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//===========================================================================
void Menu::register_droptarget(bool set)
{
    if (set) {
        if (m_bIsDropTarg) {
            LPCITEMIDLIST pidl = m_pidl_list ? first_pidl(m_pidl_list) : NULL;
            m_droptarget = init_drop_target(m_hwnd, pidl);
            if (pidl)
                m_notify = add_change_notify_entry(m_hwnd, pidl);
        }

    } else {
        if (m_notify)
            remove_change_notify_entry(m_notify), m_notify = 0;
        if (m_droptarget)
            exit_drop_target(m_droptarget), m_droptarget = NULL;
    }
}

//==============================================
// When a submenu becomes visible, the m_pParent(pItem)
// pointers are set, also the m_pChild pointer of the parent
// When it becomes invisible (pItem = NULL), they are reset.

void Menu::LinkToParentItem(MenuItem *pItem)
{
    if (m_pParentItem)
    {
        m_pParent->m_pChild = NULL;
        m_pParent = NULL;
    }
    m_pParentItem = pItem;
    if (m_pParentItem)
    {
        m_pParent = m_pParentItem->m_pMenu;
        m_pParent->m_pChild = this;
    }
}

//==============================================
// Append item to the list

MenuItem *Menu::AddMenuItem(MenuItem* pItem)
{
    // link to the list
    m_pLastItem = m_pLastItem->next = pItem;

    // set references
    pItem->m_pMenu = this;
    return pItem;
}

//==============================================
// clear all Items - except the TitleItem

void Menu::DeleteMenuItems()
{   
    MenuItem *pItem, *thisItem;

    pItem = (thisItem = m_pLastItem = m_pMenuItems)->next;
    thisItem->next = NULL;
    thisItem->m_bActive = false;
    m_pActiveItem = NULL;
    while (pItem)
        pItem=(thisItem=pItem)->next, delete thisItem;
}

//==============================================
// find a menu with IDString, i.e. for onscreen updating

Menu *Menu::find_named_menu(const char *IDString, bool fuzzy)
{
    int l = strlen(IDString);
    MenuList *ml;
    Menu *m;
    dolist (ml, g_MenuStructList)
        if ((m = ml->m)->m_IDString
            && 0 == memcmp(m->m_IDString, IDString, l)
            && (0 == m->m_IDString[l] || fuzzy))
            return ml->m;
    return NULL;
}

//===========================================================================
// some utilities

// When a menu get's the focus, it is inserted at the last position of the
// list, so that it is set to the top of the z-order with "Menu_All_BringOnTop()"
void Menu::insert_at_last (void)
{
    MenuList **mp, *ml, *mn = NULL;
    for (mp = &g_MenuWindowList; NULL != (ml = *mp); ) {
        if (this == ml->m)
            *mp = (mn = ml)->next;
        else
            mp = &ml->next;
    }
    if (NULL == mn)
        mn = (MenuList*)new_node(this);
    (*mp = mn)->next = NULL;
}

// get the list-index for item or -2, if not found
int Menu::get_item_index (MenuItem *item)
{
    int c = 0;
    MenuItem *pItem;
    if (item)
        dolist (pItem, m_pMenuItems->next) {
            if (item == pItem)
                return c;
            c++;
        }
    return -1;
}

// get the item from idex
MenuItem * Menu::nth_item(int a)
{
    int c = 0;
    MenuItem *pItem = NULL;
    if (a >= 0)
        dolist (pItem, m_pMenuItems->next) {
            if (c == a)
                break;
            c++;
        }
    return pItem;
}

// get the root in a menu chain
Menu *Menu::menu_root (void)
{
    Menu *p, *m = this;
    while (NULL != (p = m->m_pParent))
        m = p;
    return m;
}

bool Menu::has_focus_in_chain(void)
{
    Menu *p;
    for (p = menu_root(); p; p = p->m_pChild)
        if (p->m_bHasFocus)
            return true;
    return false;
}

bool Menu::has_hwnd_in_chain(HWND hwnd)
{
    Menu *p;
    for (p = menu_root(); p; p = p->m_pChild)
        if (p->m_hwnd == hwnd)
            return true;
    return false;
}

// Get the root of the last active menu
Menu *Menu::last_active_menu_root(void)
{
    MenuList *ml;
    dolist (ml, g_MenuWindowList)
        if (NULL == ml->next)
            return ml->m->menu_root();
    return NULL;
}

void Menu::set_capture(int flg)
{
    m_captureflg = flg;
    if (flg)
        SetCapture(m_hwnd);
    else
        ReleaseCapture();
}

void Menu::set_focus(void)
{
    if (m_bHasFocus)
        return;
    SetFocus(m_hwnd);
}

// --------------------------------------
// these are called from WM_SET/KILLFOCUS

void Menu::on_setfocus(HWND oldFocus)
{
    MenuItem *pItem;

    m_bHasFocus = true;
    insert_at_last();

    pItem = nth_item(m_kbditempos);
    if (pItem)
        pItem->Active(2);

    // if there is a submenu, possibly overlapping,
    // then place this behind it again
    if (m_pChild)
        menu_set_pos(NULL, SWP_NOMOVE|SWP_NOSIZE);
}

void Menu::on_killfocus(HWND newfocus)
{
    if (newfocus 
        && (newfocus == m_hwnd 
         || newfocus == m_hwndChild))
        return;
    m_bHasFocus = false;
    if (false == has_hwnd_in_chain(newfocus))
        menu_root()->Hide();
}

//===========================================================================
// Timer to do the submenu open and close delay.
void Menu::set_timer(bool active, bool set) /* BlackboxZero 1.3.2012 */
{
    int d;
    if (false == set) {
        KillTimer(m_hwnd, MENU_POPUP_TIMER);
        return;
    }
	if ( active ) { /* BlackboxZero 1.3.2012 */
		//Native Popup handling
		d = Settings_menu.popupDelay;
		if (false == m_bHasFocus && d < 100)
			d = 100;
		if (0 == d) {
			UpdateWindow(m_hwnd); // update hilite bar first
			PostMessage(m_hwnd, WM_TIMER, MENU_POPUP_TIMER, 0);
		} else {
			SetTimer(m_hwnd, MENU_POPUP_TIMER, d, NULL);
		}
	} else { /* BlackboxZero 1.3.2012 */
		//Do closeDelay
		d = Settings_menu.closeDelay;
		if (false == m_bHasFocus && d < 0)
			d = 1;
		if (0 == d) {
			UpdateWindow(m_hwnd); // update hilite bar first
			PostMessage(m_hwnd, WM_TIMER, MENU_POPUP_TIMER, 0);
		} else {
			SetTimer(m_hwnd, MENU_POPUP_TIMER, d, NULL);
		}
	}
}

//===========================================================================
// scroll helpers

// helper: get the scroll range in pixel (menuheight-title-scrollbutton)
int Menu::get_y_range(int *py0, int *ps)
{
    int y0, s, d, t;
    d = m_bNoTitle ? MenuInfo.nFrameMargin : MenuInfo.nScrollerSideOffset;
    t = m_bNoTitle ? 0 : MenuInfo.nScrollerTopOffset;
    *py0 = y0 = m_firstitem_top + t;
    *ps = s = MenuInfo.nScrollerSize;
    return m_height - y0 - s - d;
}

// get the scroll button square from scrollindex
void Menu::get_vscroller_rect(RECT* rw)
{
    int y0, s, d;
    d = m_bNoTitle ? MenuInfo.nFrameMargin : MenuInfo.nScrollerSideOffset;
    get_y_range(&y0, &s);

    rw->top = m_scrollpos + y0;
    rw->bottom = rw->top + s;
	//if ( MenuInfo.nBulletPosition == FOLDER_RIGHT )
	/* BlackboxZero 1.7.2012 */
    if ( (MenuInfo.nBulletPosition == FOLDER_RIGHT && MenuInfo.nScrollerPosition == FOLDER_DEFAULT) || (MenuInfo.nScrollerPosition == FOLDER_LEFT))
        rw->right = (rw->left = d) + s;
    else
        rw->left = (rw->right = m_width - d) - s;
}

// helper:  get the topitem-index from scrollindex
int Menu::calc_topindex (bool adjust)
{
    int y0, s, k, d, n;
    k = get_y_range(&y0, &s);
    d = m_itemcount - m_pagesize;
    if (d == 0) {
        m_scrollpos = n = 0;
    } else {
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
    RECT scroller1, scroller2;

    k = get_y_range(&y0, &s);
    i = iminmax(ymouse - s/2 - y0, 0, k);

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
    int c0, c1, c, y;
    MenuItem *pItem;

    new_top = imax(0, imin(new_top, m_itemcount - m_pagesize));
    if (m_hBitMap)
    {
        if (m_topindex == new_top)
            return;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }

    c0  = m_topindex = new_top;
    c1  = c0 + m_pagesize;
    c   = 0;
    y   = m_firstitem_top;
    pItem = m_pMenuItems;
    while (NULL != (pItem=pItem->next)) // skip TitleItem
    {
        if (c<c0 || c>=c1)
            pItem->m_nTop = -1000;
        else
            pItem->m_nTop = y, y += pItem->m_nHeight;
        c++;
    }
}

//===========================================================================

void Menu::ignore_mouse_msgs(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE));
    g_ignore_mouse_msgs = 2;
}

// Hide menu on item clicked
void Menu::hide_on_click(void)
{
    Menu *root = menu_root();
    ignore_mouse_msgs();
    if (false == root->m_bPinned)
        root->Hide();
    else
    if (m_MenuID & MENU_ID_SHCONTEXT)
        Hide();
    else
        HideChild();
}

// save menu fixed keyboard position
void Menu::write_menu_pos(void)
{
    if (this->m_kbdpos) {
        Settings_menu.pos.x = this->m_xpos;
        Settings_menu.pos.y = this->m_ypos;
        Settings_WriteRCSetting(&Settings_menu.pos.x);
        Settings_WriteRCSetting(&Settings_menu.pos.y);
    }
}

void Menu::menu_set_pos(HWND after, UINT flags)
{
    if (m_bOnTop || false == m_bPinned) {
        if (m_pChild) // keep it behind the child menu anyway
            after = m_pChild->m_hwnd;
        else if (m_bOnTop || bbactive || NULL == m_pParent)
            after = HWND_TOPMOST;
    }
    SetWindowPos(m_hwnd, after, m_xpos, m_ypos, m_width, m_height,
        flags|SWP_NOACTIVATE|SWP_NOSENDCHANGING);
}

void Menu::SetZPos(void)
{
    menu_set_pos(HWND_NOTOPMOST, SWP_NOMOVE|SWP_NOSIZE);
}


// set menu on top of the z-order
void Menu::bring_ontop(bool force_active)
{
    if (bbactive) {
        set_focus();
    } else if (force_active) {
        ForceForegroundWindow(m_hwnd);
    } else {
        menu_set_pos(HWND_TOP, SWP_NOMOVE|SWP_NOSIZE);
        insert_at_last();
    }
    HideChild();
    Menu_All_Hide_But(this);
}

//===========================================================================
// Paint the menu. The Background gradient bitmap is cached. Items are drawn
// only if they intersect with the update rectangle.

void Menu::Paint()
{
    PAINTSTRUCT ps;
    HDC hdc, hdc_screen, back;
    RECT r;
    HGDIOBJ S0, F0, B0;
    int y1,y2,c1,c2,c;
    int bw;
    MenuItem *pItem;
    StyleItem *pSI;

    hdc_screen = hdc = BeginPaint(m_hwnd, &ps);
    back = CreateCompatibleDC(hdc_screen);
    B0 = NULL;

    y1 = ps.rcPaint.top;
    y2 = ps.rcPaint.bottom;

    if (y1 == 0)
    {
        // If the entrire window must be drawn, we use a double buffer
        // to avoid flicker. Otherwise, for speed, we draw directly.
        hdc = CreateCompatibleDC(hdc_screen);
        B0 = SelectObject(hdc,
                CreateCompatibleBitmap(hdc_screen, m_width, m_height));
    }

    pItem = m_pMenuItems;

    if (NULL == m_hBitMap) // create the background
    {
        m_hBitMap = CreateCompatibleBitmap(hdc_screen, m_width, m_height);
        S0 = SelectObject(back, m_hBitMap);
        pSI = &mStyle.MenuFrame;

        bw = pSI->borderWidth;
        r.left = r.top = 0;
        r.right = m_width;
        r.bottom = m_height;

        if (bw)
            CreateBorder(back, &r, pSI->borderColor, bw);

        r.left = r.top = bw;
        r.right -= bw;
        r.bottom -= bw;

        if (false == m_bNoTitle
         && false == mStyle.MenuTitle.parentRelative
         && false == mStyle.menuTitleLabel)
            r.top = MenuInfo.nTitleHeight;

        if (r.bottom > r.top)
            MakeStyleGradient(back, &r, pSI, false);

        if (false == m_bNoTitle)
        {
            // Menu Title
            F0 = SelectObject(back, MenuInfo.hTitleFont);
            SetBkMode(back, TRANSPARENT);
            pItem->Paint(back);
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
    F0 = SelectObject(hdc, MenuInfo.hFrameFont);

    c = -1, c1 = m_topindex, c2 = c1 + m_pagesize;

    // skip items scrolled out on top
    while (c < c1 && pItem)
        pItem = pItem->next, ++c;

    while (c < c2 && pItem)
    {
        int y = pItem->m_nTop;
        if (y >= y2)
            break;
        if (y + pItem->m_nHeight > y1)
            pItem->Paint(hdc);
        pItem = pItem->next, ++c;
    }
    SelectObject(hdc, F0);

	/* BlackboxZero 1.7.2012 - Check for disabled
	** Only draw if not disabled.. */
    if (m_pagesize < m_itemcount && MenuInfo.nScrollerPosition != FOLDER_DISABLED && false == m_bIconized)
    {
        // draw the scroll button
        get_vscroller_rect(&r);
		HDC buf = CreateCompatibleDC(hdc_screen);
		
		if (NULL == m_hBmpScroll)
		{
			RECT rs;
			rs.left = 0; rs.right  = r.right - r.left;
			rs.top  = 0; rs.bottom = r.bottom - r.top;
			m_hBmpScroll = CreateCompatibleBitmap(hdc_screen, rs.right, rs.bottom);
			S0 = SelectObject(buf, m_hBmpScroll);
			pSI = &MenuInfo.Scroller;
			MakeStyleGradient(buf, &rs, pSI, pSI->bordered);
		}
		else
			S0 = SelectObject(buf, m_hBmpScroll);

		int hue = eightScale_up(Settings_menu.scrollHue);
		if (hue) {
			int x,y;
			for (x = r.left; x < r.right; ++x)
				for (y = r.top; y < r.bottom; ++y)
					SetPixel(hdc, x, y, mixcolors(
						GetPixel(hdc, x, y),
						GetPixel(buf, x - r.left, y - r.top),
						hue
					));
		} else
			BitBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, buf, 0, 0, SRCCOPY);

		SelectObject(buf, S0);
		DeleteDC(buf);
    }

    if (hdc != hdc_screen) // if using double buffer
    {
        // put buffer on screen
        BitBltRect(hdc_screen, hdc, &ps.rcPaint);
        DeleteObject(SelectObject(hdc, B0));
        DeleteDC(hdc);
    }

#ifdef CHECKFOCUS
    {
        bool focus = GetRootWindow(GetFocus()) == m_hwnd;
        RECT r; GetClientRect(m_hwnd, &r);
        COLORREF c = 0;
        if (focus) c |= 0x0000FF;
        if (m_bHasFocus) c |= 0x00cc00;
        if (0 == c) c = 0xCC0000;
        CreateBorder(hdc_screen, &r, c, 2);
    }
#endif

    EndPaint(m_hwnd, &ps);
}

//==============================================
// Calculate sizes of the menu-window

void Menu::Validate()
{
    int margin  = MenuInfo.nFrameMargin;
    int border  = mStyle.MenuFrame.borderWidth;
    int tborder = mStyle.MenuTitle.borderWidth;
    int tm      = MenuInfo.nTitleMargin;
    int w1, w2, c0, c1, h, hmax, x; SIZE size;

    MenuItem *pItem;
    HDC hDC;
    HGDIOBJ other_font;
    char opt_cmd[1000];

    w1 = w2 = c0 = c1 = h = 0;
    opt_cmd[0] = 0;

    m_bNoTitle = (m_flags & BBMENU_NOTITLE) || mStyle.menuNoTitle;

    // ---------------------------------------------------
    // measure text-sizes
    pItem = m_pMenuItems;
    hDC = CreateCompatibleDC(NULL);
    other_font = SelectObject(hDC, MenuInfo.hTitleFont);

    // title item
    if (false == m_bNoTitle) {
        pItem->Measure(hDC, &size);
        w1 = size.cx;
    }

    // now for frame items
    SelectObject(hDC, MenuInfo.hFrameFont);

    while (NULL != (pItem = pItem->next))
    {
        pItem->Measure(hDC, &size);
        pItem->m_nHeight = size.cy;
        if (size.cx > w2)
            w2 = size.cx;
        if (pItem->m_ItemID & MENUITEM_UPDCHECK) {
            // update checkmark (for stylemenu folder etc.)
            pItem->m_bChecked = get_opt_command(opt_cmd, pItem->m_pszCommand);
        }
    }

    SelectObject(hDC, other_font);
    DeleteDC(hDC);

    // ---------------------------------------------------
    // make sure that the menu is not wider than something

    m_width = imin(m_maxwidth,
        imax(w1 + 2*(MenuInfo.nTitleIndent + MenuInfo.nTitleMargin + border),
             w2 + 2*margin + MenuInfo.nItemLeftIndent + MenuInfo.nItemRightIndent
             ));

	/* BlackboxZero 12.17.2011 */
	//Check and set minimum width
	if ( m_width < m_minwidth && m_minwidth <= m_maxwidth ) {
		m_width = m_minwidth;
	}

    // ---------------------------------------------------
    // assign item positions

    // the title item
    pItem = m_pMenuItems;
    pItem->m_nLeft = border + tm;
    pItem->m_nWidth = m_width - 2*(border + tm);
    pItem->m_nHeight = MenuInfo.nTitleHeight - tborder - border;
    if (mStyle.menuTitleLabel)
        pItem->m_nHeight += tborder - mStyle.MenuFrame.marginWidth;

    if (m_bNoTitle) {
        pItem->m_nTop = -1000;
        m_firstitem_top = margin;
    } else {
        pItem->m_nTop = tm + border;
        m_firstitem_top = MenuInfo.nTitleHeight + mStyle.MenuFrame.marginWidth;
    }

    hmax = m_maxheight - m_firstitem_top;

    // assign xy-coords and width
    w2 = m_width - 2*margin;
    while (NULL != (pItem = pItem->next))
    {
        pItem->m_nLeft = margin;
        pItem->m_nWidth = w2;
        ++c0;
        if ((x = h + pItem->m_nHeight) < hmax)
            h = x, c1 = c0;
    }

    m_itemcount = c0;
    m_pagesize = c1;
    m_height = m_firstitem_top + h + margin;

    // adjust height for empty menus
    if (0 == c0 || m_bIconized) {
        if (m_bNoTitle) {
            m_height += 4;
        } else if (mStyle.menuTitleLabel || mStyle.MenuTitle.parentRelative) {
            m_height = MenuInfo.nTitleHeight + margin;
        } else {
            m_height = MenuInfo.nTitleHeight - tborder + border;
        }
    }

    // need a _new background
    if (m_hBitMap)
    {
        DeleteObject(m_hBitMap), m_hBitMap = NULL;
        InvalidateRect(m_hwnd, NULL, FALSE);
    }

    // assign y-coords
    scroll_menu(0);
}

//==============================================

void Menu::Show(int xpos, int ypos, bool fShow)
{
    Menu *p = m_pParent;

    if (p) // a submenu
    {
        int overlap = MenuInfo.nSubmenuOverlap;
        int xleft = p->m_xpos - this->m_width + overlap;
        int xright = p->m_xpos + p->m_width - overlap;
        bool at_left = MenuInfo.openLeft;

        // pop to the left or to the right ?
        if (xright + m_width > m_mon.right)
            at_left = true;
        else if (xleft < m_mon.left)
            at_left = false;
        else if (p->m_pParent)
            at_left = p->m_pParent->m_xpos > p->m_xpos;

        //xpos = at_left ? xleft : xright;
		xpos = at_left ? (xleft -= Settings_menu.spacing):(xright += Settings_menu.spacing); /* BlackboxZero 1.6.2012 */
        ypos = p->m_ypos;

        if (m_pParentItem != m_pParent->m_pMenuItems) // not the TitleItem
            ypos += m_pParentItem->m_nTop - m_firstitem_top;
    }

    // make sure the popup is on the screen
    m_xpos = iminmax(xpos, m_mon.left,
        m_mon.right - m_width);
    m_ypos = iminmax(ypos, m_mon.top,
        m_mon.bottom - (m_bPinned ? MenuInfo.nTitleHeight : m_height));

    // create the menu window, if it doesn't have one already
    make_menu_window();

    if (m_alpha != mStyle.menuAlpha)
        SetTransparency(m_hwnd, m_alpha = mStyle.menuAlpha);

    if (fShow)
        menu_set_pos(HWND_TOP, SWP_SHOWWINDOW);
    else
        menu_set_pos(NULL, SWP_NOZORDER);
}

//==============================================
// On Timer: Show/Hide submenu

void Menu::MenuTimer(UINT nTimer)
{
    if (MENU_POPUP_TIMER == nTimer)
    {
        set_timer(true, false);
        if (NULL == m_pActiveItem) {
            HideChild();
            return;
        }
        if (NULL == m_pParent && NULL == m_pChild)
            bring_ontop(false);
    }

    if (m_pActiveItem)
        m_pActiveItem->ItemTimer(nTimer);
}

void Menu::UnHilite()
{
    if (m_pActiveItem)
        m_pActiveItem->Active(0);
}

//==============================================
// Hide menus  016f:004109d4

void Menu::Hide(void)
{
    UnHilite();
    HideChild();
    if (m_bPinned)
        return;

    if (m_pParent)
        m_pParent->UnHilite();
    if (m_bHasFocus) {
        if (m_pParent)
            m_pParent->set_focus();
        else if (m_hwndRef)
            SetFocus(m_hwndRef);
    }
    destroy_menu_window(false);
}

void Menu::HideNow(void)
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
    Menu *p;
    if (m_flags & BBMENU_NOTITLE)
        bPinned = false;
    m_bPinned = bPinned;
    if (false == bPinned)
        return;
    p = menu_root();
    if (p != this) {
        LinkToParentItem(NULL);
        p->Hide();
    }
    post_autohide();
}

//==============================================
// Virtual: Implemented by SpecialFolder to update contents on folder change

void Menu::UpdateFolder(void)
{
    MenuItem *mi;
    dolist (mi, m_pMenuItems)
        if (mi->m_ItemID == MENUITEM_ID_INSSF)
            ((SFInsert*)mi)->RemoveStuff();
}

//===========================================================================
void Menu::mouse_over(bool indrag)
{
    if (m_bMouseOver)
        return;

    // hide grandchilds
    if (m_pChild) {
        m_pChild->HideChild();
        m_pChild->UnHilite();
    }

    //hilite my parentItem
    if (m_pParent) {
        m_pParent->set_timer(true, false);
        m_pParentItem->Active(2);
    }

    //set focus to this one immediately
    if (has_focus_in_chain())
        set_focus();

    m_bMouseOver = true;

    if (false == indrag && NULL == m_hwndChild && have_imp(pTrackMouseEvent))
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize  = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        pTrackMouseEvent(&tme);
    }
    else
    {
        // TrackMouseEvent does not seem to work in drag
        // operation (and on win95)
        SetTimer(m_hwnd,
            MENU_TRACKMOUSE_TIMER,
            iminmax(Settings_menu.popupDelay-10, 20, 100), NULL);
    }
}

//==============================================

void Menu::mouse_leave(void)
{
    if (false == Settings_menuKeepHilite
        && (NULL == m_pChild || false == m_pChild->m_bMouseOver))
        UnHilite();
    if (m_pChild)
        set_timer(false, true); // to close submenu

}

//==============================================

void Menu::Handle_Mouse(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    MenuItem* item;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    m_kbditempos = -1;

    if (WM_MOUSEWHEEL == uMsg)
    {
        int d = (short)HIWORD(wParam) > 0 ? -1 : 1;
        scroll_menu(d * Settings_menu.mouseWheelFactor);
        ScreenToClient(hwnd, &pt);
    }

    if (m_captureflg)
    {
        // ------------------------------------------------
        if (m_captureflg == MENU_CAPT_SCROLL) // the scroll button
        {
            if (WM_MOUSEMOVE == uMsg)
                set_vscroller(pt.y);
        }
        else
        if (m_captureflg == MENU_CAPT_ITEM) // mouse down on item
        {
            if (m_pActiveItem)
                m_pActiveItem->Mouse(hwnd, uMsg, wParam, lParam);
        }

        if (WM_MOUSEMOVE == uMsg)
            return;

        // ------------------------------------------------
    }
    else // not captured
    {
        // Set Focus, track mouse, etc...
        mouse_over(false);

        // ------------------------------------------------
        // check scrollbutton
        if (m_pagesize < m_itemcount)
        {
            RECT scroller;
            get_vscroller_rect(&scroller);

            if (pt.x >= scroller.left && pt.x < scroller.right
                && (m_bNoTitle || pt.y >= m_firstitem_top + MenuInfo.nScrollerTopOffset))
            {
                if (WM_LBUTTONDOWN == uMsg)
                {
                    set_vscroller(pt.y);
                    set_capture(MENU_CAPT_SCROLL);
                }
                else
                if (m_pChild && m_pChild->m_pParentItem->isover(pt.y))
                {
                    // ignore scroller, if an folder-item is at the same position
                    bool submenu_right = m_pChild->m_xpos > m_xpos;
                    bool scroller_right = FOLDER_LEFT == MenuInfo.nBulletPosition; /* BlackboxZero 1.7.2012 - Was FOLDER_RIGHT != */
                    if (scroller_right == submenu_right)
                        return;
                }
                mouse_leave();
                return;
            }
        }
    }

    // ------------------------------------------------
    // check through items, where the mouse is over

    item = m_pMenuItems; // the TitleItem

    // everything above the first item belongs to title
    if (pt.y < m_firstitem_top
        // also, allow moving of 'title-less' menus with control held down
      || (m_bNoTitle && (wParam & MK_CONTROL))) {

        item->Mouse(hwnd, uMsg, wParam, lParam);
        return;
    }

    // check normal items
    while (NULL != (item = item->next)) {
        if (item->isover(pt.y)) {
            item->Mouse(hwnd, uMsg, wParam, lParam);
            return;
        }
    }
}

//==============================================
// simulate mouse movement in drag operation

#ifndef BBTINY
LPCITEMIDLIST Menu::dragover(POINT* ppt)
{
    MenuItem* pItem;
    POINT pt = *ppt;

    ScreenToClient(m_hwnd, &pt);
    mouse_over(true);

    dolist(pItem, m_pMenuItems)
        if (pItem->isover(pt.y)) {
            pItem->Active(1);
            return pItem->GetPidl();
        }
    return NULL;
}

void Menu::start_drag(const char *arg_path, LPCITEMIDLIST arg_pidl)
{
    LPITEMIDLIST pidl;
    if (m_bInDrag)
        return;
    if (arg_pidl)
        pidl = duplicateIDList(arg_pidl);
    else
    if (arg_path)
        pidl = sh_getpidl(NULL, arg_path);
    else
        return;
    if (NULL == pidl)
        return;

    incref();
    m_bInDrag = true;
    drag_pidl(pidl);
    m_bInDrag = false;
    decref();
    freeIDList(pidl);
}
#endif

//===========================================================================
// Menu Keystroke handling
//===========================================================================

void Menu::kbd_hilite(MenuItem *pItem)
{
    int a;

    if (NULL == pItem) {
        UnHilite();
        return;
    }

    HideChild();
    pItem->Active(2);
    m_kbditempos = get_item_index(pItem);

    a = m_kbditempos - m_topindex;
    if (a < 0) {
        scroll_menu(a);
    } else {
        a -= m_pagesize-1;
        if (a > 0)
            scroll_menu(a);
    }

    // immediately show submenu with int & string items
    if (pItem->m_pSubmenu
        && (pItem->m_pSubmenu->m_MenuID & (MENU_ID_INT|MENU_ID_STRING)))
        pItem->ShowSubmenu();
}

//--------------------------------------
void Menu_Tab_Next(Menu *p)
{
    MenuList *ml;
    Menu *m = NULL;
    //bool backwards = 0x8000 & GetAsyncKeyState(VK_SHIFT);
    dolist (ml, Menu::g_MenuWindowList)
        if (NULL == (m = ml->m)->m_pParent && p != m)
            break;
    if (NULL == m)
        m = p;

    if (m->m_kbditempos == -1)
        m->m_kbditempos = m->m_topindex;
    m->bring_ontop(true);
}

//===========================================================================
MenuItem * Menu::kbd_get_next_shortcut(const char *d)
{
    MenuItem *pItem;
    unsigned n, dl;

    dl = strlen(d);
    for (n = 0, pItem = m_pActiveItem;; pItem = m_pMenuItems)
    {
        if (pItem)
            while (NULL != (pItem = pItem->next)) {
                const char *s, *t;
                for (t = s = pItem->m_pszTitle;; ++s)
                    if (0 == *s) {
                        s = t;
                        break;
                    } else if (*s == '&' && *++s != '&') {
                        break;
                    }
                if (strlen(s) < dl)
                    continue;
                if (2 == CompareString(
                        LOCALE_USER_DEFAULT,
                        NORM_IGNORECASE|NORM_IGNOREWIDTH|NORM_IGNOREKANATYPE,
                        d, dl, s, dl))
                    break;
            }
        if (pItem || 2 == ++n)
            return pItem;
    }
}

//--------------------------------------
bool Menu::Handle_Key(UINT msg, UINT wParam)
{
    int n, a, c, d, e, i, x, y;
    MenuItem *pItem; bool ctrl; const int c_steps = 1;

    ignore_mouse_msgs();

    if (VK_CONTROL == wParam || VK_SHIFT == wParam)
        return false;

    //--------------------------------------
    if (MENU_ID_INT & m_MenuID)
    {
        m_pMenuItems->next->Key(msg, wParam);
        return true;
    }

    //--------------------------------------
    if (WM_CHAR == msg)
    {
        char chars[16];
        MSG msg;

        i = 0;
        chars[i++] = (char)wParam;
        while (PeekMessage(&msg, m_hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            chars[i++] = (char)msg.wParam;
            if (i >= (int)sizeof chars - 1)
                break;
        }
        chars[i] = 0;

        if (!IS_SPC(chars[0])) {
            // Shortcuts
            pItem = kbd_get_next_shortcut(chars);
            if (pItem) {
                kbd_hilite(pItem);
                if (pItem == kbd_get_next_shortcut(chars)) {
                    // (there is only one choice for that accelerator...)

                    if (pItem->m_ItemID & MENUITEM_ID_FOLDER) {
                        // popup folder
                        goto k_right;
                    }

                    if (m_flags & BBMENU_SYSMENU) {
                        // for the window sysmenus (bbLeanSkin/Bar)
                        goto k_invoke;
                    }
                }
            }
        }
        return true;
    }

    //--------------------------------------
    if (WM_KEYDOWN == msg)
    {
        a = m_kbditempos;
        if (a == -1)
            a = get_item_index(m_pActiveItem);

        e = m_itemcount - 1;
        d = m_pagesize - 1;
        n = 0;
        ctrl = 0 != (0x8000 & GetKeyState(VK_CONTROL));
one_more:
        i = 1;
        switch(wParam)
        {
            set:
                pItem = nth_item(a);
                if (pItem && pItem->m_bNOP) {
                    if (n++ < e)
                        goto one_more;
                    pItem = NULL;
                }
                kbd_hilite(pItem);
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
                if (a < 0)
                    a = m_topindex;
                else
                if (a >= e)
                    a = (d < e) ? e : 0;
                else
                    a = imin(a+i, e);
                goto set;

            //--------------------------------------
            case VK_UP:
                if (ctrl)
                {
                    i = imin(c_steps, m_topindex);
                    if (i >= 0)
                        scroll_menu(-i);
                    else
                        break;
                }
                if (a < 0)
                    a = e;
                else
                if (0 == a)
                    a = (d < e) ? 0 : e;
                else
                    a = imax(a-i, 0);
                goto set;

            //--------------------------------------
            case VK_NEXT:
                if (a < 0)
                    a = m_topindex;
                c = imin (e - d - m_topindex, d);
                if (c <= 0)
                    a = e;
                else
                    a += c;
                scroll_menu(c);
                wParam = VK_DOWN;
                goto set;

            //--------------------------------------
            case VK_PRIOR:
                if (a < 0)
                    a = m_topindex;
                c = imin(m_topindex, d);
                if (c <= 0)
                    a = 0;
                else
                    a -= c;
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
            k_right:
                if (m_pActiveItem) {
                    m_pActiveItem->ShowSubmenu();
                }

            hilite_child:
                if (m_pChild) {
                    m_pChild->set_focus();
                    m_pChild->kbd_hilite(m_pChild->m_pMenuItems->next);
                }
                break;

            //--------------------------------------
            case VK_LEFT:
                if (m_pParent)
                    m_pParent->kbd_hilite(m_pParentItem);
                break;

            //--------------------------------------
            case VK_RETURN: // invoke command
            case VK_SPACE:
            k_invoke:
                if (m_pActiveItem) {
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
                if (false == m_bPinned)
                    SetPinned(true);
                else
                    m_bOnTop = false == m_bOnTop;

                SetZPos();
                break;

            //--------------------------------------
            case VK_ESCAPE:
                if (0 == (menu_root()->m_flags & BBMENU_HWND)) {
                    Menu_All_Hide();
                    focus_top_window();
                } else {
                    menu_root()->Hide();
                }
                break;
            case VK_DELETE:
                HideNow();
                break;

            //--------------------------------------
            //sortmode: extension, name, size, time
#ifndef BBTINY
            case 'E': case 'N': case 'S': case 'T':
                if (ctrl)
                {
                    static const char sm[] = ".ENS.T";
                    i = strchr(sm, wParam) - sm;
                    if (i != m_sortmode)
                        m_sortrev = 5==(m_sortmode = i);
                    else
                        m_sortrev ^= 1;
                    HideChild();
                    Redraw(2);
                }
                break;
#endif
            //--------------------------------------
            default:
                return false;
        }
        return true;
    }

    //--------------------------------------
    if (WM_SYSKEYDOWN == msg)
    {
        d = 12, x = m_xpos, y = m_ypos;
        switch(wParam)
        {
            case VK_LEFT:  x -= d; break;
            case VK_RIGHT: x += d; break;
            case VK_UP:    y -= d; break;
            case VK_DOWN:  y += d; break;

            case VK_RETURN:
            case VK_SPACE:
                if (m_pActiveItem)
                    m_pActiveItem->Invoke(INVOKE_PROP);
                return true;

            default:
                return false;
        }
        Show(x, y, false);
        write_menu_pos();
        return true;
    }

    return false;
}

//===========================================================================
// Window proc for the popup menues

LRESULT Menu::wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        //====================
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

        //====================
        case WM_CREATE:
            if (Settings_menu.sticky)
                MakeSticky(hwnd);
            break;

        case WM_DESTROY:
            RemoveSticky(hwnd);
            break;

        //====================
        case WM_SETFOCUS:
#ifdef CHECKFOCUS
            dbg_printf("WM_SETFOCUS %s %x <- %x", m_pMenuItems->m_pszTitle, hwnd, wParam);
            InvalidateRect(hwnd, NULL, FALSE);
#endif
            on_setfocus((HWND)wParam);
            break;

        //====================
        case WM_KILLFOCUS:
#ifdef CHECKFOCUS
            dbg_printf("WM_KILLFOCUS %s %x -> %x", m_pMenuItems->m_pszTitle, hwnd, wParam);
            InvalidateRect(hwnd, NULL, FALSE);
#endif
            on_killfocus((HWND)wParam);
            break;

        //====================
        case WM_CLOSE:
            HideNow();
            break;

        case BB_FOLDERCHANGED:
            if (NULL == m_pChild || m_pChild->m_MenuID != MENU_ID_SHCONTEXT)
                RedrawGUI(BBRG_FOLDER);
            break;

        case BB_DRAGOVER:
            return (LRESULT)dragover((POINT *)lParam);

        //====================
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            if (false == Handle_Key(uMsg, wParam))
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;

        //====================
        case WM_MOUSELEAVE:
            //dbg_printf("mouseleave %x", hwnd);
            if (m_bMouseOver)
            {
                m_bMouseOver = false;
                mouse_leave();
            }
            break;

        //====================
        case WM_MOUSEWHEEL:
            Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        //====================
        case WM_MOUSEMOVE:
            if (g_ignore_mouse_msgs)
                -- g_ignore_mouse_msgs;
            else
                Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
            m_dblClicked = true;
            Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            m_dblClicked = false;
            Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            set_capture(0);
            Handle_Mouse(hwnd, uMsg, wParam, lParam);
            break;

        case WM_CAPTURECHANGED:
            m_captureflg = 0;
            break;

        //====================
        case WM_TIMER:
            if (MENU_TRACKMOUSE_TIMER == wParam)
            {
                if (hwnd != window_under_mouse())
                {
                    PostMessage(hwnd, WM_MOUSELEAVE, 0, 0);
                    KillTimer(hwnd, wParam);
                }
                break;
            }
            //dbg_printf("Timer %x %d", hwnd, wParam);
            MenuTimer(wParam);
            break;

        //====================
        case WM_WINDOWPOSCHANGING:
            if (m_bMoving && Settings_menu.snapWindow)
            {
                SnapWindowToEdge((WINDOWPOS*)lParam, 0, SNAP_NOPLUGINS);
            }
            break;

        case WM_WINDOWPOSCHANGED:
            if (0 == (((LPWINDOWPOS)lParam)->flags & SWP_NOMOVE))
            {
                m_xpos = ((LPWINDOWPOS)lParam)->x;
                m_ypos = ((LPWINDOWPOS)lParam)->y;
            }
            break;

        //====================
        case WM_ENTERSIZEMOVE:
            m_bMoving = true;
            break;

        //====================
        case WM_EXITSIZEMOVE:
            m_bMoving = false;
            GetMonitorRect(hwnd, &m_mon, GETMON_FROM_WINDOW);
            write_menu_pos();
            break;

        //====================
        case WM_PAINT:
            Paint();
            break;

        //====================
        case WM_ERASEBKGND:
            return TRUE;

        //===============================================
        // String pItem - parent window notifications
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
            return (LRESULT)GetStockObject(NULL_BRUSH);

        //===============================================
    }
    return 0;
}

//===========================================================================
// misc menu helpers

// Create the one item submenu for MakeMenuItemInt and MakeMenuItemString
MenuItem *helper_menu(Menu *PluginMenu, const char* Title, int menuID, MenuItem *pItem)
{
    Menu *sub;
    if (PluginMenu->m_IDString) {
        char IDString[200];

        // build an idstring from the parent menu's and the title
        sprintf (IDString, "%s:%s", PluginMenu->m_IDString, Title);
        sub = Menu::find_named_menu(IDString);
        if (sub) {
            // re-use existing menu&item to avoid changes in the
            // edit control while the user is working on it
            sub->incref();
            sub->m_OldPos = -1;
            delete pItem;
            return MakeSubmenu(PluginMenu, sub);
        }
        sub = MakeNamedMenu(Title, IDString, PluginMenu->m_bPopup);
    } else {
        sub = MakeMenu(Title);
    }
    sub->AddMenuItem(pItem);
    sub->m_MenuID = menuID;
    return MakeSubmenu(PluginMenu, sub);
}

// used with 'MakeMenuItemInt/String'-made items
MenuItem *MenuItem::get_real_item(void)
{
    if (this && this->m_pSubmenu
     && (this->m_pSubmenu->m_MenuID & (MENU_ID_STRING|MENU_ID_INT)))
        return this->m_pSubmenu->m_pMenuItems->next;
    return NULL;
}

void Menu::Sort(MenuItem **ppItems, int(*cmp_fn)(MenuItem **, MenuItem**))
{
    MenuItem **a, **b, *i; int n = 0;
    dolist(i, *ppItems) n++;  // get size
    if (n < 2) return; // nothing to do
    b = a = (MenuItem**)m_alloc(n * sizeof *a); // make array
    dolist(i, *ppItems) *b = i, b++; // store pointers
    qsort(a, n, sizeof *a, (int(*)(const void*,const void*))cmp_fn);
    do a[--n]->next = i, i = a[n]; while (n); // make list
    m_free(a); // free array
    *ppItems = i;
}

static int item_compare(MenuItem** m1, MenuItem** m2)
{
    return stricmp((*m1)->m_pszTitle, (*m2)->m_pszTitle);
}

// for invocation from the keyboard:
// menu is present:
//      - has focus: hide it and switch to application
//      - does not have focus: set focus to the menu
// is not present: return false -> show the menu.

bool Menu_ToggleCheck(const char *menu_id)
{
    Menu *m;
    char IDString[MAX_PATH];

    if (menu_id[0])
        m = Menu::find_named_menu(Core_IDString(IDString, menu_id));
    else
        m = Menu::last_active_menu_root();

    if (NULL == m || NULL == m->m_hwnd)
        return false;

    if (m->has_focus_in_chain()) {
        Menu_All_Hide();
        focus_top_window();
        return true;
    }

    if (m->m_bPinned) {
        m->bring_ontop(true);
        return true;
    }

    return false;
}

//===========================================================================
void Menu::RedrawGUI(int flags)
{
    if (flags & BBRG_FOLDER) {
        Redraw(2);
    } else {
        // sent from bbstylemaker
        if ((flags & BBRG_MENU) && (flags & BBRG_PRESSED) && NULL == m_pActiveItem) {
            MenuItem *pItem = m_pMenuItems;
            while (NULL != (pItem = pItem->next))
                if (false == pItem->m_bNOP) {
                    pItem -> Active(2); // hilite some item
                    break;
                }
        }
        Redraw(0);
    }
}

//===========================================================================
// For menu updating. Since all items are deleted and rebuilt, this
// tries to make that happen without the user would notice.

void Menu::SaveState()
{
    if (NULL == m_hwnd || m_saved)
        return;
    // store position of hilited item
    m_OldPos = get_item_index(m_pActiveItem);
    // store childmenu
    m_OldChild = m_pChild;
    m_saved = true;
    // dbg_printf("savee %s", m_pMenuItems->m_pszTitle);
}

void Menu::RestoreState(void)
{
    MenuItem *pItem;
    // dbg_printf("restored %s", m_pMenuItems->m_pszTitle);
    if (m_OldChild) {
        // The menu had an open submenu when it was updated, so
        // this if possible tries to link it to an FolderItem, again.
        dolist (pItem, m_pMenuItems) {
            Menu *pSub = pItem->m_pSubmenu;
            if (NULL == pSub) {
                if (pItem->m_ItemID == MENUITEM_ID_SF)
                    pSub = Menu::find_special_folder(pItem->m_pidl_list);
                else
                    pSub = pItem->m_pRightmenu;
            }
            if (pSub == m_OldChild) {
                pSub->incref(), pItem->LinkSubmenu(pSub);
                pItem->m_pSubmenu->LinkToParentItem(pItem);
                break;
            }
        }
        if (NULL == pItem) {
            // Lost child - the parent menu or item doesn't exist anymore.
            m_OldChild->Hide();
            m_OldPos = -1;
        }
        m_OldChild = NULL;
    }

    // lookup the item that was previously hilited
    pItem = nth_item(m_OldPos);
    if (pItem)
        pItem->Active(1);
    m_saved = false;
}

//===========================================================================

//===========================================================================
// update and relink all visible menus within an updated menu structure

void Menu::Redraw(int mode)
{
    MenuItem *pItem;
    if (m_hwnd)
    {
        if (mode != 0) {
            SaveState();
            if (mode == 2)
                UpdateFolder();
        }
        // recalculate sizes
        Validate();
        if (mode != 0) {
            RestoreState();
        }
        // update window position/size
        Show(m_xpos, m_ypos, false);
        if (mode == 2)
            return;
    }

    // Redraw submenus recursively
    dolist (pItem, m_pMenuItems)
        if (pItem->m_pSubmenu)
            pItem->m_pSubmenu->Redraw(mode);
        else
        if (pItem->m_pRightmenu)
            pItem->m_pRightmenu->Redraw(mode);
}

//===========================================================================
//
//  Global Menu Interface
//
//===========================================================================
// Below are the global functions to do something with menus. No other menu
// functions should be called from outside

void Menu_Init(void)
{
    register_menuclass();
    Menu_Reconfigure();
}

void Menu_ResetFonts(void)
{
    if (MenuInfo.hTitleFont)
        DeleteObject(MenuInfo.hTitleFont);
    if (MenuInfo.hFrameFont)
        DeleteObject(MenuInfo.hFrameFont);
    MenuInfo.hTitleFont =
    MenuInfo.hFrameFont = NULL;
}

bool Menu::del_menu(Menu *m, void *ud)
{
    m->destroy_menu_window(true);
    return true;
}

void Menu_Exit(void)
{
    MenuEnum(Menu::del_menu, NULL);
    Menu_ResetFonts();
    un_register_menuclass();
}

// Toggle visibility state
bool Menu::toggle_menu(Menu *m, void *ud)
{
    ShowWindow(m->m_hwnd, *(bool*)ud ? SW_HIDE : SW_SHOWNA);
    return true;
}

void Menu_All_Toggle(bool hidden)
{
    MenuEnum(Menu::toggle_menu, &hidden);
}

// update menus after style changed
bool Menu::redraw_menu(Menu *m, void *ud)
{
    if (NULL == m->m_pParent)
        m->RedrawGUI(*(int*)ud);
    return true;
}

void Menu_All_Redraw(int flags)
{
    MenuEnum(Menu::redraw_menu, &flags);
}

bool Menu::hide_menu(Menu *m, void *ud)
{
    if (NULL == m->m_pParent && m != (Menu*)ud)
        m->Hide();
    return true;
}

// Hide all menus, which are not pinned, except p, unless NULL
void Menu_All_Hide_But(Menu *p)
{
    MenuEnum(Menu::hide_menu, p);
}

void Menu_All_Hide(void)
{
    Menu_All_Hide_But(NULL);
}

// bring all menus on top, restoring the previous z-order
void Menu_All_BringOnTop(void)
{
    MenuList *ml;
    Menu *m;
    dolist (ml, Menu::g_MenuWindowList)
        SetOnTop(ml->m->m_hwnd);
    m = Menu::last_active_menu_root();
    if (m) 
        m->set_focus();
}

bool Menu_Exists(bool pinned)
{
    MenuList *ml;
    dolist (ml, Menu::g_MenuWindowList)
        if (pinned == ml->m->m_bPinned)
            return true;
    return false;
}

// this is used in SnapWindowToEdge to avoid plugins snap to menus.
bool Menu_IsA(HWND hwnd)
{
    return (LONG_PTR)Menu::WindowProc == GetWindowLongPtr(hwnd, GWLP_WNDPROC);
}

void Menu_Stats(struct menu_stats *st)
{
    st->menu_count = g_menu_count;
    st->item_count = g_menu_item_count;
}

// Operate on all currently visible menus
Menu *MenuEnum(MENUENUMPROC fn, void *ud)
{
    MenuList *ml, *ml_copy = (MenuList*)copy_list(Menu::g_MenuWindowList);
    Menu *m = NULL;
    Menu::g_incref();
    dolist (ml, ml_copy) {
        m = ml->m;
        if (false == fn(m, ud))
            break;
    }
    Menu::g_decref();
    freeall(&ml_copy);
    return m;
}

int get_menu_bullet (const char *tmp)
{
    static const char * const bullet_strings[] = {
        "triangle" ,
        "square"   ,
        "diamond"  ,
        "circle"   ,
        "empty"    ,
        NULL
        };

    static const char bullet_styles[] = {
        BS_TRIANGLE  ,
        BS_SQUARE    ,
        BS_DIAMOND   ,
        BS_CIRCLE    ,
        BS_EMPTY     ,
        };

    int i = get_string_index(tmp, bullet_strings);
    if (-1 != i)
        return bullet_styles[i];
    return BS_TRIANGLE;
}

void Menu_Reconfigure(void)
{
    StyleItem *MTitle, *MFrame, *MHilite, *pSI, *pScrl;

    int tfh, titleHeight;
    int ffh, itemHeight;

    MTitle = &mStyle.MenuTitle;
    MFrame = &mStyle.MenuFrame;
    MHilite = &mStyle.MenuHilite;

    // create fonts
    Menu_ResetFonts();
    MenuInfo.hTitleFont = CreateStyleFont(MTitle);
    MenuInfo.hFrameFont = CreateStyleFont(MFrame);

    // set bullet position & style
	MenuInfo.nBulletPosition = (Settings_menu.bullet_enabled)?(stristr(mStyle.menuBulletPosition, "left")
		? FOLDER_LEFT : FOLDER_RIGHT):(FOLDER_DISABLED);

    MenuInfo.nBulletStyle = get_menu_bullet(mStyle.menuBullet);

    MenuInfo.openLeft = 0 == stricmp(Settings_menu.openDirection, "bullet")
        ? MenuInfo.nBulletPosition == FOLDER_LEFT
        : 0 == stricmp(Settings_menu.openDirection, "left")
        ;

	/* BlackboxZero 1.7.2012
	** This is kinda fugly...OKAY, it's REALLY fugly
	** If default, elseif left, elseif right, else disabled */
	MenuInfo.nScrollerPosition = stristr(Settings_menu.scrollerPosition, "default")?
		(FOLDER_DEFAULT):(stristr(Settings_menu.scrollerPosition, "left")?(FOLDER_LEFT):(
		stristr(Settings_menu.scrollerPosition, "right")?(FOLDER_RIGHT):(FOLDER_DISABLED)));

    // --------------------------------------------------------------
    // calulate metrics:

    MenuInfo.nFrameMargin = MFrame->marginWidth + MFrame->borderWidth;
    MenuInfo.nSubmenuOverlap = MenuInfo.nFrameMargin + MHilite->borderWidth;
    MenuInfo.nTitleMargin = 0;

    if (mStyle.menuTitleLabel)
        MenuInfo.nTitleMargin = MFrame->marginWidth;

    // --------------------------------------
    // title height, indent, margin

    tfh = get_fontheight(MenuInfo.hTitleFont);
    titleHeight = 2*MTitle->marginWidth + tfh;

    // xxx old behaviour xxx
    if (false == mStyle.is_070 && 0 == (MTitle->validated & V_MAR))
        titleHeight = MTitle->FontHeight + 2*mStyle.bevelWidth;
    //xxxxxxxxxxxxxxxxxxxxxx

    pSI = MTitle->parentRelative ? MFrame : MTitle;
    MenuInfo.nTitleHeight = titleHeight + MTitle->borderWidth + MFrame->borderWidth;
    MenuInfo.nTitleIndent = imax(imax(2 + pSI->bevelposition, pSI->marginWidth), (titleHeight-tfh)/2);

    if (mStyle.menuTitleLabel) {
        MenuInfo.nTitleHeight += MTitle->borderWidth + MFrame->marginWidth;
        MenuInfo.nTitleIndent += MTitle->borderWidth;
    }

    // --------------------------------------
    // item height, indent

    ffh = get_fontheight(MenuInfo.hFrameFont);
    itemHeight = MHilite->marginWidth + ffh + 2*MHilite->borderWidth;

    // xxx old behaviour xxx
    if (false == mStyle.is_070 && 0 == (MHilite->validated & V_MAR))
        itemHeight = MFrame->FontHeight + (mStyle.bevelWidth+1)/2;
    //xxxxxxxxxxxxxxxxxxxxxx

//#ifdef BBOPT_MENUICONS
	if ( Settings_menu.iconSize ) { /* BlackboxZero 1.3.2012 */
		itemHeight = imax(14, itemHeight);
		MenuInfo.nItemHeight =
		MenuInfo.nItemLeftIndent =
		MenuInfo.nItemRightIndent = itemHeight;
		MenuInfo.nIconSize = imin(itemHeight - 2, Settings_menu.iconSize); /* BlackboxZero 1.4.2012 - Was 16 */
		if (DT_LEFT == MFrame->Justify)
			MenuInfo.nItemLeftIndent += 2;
	} else {
//#else
		MenuInfo.nItemHeight = itemHeight;
		MenuInfo.nItemLeftIndent =
		MenuInfo.nItemRightIndent = imax(11, itemHeight);
	}
//#endif

#ifdef BBXMENU
    if (DT_CENTER != MFrame->Justify) {
        int n = imax(3 + MHilite->borderWidth, (itemHeight-ffh)/2);
        if (MenuInfo.nBulletPosition == FOLDER_RIGHT)
            MenuInfo.nItemLeftIndent = n;
        else
            MenuInfo.nItemRightIndent = n;
    }
#endif

    // --------------------------------------
    // from where on does it need a scroll button:
    MenuInfo.MaxWidth = Settings_menu.showBroams
        ? iminmax(Settings_menu.maxWidth*2, 320, 640)
        : Settings_menu.maxWidth;

	/* BlackboxZero 12.17.2011 */
	MenuInfo.MinWidth = Settings_menu.showBroams
        ? iminmax(Settings_menu.minWidth*2, 320, 640)
        : Settings_menu.minWidth;

    // --------------------------------------
    // setup a StyleItem for the scroll rectangle
    pScrl = &MenuInfo.Scroller;
    if (false == MTitle->parentRelative)
    {
        *pScrl = *MTitle;
        if (false == mStyle.menuTitleLabel) {
            pScrl->borderColor = MFrame->borderColor;
            pScrl->borderWidth = imin(MFrame->borderWidth, MTitle->borderWidth);
        }

    } else {
        *pScrl = *MHilite;
        if (pScrl->parentRelative) {
            if (MFrame->borderWidth) {
                pScrl->borderColor = MFrame->borderColor;
                pScrl->borderWidth = MFrame->borderWidth;
            } else {
                pScrl->borderColor = MFrame->TextColor;
                pScrl->borderWidth = 1;
            }
        }
    }

    pScrl->bordered = 0 != pScrl->borderWidth;

    MenuInfo.nScrollerSize =
        imin(itemHeight + imin(MFrame->borderWidth, pScrl->borderWidth),
            titleHeight + 2*pScrl->borderWidth
            );

    if (mStyle.menuTitleLabel) {
        MenuInfo.nScrollerTopOffset = 0;
        MenuInfo.nScrollerSideOffset = MenuInfo.nFrameMargin;
    } else {
        // merge the slider's border into the frame/title border
        if (MTitle->parentRelative)
            MenuInfo.nScrollerTopOffset = 0;
        else
            MenuInfo.nScrollerTopOffset =
                - (MFrame->marginWidth + imin(MTitle->borderWidth, pScrl->borderWidth));
        MenuInfo.nScrollerSideOffset = imax(0, MFrame->borderWidth - pScrl->borderWidth);
    }

    // Menu separator line
	/* BlackboxZero 1.8.2012 */
    //MenuInfo.separatorColor = get_mixed_color(MFrame);
	MenuInfo.separatorColor = (mStyle.MenuSepColor != CLR_INVALID)?(mStyle.MenuSepColor):(get_mixed_color(MFrame));
	if ( Settings_menu.drawSeparators ) {
		//MenuInfo.separatorWidth = Settings_menu.drawSeparators ? imax(1, MFrame->borderWidth) : 0;

	} else
		MenuInfo.separatorWidth = 0;

    MenuInfo.check_is_pr = MHilite->parentRelative
        || iabs(greyvalue(get_bg_color(MFrame))
                - greyvalue(get_bg_color(MHilite))) < 24;
}

//===========================================================================

void Menu::ShowMenu()
{
    POINT pt;
    int w, h, flags, pos;

    flags = m_flags;
    m_flags &= BBMENU_HWND|BBMENU_SYSMENU|BBMENU_NOTITLE;
    pos = flags & BBMENU_POSMASK;

    GetCursorPos(&pt);

    // check display position options
    switch (pos) {
    case BBMENU_XY:
        pt.x = m_pos.left;
        pt.y = m_pos.top;
        break;
    case BBMENU_RECT:
        if (BBMENU_KBD & flags)
            pt.x = m_pos.left;
        pt.y = m_pos.bottom;
        break;
    case BBMENU_CENTER:
        break;
    case BBMENU_CORNER:
    default:
        if (BBMENU_KBD & flags) {
            pt.x = Settings_menu.pos.x;
            pt.y = Settings_menu.pos.y;
        }
        break;
    }

    // get monitor rectangle and set max-menu-height
    GetMonitorRect(&pt, &m_mon, GETMON_FROM_POINT);
    m_maxheight = (m_mon.bottom - m_mon.top) * 80 / 100;

    if (pos == BBMENU_RECT) {
        int h1 = m_pos.top - m_mon.top;
        int h2 = m_mon.bottom - m_pos.bottom;
        m_maxheight = imax(h1, h2);
    }

    Redraw(1);

    if (m_hwnd) {
        // when its a submenu currently:
        LinkToParentItem(NULL);
        UnHilite();
        if (false == (BBMENU_KBD & flags))
            m_kbditempos = -1;
    } else {
        // calculate frame sizes
        Validate();
    }

    // now adjust menu position according to menu-width/height
    w = m_width;
    h = m_height;

    switch (pos) {
    case BBMENU_XY:
        // explicit xy-position was given, handle right/bottom alignment
        if (flags & BBMENU_XRIGHT)
            pt.x -= w;
        if (flags & BBMENU_YBOTTOM)
            pt.y -= h;
        break;
    case BBMENU_RECT:
        // auto show below or above a rectangle (taskbar etc.)
        pt.x = imax(imin(pt.x + w/2, m_pos.right) - w, m_pos.left);
        if (pt.x + w >= m_mon.right)
            pt.x = m_pos.right - w;
        if (pt.y + h >= m_mon.bottom)
            pt.y = m_pos.top - h;
        break;
    case BBMENU_CENTER:
        // center menu on screen and set on top
        pt.x = (m_mon.left + m_mon.right - w) / 2;
        pt.y = (m_mon.top + m_mon.bottom - h) / 2;
        break;
    case BBMENU_CORNER:
        if (pt.x + w >= m_mon.right)
            pt.x -= w;
        if (pt.y + h >= m_mon.bottom)
            pt.y -= h;
        break;
    default:
        if (BBMENU_KBD & flags)
            break;
        // show centered at cursor;
        pt.x -= w/2;
        pt.y -= MenuInfo.nTitleHeight/2 + MenuInfo.nTitleMargin;
        break;
    }

    if (BBMENU_KBD & flags)
        m_kbdpos = true; // flag to write new position on user move

    if (BBMENU_PINNED & flags)
        SetPinned(true);
    else if (0 == (BBMENU_KBD & flags))
        SetPinned(false);

    if (BBMENU_ONTOP & flags)
        m_bOnTop = true;

    if (NULL == m_hwndRef && NULL == m_hwnd)
        m_hwndRef = GetFocus();

    Show(pt.x, pt.y, true);

    if (0 == (flags & BBMENU_NOFOCUS))
        bring_ontop(true);

    decref();
    ignore_mouse_msgs();
}

//===========================================================================
// API: MakeNamedMenu
// Purpose:         Create or refresh a Menu
// In: HeaderText:  the menu title
// In: IDString:    An unique string that identifies the menu window
// In: popup        true: menu is to be shown, false: menu is to be refreshed
// Out: Menu *:     A pointer to a Menu structure (opaque for the client)
// Note:            A menu once it has been created must be passed to
//                  either 'MakeSubMenu' or 'ShowMenu'.
//===========================================================================

Menu *MakeNamedMenu(const char* HeaderText, const char* IDString, bool popup)
{
    Menu *pMenu = NULL;
    if (IDString)
        pMenu = Menu::find_named_menu(IDString);

    if (pMenu) {
        pMenu->incref();
        pMenu->SaveState();
        pMenu->DeleteMenuItems();
        if (HeaderText)
            replace_str(&pMenu->m_pMenuItems->m_pszTitle, NLS1(HeaderText));
    } else {
        pMenu = new Menu(NLS1(HeaderText));
        pMenu->m_IDString = new_str(IDString);
    }
    pMenu->m_bPopup = popup;
    //dbg_printf("MakeNamedMenu (%d) %x %s <%s>", popup, pMenu, HeaderText, IDString);
    return pMenu;
}

//===========================================================================
// API: MakeMenu
// Purpose: as above, for menus that dont need refreshing
//===========================================================================
Menu *MakeMenu(const char* HeaderText)
{
    return MakeNamedMenu(HeaderText, NULL, true);
}

//===========================================================================
// API: DelMenu
// Purpose: obsolete
//===========================================================================

void DelMenu(Menu *PluginMenu)
{
    // Nothing here. We just dont know wether 'PluginMenu' still
    // exists. The pointer may be invalid or even belong to
    // a totally different memory object.
}

//===========================================================================
// API: ShowMenu
// Purpose: Finalizes creation or refresh for the menu and its submenus
// IN: PluginMenu - pointer to the toplevel menu
//===========================================================================

void ShowMenu(Menu *PluginMenu)
{
    if (NULL == PluginMenu)
        return;
    // dbg_printf("ShowMenu(%d) %x %s", PluginMenu->m_bPopup, PluginMenu, PluginMenu->m_pMenuItems->m_pszTitle);

    if (PluginMenu->m_bPopup) {
        // just to signal e.g. BBSoundFX
#ifndef BBXMENU
        PostMessage(BBhwnd, BB_MENU, BB_MENU_SIGNAL, 0);
#endif
        PluginMenu->ShowMenu();
    } else {
        PluginMenu->Redraw(1);
        PluginMenu->decref();
    }
}

//===========================================================================
// API: MakeSubmenu
//===========================================================================

MenuItem* MakeSubmenu(Menu *ParentMenu, Menu *ChildMenu, const char* Title)
{
    //dbg_printf("MakeSubmenu %x %s - %x %s", ParentMenu, ParentMenu->m_pMenuItems->m_pszTitle, ChildMenu, Title);
    if (Title)
        Title = NLS1(Title);
    else
        Title = ChildMenu->m_pMenuItems->m_pszTitle;
    return ParentMenu->AddMenuItem(new FolderItem(ChildMenu, Title));
}

//===========================================================================
// API: MakeMenuItem
//===========================================================================

MenuItem *MakeMenuItem(Menu *PluginMenu, const char* Title, const char* Cmd, bool ShowIndicator)
{
    //dbg_printf("MakeMenuItem %x %s", PluginMenu, Title);
    return PluginMenu->AddMenuItem(new CommandItem(Cmd, NLS1(Title), ShowIndicator));
}

//===========================================================================
// API: MakeMenuItemInt
//===========================================================================

MenuItem *MakeMenuItemInt(Menu *PluginMenu, const char* Title, const char* Cmd,
    int val, int minval, int maxval)
{
    return helper_menu(PluginMenu, Title, MENU_ID_INT,
        new IntegerItem(Cmd, val, minval, maxval));
}

//===========================================================================
// API: MakeMenuItemString
//===========================================================================

MenuItem *MakeMenuItemString(Menu *PluginMenu, const char* Title, const char* Cmd,
    const char* init_string)
{
    return helper_menu(PluginMenu, Title, MENU_ID_STRING,
        new StringItem(Cmd, init_string));
}

//===========================================================================
// API: MakeMenuNOP
//===========================================================================

MenuItem* MakeMenuNOP(Menu *PluginMenu, const char* Title)
{
	/* BlackboxZero 1.8.2012 - For separator graident? */
    MenuItem *pItem;
	if (Title && Title[0]) {
        pItem = new MenuItem(NLS1(Title));
	} else {
        pItem = new SeparatorItem();
	}
    pItem->m_bNOP = true;
    return PluginMenu->AddMenuItem(pItem);
}

//===========================================================================
// API: MakeMenuItemPath
//===========================================================================

MenuItem* MakeMenuItemPath(Menu *ParentMenu, const char* Title, const char* path, const char* Cmd)
{
    MenuItem *pItem;
    if (Title)
        pItem = new SpecialFolderItem(NLS1(Title), path, NULL, Cmd);
    else
        pItem = new SFInsert(path, Cmd);
    return ParentMenu->AddMenuItem(pItem);
}

//===========================================================================
// API: MenuExists
//===========================================================================

bool MenuExists(const char* IDString_part)
{
    return NULL != Menu::find_named_menu(IDString_part, true);
}

//===========================================================================
// API: MenuItemOption - set some options for a menuitem
//===========================================================================

void MenuItemOption(MenuItem *pItem, int option, ...)
{
    va_list vl;
    if (NULL == pItem)
        return;
    va_start(vl, option);
    switch (option) {

        // disabled text style
        case BBMENUITEM_DISABLED:
            pItem->m_bDisabled = true;
            if (NULL == pItem->m_pszRightCommand) /* hack for the taskmenus */
                pItem->m_bNOP = true;
            break;

        // set checkmark
        case BBMENUITEM_CHECKED:
            pItem->m_bChecked = true;
            break;

        // set a command for left click
        case BBMENUITEM_LCOMMAND:
            replace_str(&pItem->m_pszCommand, va_arg(vl, const char*));
            break;

        // set a command for right click
        case BBMENUITEM_RCOMMAND:
            replace_str(&pItem->m_pszRightCommand, va_arg(vl, const char*));
            break;

        // set a command for right click
        case BBMENUITEM_RMENU:
        {
            Menu *pSub = pItem->m_pRightmenu = va_arg(vl, Menu*);
            pSub->m_MenuID = MENU_ID_RMENU;
            break;
        }

        // set a special value and display text for the integer items
        case BBMENUITEM_OFFVAL:
        {
            IntegerItem* IntItem = (IntegerItem*)pItem->get_real_item();
            if (IntItem && IntItem->m_ItemID == MENUITEM_ID_INT) {
                const char *p;
                int n;
                if (NULL == IntItem)
                    break;
                n = va_arg(vl, int);
                p = va_arg(vl, const char*);
                IntItem->m_offvalue = n;
                IntItem->m_offstring = p ? p : NLS0("off");
            }
            break;
        }

        // set a flag that the checkmarks are updated each time
        case BBMENUITEM_UPDCHECK:
            pItem->m_ItemID |= MENUITEM_UPDCHECK;
            break;

        // set a special justify mode (DT_LEFT/DT_CENTER/DT_RIGHT)
        case BBMENUITEM_JUSTIFY:
            pItem->m_Justify = va_arg(vl, int);
            break;

//#ifdef BBOPT_MENUICONS
        // set an icon for this item by "path\to\icon[,#iconid]"
        case BBMENUITEM_SETICON:
			if ( Settings_menu.iconSize ) /* BlackboxZero 1.3.2012 */
				replace_str(&pItem->m_pszIcon, va_arg(vl, const char*));
            break;

        // set an icon for this item by HICON
        case BBMENUITEM_SETHICON:
			if ( Settings_menu.iconSize ) /* BlackboxZero 1.3.2012 */
            pItem->m_hIcon = CopyIcon(va_arg(vl, HICON));
            break;
//#endif
    }
}

//===========================================================================
// API: MenuOption - set some special features for a individual menu
//===========================================================================

void MenuOption(Menu *pMenu, int flags, ...)
{
    va_list vl;
    int pos;
    if (NULL == pMenu)
        return;

    va_start(vl, flags);

    pos = flags & BBMENU_POSMASK;
    pMenu->m_flags |= (flags & ~BBMENU_POSMASK) | pos;

    if (pos == BBMENU_XY) {
        pMenu->m_pos.left = va_arg(vl, int),
        pMenu->m_pos.top = va_arg(vl, int);
    } else if (pos == BBMENU_RECT)
        pMenu->m_pos = *va_arg(vl, RECT*);

    if (flags & BBMENU_MAXWIDTH)
        pMenu->m_maxwidth = va_arg(vl, int);

    if (flags & BBMENU_HWND)
        pMenu->m_hwndRef = va_arg(vl, HWND);

    if (flags & BBMENU_ISDROPTARGET)
        pMenu->m_bIsDropTarg = true;

    if (flags & BBMENU_SORT)
        Menu::Sort(&pMenu->m_pMenuItems->next, item_compare);
}

//===========================================================================

