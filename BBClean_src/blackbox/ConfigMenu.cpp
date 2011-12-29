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

#include "BB.h"
#include "Settings.h"
#include "Desk.h"
#include "PluginManager.h"
#include "Workspaces.h"
#include "Toolbar.h"
#include "Menu/MenuMaker.h"
#include "blackbox.h"

#define SUB_PLUGIN_LOAD 1
#define SUB_PLUGIN_SLIT 2

//===========================================================================
//  Trying to figure out the structure of this file - it looks like this
//  is a list of possible settings that go into rc files?
//
//  - Rager
static struct int_item { int *v; short minval, maxval, offval; }  int_items[] = {
    { &Settings_menuMousewheelfac          ,  1,  10,   -2  },
	{ &Settings_menuPopupDelay             ,  0, 1000,  -2  },//1000 was 400
	{ &Settings_menuCloseDelay			   ,  0, 1000,  -2  },
    { &Settings_menuMaxWidth               ,100, 600,   -2  },
	{ &Settings_menuMinWidth               , 25, 600,   -2  },
    { &Settings_menuAlphaValue             ,  0, 255,  255  },
	{ &Settings_menuIconSize               , -1,  32,    0  },
	{ &Settings_menuIconSaturation         ,  0, 255,  255  },
	{ &Settings_menuIconHue                ,  0, 255,    0  },
	{ &Settings_menuScrollHue              ,  0, 255,    0  },
    { (int*)&Settings_desktopMargin.left   , -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.right  , -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.top    , -1, 10000, -1  },
    { (int*)&Settings_desktopMargin.bottom , -1, 10000, -1  },
    { &Settings_edgeSnapThreshold          ,  0, 20,     0  },
    { &Settings_edgeSnapPadding            , -4, 100,   -4  },
    { &Settings_toolbarWidthPercent        , 10, 100,   -2  },
    { &Settings_toolbarAlphaValue          ,  0, 255,  255  },
	{ &Settings_menuPositionAdjustX			, -500, 500, 0	},
	{ &Settings_menuPositionAdjustY			, -500, 500, 0	},
    { NULL, 0, 0, 0}
};

static struct int_item *get_int_item(const void *v)
{
    struct int_item *p = int_items;
    do if (p->v == v) return p;
    while ((++p)->v);
    return NULL;
}

static bool is_string_item(const void *v)
{
    return v == &Settings_preferredEditor
        || v == Settings_toolbarStrftimeFormat
    ;
}

static bool is_fixed_string(const void *v)
{
    return v == Settings_focusModel
        || v == Settings_menuBulletPosition_cfg
		|| v == Settings_menuScrollPosition_cfg
		|| v == Settings_menuAlphaMethod_cfg
		|| v == Settings_menuPositionAdjustPlacement
        || v == Settings_toolbarPlacement
		|| v == Settings_menuSeparatorStyle
        ;
}

//===========================================================================
static Menu * GetPluginMenu(
    const char *title, char *menu_id, bool pop, int mode, struct plugins **qp)
{
    char *save_id = strchr(menu_id, 0);
    Menu *pMenu = MakeNamedMenu(title, menu_id, pop);

    struct plugins *q;
    while (NULL != (q = *qp))
    {
        *qp = q->next;

        if (q->is_comment)
        {
            if (mode == SUB_PLUGIN_SLIT)
                continue;

            char command[40], label[80], *cp = q->fullpath;
            if (false == get_string_within(command, &cp, "[]", false))
                continue;

            get_string_within(label, &cp, "()", false);
            if (0 == stricmp(command, "nop"))
                MakeMenuNOP(pMenu, label);
            else
            if (0 == stricmp(command, "submenu") && *label)
            {
                sprintf(save_id, "_%s", label);
                MakeSubmenu(pMenu, GetPluginMenu(label, menu_id, pop, mode, qp), label);
            }
            else
            if (0 == stricmp(command, "end"))
                break;

            continue;
        }

        bool checked; const char *cmd;
        if (mode == SUB_PLUGIN_LOAD)
        {
            cmd = "@BBCfg.plugin.load %s";
            checked = q->enabled;
        }
        else
        if (mode == SUB_PLUGIN_SLIT)
        {
            if (false == q->enabled)
                continue;

            if (NULL == hSlit)
                continue;

            if (NULL == q->beginSlitPlugin && NULL == q->beginPluginEx)
                continue;

            cmd = "@BBCfg.plugin.inslit %s";
            checked = q->useslit;
        }
        else
            continue;

        char buf[80]; sprintf(buf, cmd, q->name); //, false_true_string(false==checked));
        MakeMenuItem(pMenu, q->name, buf, checked);
    }

	//Add grip to plugin and plugin/slit menu's
	if ( Settings_menusGripEnabled ) {
		MakeMenuGrip(pMenu, title);
	}

	*save_id = 0;
    return pMenu;
}

