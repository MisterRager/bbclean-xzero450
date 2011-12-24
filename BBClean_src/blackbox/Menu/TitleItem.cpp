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
*/ // the menu title bar

#include "../BB.h"
#include "../Settings.h"
#include "MenuMaker.h"
#include "Menu.h"

//===========================================================================

void TitleItem::Paint(HDC hDC)
{
    StyleItem *pSI = &mStyle.MenuTitle;
    RECT rect; GetItemRect(&rect);
    int spacing = MenuInfo.nTitleIndent;
    int just = pSI->Justify | DT_MENU_STANDARD;
    int bw = pSI->borderWidth;
    COLORREF bc = pSI->borderColor;

    rect.bottom -= bw;
    if (false == pSI->parentRelative)
    {
        MakeStyleGradient(hDC, &rect, pSI, false);
        if (bw) draw_line_h(hDC, rect.left, rect.right, rect.bottom, bw, bc);
	}

    _InflateRect(&rect, -spacing, 0);

	if (pSI->parentRelative)
	{
        if (bw) draw_line_h(hDC, rect.left, rect.right, rect.bottom, bw, bc);
		rect.bottom -= MenuInfo.nTitlePrAdjust;
		just = just & ~DT_VCENTER | DT_BOTTOM;
	}

    if (pSI->FontHeight)
        BBDrawText(hDC, GetDisplayString(), -1, &rect, just, pSI);
}

//===========================================================================

//===========================================================================
//#define MENU_ROLLUP

void TitleItem::Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam)
{
    Menu *p = m_pMenu;

    switch(uMsg) {
    case WM_RBUTTONUP:
        if (m_bActive)
        {
#ifdef MENU_ROLLUP
            if (0x8000 & GetAsyncKeyState(VK_MENU))
            {
                p->m_bOnTop = false == p->m_bOnTop;
                p->SetZPos();
            }
            else
#endif
            if (wParam & MK_SHIFT)
                ShowContextMenu(NULL, m_pidl);
            else
                p->HideThis();
        }
        break;

    case WM_LBUTTONDBLCLK:
#ifdef MENU_ROLLUP
        p->m_bIconized = false == p->m_bIconized;
        p->redraw();
#else
        p->m_bOnTop = false == p->m_bOnTop;
        p->SetZPos();
#endif
        break;

    case WM_LBUTTONDOWN:
        UpdateWindow(hwnd);
        p->SetPinned(true);
        SetCursor(MenuInfo.move_cursor);
        SendMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
        SetCursor(MenuInfo.arrow_cursor);
        p->SetZPos();
        if (false == p->m_bOnTop)
        {
            HWND hwnd = window_under_mouse();
            if (hwnd && (WS_EX_TOPMOST & GetWindowLong(hwnd, GWL_EXSTYLE)))
                p->m_bOnTop = true, p->SetZPos();
        }
        break;

    case WM_MOUSEMOVE:
    case WM_RBUTTONDOWN:
        Active(1);
        break;
    }
}

//===========================================================================

//===========================================================================

void MenuGrip::Paint(HDC hDC)
{
	StyleItem *pSI = &mStyle.MenuGrip;
	RECT rect; GetItemRect(&rect);
	//int spacing = MenuInfo.nTitleIndent;
	int spacing = pSI->marginWidth;
	int just = pSI->Justify | DT_MENU_STANDARD;
	int bw = pSI->borderWidth;
	COLORREF bc = pSI->borderColor;

	//Adjust to for gradient smaller than the menu window?
	rect.left  += spacing;
	rect.right -= spacing;

	rect.bottom -= bw;
	if (false == pSI->parentRelative)
	{
		MakeStyleGradient(hDC, &rect, pSI, pSI->marginWidth?(pSI->bordered):(false));
		draw_line_h(hDC, rect.left, rect.right, rect.top, bw, bc);
	}

	if (pSI->parentRelative)
	{
		draw_line_h(hDC, rect.left, rect.right, rect.top, bw, bc);
		rect.bottom -= MenuInfo.nTitlePrAdjust;
		just = just & ~DT_VCENTER | DT_BOTTOM;
	}

	//BBDrawText(hDC, GetDisplayString(), -1, &rect, just, pSI);
}

void MenuGrip::Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam)
{
	Menu *p = m_pMenu;

	switch(uMsg) {
	case WM_RBUTTONUP:
		if (m_bActive)
		{
#ifdef MENU_ROLLUP
			if (0x8000 & GetAsyncKeyState(VK_MENU))
			{
				p->m_bOnTop = false == p->m_bOnTop;
				p->SetZPos();
			}
			else
#endif
			if (wParam & MK_SHIFT)
				ShowContextMenu(NULL, m_pidl);
			else
				p->HideThis();
		}
		break;

	case WM_LBUTTONDBLCLK:
#ifdef MENU_ROLLUP
		p->m_bIconized = false == p->m_bIconized;
		p->redraw();
#else
		p->m_bOnTop = false == p->m_bOnTop;
		p->SetZPos();
#endif
		break;

	case WM_LBUTTONDOWN:
		UpdateWindow(hwnd);
		p->SetPinned(true);
		SetCursor(MenuInfo.move_cursor);
		SendMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
		SetCursor(MenuInfo.arrow_cursor);
		p->SetZPos();
		if (false == p->m_bOnTop)
		{
			HWND hwnd = window_under_mouse();
			if (hwnd && (WS_EX_TOPMOST & GetWindowLong(hwnd, GWL_EXSTYLE)))
				p->m_bOnTop = true, p->SetZPos();
		}
		break;

	case WM_MOUSEMOVE:
	case WM_RBUTTONDOWN:
		Active(1);
		break;
	}
}
//===========================================================================
