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

// generic Menu Items

#include "../BB.h"
#include "../Settings.h"
#include "Menu.h"

MenuItem::MenuItem(const char* pszTitle)
{
    memset(&next, 0, sizeof *this - sizeof(void*));

    m_Justify   = MENUITEM_STANDARD_JUSTIFY;
    //m_ItemID    = MENUITEM_ID_NORMAL;
    //m_nSortPriority = M_SORT_NORMAL;
    m_pszTitle  = new_str(pszTitle);

    ++g_menu_item_count;
}

MenuItem::~MenuItem()
{
    UnlinkSubmenu();
    if (m_pRightmenu)
        m_pRightmenu->decref();
    free_str(&m_pszTitle);
    free_str(&m_pszCommand);
    free_str(&m_pszRightCommand);
    delete_pidl_list(&m_pidl_list);

//#ifdef BBOPT_MENUICONS
    free_str(&m_pszIcon);
    if (m_hIcon)
        DestroyIcon(m_hIcon);
//#endif

    --g_menu_item_count;
}

void MenuItem::LinkSubmenu(Menu *pSubMenu)
{
    UnlinkSubmenu();
    m_pSubmenu = pSubMenu;
}

void MenuItem::UnlinkSubmenu(void)
{
    if (m_pSubmenu)
    {
        Menu *sub = m_pSubmenu;
        m_pSubmenu = NULL;

        if (this == sub->m_pParentItem)
            // i.e. the submenu is currently visible
            sub->LinkToParentItem(NULL);

        sub->decref();
    }
}

void MenuItem::ShowRightMenu(Menu *pSub)
{
    if (pSub) {
        m_pMenu->HideChild();
        LinkSubmenu(pSub);
        ShowSubmenu();
    }
}

#ifndef BBTINY
void MenuItem::ShowContextMenu(const char *path, LPCITEMIDLIST pidl)
{
    ShowRightMenu(MakeContextMenu(path, pidl));
}
#endif

LPCITEMIDLIST MenuItem::GetPidl(void)
{
    return m_pidl_list ? first_pidl(m_pidl_list) : NULL;
}

//===========================================================================
// Execute the command that is associated with the menu item
void MenuItem::Invoke(int button)
{
    // default implementation is a nop
}

//====================
void MenuItem::Key(UINT nMsg, WPARAM wParam)
{
    // default implementation is a nop
}

//====================
void MenuItem::ItemTimer(UINT nTimer)
{
    if (MENU_POPUP_TIMER == nTimer)
    {
        ShowSubmenu();
    }
}

//====================
// Mouse commands:

void MenuItem::Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam)
{
    int i = 0;
    switch(uMsg)
    {
        case WM_MBUTTONUP:
            i = INVOKE_MID;
            break;

        case WM_RBUTTONUP:
            i = INVOKE_RIGHT;
            break;

        case WM_LBUTTONUP:
            i = INVOKE_LEFT;
            if (m_pMenu->m_dblClicked)
                i |= INVOKE_DBL;

            if (0x8000 & GetAsyncKeyState(VK_MENU))
            {
                if (i & INVOKE_DBL)
                    i = INVOKE_PROP;
                else
                    i = 0;
            }
            break;

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            m_pMenu->set_capture(MENU_CAPT_ITEM);
            Active(1);
            break;

        case WM_MOUSEMOVE:
            if (m_pMenu->m_captureflg && hwnd != window_under_mouse())
            {
                // start drag operation on mouseleave
                m_pMenu->set_capture(0);
                i = INVOKE_DRAG;
                break;
            }
            Active(1);
            break;

        case WM_MOUSEWHEEL:
            m_pMenu->HideChild();
            Active(2);
            break;
    }

    if (i && m_bActive && false == m_bNOP)
        Invoke(i);
}

//====================

void MenuItem::Active(int active)
{
    bool new_active;
    RECT r;

    new_active = active > 0;
    if (m_bActive == new_active)
        return;
    m_bActive = new_active;

    GetItemRect(&r);
    InvalidateRect(m_pMenu->m_hwnd, &r, false);

    if (false == new_active) {
        m_pMenu->m_pActiveItem = NULL;
        return;
    }

    m_pMenu->UnHilite();
    m_pMenu->m_pActiveItem = this;

    if (active == 1)
        m_pMenu->set_timer(true, true);

    if (m_pMenu->m_MenuID == MENU_ID_STRING && bbactive) {
        if (m_ItemID == MENUITEM_ID_STR)
            SetFocus(m_pMenu->m_hwndChild);
        else
            SetFocus(m_pMenu->m_hwnd);
    }
}