//===========================================================================

Menu *CfgMenuMaker(const char *title, const struct cfgmenu *pm, bool pop, char *menu_id)
{
    if (SUB_PLUGIN_LOAD == (long)pm || SUB_PLUGIN_SLIT == (long)pm)
    {
        struct plugins *q = bbplugins;
        return GetPluginMenu(title, menu_id, pop, (long)pm, &q);
    }

    char *save_id = strchr(menu_id, 0);
    Menu *pMenu = MakeNamedMenu(title, menu_id, pop);

    char buf[80]; strcpy(buf, "@BBCfg.");
    while (pm->text)
    {
        const char *item_text = pm->text;
        if (pm->command)
        {
            const char *cmd = pm->command;
            if ('@' != *cmd) strcpy(buf+7, cmd), cmd=buf;
            struct int_item *iip = get_int_item(pm->pvalue);
            if (iip)
            {
                MenuItem *pItem = MakeMenuItemInt(
                    pMenu, item_text, cmd, *iip->v, iip->minval, iip->maxval);
                if (-2 != iip->offval)
                    MenuItemInt_SetOffValue(
                        pItem, iip->offval, 10000 == iip->maxval ? "auto" : NULL);
            }
            else
            if (is_string_item(pm->pvalue))
            {
                MakeMenuItemString(pMenu, item_text, cmd, (LPCSTR)pm->pvalue);
            }
            else
            {
                bool checked = false;
                if (is_fixed_string(pm->pvalue))
                    checked = 0==stricmp((const char *)pm->pvalue, strrchr(cmd, ' ')+1);
                else
                if (pm->pvalue)
                    checked = *(bool*)pm->pvalue;

                bool disabled = false;
                if (pm->pvalue == &Settings_smartWallpaper)
                {
                    if (Settings_desktopHook || false == Settings_background_enabled)
                        disabled = true;
                }

                MakeMenuItem(pMenu, item_text, cmd, checked && false == disabled);
                if (disabled) DisableLastItem(pMenu);

            }
        }
        else
        {
            struct cfgmenu* sub = (struct cfgmenu*)pm->pvalue;
            if (sub)
            {
                sprintf(save_id, "_%s", item_text);
                Menu *pSub = CfgMenuMaker(item_text, sub, pop, menu_id);
                if (pSub)
                {
                    MakeSubmenu(pMenu, pSub, item_text);
                    if (SUB_PLUGIN_SLIT == (long)sub && NULL == hSlit)
                        DisableLastItem(pMenu);
                }
            }
            else
            {
                MakeMenuNOP(pMenu, item_text);
            }
        }
        pm++;
    }
	*save_id = 0;

	//Adds menu grip to the Configuration Menu and it's immeadiate sub menu's
	if ( Settings_menusGripEnabled ) {
		MakeMenuGrip(pMenu, title);
		//MakeMenuGrip(pMenu, strlen(pMenu->m_pMenuItems->m_pszTitle)?(pMenu->m_pMenuItems->m_pszTitle):(""));
	}

	return pMenu;
}

