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
#include "bbshell.h"
#include "MenuMaker.h"

// when no menu.rc file is found, use this default menu
static const char default_root_menu[] =
    "[begin]"
    "\n[path](Programs){PROGRAMS|COMMON_PROGRAMS}"
    "\n[path](Desktop){DESKTOP}"
    "\n[submenu](Blackbox)"
    "\n" "[config](Configuration)"
    "\n" "[stylesmenu](Styles){styles}"
    "\n" "[submenu](Edit)"
    "\n"    "[editstyle](current style)"
    "\n"    "[editmenu](menu.rc)"
    "\n"    "[editplugins](plugins.rc)"
    "\n"    "[editblackbox](blackbox.rc)"
    "\n"    "[editextensions](extensions.rc)"
    "\n"    "[end]"
    "\n" "[about](About)"
    "\n" "[reconfig](Reconfigure)"
    "\n" "[restart](Restart)"
    "\n" "[exit](Quit)"
    "\n" "[end]"
    "\n[submenu](Goodbye)"
    "\n" "[logoff](Log Off)"
    "\n" "[reboot](Reboot)"
    "\n" "[shutdown](Shutdown)"
    "\n" "[end]"
    "\n[end]"
    ;

// Special menu commands
static const char * const menu_cmds[] = {

    // ------------------
    "begin"             ,
    "end"               ,
    "submenu"           ,
    "include"           ,
    "nop"               ,
    "sep"               ,
    "separator"         ,
    // ------------------
    "path"              ,
    "insertpath"        ,
    "stylesmenu"        ,
    "stylesdir"         ,

    "themesmenu"        ,
    // ------------------
    "workspaces"        ,
    "icons"             ,
    "tasks"             ,
    "config"            ,
    "exec"              ,
    // ------------------
    NULL
};

enum menu_cmd_tokens
{
    // ------------------
    e_begin             ,
    e_end               ,
    e_submenu           ,
    e_include           ,
    e_nop               ,
    e_sep1              ,
    e_sep2              ,
    // ------------------
    e_path              ,
    e_insertpath        ,
    e_stylesmenu        ,
    e_stylesdir         ,

    e_themesmenu        ,
    // ------------------
    e_workspaces        ,
    e_icons             ,
    e_tasks             ,
    e_config            ,
    e_exec              ,
    // ------------------
    e_no_end            ,
    e_other             = -1
};


// menu include file handling
#define MAXINCLUDELEVEL 10

struct menu_src {
    int level;
    const char *default_menu;
    bool popup;
    FILE *fp[MAXINCLUDELEVEL];
    char path[MAXINCLUDELEVEL][MAX_PATH];
};

static bool add_inc_level(struct menu_src *src, const char *path)
{
    FILE *fp;
    if (src->level >= MAXINCLUDELEVEL)
        return false;
    fp = FileOpen(path);
    if (NULL == fp)
        return false;
    src->fp[src->level] = fp;
    strcpy(src->path[src->level], path);
    ++src->level;
    return true;
}

static void dec_inc_level(struct menu_src *src)
{
    FileClose(src->fp[--src->level]);
}

