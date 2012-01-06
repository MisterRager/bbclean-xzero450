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

#ifndef _BBSETTINGS_H_
#define _BBSETTINGS_H_

#ifndef BBSETTING
#define BBSETTING extern
#endif

#include "Stylestruct.h"

//===========================================================================
// Settings.cpp functions

void Settings_ReadRCSettings(void);
void Settings_ReadStyleSettings(void);
void Settings_WriteRCSetting(const void *);
int Settings_ItemSize(int w);
COLORREF get_bg_color(StyleItem *pSI);
COLORREF get_mixed_color(StyleItem *pSI);
int checkfont (const char *face);

//=====================================================
// Style definitions

BBSETTING StyleStruct mStyle;

//====================
// Toolbar Config
BBSETTING struct toolbar_setting {
    char placement[20];
    int  widthPercent;
    char strftimeFormat[40];
    bool onTop;
    bool autoHide;
    bool pluginToggle;
    bool enabled;
    bool alphaEnabled;
    int  alphaValue;
} Settings_toolbar;

//====================
// Menu Config
BBSETTING struct menu_setting {
    struct { int x, y; } pos;
    int		popupDelay;
	int		closeDelay;/* BlackboxZero 1.3.2012 */
    int		mouseWheelFactor;
	int		minWidth;/* BlackboxZero 12.17.2011 */
    int		maxWidth;
    char	openDirection[20];
    bool	onTop;
    bool	sticky;
    bool	pluginToggle;
    bool	showBroams;
    bool	showHiddenFiles;
    bool	sortByExtension;
    bool	drawSeparators;
    bool	snapWindow;
    bool	dropShadows;
    bool	alphaEnabled;
    int		alphaValue;
	/* BlackboxZero 1.3.2012 - */
	int		iconSize;
	int		iconSaturation;
	int		iconHue;
} Settings_menu;

//====================
// workspaces
BBSETTING bool Settings_styleXPFix;
BBSETTING bool Settings_followActive;
BBSETTING bool Settings_altMethod;
BBSETTING int  Settings_workspaces;
BBSETTING char Settings_workspaceNames[200];

//====================
// Plugin Snap
BBSETTING int Settings_snapThreshold;
BBSETTING int Settings_snapPadding;
BBSETTING bool Settings_snapPlugins;

//====================
// Desktop

// Margins
BBSETTING RECT Settings_desktopMargin;
BBSETTING bool Settings_fullMaximization;

// Background
BBSETTING bool Settings_enableBackground;
BBSETTING bool Settings_smartWallpaper;

// Options
BBSETTING bool Settings_desktopHook;
BBSETTING bool Settings_hideExplorer;
BBSETTING bool Settings_hideExplorerTray;

//====================
// Other

// window behaviour
BBSETTING bool Settings_opaqueMove;
BBSETTING char Settings_focusModel[40];
BBSETTING int  Settings_autoRaiseDelay;

// misc
BBSETTING char Settings_preferredEditor[MAX_PATH];
BBSETTING bool Settings_useDefCursor;
BBSETTING bool Settings_arrowUnix;
BBSETTING bool Settings_globalFonts;
BBSETTING bool Settings_imageDither;
BBSETTING bool Settings_shellContextMenu;
BBSETTING bool Settings_UTF8Encoding;
BBSETTING bool Settings_OldTray;
BBSETTING int Settings_contextMenuAdjust[2];
BBSETTING int Settings_LogFlag;

// feature select
BBSETTING bool Settings_disableTray;
BBSETTING bool Settings_disableDesk;
BBSETTING bool Settings_disableDDE;
BBSETTING bool Settings_disableVWM;
BBSETTING bool Settings_disableMargins;

//====================
// --- unused *nix settings ---
//BBSETTING bool Settings_focusLastWindow;
//BBSETTING bool Settings_focusNewWindows;
//BBSETTING char Settings_windowPlacement[40];
//BBSETTING char Settings_colPlacementDirection[40];
//BBSETTING char Settings_rowPlacementDirection[40];
//BBSETTING bool Settings_desktopWheel;

//===========================================================================
// Settings.cpp internal definitions

#define V_MAR 0x0200
/* BlackboxZero 1.4.2012 */
#define V_SHADOWX		0x2000
#define V_SHADOWY		0x4000
#define V_SHADOWCOLOR	0x8000
#define V_OUTLINECOLOR	0x10000
#define V_FROMSPLITTO	0x20000
#define	V_TOSPLITTO		0x40000

#define V_SHADOW (V_SHADOWX|V_SHADOWY|V_SHADOWCOLOR)

void ReadStyle(const char *style, StyleStruct *pStyle);

#ifdef BBSETTINGS_INTERNAL

#define V_TEX 0x0001
#define V_CO1 0x0002
#define V_CO2 0x0004
#define V_TXT 0x0008
#define V_PIC 0x0010
#define V_FON 0x0020
#define V_FHE 0x0040
#define V_FWE 0x0080
#define V_JUS 0x0100
#define V_MAR 0x0200
#define V_BOW 0x0400
#define V_BOC 0x0800
#define V_DIS 0x1000


struct items {
    short type;
    short sn;
    const char *rc_string;
    int sn_def;
    unsigned flags;
};

const struct items *GetStyleItems(void);
void* StyleStructPtr(int sn_index, StyleStruct *pStyle);

#define A_TEX (V_TEX|V_CO1|V_CO2|V_BOW|V_BOC)
#define A_FNT (V_FON|V_FHE|V_FWE|V_JUS)
#define I_DEF 0x10000
#define I_ACT 0x20000
#define I_BUL 0x40000

enum style_init_types
{
    C_INT = 1,
    C_BOL,
    C_STR,
    C_COL,
    C_STY,

    C_TEX,
    C_CO1,
    C_CO2,
    C_TXT,
    C_PIC,
    C_DIS,

    C_FON,
    C_FHE,
    C_FWE,
    C_JUS,

    C_MAR,
    C_BOC,
    C_BOW,

    C_SHT,
    C_SHP,
    C_SHE

	/* BlackboxZero 1.4.2012 */
	,
	C_SHAX,
	C_SHAY,
	C_CO5,
	C_CO6,
	C_CO1ST,
	C_CO2ST
};
#endif

//===========================================================================
#endif // _BBSETTINGS_H_
