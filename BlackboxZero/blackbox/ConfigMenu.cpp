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

#include "BB.h"
#include "Settings.h"
#include "Desk.h"
#include "PluginManager.h"
#include "Workspaces.h"
#include "Tray.h"
#include "Toolbar.h"
#include "Menu/MenuMaker.h"

//===========================================================================
static const struct int_item {
    const int *v; short minval, maxval, offval;
} int_items[] = {
    { &Settings_menu.mouseWheelFactor       ,   1,    10, -2  },
    { &Settings_menu.popupDelay             ,   0,   400, -2  },
    { &Settings_menu.maxWidth               , 100,   600, -2  },
    { &Settings_menu.alphaValue             ,   0,   255, 255 },
    { (int*)&Settings_desktopMargin.left    ,  -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.right   ,  -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.top     ,  -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.bottom  ,  -1, 10000, -1  },
    { &Settings_snapThreshold               ,   0,    20,  0  },
    { &Settings_snapPadding                 ,  -4,   100, -4  },
    { &Settings_toolbar.widthPercent        ,  10,   100, -2  },
    { &Settings_toolbar.alphaValue          ,   0,   255, 255 },
    { &Settings_autoRaiseDelay              ,   0, 10000, -1  },
    { &Settings_workspaces                  ,   1,    24, -1  },
    { NULL, 0, 0, 0}
};

static const struct int_item *get_int_item(const void *v)
{
    const struct int_item *p = int_items;
    do if (p->v == v) return p; while ((++p)->v);
    return NULL;
}

static bool is_string_item(const void *v)
{
    return v == &Settings_preferredEditor
        || v == Settings_toolbar.strftimeFormat
        || v == Settings_workspaceNames
        ;
}

static bool is_fixed_string(const void *v)
{
    return v == Settings_focusModel
        || v == Settings_menu.openDirection
        || v == Settings_toolbar.placement
        ;
}

//===========================================================================

static const struct cfgmenu cfg_sub_plugins[] = {
    { NLS0("Load/Unload"),          NULL, (const void*)SUB_PLUGIN_LOAD },
    { NLS0("In Slit"),              NULL, (const void*)SUB_PLUGIN_SLIT },
    { "", NULL, NULL },
#ifndef BBTINY
    { NLS0("Add Plugin ..."),       "plugin.add", NULL },
#endif
    { NLS0("Edit plugins.rc"),      "@BBCore.editPlugins", NULL },
    { NLS0("About Plugins"),        "@BBCore.aboutPlugins", NULL },
    { NULL,NULL,NULL },
};