//===========================================================================
// FIXME: howto forward-declare static variables?
extern const struct cfgmenu cfg_main[];
extern const struct cfgmenu cfg_sub_plugins[];
extern const struct cfgmenu cfg_sub_menu[];
extern const struct cfgmenu cfg_sub_menubullet[];
extern const struct cfgmenu cfg_sub_menuscroll[];
extern const struct cfgmenu cfg_sub_menualphamethod[];
extern const struct cfgmenu cfg_sub_menuShadow[];
extern const struct cfgmenu cfg_sub_menupositionadjustplacement[];
extern const struct cfgmenu cfg_sub_graphics[];
extern const struct cfgmenu cfg_sub_misc[];
extern const struct cfgmenu cfg_sub_dm[];
extern const struct cfgmenu cfg_sub_focusmodel[];
extern const struct cfgmenu cfg_sub_snap[];
extern const struct cfgmenu cfg_sub_workspace[];
extern const struct cfgmenu cfg_sub_session[];
extern const struct cfgmenu cfg_sub_menuseparatorstyle[];

Menu *MakeConfigMenu(bool popup)
{
    char menu_id[200];
    strcpy(menu_id, "IDRoot_configuration");
    Menu *m = CfgMenuMaker(NLS0("Configuration"), cfg_main, popup, menu_id);
    return m;
}

const struct cfgmenu cfg_main[] = {
    { NLS0("Plugins"),            NULL, cfg_sub_plugins },
    { NLS0("Menus"),              NULL, cfg_sub_menu },
    { NLS0("Graphics"),           NULL, cfg_sub_graphics },
    { NLS0("Misc."),              NULL, cfg_sub_misc },
	{ NLS0("Session"),            NULL, cfg_sub_session },
    { NULL }
};

const struct cfgmenu cfg_sub_plugins[] = {
    { NLS0("Load/Unload"),        NULL, (void*)SUB_PLUGIN_LOAD },
    { NLS0("In Slit"),            NULL, (void*)SUB_PLUGIN_SLIT },
    { NLS0("Add Plugin..."),      "plugin.add", NULL },
    { NLS0("Edit plugins.rc"),    "@BBCore.editPlugins", NULL },
    { NLS0("About Plugins"),      "@BBCore.aboutPlugins", NULL },
    { NULL }
};

const struct cfgmenu cfg_sub_session[] = {
	{ NLS0("Enable Screensaver"),   "toggleScreensaver", &Session_screensaverEnabled},
	{ NULL }
};
const struct cfgmenu cfg_sub_menu[] = {
    { NLS0("Bullet Position"),    NULL, cfg_sub_menubullet },
	{ NLS0("Scroll Position"),    NULL, cfg_sub_menuscroll },
    { NLS0("Maximal Width"),      "menu.maxWidth",        &Settings_menuMaxWidth  },
	{ NLS0("Minimal Width"),      "menu.minWidth",        &Settings_menuMinWidth  },
	{ NLS0("Default Icon Size"),  "menu.icon.Size",       &Settings_menuIconSize  },
	{ NLS0("Icon Saturation"),    "menu.icon.Saturation", &Settings_menuIconSaturation  },
	{ NLS0("Icon Hue"),           "menu.icon.Hue",        &Settings_menuIconHue  },
    { NLS0("Popup Delay"),        "menu.popupDelay",      &Settings_menuPopupDelay  },
	{ NLS0("Close Delay"),		  "menu.closeDelay",	  &Settings_menuCloseDelay	},
    { NLS0("Wheel Factor"),       "menu.mouseWheelFactor", &Settings_menuMousewheelfac  },
    { "", NULL, NULL },
	{ NLS0("Scroll Button Hue"),  "menu.scrollButton.Hue", &Settings_menuScrollHue  },
	//{ NLS0("Transparency"),       "menu.alpha.Enabled",   &Settings_menuAlphaEnabled  },
	{ NLS0("Alpha Method"),		  NULL,	  cfg_sub_menualphamethod	},
    { NLS0("Alpha Value"),        "menu.alpha.Value",     &Settings_menuAlphaValue  },
    { "", NULL, NULL },
	{ NLS0("Grip"),				  "menu.grip.enabled",		&Settings_menusGripEnabled		},
	{ NLS0("Shadow"),			  "menu.shadow.enabled",	&Settings_menuShadowsEnabled	},
	{ NLS0("Separator style"),    NULL,  cfg_sub_menuseparatorstyle },
	{ "", NULL, NULL },
    { NLS0("Always On Top"),      "menu.onTop",           &Settings_menusOnTop  },
    { NLS0("Snap To Edges"),      "menu.snapWindow",      &Settings_menusSnapWindow  },
    { NLS0("Toggle With Plugins"), "menu.pluginToggle",   &Settings_menuspluginToggle  },
    { NLS0("Sort By Extension"),  "menu.sortbyExtension", &Settings_menusExtensionSort  },
	{ NLS0("Separate Folders/Other"), "menu.separateFolders", &Settings_menusSeparateFolders  },
    { "", NULL, NULL },
    { NLS0("Show Bro@ms"),        "menu.showBr@ams",      &Settings_menusBroamMode },
	{ NLS0("PositionAdjust Placement"),	NULL,	cfg_sub_menupositionadjustplacement },
	{ NLS0("PositionAdjust X"),    "menu.positionAdjust.x", &Settings_menuPositionAdjustX  },
	{ NLS0("PositionAdjust Y"),    "menu.positionAdjust.y", &Settings_menuPositionAdjustY  },
	{ NULL }
};

