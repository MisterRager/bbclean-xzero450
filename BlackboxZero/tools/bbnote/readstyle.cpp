/*---------------------------------------------------------------------------*

  This file is part of the BBNote source code

  Copyright 2003-2009 grischka@users.sourceforge.net

  BBNote is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 *---------------------------------------------------------------------------*/
// readstyle.cpp - read blackbox style

#include "bbstyle.h"
#define BBSETTINGS_INTERNAL
#include "Settings.h"

#define ST static

StyleStruct mStyle;
menu_setting Settings_menu;
bool Settings_menusBroamMode = false;
int Settings_menuMaxWidth = 200;
struct MenuInfo MenuInfo;

struct FolderItem
{
    static int m_nBulletStyle;
    static int m_nBulletPosition;
};

int FolderItem::m_nBulletStyle;
int FolderItem::m_nBulletPosition;

LPCSTR stylePath(LPCSTR styleFileName)
{
    static char path[MAX_PATH];
    if (styleFileName) strcpy(path, styleFileName);
    return path;
}

LPCSTR extensionsrcPath(LPCSTR extensionsrcFileName)
{
    return "";
}

COLORREF get_bg_color(StyleItem *pSI)
{
    if (B_SOLID == pSI->type) // && false == pSI->interlaced)
        return pSI->Color;
    return mixcolors(pSI->Color, pSI->ColorTo, 128);
}

COLORREF get_mixed_color(StyleItem *pSI)
{
    COLORREF b = get_bg_color(pSI);
    COLORREF t = pSI->TextColor;
    if (greyvalue(b) > greyvalue(t))
        return mixcolors(t, b, 96);
    else
        return mixcolors(t, b, 144);
}

//===========================================================================
// Function: get_fontheight
//===========================================================================

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

void MenuMaker_Clear(void)
{
    if (MenuInfo.hTitleFont) DeleteObject(MenuInfo.hTitleFont);
    if (MenuInfo.hFrameFont) DeleteObject(MenuInfo.hFrameFont);
    MenuInfo.hTitleFont = MenuInfo.hFrameFont = NULL;
}

void Menu_Reconfigure();

void GetStyle (const char *styleFile)
{
    if (styleFile)
    {
        stylePath(styleFile);
        ReadStyle(styleFile, &mStyle);
    }

    bimage_init(true, mStyle.is_070);

    Menu_Reconfigure();
    mStyle.borderWidth = mStyle.MenuFrame.borderWidth;
}

//===========================================================================

void Menu_Clear(void)
{
    if (MenuInfo.hTitleFont)
        DeleteObject(MenuInfo.hTitleFont);
    if (MenuInfo.hFrameFont)
        DeleteObject(MenuInfo.hFrameFont);
    MenuInfo.hTitleFont =
    MenuInfo.hFrameFont = NULL;
}

//===========================================================================

void Menu_Reconfigure(void)
{
    // clear fonts
    Menu_Clear();

    StyleItem *MTitle = &mStyle.MenuTitle;
    StyleItem *MFrame = &mStyle.MenuFrame;
    StyleItem *MHilite = &mStyle.MenuHilite;
    StyleItem *pSI;

    // create fonts
    MenuInfo.hTitleFont = CreateStyleFont(MTitle);
    MenuInfo.hFrameFont = CreateStyleFont(MFrame);

    // set bullet position & style
    MenuInfo.nBulletPosition =
        stristr(mStyle.menuBulletPosition, "left") ? FOLDER_LEFT : FOLDER_RIGHT;

    MenuInfo.nBulletStyle =
        get_menu_bullet(mStyle.menuBullet);

    if (0 == stricmp(Settings_menu.openDirection, "bullet"))
        MenuInfo.openLeft = MenuInfo.nBulletPosition == FOLDER_LEFT;
    else
        MenuInfo.openLeft = 0 == stricmp(Settings_menu.openDirection, "left");

    // --------------------------------------------------------------
    // calulate metrics:

    MenuInfo.nFrameMargin = MFrame->marginWidth + MFrame->borderWidth;
    MenuInfo.nSubmenuOverlap = MenuInfo.nFrameMargin + MHilite->borderWidth;
    MenuInfo.nTitleMargin = 0;

    if (mStyle.menuTitleLabel)
        MenuInfo.nTitleMargin = MFrame->marginWidth;

    // --------------------------------------
    // title height, indent, margin

    int tfh = get_fontheight(MenuInfo.hTitleFont);
    int titleHeight = 2*MTitle->marginWidth + tfh;

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

    int ffh = get_fontheight(MenuInfo.hFrameFont);
    int itemHeight = MHilite->marginWidth + ffh;

    // xxx old behaviour xxx
    if (false == mStyle.is_070 && 0 == (MHilite->validated & V_MAR))
        itemHeight = MFrame->FontHeight + (mStyle.bevelWidth+1)/2;
    //xxxxxxxxxxxxxxxxxxxxxx

#ifdef BBOPT_MENUICONS
    itemHeight = imax(14, itemHeight);
    MenuInfo.nItemHeight =
    MenuInfo.nItemLeftIndent =
    MenuInfo.nItemRightIndent = itemHeight;
    MenuInfo.nIconSize = imin(itemHeight - 2, 16);
    if (DT_LEFT == MFrame->Justify)
        MenuInfo.nItemLeftIndent += 1;
#else
    MenuInfo.nItemHeight = itemHeight;
    MenuInfo.nItemLeftIndent =
    MenuInfo.nItemRightIndent = imax(11, itemHeight);
#endif

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

    // --------------------------------------
    // setup a StyleItem for the scroll rectangle
    StyleItem *pScrl = &MenuInfo.Scroller;
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
    MenuInfo.separatorColor = get_mixed_color(MFrame);
    MenuInfo.separatorWidth = Settings_menu.drawSeparators ? imax(1, MFrame->borderWidth) : 0;
    MenuInfo.check_is_pr = MHilite->parentRelative
        || iabs(greyvalue(get_bg_color(MFrame))
                - greyvalue(get_bg_color(MHilite))) < 24;
}

//===========================================================================

