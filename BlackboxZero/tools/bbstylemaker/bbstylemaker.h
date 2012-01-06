/*
 ============================================================================

  This file is part of the bbStyleMaker source code
  Copyright 2003-2009 grischka@users.sourceforge.net

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbStyleMaker is free software, released under the GNU General Public
  License (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#define APPNAME "bbStyleMaker"
#define APPNAME_VER "bbStyleMaker 1.31"

#include "bbroot.h"
#include "bbrc.h"

// ------------------------------------------------
typedef struct NStyleItem
{
    /* 0.0.80 */
    int bevelstyle;
    int bevelposition;
    int type;
    bool parentRelative;
    bool interlaced;

    /* 0.0.90 */
    COLORREF Color;
    COLORREF ColorTo;
    COLORREF TextColor;
    int FontHeight;
    int FontWeight;
    int Justify;
    int validated;  /* bblean internal */

    char Font[128];

    /* bbLean 1.16 */
    int nVersion;
    int marginWidth;
    int borderWidth;
    COLORREF borderColor;
    COLORREF foregroundColor;
    COLORREF disabledColor;
    bool bordered;
    bool FontShadow; /* xoblite */

    char reserved[102]; /* keep sizeof(StyleItem) = 300 */

    /* ------------------------- */
    bool is_070;

    struct style_ref {
        struct {
            COLORREF Color;
            COLORREF ColorTo;
            COLORREF TextColor;
            COLORREF foregroundColor;
            COLORREF disabledColor;
            COLORREF borderColor;
        } CREF;

        struct {
            COLORREF Color;
            COLORREF ColorTo;
            COLORREF TextColor;
            COLORREF foregroundColor;
            COLORREF disabledColor;
            COLORREF borderColor;
        } HREF;

        int FontHeight;
        int FontWeight;
        char Font[128];
    } style_ref;

    /* ------------------------- */

} NStyleItem;

#define ROOTCOMMAND_SIZE (MAX_PATH+80)

typedef struct NStyleStruct
{
    NStyleItem Toolbar;
    NStyleItem ToolbarButton;
    NStyleItem ToolbarButtonPressed;
    NStyleItem ToolbarLabel;
    NStyleItem ToolbarWindowLabel;
    NStyleItem ToolbarClock;

    NStyleItem MenuTitle;
    NStyleItem MenuFrame;
    NStyleItem MenuHilite;

    NStyleItem windowTitleFocus;
    NStyleItem windowLabelFocus;
    NStyleItem windowHandleFocus;
    NStyleItem windowGripFocus;
    NStyleItem windowButtonFocus;
    NStyleItem windowButtonPressed;

    NStyleItem windowTitleUnfocus;
    NStyleItem windowLabelUnfocus;
    NStyleItem windowHandleUnfocus;
    NStyleItem windowGripUnfocus;
    NStyleItem windowButtonUnfocus;

    NStyleItem windowFrameFocus;
    NStyleItem windowFrameUnfocus;

    NStyleItem Slit;

    COLORREF borderColor;
    int borderWidth;
    int bevelWidth;
    int handleHeight;
    char menuBullet[16];
    char menuBulletPosition[16];
    char rootCommand[ROOTCOMMAND_SIZE];
    bool menuTitleLabel;
    bool menuNoTitle;

    struct rootinfo rootInfo;
    NStyleItem rootStyle;
    bool is_070;

} NStyleStruct;

// ------------------------------------------------
extern char rcpath[];
extern StyleStruct gui_style;
extern const char *style_info_keys[];
extern char *style_info[5];
extern bool write_070;
#define STYLEITEM_VERSION 3
void parse_font(StyleItem *si, const char *font);

// ------------------------------------------------
extern NStyleStruct work_style;
extern struct NStyleItem *P0, *B0;
extern bool P0_dis;
extern struct NStyleItem Palette[10];

/*----------------------------------------------------------------------------*/

int bb_msgbox(HWND hwnd, const char *s, const char *t, int f);
int bbstylemaker_create(void);

int get_save_filename(HWND hwnd, char *buffer, const char *title, const char *filter);
int choose_font(HWND hwnd, NStyleItem *pSI);

int is_style_old(const char *style);
int is_style_changed(void);
int readstyle(const char *fname, StyleStruct*, int root);
int writestyle(const char *fname, StyleStruct*, char **style_info, int flags);
void bb_rcreader_init(void);

int Settings_ItemSize(int i);
int bsetroot_parse(NStyleStruct *pss, const char *command);
void make_bsetroot_string(NStyleStruct *pss, char *out, int all);
int get_bulletstyle (const char *tmp);
int get_bulletpos (const char *tmp);
const char * get_bullet_string (int s);

/*----------------------------------------------------------------------------*/
typedef union {
    COLORREF cr;
    struct {
        unsigned char Red;
        unsigned char Green;
        unsigned char Blue;
        unsigned char Alpha;
    } rgb;
} RGBREF;

RGBREF _RGBtoHSL(RGBREF lRGBColor);
RGBREF _HSLtoRGB(RGBREF lHSLColor);
RGBREF _YUVtoRGB(RGBREF lYUVColor);
RGBREF _RGBtoYUV(RGBREF lRGBColor);
RGBREF _YIQtoRGB(RGBREF lYIQColor);
RGBREF _RGBtoYIQ(RGBREF lRGBColor);
RGBREF _XYZtoRGB(RGBREF lXYZColor);
RGBREF _RGBtoXYZ(RGBREF lRGBColor);

#define RGBtoHSL(r) _RGBtoHSL(*(RGBREF*)&(r)).cr
#define HSLtoRGB(r) _HSLtoRGB(*(RGBREF*)&(r)).cr
#define YUVtoRGB(r) _YUVtoRGB(*(RGBREF*)&(r)).cr
#define RGBtoYUV(r) _RGBtoYUV(*(RGBREF*)&(r)).cr
#define YIQtoRGB(r) _YIQtoRGB(*(RGBREF*)&(r)).cr
#define RGBtoYIQ(r) _RGBtoYIQ(*(RGBREF*)&(r)).cr
#define XYZtoRGB(r) _XYZtoRGB(*(RGBREF*)&(r)).cr
#define RGBtoXYZ(r) _RGBtoXYZ(*(RGBREF*)&(r)).cr

/*----------------------------------------------------------------------------*/