const struct cfgmenu cfg_sub_menualphamethod[] = {
	{ NLS0("Default"),            "menu.alphaMethod default",	Settings_menuAlphaMethod_cfg },
	{ NLS0("Classic"),            "menu.alphaMethod classic",	Settings_menuAlphaMethod_cfg },
	{ NLS0("Single Color"),       "menu.alphaMethod single",	Settings_menuAlphaMethod_cfg },
	{ NLS0("Mixture"),			  "menu.alphaMethod mixture",	Settings_menuAlphaMethod_cfg },
	{ NULL }
};

const struct cfgmenu cfg_sub_menupositionadjustplacement[] = {
	{ NLS0("Default"),			"menu.positionAdjustPlacement default",		Settings_menuPositionAdjustPlacement },
	{ NLS0("TopLeft"),			"menu.positionAdjustPlacement TopLeft",		Settings_menuPositionAdjustPlacement },
	{ NLS0("TopCenter"),		"menu.positionAdjustPlacement TopCenter",	Settings_menuPositionAdjustPlacement },
	{ NLS0("TopRight"),			"menu.positionAdjustPlacement TopRight",	Settings_menuPositionAdjustPlacement },
	{ NLS0("MiddleLeft"),		"menu.positionAdjustPlacement MiddleLeft",	Settings_menuPositionAdjustPlacement },
	{ NLS0("MiddleCenter"),		"menu.positionAdjustPlacement MiddleCenter",Settings_menuPositionAdjustPlacement },
	{ NLS0("MiddleRight"),		"menu.positionAdjustPlacement MiddleRight",	Settings_menuPositionAdjustPlacement },
	{ NLS0("BottomLeft"),		"menu.positionAdjustPlacement BottomLeft",	Settings_menuPositionAdjustPlacement },
	{ NLS0("BottomCenter"),		"menu.positionAdjustPlacement BottomCenter",Settings_menuPositionAdjustPlacement },
	{ NLS0("BottomRight"),		"menu.positionAdjustPlacement BottomRight",	Settings_menuPositionAdjustPlacement },
	{ NLS0("Custom"),			"menu.positionAdjustPlacement Custom",		Settings_menuPositionAdjustPlacement },
	{ NULL }
};

const struct cfgmenu cfg_sub_menubullet[] = {
    { NLS0("Default"),            "menu.bulletPosition default", Settings_menuBulletPosition_cfg },
    { NLS0("Left"),               "menu.bulletPosition left",    Settings_menuBulletPosition_cfg },
    { NLS0("Right"),              "menu.bulletPosition right",   Settings_menuBulletPosition_cfg },
    { NULL }
};

const struct cfgmenu cfg_sub_menuseparatorstyle[] = {
	{ NLS0("Gradient"),           "menu.separatorStyle gradient", Settings_menuSeparatorStyle },
	{ NLS0("Flat"),               "menu.separatorStyle flat",     Settings_menuSeparatorStyle },
	{ NLS0("Bevel"),              "menu.separatorStyle bevel",    Settings_menuSeparatorStyle },
	{ NLS0("None"),               "menu.separatorStyle none",     Settings_menuSeparatorStyle },
	{ "", NULL, NULL },
	{ NLS0("Full width"),         "menu.fullSeparatorWidth",      &Settings_menuFullSeparatorWidth },
	{ NLS0("Compact mode"),       "menu.compactSeparators",       &Settings_compactSeparators },
	{ NULL }
};