static const struct cfgmenu cfg_sub_opendirection[] = {
    { NLS0("Bullet"),               "menu.openDirection bullet",    Settings_menu.openDirection },
    { NLS0("Left"),                 "menu.openDirection left",      Settings_menu.openDirection },
    { NLS0("Right"),                "menu.openDirection right",     Settings_menu.openDirection },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_menu_files[] = {
    { NLS0("Sort By Extension"),    "menu.sortByExtension", &Settings_menu.sortByExtension  },
    { NLS0("Show Hidden Files"),    "menu.showHiddenFiles", &Settings_menu.showHiddenFiles  },
    { NULL,NULL,NULL }
};


static const struct cfgmenu cfg_sub_menu[] = {
    { NLS0("Open Direction"),        NULL, cfg_sub_opendirection },
    { NLS0("Maximal Width"),        "menu.maxWidth",        &Settings_menu.maxWidth  },
    { NLS0("Popup Delay"),          "menu.popupDelay",      &Settings_menu.popupDelay  },
    { NLS0("Wheel Factor"),         "menu.mouseWheelFactor", &Settings_menu.mouseWheelFactor  },
    { NLS0("Files"),                NULL, cfg_sub_menu_files },
    { "", NULL, NULL },
    { NLS0("Transparency"),         "menu.alpha.Enabled",   &Settings_menu.alphaEnabled  },
    { NLS0("Alpha Value"),          "menu.alpha.Value",     &Settings_menu.alphaValue  },
    { NLS0("Draw Separators"),      "menu.drawSeparators",  &Settings_menu.drawSeparators  },
    { NLS0("Drop Shadows"),         "menu.dropShadows",     &Settings_menu.dropShadows },
    { "", NULL, NULL },
    { NLS0("Always On Top"),        "menu.onTop",           &Settings_menu.onTop  },
    { NLS0("Snap To Edges"),        "menu.snapWindow",      &Settings_menu.snapWindow  },
    { NLS0("On All Workspaces"),    "menu.sticky",          &Settings_menu.sticky  },
    { NLS0("Toggle With Plugins"),  "menu.pluginToggle",    &Settings_menu.pluginToggle  },
    { "", NULL, NULL },
    { NLS0("Show Bro@ms"),          "menu.showBro@ms",      &Settings_menu.showBroams },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_graphics[] = {
    { NLS0("Enable Background"),    "enableBackground",     &Settings_enableBackground },
#ifndef BBTINY
    { NLS0("Smart Wallpaper"),      "smartWallpaper",       &Settings_smartWallpaper  },
#endif
    { "", NULL, NULL },
    { NLS0("*Nix Bullets"),         "bulletUnix",           &mStyle.bulletUnix  },
    { NLS0("*Nix Arrows"),          "arrowUnix",            &Settings_arrowUnix  },
    { "", NULL, NULL },
    { NLS0("Image Dithering"),      "imageDither",          &Settings_imageDither  },
    { NLS0("Global Font Override"), "globalFonts",          &Settings_globalFonts  },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_dm[] = {
    { NLS0("Top"),                  "desktop.marginTop",    &Settings_desktopMargin.top  },
    { NLS0("Bottom"),               "desktop.marginBottom", &Settings_desktopMargin.bottom  },
    { NLS0("Left"),                 "desktop.marginLeft",   &Settings_desktopMargin.left  },
    { NLS0("Right"),                "desktop.marginRight",  &Settings_desktopMargin.right  },
    { NLS0("Full Maximization"),    "fullMaximization" ,    &Settings_fullMaximization  },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_focusmodel[] = {
    { NLS0("Click To Focus"),       "focusModel ClickToFocus",  Settings_focusModel },
    { NLS0("Sloppy Focus"),         "focusModel SloppyFocus",   Settings_focusModel },
    { NLS0("Auto Raise"),           "focusModel AutoRaise",     Settings_focusModel },
    { NLS0("Auto Raise Delay"),     "autoRaiseDelay", &Settings_autoRaiseDelay },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_snap[] = {
    { NLS0("Snap To Plugins"),      "snap.plugins",     &Settings_snapPlugins },
    { NLS0("Padding"),              "snap.padding",     &Settings_snapPadding },
    { NLS0("Threshold"),            "snap.threshold",   &Settings_snapThreshold },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_workspace[] = {
    { NLS0("Alternative Method"),   "workspaces.altMethod",         &Settings_altMethod },
    { NLS0("Style-XP Fix"),         "workspaces.styleXPFix",        &Settings_styleXPFix },
    { NLS0("Follow Active Task"),   "workspaces.followActive",      &Settings_followActive },
    { NLS0("Recover Windows"),      "@BBCore.ShowRecoverMenu",      NULL },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_sub_misc[] = {
    { NLS0("Desktop Margins"),      NULL, cfg_sub_dm },
    { NLS0("Focus Model"),          NULL, cfg_sub_focusmodel },
    { NLS0("Snap"),                 NULL, cfg_sub_snap },
    { NLS0("Workspaces"),           NULL, cfg_sub_workspace },
    { "", NULL, NULL },
#ifndef BBTINY
    { NLS0("Enable Toolbar"),       "toolbar.enabled",      &Settings_toolbar.enabled  },
#endif
    { NLS0("Opaque Window Move"),   "opaqueMove",           &Settings_opaqueMove },
    { NLS0("Use UTF-8 Encoding"),   "UTF8Encoding",         &Settings_UTF8Encoding },
    { NLS0("Blackbox Editor"),      "preferredEditor",      &Settings_preferredEditor },
    { "", NULL, NULL },
    { NLS0("Show Appnames"),        "@BBCore.showAppnames", NULL },
    { NULL,NULL,NULL }
};

static const struct cfgmenu cfg_main[] = {
    { NLS0("Plugins"),              NULL, cfg_sub_plugins },
    { NLS0("Menus"),                NULL, cfg_sub_menu },
    { NLS0("Graphics"),             NULL, cfg_sub_graphics },
    { NLS0("Misc."),                NULL, cfg_sub_misc },
    { NULL,NULL,NULL }
};

Menu *MakeConfigMenu(bool popup)
{
    char menu_id[200];
    Menu *m;

    Core_IDString(menu_id, "Configuration");
    m = CfgMenuMaker(NLS0("Configuration"), "@BBCfg.", cfg_main, popup, menu_id);

#if 0
    char buff[MAX_PATH];
    FindRCFile(buff, "plugins\\bbleanskin\\bblsmenu.rc", NULL);
    Menu *s = MakeRootMenu("Configuration_BBLS", buff, NULL, popup);
    if (s) MakeSubmenu(m, s, NULL);
#endif

    return m;
}

//===========================================================================

Menu *CfgMenuMaker(const char *title, const char *defbroam, const struct cfgmenu *pm, bool pop, char *menu_id)
{
    char buf[100];
    char *broam_dot;
    char *end_id;
    Menu *pMenu, *pSub;

    end_id = strchr(menu_id, 0);
    pMenu = MakeNamedMenu(title, menu_id, pop);
    broam_dot = strchr(strcpy_max(buf, defbroam, sizeof buf), 0);

    for (;pm->text; ++pm)
    {
        const char *item_text = pm->text;
        const void *v = pm->pvalue;
        const char *cmd = pm->command;
        const struct int_item *iip;
        bool disabled, checked;
        MenuItem *pItem;

        disabled = checked = false;
        if (cmd)
        {
            if ('@' != *cmd)
                strcpy(broam_dot, cmd), cmd = buf;

            if (NULL != (iip = get_int_item(v))) {
                pItem = MakeMenuItemInt(
                    pMenu, item_text, cmd, *iip->v, iip->minval, iip->maxval);
                if (-2 != iip->offval)
                    MenuItemOption(pItem, BBMENUITEM_OFFVAL,
                        iip->offval, 10000 == iip->maxval ? NLS0("auto") : NULL);
                continue;
            } else if (is_string_item(v)) {
                MakeMenuItemString(pMenu, item_text, cmd, (const char*)v);
                continue;
            } else if (is_fixed_string(v)) {
                checked = 0 == stricmp((const char *)v, strchr(cmd, ' ')+1);
            } else if (v) {
                checked = *(bool*)v;
            }

            disabled = (v == &Settings_styleXPFix && Settings_altMethod)
                    || (v == &Settings_menu.dropShadows && false == usingXP)
                    ;
            pItem = MakeMenuItem(pMenu, item_text, cmd, checked && false == disabled);
        }
        else if (v)
        {
            sprintf(end_id, "_%s", item_text);
            if ((DWORD_PTR)v <= SUB_PLUGIN_SLIT)
                pSub = PluginManager_GetMenu(item_text, menu_id, pop, (int)(DWORD_PTR)v);
            else
                pSub = CfgMenuMaker(item_text, defbroam, (struct cfgmenu*)v, pop, menu_id);
            if (NULL == pSub)
                continue;
            pItem = MakeSubmenu(pMenu, pSub, NULL);
        }
        else
        {
            pItem = MakeMenuNOP(pMenu, item_text);
        }

        if (disabled)
            MenuItemOption(pItem, BBMENUITEM_DISABLED);
    }
    return pMenu;
}

//===========================================================================

static const struct cfgmenu * find_cfg_item(
    const char *cmd,
    const struct cfgmenu *pmenu,
    const struct cfgmenu **pp_menu)
{
    const struct cfgmenu *p;
    for (p = pmenu; p->text; ++p)
        if (p->command) {
            if (0 == memicmp(cmd, p->command, strlen(p->command))) {
            *pp_menu = pmenu;
            return p;
            }
        } else if (p->pvalue >= (void*)100) {
            const struct cfgmenu* psub;
            psub = find_cfg_item(cmd, (struct cfgmenu*)p->pvalue, pp_menu);
            if (psub)
                return psub;
        }
    return NULL;
}

const void *exec_internal_broam(
    const char *arg,
    const struct cfgmenu *menu_root,
    const struct cfgmenu **p_menu,
    const struct cfgmenu**p_item)
{
    const void *v = NULL;
    *p_item = find_cfg_item(arg, menu_root, p_menu);
    if (NULL == *p_item)
        return v;

    v = (*p_item)->pvalue;
    if (v) {
        // scan for a possible argument to the command
        while (!IS_SPC(*arg))
            ++arg;
        skip_spc(&arg);
        // now set the appropriate variable
        if (is_fixed_string(v)) {
            strcpy((char *)v, arg);
        } else if (is_string_item(v)) {
            strcpy((char *)v, arg);
        } else if (get_int_item(v)) {
            if (*arg) *(int*)v = atoi(arg);
        } else {
            set_bool((bool*)v, arg);
        }
        // write to blackbox.rc or extensions.rc (automatic)
        Settings_WriteRCSetting(v);
    }
    return v;
}

int exec_cfg_command(const char *argument)
{
    const struct cfgmenu *p_menu, *p_item;
    const void *v;

    // is it a plugin related command?
    if (0 == memicmp(argument, "plugin.", 7)) {
        if (0 == PluginManager_handleBroam(argument+7))
            return 0;
        Menu_Update(MENU_UPD_CONFIG);
        return 1;
    }

    // search the item in above structures
    v = exec_internal_broam(argument, cfg_main, &p_menu, &p_item);
    if (NULL == p_item)
        return 0;

    // now take care for some item-specific refreshes
    if (v == &Settings_toolbar.enabled) {
        if (Settings_toolbar.enabled)
            beginToolbar(hMainInstance);
        else
            endToolbar(hMainInstance);
        Menu_Update(MENU_UPD_CONFIG);
    } else if (v == &Settings_menu.sortByExtension
            || v == &Settings_menu.showHiddenFiles) {
        PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU|BBRG_FOLDER, 0);
    } else if (v == &Settings_smartWallpaper) {
        Desk_Reset(true);
    } else if (v == &Settings_menu.dropShadows) {
        Menu_Exit(), Menu_Init();
    } else if (v == &Settings_UTF8Encoding) {
        Workspaces_GetCaptions();
        Tray_SetEncoding();
        PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU|BBRG_FOLDER, 0);
    }

    if (p_menu == cfg_sub_graphics || v == &Settings_UTF8Encoding)
        PostMessage(BBhwnd, BB_RECONFIGURE, 0, 0); // full reconfigure
    else
        PostMessage(BBhwnd, BB_RECONFIGURE, 1, 0); // bypass plugins

    return 1;
}

//===========================================================================