// recursive parsing of menu file
static Menu* ParseMenu(struct menu_src *src, const char *title, const char *IDString)
{
    char line[4000];
    char data[4000];
    char buffer[4000];
    char command[40];
    char label[MAX_PATH];

    Menu *pMenu, *pSub;
    MenuItem *pItem;
    int f, e_cmd;
    const char *p_label, *p_data, *cp, *p_cmd;

    pMenu = NULL;
    for(;;)
    {
        p_label = NULL;
        p_data = data;

        if (0 == src->level) {
            // read default menu from string
            NextToken(line, &src->default_menu, "\n");
        } else {
            f = ReadNextCommand(src->fp[src->level-1], line, sizeof(line));
            if (!f) {
                if (src->level > 1) {
                    dec_inc_level(src);
                    continue; // continue from included file
                }
                e_cmd = e_no_end;
                goto skip;
            }
        }

        cp = replace_environment_strings(line, sizeof line);

        //dbg_printf("Menu %08x line:%s", pMenu, line);

        // get the command
        if (false == get_string_within(command, sizeof command, &cp, "[]"))
            continue;
        // search the command
        e_cmd = get_string_index(command, menu_cmds);

        if (get_string_within(label, sizeof label, &cp, "()"))
            p_label = label;

        if (false == get_string_within(data, sizeof data, &cp, "{}"))
            p_data = label;

skip:
        if (NULL == pMenu)
        {
            if (e_begin == e_cmd) {
                // If the line contains [begin] we create the menu
                // If no menu title has been defined, display Blackbox version...
#ifdef BBXMENU
                if (src->default_menu)
                    strcpy(label, "bbXMenu");
                else
#endif
                if (0 == label[0] && src->default_menu)
                    p_label = GetBBVersion();
                pMenu = MakeNamedMenu(p_label, IDString, src->popup);
                continue;
            }

            if (NULL == title)
                title = NLS0("missing [begin]");

            pMenu = MakeNamedMenu(title, IDString, src->popup);
        }

        pItem = NULL;

        switch (e_cmd) {

        //====================
        // [begin] is like [submenu] when within the menu
        case e_begin:
        case e_submenu:
            sprintf(buffer, "%s_%s", IDString, label);
            strlwr(buffer + strlen(IDString));
            pSub = ParseMenu(src, p_data, buffer);
            if (pSub)
                pItem = MakeSubmenu(pMenu, pSub, label);
            else
                pItem = MakeMenuNOP(pMenu, label);
            break;

        //====================
        case e_no_end:
            MakeMenuNOP(pMenu, NLS0("missing [end]"));
        case e_end:
            MenuOption(pMenu, BBMENU_ISDROPTARGET);
            return pMenu;

        //====================
        case e_include:
        {
            char dir[MAX_PATH];
            file_directory(dir, src->path[src->level-1]);
            replace_shellfolders_from_base(buffer, p_data, false, dir);
            if (false == add_inc_level(src, buffer)) {
                replace_shellfolders(buffer, p_data, false);
                if (false == add_inc_level(src, buffer))
                    MakeMenuNOP(pMenu, NLS0("[include] failed"));
            }
            continue;
        }

        //====================
        // a [nop] item will insert an inactive item with optional text
        case e_nop:
            pItem = MakeMenuNOP(pMenu, label);
            break;

        // a insert separator, we treat [sep] like [nop] with no label
        case e_sep1:
        case e_sep2:
            pItem = MakeMenuNOP(pMenu, NULL);
            break;

        //====================
        // a [path] item is pointing to a dynamic folder...
        case e_path:
            p_cmd = get_special_command(&p_data, buffer, sizeof buffer);
            pItem = MakeMenuItemPath(pMenu, label, p_data, p_cmd);
            break;

        // a [insertpath] item will insert items from a folder...
        case e_insertpath:
            p_cmd = get_special_command(&p_data, buffer, sizeof buffer);
            pItem = MakeMenuItemPath(pMenu, NULL, p_data, p_cmd);
            break;

        // a [stylemenu] item is pointing to a dynamic style folder...
        case e_stylesmenu:
            pItem = MakeMenuItemPath(pMenu, label, p_data, MM_STYLE_BROAM);
            break;

        // a [styledir] item will insert styles from a folder...
        case e_stylesdir:
            pItem = MakeMenuItemPath(pMenu, NULL, p_data, MM_STYLE_BROAM);
            break;

        case e_themesmenu:
            pItem = MakeMenuItemPath(pMenu, label, p_data, MM_THEME_BROAM);
            break;

        //====================
        // special items...
        case e_workspaces:
            pItem = MakeSubmenu(pMenu, MakeDesktopMenu(0, src->popup), p_label);
            break;
        case e_icons:
            pItem = MakeSubmenu(pMenu, MakeDesktopMenu(1, src->popup), p_label);
            break;
        case e_tasks:
            pItem = MakeSubmenu(pMenu, MakeDesktopMenu(2, src->popup), p_label);
            break;
        case e_config:
            pItem = MakeSubmenu(pMenu, MakeConfigMenu(src->popup), p_label);
            break;

        //====================
        case e_exec:
            if ('@' == data[0]) {
                pItem = MakeMenuItem(pMenu, label, data, false);
                MenuItemOption(pItem, BBMENUITEM_UPDCHECK);
            } else {
                goto core_broam;
            }
            break;

        //====================
        case e_other:
            f = get_workspace_number(command); // check for 'workspace1..'
            if (-1 != f) {
                pItem = MakeSubmenu(pMenu, MakeTaskFolder(f, src->popup), p_label);
            } else {
                p_data = data;
                goto core_broam;
            }
            break;

        //====================
        // everything else is converted to a '@BBCore.xxx' broam
        core_broam:
            f = sprintf(buffer, "@bbCore.%s", command);
            strlwr(buffer+8);
            if (p_data[0])
                sprintf(buffer + f, " %s", p_data);
            pItem = MakeMenuItem(pMenu, label[0]?label:command, buffer, false);
            break;
        }

#ifdef BBOPT_MENUICONS
        if (pItem && get_string_within(label,  sizeof label, &cp, "<>"))
            MenuItemOption(pItem, BBMENUITEM_SETICON, label);
#endif
    }
}

// generate MenuID for core menus (root, config, workspaces)
char *Core_IDString(char *buffer, const char *menu_id)
{
    const char id[] = "Core_";
    memcpy(buffer, id, sizeof id-1);
    strlwr(strcpy(buffer + sizeof id-1, menu_id));
    return buffer;
}

// toplevel entry for menu parser
Menu * MakeRootMenu(const char *menu_id, const char *path, const char *default_menu, bool pop)
{
    Menu *m = NULL;
    char IDString[MAX_PATH];
    struct menu_src src;

    src.level = 0;
    src.default_menu = default_menu;
    src.popup = pop;

    if (false == add_inc_level(&src, path)) {
        if (NULL == default_menu)
            return m;
    }

    m = ParseMenu(&src, NULL, Core_IDString(IDString, menu_id));

    while (src.level)
        dec_inc_level(&src);
    return m;
}

