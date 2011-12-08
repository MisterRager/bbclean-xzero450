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
#include "MenuMaker.h"
#include "Menu.h"

int FolderItem::m_nBulletPosition = DT_RIGHT;
int FolderItem::m_nBulletStyle = BS_TRIANGLE;
BYTE **FolderItem::m_byBulletBmp = NULL;
int FolderItem::m_nBulletBmpSize = 0;

//===========================================================================

FolderItem::FolderItem(Menu* pSubMenu, const char* pszTitle, const char* pszIcon) : MenuItem(pszTitle)
{
	m_nSortPriority = M_SORT_FOLDER;
	m_ItemID = MENUITEM_ID_FOLDER;
	LinkSubmenu(pSubMenu);
	m_pszIcon = new_str(pszIcon);
	GetIcon();
}

void FolderItem::Invoke(int button)
{
	if (INVOKE_LEFT == button)
	{
		if (m_pszCommand)
		{
			m_pMenu->hide_on_click();
			post_command(m_pszCommand);
			return;
		}

		if (m_pSubMenu)
			m_pSubMenu->SetPinned(false);

		ShowSubMenu();
	}
}

//===========================================================================
// Paints the folder item, the parent class will paint first

void FolderItem::Paint(HDC hDC)
{
	MenuItem::Paint(hDC);

	COLORREF C =
		m_bActive
		? mStyle.MenuHilite.foregroundColor
		: (MI_NOP_DISABLED & m_isNOP)
		? mStyle.MenuFrame.disabledColor
		: mStyle.MenuFrame.foregroundColor
		;

	int r = mStyle.bulletUnix ? 2 : 3;

	int yOffset = m_nTop + m_nHeight / 2;
	int xOffset;
	if (FolderItem::m_nBulletPosition == DT_RIGHT)
		xOffset = m_nLeft + m_nWidth - MenuInfo.nItemIndent / 2 - 1;
	else
		xOffset = m_nLeft + MenuInfo.nItemIndent / 2;

	int xLeft   = xOffset - r;
	int xRight  = xOffset + r;
	int yTop    = yOffset - r;
	int yBottom = yOffset + r;

	HGDIOBJ oldPen = SelectObject(hDC, CreatePen(PS_SOLID, 1, C));
	switch (m_nBulletStyle)
	{
		case BS_TRIANGLE:
			arrow_bullet (hDC, xOffset, yOffset, m_nBulletPosition == DT_LEFT ? -1 : 1);
			break;

		case BS_SQUARE:
			MoveToEx(hDC, xRight, yBottom, NULL);
			LineTo  (hDC, xRight, yTop);
			LineTo  (hDC, xLeft,  yTop);
			LineTo  (hDC, xLeft,  yBottom);
			LineTo  (hDC, xRight, yBottom);
			break;

		case BS_DIAMOND:
			int  x1, x2, y;
			for (x2=1+(x1=xOffset), y = yTop; y <= yBottom; y++)
			{
				MoveToEx(hDC, x1, y, NULL);
				LineTo  (hDC, x2, y);
				if (y<yOffset) x1--, x2++;
				else     x1++, x2--;
			}
			break;

		case BS_CIRCLE:
			Arc(hDC, xLeft, yTop, xRight+1, yBottom+1, xOffset, 0, xOffset, 0);
			break;

		case BS_BMP:
			if (m_byBulletBmp == NULL) break;
			// if active, draw lower of Bitmap
			int nBmpPosY = m_bActive ? m_nBulletBmpSize : 0;
			// triming draw area
			int nBulletSize = imin(m_nBulletBmpSize, imin(m_nHeight, m_nWidth));
			// start/end pos
			int xs = xOffset - nBulletSize/2;
			int ys = yOffset - nBulletSize/2;
			for(int j = 0; j < nBulletSize; j++){
				for(int i = 0; i < nBulletSize; i++){
					SetPixel(hDC, xs+i, ys+j, mixcolors(C, GetPixel(hDC, xs+i, ys+j), m_byBulletBmp[nBmpPosY+j][i]));
				}
			}
			break;
	}

	DeleteObject(SelectObject(hDC, oldPen));
}

//===========================================================================