//====================

void MenuItem::ShowSubmenu()
{
    if (m_pMenu->m_pChild)
    {
        if (this == m_pMenu->m_pChild->m_pParentItem)
            return;
        m_pMenu->HideChild();
    }

    if (NULL == m_pSubmenu
     || m_pSubmenu->m_bPinned
     || m_bDisabled)
        return;

    m_pSubmenu->Hide();

    // inherit some values from parent menu
    m_pSubmenu->m_bOnTop = m_pMenu->m_bOnTop;
    m_pSubmenu->m_flags |= m_pMenu->m_flags & BBMENU_NOTITLE;
    m_pSubmenu->m_maxheight = m_pMenu->m_maxheight;
    m_pSubmenu->m_mon = m_pMenu->m_mon;

    m_pSubmenu->Validate();
    m_pSubmenu->LinkToParentItem(this);
    m_pSubmenu->Show(0, 0, true);
    PostMessage(BBhwnd, BB_SUBMENU, 0, 0); //for bbsoundfx
}

//===========================================================================

//===========================================================================

void MenuItem::GetItemRect(RECT* r)
{
    r->top    = m_nTop;
    r->bottom = m_nTop + m_nHeight;
    r->left   = m_nLeft;
    r->right  = m_nLeft + m_nWidth;
}

void MenuItem::GetTextRect(RECT* r)
{
    r->top    = m_nTop;
    r->bottom = m_nTop  + m_nHeight;
    r->left   = m_nLeft + MenuInfo.nItemLeftIndent;
    r->right  = m_nLeft + m_nWidth - MenuInfo.nItemRightIndent;
}

const char* MenuItem::GetDisplayString(void)
{
    if (Settings_menu.showBroams)
    {
        if ((MENUITEM_ID_CMD & m_ItemID)
            && m_pszCommand
            && m_pszCommand[0] == '@')
            return m_pszCommand;
#if 0
        if (m_pMenu->m_pMenuItems == this && m_pMenu->m_IDString)
            return m_pMenu->m_IDString;
#endif
    }
    return m_pszTitle;
}

//===========================================================================

void MenuItem::Measure(HDC hDC, SIZE *size)
{
    const char *title = GetDisplayString();
    RECT r = { 0, 0, 0, 0 };
    bbDrawText(hDC, title, &r, DT_MENU_MEASURE_STANDARD, 0);
    size->cx = r.right;
    size->cy = MenuInfo.nItemHeight;
//#ifdef BBOPT_MENUICONS
    if (m_hIcon && Settings_menu.iconSize)/* BlackboxZero 1.3.2012 */
        size->cy = imax(MenuInfo.nIconSize+2, size->cy);
//#endif
}

//===========================================================================