const struct cfgmenu cfg_sub_menuscroll[] = {
	{ NLS0("Default"),            "menu.scrollPosition default", Settings_menuScrollPosition_cfg },
	{ NLS0("Left"),               "menu.scrollPosition left",    Settings_menuScrollPosition_cfg },
	{ NLS0("Right"),              "menu.scrollPosition right",   Settings_menuScrollPosition_cfg },
	{ NLS0("None"),              "menu.scrollPosition none",   Settings_menuScrollPosition_cfg },
	{ NULL }
};

const struct cfgmenu cfg_sub_graphics[] = {
    { NLS0("Enable Background"),  "enableBackground",   &Settings_background_enabled },
    { NLS0("Smart Wallpaper"),    "smartWallpaper",     &Settings_smartWallpaper  },
    { "", NULL, NULL },
    { NLS0("*Nix Bullets"),       "bulletUnix",         &mStyle.bulletUnix  },
    { NLS0("*Nix Arrows"),        "arrowUnix",          &Settings_arrowUnix  },
    //{ NLS0("*Nix Metrics"),       "metricsUnix",        &Settings_newMetrics  },
    { "", NULL, NULL },
    { NLS0("Image Dithering"),    "imageDither",        &Settings_imageDither  },
    { NLS0("Global Font Override"), "globalFonts",      &Settings_globalFonts  },
    { NULL }
};

const struct cfgmenu cfg_sub_misc[] = {
    { NLS0("Desktop Margins"),    NULL, cfg_sub_dm },
    { NLS0("Focus Model"),        NULL, cfg_sub_focusmodel },
    { NLS0("Snap"),               NULL, cfg_sub_snap },
    { NLS0("Workspaces"),         NULL, cfg_sub_workspace },
    { "", NULL, NULL },
    { NLS0("Enable Toolbar"),     "toolbar.enabled",      &Settings_toolbarEnabled  },
    { NLS0("Opaque Window Move"), "opaqueMove",           &Settings_opaqueMove },
    { NLS0("Blackbox Editor"),    "setBBEditor",          &Settings_preferredEditor },
    { "", NULL, NULL },
    { NLS0("Show Appnames"),      "@BBCore.showAppnames", NULL },
    { NULL }
};

const struct cfgmenu cfg_sub_dm[] = {
    { NLS0("Top"),                "desktop.marginTop",    &Settings_desktopMargin.top  },
    { NLS0("Bottom"),             "desktop.marginBottom", &Settings_desktopMargin.bottom  },
    { NLS0("Left"),               "desktop.marginLeft",   &Settings_desktopMargin.left  },
    { NLS0("Right"),              "desktop.marginRight",  &Settings_desktopMargin.right  },
    { NLS0("Full Maximization"),  "fullMaximization" ,    &Settings_fullMaximization  },
    { NULL }
};

const struct cfgmenu cfg_sub_focusmodel[] = {
    { NLS0("Click To Focus"),     "focusModel ClickToFocus", Settings_focusModel },
    { NLS0("Sloppy Focus"),       "focusModel SloppyFocus",  Settings_focusModel },
    { NLS0("Auto Raise"),         "focusModel AutoRaise",    Settings_focusModel },
    { NULL }
};

const struct cfgmenu cfg_sub_snap[] = {
    { NLS0("Snap To Plugins"),    "snap.plugins", &Settings_edgeSnapPlugins },
    { NLS0("Padding"),            "snap.padding",  &Settings_edgeSnapPadding },
    { NLS0("Threshold"),          "snap.threshold", &Settings_edgeSnapThreshold },
    { NULL }
};