// show one of the root menus
bool MenuMaker_ShowMenu(int id, const char* param)
{
    char buffer[MAX_PATH];
    Menu *m;
    int x, y, n, flags, toggle;

    static const char * const menu_string_ids[] = {
        "",
        "root",
        "workspaces",
        "icons",
        "tasks",
        "configuration",
        NULL
    };

    enum {
        e_lastmenu,
        e_root,
        e_workspaces,
        e_icons,
        e_tasks,
        e_configuration,
    };

    x = y = flags = n = toggle = 0;


    switch (id)
    {
        case BB_MENU_BROAM: // @ShowMenu ...
            while (param[0] == '-') {
                const char *p = NextToken(buffer, &param, NULL);
                if (0 == strcmp(p, "-at")) {
                    for (;;++param) {
                        if (*param == 'r')
                            flags |= BBMENU_XRIGHT;
                        else if (*param == 'b')
                            flags |= BBMENU_YBOTTOM;
                        else
                            break;
                    }
                    x = atoi(NextToken(buffer, &param, " ,"));
                    param += ',' == *param;
                    y = atoi(NextToken(buffer, &param, NULL));
                    flags |= BBMENU_XY;
                } else if (0 == strcmp(p, "-key")) {
                    flags |= BBMENU_KBD;
                } else if (0 == strcmp(p, "-toggle")) {
                    toggle = 1;
                } else if (0 == strcmp(p, "-pinned")) {
                    flags |= BBMENU_PINNED;
                } else if (0 == strcmp(p, "-ontop")) {
                    flags |= BBMENU_ONTOP;
                } else if (0 == strcmp(p, "-notitle")) {
                    flags |= BBMENU_NOTITLE;
                }
            }
            break;
        case BB_MENU_ROOT: // Main menu
            param = "root";
            break;
        case BB_MENU_TASKS: // Workspaces menu
            param = "workspaces";
            break;
        case BB_MENU_ICONS: // Iconized tasks menu
            param = "icons";
            break;
        case BB_MENU_UPDATE:
            Menu_Update(MENU_UPD_ROOT);
            Menu_All_Redraw(0);
            return false;
        case BB_MENU_SIGNAL: // just to signal e.g. BBSoundFX
            return true;
        default:
            return false;
    }

    // If invoked by kbd and the menu currently has focus,
    // hide it and return
    if (((flags & BBMENU_KBD) || toggle) && Menu_ToggleCheck(param))
        return false;

    //DWORD t1 = GetTickCount();

    switch (get_string_index(param, menu_string_ids))
    {
        case e_root:
        case e_lastmenu:
            m = MakeRootMenu("root", menuPath(NULL), default_root_menu, true);
            break;

        case e_workspaces:
            m = MakeDesktopMenu(0, true);
            break;

        case e_icons:
            m = MakeDesktopMenu(1, true);
            break;

        case e_tasks:
            m = MakeDesktopMenu(2, true);
            break;

        case e_configuration:
            m = MakeConfigMenu(true);
            break;

        default:
            n = get_workspace_number(param); // e.g. 'workspace1'
            if (-1 != n) {
                m = MakeTaskFolder(n, true);
            } else if (FindRCFile(buffer, param, NULL)) {
                m = MakeRootMenu(param, buffer, NULL, true);
            } else {
                const char *cmd = get_special_command(&param, buffer, sizeof buffer);
                m = MakeFolderMenu(NULL, param, cmd);
            }
            break;
    }

    if (NULL == m)
        return false;

    MenuOption(m, flags, x, y);
    ShowMenu(m);

    //dbg_printf("showmenu time %d", GetTickCount() - t1);
    return true;
}

// update one of the core menus
void Menu_Update(int id)
{
    switch (id)
    {
        case MENU_UPD_ROOT:
            // right click menu or any of its submenus
            if (MenuExists("Core_root")) {
                ShowMenu(MakeRootMenu("root", menuPath(NULL),
                    default_root_menu, false));
                break;
            }
            // fall though
        case MENU_UPD_CONFIG:
            // the core config menu
            if (MenuExists("Core_configuration"))
                ShowMenu(MakeConfigMenu(false));
            break;

        case MENU_UPD_TASKS:
            // desktop workspaces menu etc.
            if (MenuExists("Core_tasks"))
            {
                if (MenuExists("Core_tasks_workspace"))
                    ShowMenu(MakeDesktopMenu(0, false));
                else
                if (MenuExists("Core_tasks_icons"))
                    ShowMenu(MakeDesktopMenu(1, false));

                if (MenuExists("Core_tasks_menu"))
                    ShowMenu(MakeDesktopMenu(2, false));

                if (MenuExists("Core_tasks_recoverwindows"))
                    ShowMenu(MakeRecoverMenu(false));
            }
            break;
    }
}