void MenuItem::Paint(HDC hDC)
{
    RECT rc, rhi;
    StyleItem *pSI;
    COLORREF TC, BC;
    int j;

    GetTextRect(&rc);
    pSI = &mStyle.MenuFrame;

    if (m_bActive && false == m_bNOP) {
        // draw hilite bar
        GetItemRect(&rhi);
        pSI = &mStyle.MenuHilite;
        MakeStyleGradient(hDC, &rhi, pSI, pSI->bordered);
        TC = pSI->TextColor;
        BC = pSI->foregroundColor;
    } else if (m_bDisabled) {
        BC = TC = pSI->disabledColor;
    } else {
        TC = pSI->TextColor;
        BC = pSI->foregroundColor;
    }

    j = m_Justify;
    if (MENUITEM_CUSTOMTEXT != j) {
        if (MENUITEM_STANDARD_JUSTIFY == j)
            j = mStyle.MenuFrame.Justify;
        // draw menu item text
        //bbDrawText(hDC, GetDisplayString(), &rc, j | DT_MENU_STANDARD, TC);
		/* BlackboxZero 1.5.2012 */
		BBDrawText(hDC, GetDisplayString(), -1, &rc, j | DT_MENU_STANDARD, pSI);
    }

//#ifdef BBOPT_MENUICONS
	if ( Settings_menu.iconSize ) /* BlackboxZero 1.3.2012 */
		this->DrawIcon(hDC);
//#endif

    if (m_bChecked) // draw check-mark
    {
        int d, atright;

//#ifdef BBOPT_MENUICONS
		if ( Settings_menu.iconSize ) { /* BlackboxZero 1.3.2012 */
			if (m_ItemID & MENUITEM_ID_FOLDER)
				atright = MenuInfo.nBulletPosition == FOLDER_LEFT;
			else
				atright = true;
		} else {
//#else
			if (MenuInfo.nItemLeftIndent != MenuInfo.nItemRightIndent)
				atright = MenuInfo.nBulletPosition != FOLDER_LEFT;
			else
			if (m_ItemID & MENUITEM_ID_FOLDER)
				atright = MenuInfo.nBulletPosition == FOLDER_LEFT;
			else
				atright = j != DT_LEFT;
		}
//#endif
        rc.bottom = (rc.top = m_nTop) + m_nHeight + 1;
        if (atright) {
            d = MenuInfo.nItemRightIndent + mStyle.MenuHilite.borderWidth;
            rc.left = (rc.right = m_nLeft + m_nWidth) - d + 1;
        } else {
            d = MenuInfo.nItemLeftIndent + mStyle.MenuHilite.borderWidth;
            rc.right = (rc.left = m_nLeft) + d;
        }

#if 1
        bbDrawPix(hDC, &rc, BC, BS_CHECK);
#else
        {
            bool pr, lit;
            const int w = 6;
            pSI = &mStyle.MenuHilite;
            pr = pSI->parentRelative;
            lit = m_bActive && false == m_bNOP;
            if (lit != pr)
                pSI = &mStyle.MenuFrame;
            rc.left   = (rc.left + rc.right - w)/2;
            rc.right  = rc.left + w;
            rc.top    = (rc.top + rc.bottom - w)/2;
            rc.bottom = rc.top + w;
            if (pr)
                MakeGradient(hDC, rc, B_SOLID, pSI->TextColor, 0, false, BEVEL_FLAT, 0, 0, 0, 0);
            else
                MakeStyleGradient(hDC, &rc, pSI, false);
        }
#endif
    }
}

//===========================================================================

void SeparatorItem::Measure(HDC hDC, SIZE *size)
{
    if (Settings_menu.drawSeparators)
        //size->cy = MenuInfo.separatorWidth + imax(MenuInfo.nItemHeight*2/5 & ~1, 2*mStyle.MenuFrame.marginWidth);
		/* BlackboxZero 1.8.2012 */
		size->cy = Settings_menu.separatorCompact?(5):(MenuInfo.separatorWidth + 2*mStyle.MenuFrame.marginWidth);
    else
        size->cy = MenuInfo.nItemHeight*3/5;

    size->cx = 0;
}

