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
*/ // generic Menu Items

#include "../BB.h"
#include "../Settings.h"
#include "MenuMaker.h"
#include "Menu.h"
#include <shlobj.h>
#include <shellapi.h>

int MenuItem::center_justify = DT_CENTER;
int MenuItem::left_justify   = DT_LEFT;

int g_menu_item_count;

//===========================================================================

//===========================================================================

MenuItem::MenuItem(const char* pszTitle)
{
	next        = NULL;

	m_pMenu     =
	m_pSubMenu  = NULL;

	m_nWidth    =
	m_nHeight   = 0;

	m_isNOP     =
	m_bActive   =
	m_isChecked = false;

	m_nSortPriority = M_SORT_NORMAL;
	m_ItemID    = MENUITEM_ID_NORMAL;

	m_pszTitle  = new_str(pszTitle);
	m_pszCommand = NULL;
	m_pidl      = NULL;
	m_hIcon     = NULL;
	m_pszIcon   = NULL;
	m_iconMode  = IM_NONE;

	++g_menu_item_count;
}

MenuItem::~MenuItem()
{
	UnlinkSubmenu();
free_str(&m_pszIcon);
	if (m_pszTitle) m_free(m_pszTitle);
	if (m_pszCommand) m_free(m_pszCommand);
	if (m_pidl) m_free(m_pidl);
	if (m_hIcon) DestroyIcon(m_hIcon);
	switch (m_iconMode)
	{
		case IM_PATH: m_free((char *)m_im_stuff); break;
	}

	--g_menu_item_count;
}

void MenuItem::LinkSubmenu(Menu *pSubMenu)
{
	UnlinkSubmenu();
	m_pSubMenu = pSubMenu;
}

void MenuItem::UnlinkSubmenu(void)
{
	if (m_pSubMenu)
	{
		Menu *pSubMenu = m_pSubMenu;
		m_pSubMenu = NULL;
		if (this==pSubMenu->m_pParentItem)
			pSubMenu->LinkToParentItem(NULL);
		pSubMenu->decref();
	}
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
		ShowSubMenu();
	}
}

//====================

// Mouse command MOUSEMOVE, LB_PRESSED, ...
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
			if (m_pMenu->m_dblClicked) i |= INVOKE_DBL;
			break;

		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetCapture(hwnd);

		case WM_MOUSEMOVE:
			Active(1);
			break;
	}

	if (i && m_bActive)// && 0 == m_isNOP)
	{
		Menu::g_DiscardMouseMoves = 5;
		Invoke(i);
	}
}

//====================

void MenuItem::Active(int active)
{
	bool new_active = active > 0;
	if (m_bActive == new_active) return;
	m_bActive = new_active;

	RECT r; GetItemRect(&r);
	InvalidateRect(m_pMenu->m_hwnd, &r, false);

	if (false == new_active)
	{
		m_pMenu->m_pActiveItem = NULL;
		return;
	}

	m_pMenu->UnHilite();
	m_pMenu->m_pActiveItem = this;
	//if (active == 1) m_pMenu->set_timer(true);
	if (active == 1) m_pMenu->set_timer(true, true);
}

//===========================================================================

void MenuItem::ShowSubMenu() {
	if (m_pSubMenu == m_pMenu->m_pChild)
		return;

	m_pMenu->HideChild();

	if (NULL == m_pSubMenu || m_pSubMenu->IsPinned())
		return;

	m_pSubMenu->m_bIconized = false;
	m_pSubMenu->m_bOnTop = m_pMenu->m_bOnTop;
	m_pSubMenu->LinkToParentItem(this);
	m_pSubMenu->Validate();
	m_pSubMenu->Show(0, 0, 0);
	m_pSubMenu->HideChild(); // submenu might be on the screen already
	PostMessage(BBhwnd, BB_SUBMENU, 0, 0); //for bbsoundfx
}

//===========================================================================

//===========================================================================

