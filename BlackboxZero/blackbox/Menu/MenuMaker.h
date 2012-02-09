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

#ifndef _MENUMAKER_H_
#define _MENUMAKER_H_

// This file defines the public menu interface.

//====================
// MenuMaker.cpp

bool MenuMaker_ShowMenu(int id, const char* param);

//====================
// Menu.cpp

struct menu_stats {
    int menu_count;
    int item_count;
};

void Menu_Init(void);
void Menu_Exit(void);
void Menu_Reconfigure(void);
void Menu_Stats(struct menu_stats *st);
bool Menu_IsA(HWND);
bool Menu_Exists(bool pinned);

void Menu_All_Redraw(int flags);
void Menu_All_Toggle(bool hidden);
void Menu_All_BringOnTop(void);
void Menu_All_Hide(void);
bool Menu_ToggleCheck(const char* menu_id);

void Menu_Update(int id);
// id for Menu_Update:
#define MENU_UPD_ROOT    1
#define MENU_UPD_TASKS   2
#define MENU_UPD_CONFIG  3

char *Core_IDString(char *buffer, const char *menu_id);
bool get_opt_command(char *opt_cmd, const char *cmd);
Menu * MakeRootMenu(const char *menu_id, const char *path, const char *default_menu, bool pop);

//====================
// ConfigMenu.cpp

struct cfgmenu
{
    const char *text;
    const char *command;
    const void *pvalue;
};

Menu* MakeConfigMenu(bool popup);
Menu *CfgMenuMaker(const char *title, const char *defbroam, const struct cfgmenu *pm, bool pop, char *menu_id);
int exec_cfg_command(const char *pszCommand);
const void *exec_internal_broam(const char *argument, const struct cfgmenu *menu_root, const struct cfgmenu **p_menu, const struct cfgmenu **p_item);

//====================
// DesktopMenu.cpp

Menu* MakeDesktopMenu(int mode, bool popup);
Menu* MakeRecoverMenu(bool popup);
Menu* MakeTaskFolder(int workspace, bool popup);
void ShowRecoverMenu(void);

//====================
// SpecialFolder.cpp

Menu *MakeFolderMenu(const char *title, const char* path, const char *cmd);

//====================
// ContextMenu.cpp

Menu *MakeContextMenu(const char *path, const void *pidl);


//====================
// special broams

#define MM_STYLE_BROAM "@BBCore.style %s"
#define MM_EDIT_BROAM "@BBCore.edit %s"
#define MM_THEME_BROAM "@BBCore.theme %s"
#define MM_ROOT_BROAM "@BBCore.rootCommand %s"

//===========================================================================
#endif /* ndef _MENUMAKER_H_s */
