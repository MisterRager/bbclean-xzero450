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
	m_pszTitle_old = new_str(pszTitle);
	m_pszCommand = NULL;
	m_hIcon     = NULL;
	m_pszIcon   = NULL;
	m_pidl      = NULL;
	m_pSI       = NULL;

	++g_menu_item_count;
}

MenuItem::~MenuItem()
{
	UnlinkSubmenu();
	free_str(&m_pszTitle);
	free_str(&m_pszTitle_old);
	free_str(&m_pszCommand);
	free_str(&m_pszIcon);
	if (m_hIcon) DestroyIcon(m_hIcon);
	if (m_pidl) m_free(m_pidl);
	if (m_pSI) m_free(m_pSI);
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

	if (0 == (m_isNOP & (MI_NOP_TEXT | MI_NOP_SEP | MI_NOP_DISABLED))){
		RECT r; GetItemRect(&r);
		InvalidateRect(m_pMenu->m_hwnd, &r, false);
	}

	if (false == new_active)
	{
		m_pMenu->m_pActiveItem = NULL;
		return;
	}

	m_pMenu->UnHilite();
	m_pMenu->m_pActiveItem = this;
	if (active == 1) m_pMenu->set_timer(true);
}

//===========================================================================

void MenuItem::ShowSubMenu()
{
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

void MenuItem::GetTextRect(RECT* r)
{
	int nIndent = MenuInfo.nItemIndent;
	_SetRect(r, m_nLeft + nIndent, m_nTop, m_nLeft + m_nWidth - nIndent, m_nTop + m_nHeight);
	MenuItem* item;
	dolist(item, m_pMenu->m_pMenuItems){
		if(item->m_hIcon){
			if (FolderItem::m_nBulletPosition != DT_LEFT){
				_OffsetRect(r, MenuInfo.nIconItemIndent - nIndent, 0);
				break;
			}
		}
	}
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
	RECT r = { 0, 0, 0, 0 };
	DrawText(hDC, title, -1, &r, DT_MENU_MEASURE_STANDARD);
	if (m_isNOP && m_ItemID == MENUITEM_ID_NORMAL && 0 == title[0])
	{
		// empty [nop]: height = 66%
		size->cx = 0;
		size->cy = MenuInfo.nItemHeight * 5 / 8;
	}
	else if(m_hIcon){
		size->cx = r.right + MenuInfo.nIconItemIndent - MenuInfo.nItemIndent;
		size->cy = MenuInfo.nIconItemHeight;
	}
	else{
		size->cx = r.right;
		size->cy = MenuInfo.nItemHeight;
		MenuItem *item;
		dolist(item, m_pMenu->m_pMenuItems){
			if(item->m_hIcon){
				size->cx = r.right + MenuInfo.nIconItemIndent - MenuInfo.nItemIndent;
				break;
			}
		}
	}
}

//===========================================================================

void MenuItem::Paint(HDC hDC)
{
	RECT rect;

	StyleItem styleitem = mStyle.MenuFrame;
	bool lit = false;

	if (m_bActive && 0 == (m_isNOP & (MI_NOP_TEXT | MI_NOP_SEP)) && (0 == (m_isNOP & MI_NOP_DISABLED) || m_pSubMenu))
	{
		// draw hilite bar
		GetItemRect(&rect);
		styleitem = mStyle.MenuHilite;
		MakeStyleGradient(hDC, &rect, &styleitem, styleitem.bordered);
		lit = true;
	}
	else if (m_isNOP & MI_NOP_DISABLED)
		styleitem.TextColor = mStyle.MenuFrame.disabledColor;
	else if (m_pSI && !m_bActive)
		styleitem = *m_pSI;

	// draw menu item text
	const char *title = GetDisplayString();
	GetTextRect(&rect);

	if (0 == (m_ItemID & (~MENUITEM_ID_CI & (MENUITEM_ID_CIInt|MENUITEM_ID_CIStr))) || Settings_menusBroamMode)
		BBDrawText(hDC, title, -1, &rect, mStyle.MenuFrame.Justify | DT_MENU_STANDARD, &styleitem);
	else if (m_ItemID != MENUITEM_ID_CIStr)
		BBDrawText(hDC, title, -1, &rect, DT_CENTER | DT_MENU_STANDARD, &styleitem);

	if (m_isChecked) // draw check-mark
	{
		styleitem = mStyle.MenuHilite;
		bool pr = styleitem.parentRelative;
		if (lit != pr) styleitem = mStyle.MenuFrame;

		int x, y = m_nTop + m_nHeight / 2;
		if ((FolderItem::m_nBulletPosition == DT_RIGHT) == (0 == (MENUITEM_ID_FOLDER & m_ItemID)))
			x = m_nLeft + m_nWidth - MenuInfo.nItemIndent / 2 - 1;
		else
			x = m_nLeft + MenuInfo.nItemIndent / 2;

		const int r = 3;
		_SetRect(&rect ,x - r ,y - r ,x + r ,y + r);

		if (pr) MakeGradient(hDC, rect, B_SOLID, styleitem.TextColor, 0, false, BEVEL_FLAT, 0, 0, 0, 0);
		else MakeStyleGradient(hDC, &rect, &styleitem, false);
	}

	// draw icon
	DrawIcon(hDC);
}

//===========================================================================

void MenuItem::GetIcon(){
		int nIconSize = Settings_menuIconSize;
	if(m_pszIcon && m_pszIcon[0]){
		if (nIconSize){
			char *pszPath = NULL; // menu icon path(ico, exe, dll, icl ...)
			char *pszTemp = NULL; // temporary
			int nIndex = 0;       // default icon index is 0

			strcpy(pszTemp = (char*)m_alloc(sizeof(char)*strlen(m_pszIcon)+1), m_pszIcon);

			// get icon index
			// separate path and icon with ","
			if (char *p = strrchr(pszTemp, ',')){
				*p = '\0';
				nIndex = atoi(p+1);
			}

			// delete the double quotation included in the path 
			unquote(pszPath = (char*)m_alloc(sizeof(char)*strlen(pszTemp)+1), pszTemp);

			m_free(pszTemp);

			// get icon handle
			if (nIconSize <= 16){ // small icon
				ExtractIconEx(pszPath, nIndex, NULL, &m_hIcon, 1);
			}
			else{ // large icon
				ExtractIconEx(pszPath, nIndex, &m_hIcon, NULL, 1);
			}

			m_free(pszPath);
		}
	}
}

//===========================================================================

void MenuItem::DrawIcon(HDC hDC){
	if(m_hIcon){
		RECT rect; GetItemRect(&rect);
		int nIconSize = Settings_menuIconSize;
		if (FolderItem::m_nBulletPosition != DT_LEFT){
			rect.left += (MenuInfo.nIconItemIndent - nIconSize)/2;
		}
		else{
			rect.left = rect.right - (MenuInfo.nIconItemIndent + nIconSize)/2;
		}
		rect.top += (MenuInfo.nIconItemHeight - nIconSize)/2;
		DrawIconEx(hDC, rect.left, rect.top, m_hIcon, nIconSize, nIconSize, 0, 0, DI_NORMAL);
	}
}

//===========================================================================