void MenuItem::GetItemRect(RECT* r)
{
	_SetRect(r, m_nLeft, m_nTop, m_nLeft + m_nWidth, m_nTop + m_nHeight);
}

void MenuItem::GetTextRect(RECT* r, int iconSize)
{
	r->top    = m_nTop;
	r->bottom = m_nTop  + m_nHeight;

	r->left   = m_nLeft + MenuInfo.nItemIndent[iconSize];
	r->right  = m_nLeft + m_nWidth - MenuInfo.nItemIndent[iconSize];
}

const char* MenuItem::GetDisplayString(void)
{
	if (Settings_menusBroamMode
		&& (MENUITEM_ID_CI & m_ItemID)
		&& m_pszCommand
		&& m_pszCommand[0] == '@')
		return m_pszCommand;

	return m_pszTitle;
}

//===========================================================================

void MenuItem::Measure(HDC hDC, SIZE *size)
{
	const char *title = GetDisplayString();
	int is = m_pMenu->m_iconSize;
	if (-2 == is) is = MenuInfo.nIconSize;
	if (m_isNOP && m_ItemID == MENUITEM_ID_NORMAL && 0 == title[0])
	{
		// empty [nop]: height = 66%
		size->cx = 0;
		if (Settings_compactSeparators == true) {
			size->cy = 5;
		} else {
			size->cy = MenuInfo.nItemHeight[is] * 5 / 8;
		}
		return;
	}

	RECT r = { 0, 0, 0, 0 };
	DrawText(hDC, title, -1, &r, DT_MENU_MEASURE_STANDARD);
	size->cx = r.right;
	size->cy = MenuInfo.nItemHeight[is];
}

//===========================================================================