const struct cfgmenu cfg_sub_workspace[] = {
    { NLS0("Follow Active Task"), "workspaces.followActive",    &Settings_followActive },
    { NLS0("Follow Moved Task"),   "workspaces.followMoved",       &Settings_followMoved },
    { NLS0("Restore To Current"), "workspaces.restoreToCurrent", &Settings_restoreToCurrent },
    { NLS0("Alternate Method"),   "workspaces.altMethod",       &Settings_altMethod },
    //{ NLS0("Mousewheel Changing"), "workspaces.wheeling",     &Settings_desktopWheel },
    //{ NLS0("XP-Fix (Max'd Windows)"), "workspaces.xpfix",    &Settings_workspacesPCo },
    { NULL }
};

void RedrawConfigMenu(void)
{
    Menu_All_Update(MENU_IS_CONFIG);
}

//===========================================================================

static const struct cfgmenu * find_cfg_item(
    const char *cmd, const struct cfgmenu *pmenu, const struct cfgmenu **pp_menu)
{
    const struct cfgmenu *p;
    for (p = pmenu; p->text; ++p)
        if (NULL == p->command)
        {
            const struct cfgmenu* psub = (struct cfgmenu*)p->pvalue;
            if ((unsigned long)psub >= 100 && NULL != (psub = find_cfg_item(cmd, psub, pp_menu)))
                return psub;
        }
        else
        if (0==memicmp(cmd, p->command, strlen(p->command)))
        {
            *pp_menu = pmenu;
            return p;
        }
    return NULL;
}

const void *exec_internal_broam(
        const char *argument, const struct cfgmenu *menu_root,
        const struct cfgmenu **p_menu, const struct cfgmenu**p_item)
{
    const void *v = NULL;

    *p_item = find_cfg_item(argument, menu_root, p_menu);
    if (NULL == *p_item) return v;

    // scan for a possible argument to the command
    while (!IS_SPC(*argument)) ++argument;
    while (' ' == *argument) ++argument;

    v = (*p_item)->pvalue;
    if (NULL == v) return v;

    // now set the appropriate variable
    if (is_fixed_string(v) || is_string_item(v))
    {
        strcpy((char *)v, argument);
    }
    else
    if (get_int_item(v))
    {
        if (*argument) *(int*)v = atoi(argument);
    }
    else
    {
        set_bool((bool*)v, argument);
    }

    // write to blackbox.rc or extensions.rc (automatic)
    Settings_WriteRCSetting(v);
    return v;
}

void exec_cfg_command(const char *argument)
{
    // is it a plugin related command?
    if (0 == memicmp(argument, "plugin.", 7))
    {
        PluginManager_handleBroam(argument+7);
        RedrawConfigMenu();
        return;
    }

    // search the item in above structures
    const struct cfgmenu *p_menu, *p_item;
    const void *v = exec_internal_broam(argument, cfg_main, &p_menu, &p_item);
    if (NULL == v) return;

    // update the menu checkmarks
    RedrawConfigMenu();

    // now take care for some item-specific refreshes
	if (cfg_sub_menu == p_menu || cfg_sub_menubullet == p_menu || cfg_sub_menuseparatorstyle  == p_menu )
    {
        if (v == &Settings_menusExtensionSort)
            PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU|BBRG_FOLDER, 0);
        else
            PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_MENU, 0);
    }
    else
    if (cfg_sub_graphics == p_menu)
    {
        PostMessage(BBhwnd, BB_RECONFIGURE, 0, 0);
        if (v == &Settings_smartWallpaper)
            Desk_reset_rootCommand();
    }
    else
    if (cfg_sub_dm == p_menu)
    {
        SetDesktopMargin(NULL, BB_DM_REFRESH, 0);
    }
    else
    if (cfg_sub_workspace == p_menu)
    {
        Workspaces_Reconfigure();
    }
    else
    if (v == Settings_focusModel)
    {
        set_focus_model(Settings_focusModel);
    }
    else
    if (v == &Settings_opaqueMove)
    {
        set_opaquemove();
    }
    else
	if (v == &Session_screensaverEnabled)
	{
		SetScreenSaverActive(Session_screensaverEnabled);
	}
	else
    if (v == &Settings_toolbarEnabled)
    {
        if (Settings_toolbarEnabled)
            beginToolbar(hMainInstance);
        else
            endToolbar(hMainInstance);
    }
}

//===========================================================================