void SeparatorItem::Paint(HDC hDC)
{
	/* BlackboxZero 1.8.2012
	** Dont' use original..
    RECT rect; int y, d;
    if (false == Settings_menu.drawSeparators)
        return;
    GetTextRect(&rect);
    d = MenuInfo.separatorWidth;
    y = (rect.top + rect.bottom - d) / 2;
    draw_line_h(hDC, rect.left, rect.right, y, d, MenuInfo.separatorColor);
	** */

	StyleItem *pSI = &mStyle.MenuFrame;
	int x, y = m_nTop + m_nHeight / 2;
	// Noccy: Looks like we have to remove some pixels here to prevent it from overwriting the right border.
	int left  = m_nLeft + ((Settings_menu.separatorFullWidth)?1:mStyle.MenuSepMargin) - 1;
	int right = m_nLeft + m_nWidth - ((Settings_menu.separatorFullWidth)?1:mStyle.MenuSepMargin);
	// int dist = (m_nWidth + 1) / 2 - ((Settings_menuFullSeparatorWidth==true)?mStyle.MenuFrame.borderWidth:mStyle.MenuSepMargin);
	int dist = (m_nWidth+1) / 2 - ((Settings_menu.separatorFullWidth)?1:mStyle.MenuSepMargin);
	COLORREF c = mStyle.MenuSepColor;
	COLORREF cs = mStyle.MenuSepShadowColor;//pSI->ShadowColor; /*12.08.2011*/

	if (pSI->ShadowXY) {
		int yS = y + pSI->ShadowY;
		int leftS  = left + pSI->ShadowX;
		int rightS = right + pSI->ShadowX;
		if (0 == stricmp(Settings_menu.separatorStyle,"gradient")) {
			// Gradient shadow
			for (x = 0; x <= dist; ++x) {
				int pos, hue = x * 255 / dist;
				pos = leftS + x;
				SetPixel(hDC, pos, yS, mixcolors(cs, GetPixel(hDC, pos, y), hue));
				pos = rightS - x;
				SetPixel(hDC, pos, yS, mixcolors(cs, GetPixel(hDC, pos, y), hue));
			}
		} else if (0 == stricmp(Settings_menu.separatorStyle,"flat")) {
			// Flat shadow
			for (x = 0; x <= dist; ++x) {
				int pos;
				pos = leftS + x;
				SetPixel(hDC, pos, yS, cs);
				pos = rightS - x;
				SetPixel(hDC, pos, yS, cs);
			}
		} /*else if (0 == stricmp(Settings_menu.separatorStyle,"bevel")) {
				// Bevel shadow is simply none...
		}*/
	}

	//Draw Separator
	if (0 == stricmp(Settings_menu.separatorStyle,"gradient")) {
		for (x = 0; x <= dist; ++x) {
			int pos, hue = x * 255 / dist;
			pos = left + x;
			SetPixel(hDC, pos, y, mixcolors(c, GetPixel(hDC, pos, y), hue));
			pos = right - x;
			SetPixel(hDC, pos, y, mixcolors(c, GetPixel(hDC, pos, y), hue));
		}
	} else if (0 == stricmp(Settings_menu.separatorStyle,"flat")) {
		for (x = 0; x <= dist; ++x) {
			int pos; //, hue = x * 255 / dist;
			pos = left + x; SetPixel(hDC, pos, y, c);
			pos = right - x; SetPixel(hDC, pos, y, c);
		}
	} else if (0 == stricmp(Settings_menu.separatorStyle,"bevel")) {
		for (x = 0; x <= dist; ++x) {
			int pos;
			pos = left + x; SetPixel(hDC, pos, y, mixcolors(0x00000000, GetPixel(hDC, pos, y), 160));
			pos = right - x; SetPixel(hDC, pos, y, mixcolors(0x00000000, GetPixel(hDC, pos, y), 160));
			pos = left + x; SetPixel(hDC, pos, y+1, mixcolors(0x00FFFFFF, GetPixel(hDC, pos, y+1), 160));
			pos = right - x; SetPixel(hDC, pos, y+1, mixcolors(0x00FFFFFF, GetPixel(hDC, pos, y+1), 160));
		}
	}
}

//===========================================================================
//#ifdef BBOPT_MENUICONS

#include <shellapi.h>
#include "../../plugins/bbPlugin/drawico.cpp"

void MenuItem::DrawIcon(HDC hDC)
{
    int size, px, py, d;
/*
    if (m_ItemID & MENUITEM_ID_FOLDER)
        return;
*/
    if (NULL == m_hIcon) {

        // if no icon yet, try to load it, either from explicit string
        // or from pidl_list

        if (m_pszIcon) {
            char path[MAX_PATH];
            const char *p;
            int index;

            p = Tokenize(m_pszIcon, path, ",");
            index = 0;
            if (p) {
                index = atoi(p);
                if (index)
                    --index;
            }
            unquote(path);
            ExtractIconEx(path, index, NULL, &m_hIcon, 1);

        } else if (m_pidl_list) {
			m_hIcon = sh_geticon(first_pidl(m_pidl_list), (Settings_menu.iconSize>16)?(32):(16));

        }

        if (NULL == m_hIcon)
            return;
    }

    size = MenuInfo.nIconSize;
    d = (m_nHeight - size) / 2;
    px = m_nLeft + d;
    py = m_nTop + d;
/*
    DrawIconEx(hDC,
        px, py, m_hIcon,
        size, size, 0,
        NULL, DI_NORMAL
        );
*/
    DrawIconSatnHue(hDC,
        px, py, m_hIcon,
        //size, size, 0,
		Settings_menu.iconSize, Settings_menu.iconSize, 0, /* BlackboxZero 1.4.2012 */
        NULL, DI_NORMAL,
        //false == m_bActive, 40, 0
		false == m_bActive, Settings_menu.iconSaturation, Settings_menu.iconHue /* BlackboxZero 1.3.2012 */
        );
}
//#endif

//===========================================================================