#include "DrawIco.cpp"
void MenuItem::Paint(HDC hDC)
{
	RECT rect;

	COLORREF cr0 = (COLORREF)-1;
	bool lit = false;
	StyleItem *pSI = &mStyle.MenuFrame;

	if (m_bActive && 0 == (m_isNOP & (MI_NOP_TEXT | MI_NOP_SEP)) && (0 == (m_isNOP & MI_NOP_DISABLED) || m_pSubMenu)) {
		// draw hilite bar
		GetItemRect(&rect);
		pSI = &mStyle.MenuHilite;
		MakeStyleGradient(hDC, &rect, pSI, pSI->bordered);
		cr0 = SetTextColor(hDC, pSI->TextColor);
		lit = true;
	} else if (m_isNOP & MI_NOP_DISABLED) {
		cr0 = SetTextColor(hDC, mStyle.MenuFrame.disabledColor);
	}

	//dbg_printf("Menu separator style is: %s",Settings_menuSeparatorStyle);

	// draw separator
	if (m_isNOP & MI_NOP_LINE) {
		int x, y = m_nTop + m_nHeight / 2;
		// Noccy: Looks like we have to remove some pixels here to prevent it from overwriting the right border.
		int left  = m_nLeft + ((Settings_menuFullSeparatorWidth)?1:mStyle.MenuSepMargin) - 1;
		int right = m_nLeft + m_nWidth - ((Settings_menuFullSeparatorWidth)?1:mStyle.MenuSepMargin);
		// int dist = (m_nWidth + 1) / 2 - ((Settings_menuFullSeparatorWidth==true)?mStyle.MenuFrame.borderWidth:mStyle.MenuSepMargin);
		int dist = (m_nWidth+1) / 2 - ((Settings_menuFullSeparatorWidth)?1:mStyle.MenuSepMargin);
		COLORREF c = mStyle.MenuSepColor;
		COLORREF cs = mStyle.MenuSepShadowColor;//pSI->ShadowColor; /*12.08.2011*/

		if (pSI->ShadowXY)
		{
			int yS = y + pSI->ShadowY;
			int leftS  = left + pSI->ShadowX;
			int rightS = right + pSI->ShadowX;
			if (0 == stricmp(Settings_menuSeparatorStyle,"gradient"))
			{
				// Gradient shadow
				for (x = 0; x <= dist; ++x)
				{
					int pos, hue = x * 255 / dist;
					pos = leftS + x; SetPixel(hDC, pos, yS, mixcolors(cs, GetPixel(hDC, pos, y), hue));
					pos = rightS - x; SetPixel(hDC, pos, yS, mixcolors(cs, GetPixel(hDC, pos, y), hue));
				}
			} else
			if (0 == stricmp(Settings_menuSeparatorStyle,"flat"))
			{
				// Flat shadow
				for (x = 0; x <= dist; ++x)
				{
					int pos;
					pos = leftS + x; SetPixel(hDC, pos, yS, cs);
					pos = rightS - x; SetPixel(hDC, pos, yS, cs);
				}
			} else
			if (0 == stricmp(Settings_menuSeparatorStyle,"bevel"))
			{
				// Bevel shadow is simply none...
			}
		}
		if (0 == stricmp(Settings_menuSeparatorStyle,"gradient"))
		{
			for (x = 0; x <= dist; ++x)
			{
				int pos, hue = x * 255 / dist;
				pos = left + x; SetPixel(hDC, pos, y, mixcolors(c, GetPixel(hDC, pos, y), hue));
				pos = right - x; SetPixel(hDC, pos, y, mixcolors(c, GetPixel(hDC, pos, y), hue));
			}
		} else
		if (0 == stricmp(Settings_menuSeparatorStyle,"flat"))
		{
			for (x = 0; x <= dist; ++x)
			{
				int pos; //, hue = x * 255 / dist;
				pos = left + x; SetPixel(hDC, pos, y, c);
				pos = right - x; SetPixel(hDC, pos, y, c);
			}
		} else
		if (0 == stricmp(Settings_menuSeparatorStyle,"bevel"))
		{
			for (x = 0; x <= dist; ++x)
			{
				int pos;
				pos = left + x; SetPixel(hDC, pos, y, mixcolors(0x00000000, GetPixel(hDC, pos, y), 160));
				pos = right - x; SetPixel(hDC, pos, y, mixcolors(0x00000000, GetPixel(hDC, pos, y), 160));
				pos = left + x; SetPixel(hDC, pos, y+1, mixcolors(0x00FFFFFF, GetPixel(hDC, pos, y+1), 160));
				pos = right - x; SetPixel(hDC, pos, y+1, mixcolors(0x00FFFFFF, GetPixel(hDC, pos, y+1), 160));
			}
		}
	}

	int iconSize = m_pMenu->m_iconSize;
	if (-2 == iconSize) iconSize = MenuInfo.nIconSize;
	GetTextRect(&rect, iconSize);

	// [load and ]draw menu item icon
	if (iconSize) {
		bool bSmallIcon = (16 >= iconSize);
		// load menu item icon
		if (NULL == m_hIcon || bSmallIcon != m_bSmallIcon) {
			DestroyIcon(m_hIcon), m_hIcon = NULL;
			m_bSmallIcon = bSmallIcon;
			switch (m_iconMode) {
				case IM_PIDL:
					{
						const _ITEMIDLIST *pidl = (MENUITEM_ID_SF == m_ItemID) ?
							((SpecialFolderItem*)this)->check_pidl() : m_pidl;

						if (pidl) {
							SHFILEINFO sfi;
							HIMAGELIST sysimgl = (HIMAGELIST)SHGetFileInfo((LPCSTR)pidl, 0, &sfi, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_SYSICONINDEX | (bSmallIcon ? SHGFI_SMALLICON : SHGFI_LARGEICON));
							if (sysimgl) m_hIcon = ImageList_GetIcon(sysimgl, sfi.iIcon, ILD_NORMAL);
						}
					}
					break;

				case IM_TASK:
					{
						const struct tasklist *tl = (struct tasklist *)m_im_stuff;
						m_hIcon = CopyIcon(bSmallIcon ? tl->icon : tl->icon_big);
					}
					break;

				case IM_PATH:
					{
						char *icon = (char *)m_im_stuff;

						if (strchr(icon,';') == NULL) {
							// Load icon
							char *path = strrchr(icon, ',');
							int idx;
							if (path)
								idx = atoi(path + 1), *path = 0;
							else
								idx = 0;
							if (bSmallIcon)
								ExtractIconEx(icon, idx, NULL, &m_hIcon, 1);
							else
								ExtractIconEx(icon, idx, &m_hIcon, NULL, 1);
							if (path) *path = ',';
						} else {
							// Load resource icon
							// LoadLibrary()...
							// LoadIcon(0, MAKEINTRESOURCE(100));
							// Copy icon
							// FreeLibrary()...
						}
					}
					break;
			}
		}

		// draw menu item icon
		if (m_hIcon)
		{
			int top = rect.top + (m_nHeight - iconSize) / 2;
			int adjust = (MenuInfo.nItemIndent[iconSize] - iconSize) / 2;
			int left = ((DT_LEFT == FolderItem::m_nBulletPosition) ? rect.right : m_nLeft) + adjust;
			drawIco(left, top, iconSize, m_hIcon, hDC, !m_bActive, Settings_menuIconSaturation, Settings_menuIconHue);
		}
	}


	/*
		Noccy: Added DT_NOPREFIX to BBDrawText to prevent ampersand (&) to be interpreted as
		a hotkey.
		Note: Reverted.
	*/

	// draw menu item text
	const char *title = GetDisplayString();
	if ( !Settings_menuIconSize ) {
		if ( m_pSubMenu && m_pSubMenu->m_IDString ) {
			char	tmp[128];

			sprintf(tmp, "%s", m_pSubMenu->m_IDString);
			if ( stricmp(tmp, "IDroot_workspaces_setup") == 0 ||
				 stricmp(tmp, "IDroot_icons") == 0 ||
			   ( m_pSubMenu->m_IDString[0] == 'I' && m_pSubMenu->m_IDString[7] == 'w' && m_pSubMenu->m_IDString[15] == 'e' && stricmp(tmp, "IDroot_workspaces") ) 
			   ) {
				   rect.left = 12;
				}
		}	
	}

	if (0 == (m_ItemID & (~MENUITEM_ID_CI & (MENUITEM_ID_CIInt|MENUITEM_ID_CIStr))) || Settings_menusBroamMode)
		BBDrawText(hDC, title, -1, &rect, mStyle.MenuFrame.Justify | DT_MENU_STANDARD, pSI);
	else
	if (m_ItemID != MENUITEM_ID_CIStr)
		BBDrawText(hDC, title, -1, &rect, DT_CENTER | DT_MENU_STANDARD, pSI);

	// set back previous textColor
	if ((COLORREF)-1 != cr0) SetTextColor(hDC, cr0);

	if (m_isChecked) // draw check-mark
	{
		pSI = &mStyle.MenuHilite;
		bool pr = pSI->parentRelative;
		if (lit != pr) pSI = &mStyle.MenuFrame;

		int x, y = m_nTop + m_nHeight / 2;
		if ( (FolderItem::m_nBulletPosition == DT_RIGHT) == (0 == (MENUITEM_ID_FOLDER & m_ItemID)) ) {
			Settings_menuIconSize ? (x = m_nLeft + m_nWidth - MenuInfo.nItemIndent[iconSize] / 2 - 1) : (x = m_nLeft + m_nWidth - 8);
		} else {
			if ( Settings_menuIconSize ) {
				x = m_nLeft + MenuInfo.nItemIndent[iconSize] / 2;
			} else {
				x = (m_nLeft + 6);
			}
		}

		const int r = 3;
		rect.left   = x - r;
		rect.right  = x + r;
		rect.top    = y - r;
		rect.bottom = y + r;

		if (pr) MakeGradient(hDC, rect, B_SOLID, pSI->TextColor, 0, false, BEVEL_FLAT, 0, 0, 0, 0);
		else MakeStyleGradient(hDC, &rect, pSI, false);
	}
}

//===========================================================================
