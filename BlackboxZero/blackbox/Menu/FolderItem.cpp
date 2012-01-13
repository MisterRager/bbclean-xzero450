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
#include "Menu.h"

//===========================================================================

FolderItem::FolderItem(Menu* pSubMenu, const char* pszTitle) : MenuItem(pszTitle)
{
    m_nSortPriority = M_SORT_FOLDER;
    m_ItemID = MENUITEM_ID_FOLDER;
    LinkSubmenu(pSubMenu);
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

        if (m_pSubmenu)
            m_pSubmenu->SetPinned(false);

        ShowSubmenu();
    }
}

// Paints the folder item, the parent class will paint first
void FolderItem::Paint(HDC hDC)
{
    COLORREF c;
    RECT rc;
    int d, bstyle;

    MenuItem::Paint(hDC);

	/* BlackboxZero 1.7.2012 */
	if ( !Settings_menu.bullet_enabled )
		return;

    bstyle = MenuInfo.nBulletStyle;
    if (BS_EMPTY == bstyle)
        return;

    rc.bottom = (rc.top = m_nTop) + m_nHeight + 1;

    if (MenuInfo.nBulletPosition == FOLDER_RIGHT) {
        d = MenuInfo.nItemRightIndent + mStyle.MenuHilite.borderWidth;
        rc.left = (rc.right = m_nLeft + m_nWidth) - d + 1;
    } else {
        d = MenuInfo.nItemLeftIndent + mStyle.MenuHilite.borderWidth;
        rc.right = (rc.left = m_nLeft) + d;
        bstyle = -bstyle;
    }

    if (m_bActive && false == m_bNOP)
        c = mStyle.MenuHilite.foregroundColor;
    else if (m_bDisabled)
        c = mStyle.MenuFrame.disabledColor;
    else
        c = mStyle.MenuFrame.foregroundColor;

    bbDrawPix(hDC, &rc, c, bstyle);
}

//===========================================================================

