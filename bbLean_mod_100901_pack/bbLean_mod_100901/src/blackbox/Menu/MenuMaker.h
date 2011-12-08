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

#ifndef __MENUMAKER_H
#define __MENUMAKER_H

//====================
// MenuMaker.cpp

void MenuMaker_Init();
void MenuMaker_Exit();
void MenuMaker_Configure();
int  MenuMaker_LoadFolder(MenuItem **, const struct _ITEMIDLIST *pIDFolder, const char  *optional_command);
bool MenuMaker_ShowMenu(int id, LPARAM param);

struct MenuInfo
{
	HCURSOR move_cursor;
	HCURSOR arrow_cursor;
	HFONT hFrameFont;
	HFONT hTitleFont;
	int MaxMenuHeight;
	int MaxMenuWidth;

	int nTitleHeight;
	int nTitleIndent;
	int nTitlePrAdjust;

	int nItemHeight;
	int nItemIndent;
	int nFrameMargin;

	int nScrollerSize;
	int nScrollerSideOffset;
	int nScrollerTopOffset;
	StyleItem Scroller;
	int nAvgFontWidth;

	int nIconItemHeight;
	int nIconItemIndent;

};

extern struct MenuInfo MenuInfo;

//====================
// Menu.cpp

void Menu_All_Redraw(int flags);
void Menu_All_Update(int special);
void Menu_All_Delete(void);
void Menu_All_Toggle(bool hide);
void Menu_All_BringOnTop(void);
void Menu_All_Hide(void);
bool Menu_Activate_Last(void);
void Menu_Tab_Next(Menu *start);

#define MENU_IS_ROOT    1
#define MENU_IS_TASKS   2
#define MENU_IS_CONFIG  3

void MenuItemInt_SetOffValue(MenuItem *, int, const char*);
void SetFolderItemCommand(MenuItem *Item, const char *buf);
void CheckMenuItem(MenuItem *Item, bool checked);
void init_check_optional_command(const char *cmd, char *current_optional_command);
bool get_string_within (char *dest, char **p_src, const char *delim, bool right);

//====================
// ConfigMenu.cpp

struct cfgmenu
{
	const char *text;
	const char *command;
	const void *pvalue;
};

Menu *CfgMenuMaker(const char *title, const struct cfgmenu *pm, bool pop, char *menu_id);
Menu* MakeConfigMenu(bool popup);
void  RedrawConfigMenu(void);
void exec_cfg_command(const char *pszCommand);
const void *exec_internal_broam(const char *argument, const struct cfgmenu *menu_root, const struct cfgmenu **p_menu, const struct cfgmenu **p_item);

//====================
// DesktopMenu.cpp

Menu* MakeDesktopMenu(bool popup);
Menu* MakeIconsMenu(bool popup);
Menu* MakeProcessesMenu(bool popup);
Menu * GetTaskFolder(int workspace, bool popup);

//===========================================================================
#endif /* __MENUMAKER_H */
