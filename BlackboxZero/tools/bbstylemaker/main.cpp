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
// bbStyleMaker main.cpp

#define BBLEAN_116m

#include "BBApi.h"
#include "win0x500.h"
#include "bblib.h"
#define BBSETTINGS_INTERNAL
#include "Settings.h"
#include "stylestruct.h"
#include "bimage.h"
#include "BBSendData.h"
#include "bbstylemaker.h"
#include "bbdialog.h"
#include "bImage.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <time.h>
#include <locale.h>

#define ST static

const char HELPMSG[] =
    APPNAME_VER
    "\n---"
    "\nCopyright 2003-2009 grischka"
    "\nhttp://bb4win.sourceforge.net/bblean"
#if 0
    "\n"
    "\n"
    "Quick Reference:"
    "\n---"
    "\nDragging the mouse from the topmost color display is a"
    "\ncolor picker."
    "\n---"
    "\nThe other nine color boxes are palettes to store a style-"
    "\nproperty temporarily."
    "\n---"
    "\nLeft-click a palette entry to set the currently selected"
    "\nstyle item."
    "\n---"
    "\nRight-click a palette entry to set the palette itself."
    "\n---"
    "\nHold the control key to link/unlink the sliders temporarily."
    "\n---"
    "\n*border and *font apply changes to several elements at"
    "\nthe same time."
    "\n---"
#endif
    ;

/*----------------------------------------------------------------------------*/
// global variables

#define UPDOWN_TIMER 2
#define UPDATE_TIMER 3
#define SLIDER_TIMER 4

HINSTANCE g_hInstance;

// style for bbstylemaker
StyleStruct gui_style;

// dialog and buttons
#include "bbdialog.cpp"

const char *fname(const char *path);
void set_style_defaults(NStyleStruct *pss, int f);
#define SSD_OPT_SENDBB 0
#define SSD_OPT_FIRST 1
#define SSD_OPT_WRITE 2

// blackbox window
HWND BBhwnd;
StyleStruct bb_style;

// bbStyleMaker.rc
char rcpath[MAX_PATH];

// style output option
bool write_070;
bool read_070;

// original style
NStyleStruct ref_style;
char ref_rootCommand[ROOTCOMMAND_SIZE];

// style in work
NStyleStruct work_style;

// filename of style in work
char work_stylefile[MAX_PATH];

// saved palette during screen picking
struct NStyleItem save_palette;

// currently active StyleItem or NULL;
struct NStyleItem *P0;
bool P0_dis; // using disableColor

// border selection
struct NStyleItem *B0;

// color palettes
struct NStyleItem Palette[10];

// font as shown in the gui
static char fontname[64];
// hex colors as shown in the gui
static char cbuf1[32];
static char cbuf2[32];

// sliders lock info
struct c_lock
{
    COLORREF *cptr;
    COLORREF *hslptr;
    COLORREF *cref;
    bool lock;
    bool lock_ctrl;
};

struct c_lock L1, L2;
struct c_lock *pL1, *pL2;

#define PREF(s) (&(s)->style_ref)
#define PREF_SIZE sizeof P0->style_ref
#define CREF_SIZE (sizeof(P0->style_ref.CREF)*2)

#define RFONT(s) (&(s)->style_ref)

#define COPYREF(d,s,C) \
    (PREF(d)->CREF.C = PREF(s)->CREF.C, \
     PREF(d)->HREF.C = PREF(s)->HREF.C)

#define SETREF(LOCK,C) \
    ((p##LOCK = &LOCK)->cptr = &P0->C, \
      p##LOCK->cref = &PREF(P0)->CREF.C, \
      p##LOCK->hslptr = &PREF(P0)->HREF.C)


// current state of selected options:
const int n_sections = 5;
// const int n_items[5] = { 6, 5, 7, 7, 2 };
const int n_colorsel = 3;

int v_section;
int v_item;
int a_items[8];
int v_colorsel;
bool v_all_border;

int v_colorsel_set;
bool v_upd_all;
int style_version;
int v_hsl;
int v_changed;
int v_root_changed;


// forward decls
LRESULT CALLBACK GUIProc(HWND, UINT, WPARAM, LPARAM);
void fix_P0(void);
void set_font_default(NStyleItem *pSI);
void adjust_style_065(NStyleStruct *pss);

void new_flags(void) {
    ref_style = work_style;
    v_changed = false;
    v_root_changed = false;
}

/*----------------------------------------------------------------------------*/
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int iCmdShow)
{
    HWND hwnd;

    // are we already running
    hwnd = FindWindow(APPNAME, NULL);
    if (hwnd) {
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        return 0;
    }

    g_hInstance = hInstance;
    set_my_path(NULL, rcpath, APPNAME ".rc");
    bimage_init(true, true);
    bb_rcreader_init();

    if (0 == bbstylemaker_create())
        return 1;

    return do_message_loop();
}

/*----------------------------------------------------------------------------*/
const char *fname(const char *path)
{
    const char *p = strchr(path, 0);
    while (p > path && p[-1] != '\\' && p[-1] != '/')
        --p;
    return p;
}

const char *fextension(const char *path)
{
    const char *e, *p;
    p = fname(path);
    e = strrchr(p, '.');
    return e ? e : strchr(p, 0);
}

/*----------------------------------------------------------------------------*/
// dialog resource

const struct button main_buttons[] = {
#if 0
#include "dlgitems.rc"
#else

{ ""              , BN_RECT  , SEC_RCT  ,  10,  32,  74, 136, BN_EXT },
{ "section"       , BN_STR   , SEC_HDR  ,  10,  32,  74,  18, BN_EXT },

{ "toolbar"       , BN_CHK   , SEC_TOO  ,  10,  54,  74,  14, BN_RAD|BN_GRP },
{ "menu"          , BN_CHK   , SEC_MEN  ,  10,  70,  74,  14, BN_RAD },
{ "window f"      , BN_CHK   , SEC_WIF  ,  10,  86,  74,  14, BN_RAD },
{ "window u"      , BN_CHK   , SEC_WIU  ,  10, 102,  74,  14, BN_RAD },
{ "slit"          , BN_CHK   , SEC_SLI  ,  10, 118,  74,  14, BN_RAD },
{ "other"         , BN_CHK   , SEC_MIS  ,  10, 134,  74,  14, BN_RAD },

{ ""              , BN_RECT  , ITM_RCT  ,  10, 172,  74, 136, BN_EXT },
{ "item"          , BN_STR   , ITM_HDR  ,  10, 172,  74,  18, BN_EXT },

{ "frame"         , BN_CHK   , TOO_BAC  ,  10, 194,  74,  14, BN_RAD|BN_GRP },
{ "button"        , BN_CHK   , TOO_BUT  ,  10, 210,  74,  14, BN_RAD },
{ "button p"      , BN_CHK   , TOO_BUP  ,  10, 226,  74,  14, BN_RAD },
{ "label"         , BN_CHK   , TOO_LAB  ,  10, 242,  74,  14, BN_RAD },
{ "winlabel"      , BN_CHK   , TOO_WLB  ,  10, 258,  74,  14, BN_RAD },
{ "clock"         , BN_CHK   , TOO_CLK  ,  10, 274,  74,  14, BN_RAD },

{ "title"         , BN_CHK   , MEN_TIT  ,  10, 194,  74,  14, BN_RAD|BN_GRP },
{ "frame"         , BN_CHK   , MEN_FRM  ,  10, 210,  74,  14, BN_RAD },
{ "active"        , BN_CHK   , MEN_HIL  ,  10, 226,  74,  14, BN_RAD },
{ "bullet"        , BN_CHK   , MEN_BUL  ,  10, 242,  74,  14, BN_RAD },
{ "label"         , BN_CHK   , MEN_LBL  ,  10, 266,  74,  14, 0 },
{ "hidden"        , BN_CHK   , MEN_NTI  ,  10, 282,  74,  14, 0 },

{ "title"         , BN_CHK   , WIN_TIT  ,  10, 194,  74,  14, BN_RAD|BN_GRP },
{ "label"         , BN_CHK   , WIN_LAB  ,  10, 210,  74,  14, BN_RAD },
{ "handle"        , BN_CHK   , WIN_HAN  ,  10, 226,  74,  14, BN_RAD },
{ "grip"          , BN_CHK   , WIN_GRP  ,  10, 242,  74,  14, BN_RAD },
{ "button"        , BN_CHK   , WIN_BUT  ,  10, 258,  74,  14, BN_RAD },
{ "button p"      , BN_CHK   , WIN_BUP  ,  10, 274,  74,  14, BN_RAD },
{ "frame"         , BN_CHK   , WIN_FRM  ,  10, 290,  74,  14, BN_RAD },

{ "frame"         , BN_CHK   , SLT_FRM  ,  10, 194,  74,  14, BN_RAD|BN_GRP|BN_ON },

{ "bsetroot"      , BN_CHK   , MIS_ROT  ,  10, 194,  74,  14, BN_RAD|BN_GRP },
{ "styleinfo"     , BN_CHK   , MIS_INF  ,  10, 210,  74,  14, BN_RAD },
{ "edit rc"       , BN_BTN   , CMD_CFG  ,  12, 270,  68,  16, 0 },
{ "reload rc"     , BN_BTN   , CMD_RST  ,  12, 290,  68,  16, 0 },

{ ""              , BN_RECT  , GRD_RCT  ,  90,  32,  82, 208, BN_EXT },
{ "texture"       , BN_STR   , GRD_HDR  ,  90,  32,  82,  18, BN_EXT },

{ "solid"         , BN_CHK   , GRD_SOL  ,  90,  70,  82,  14, BN_RAD|BN_GRP },
{ "horizontal"    , BN_CHK   , GRD_HOR  ,  90,  86,  82,  14, BN_RAD },
{ "vertical"      , BN_CHK   , GRD_VER  ,  90, 102,  82,  14, BN_RAD },
{ "diagonal"      , BN_CHK   , GRD_DIA  ,  90, 118,  82,  14, BN_RAD },
{ "crossdiag."    , BN_CHK   , GRD_CDI  ,  90, 134,  82,  14, BN_RAD },
{ "pipecross"     , BN_CHK   , GRD_PIP  ,  90, 150,  82,  14, BN_RAD },
{ "elliptic"      , BN_CHK   , GRD_ELL  ,  90, 166,  82,  14, BN_RAD },
{ "rectangle"     , BN_CHK   , GRD_REC  ,  90, 182,  82,  14, BN_RAD },
{ "pyramid"       , BN_CHK   , GRD_PYR  ,  90, 198,  82,  14, BN_RAD },
{ "parentrel"     , BN_CHK   , GRD_PRR  ,  90,  54,  82,  14, BN_RAD },
{ "interlaced"    , BN_CHK   , GRD_INL  ,  90, 222,  82,  14, 0 },

{ ""              , BN_RECT  , BEV_RCT  , 178,  32,  74, 208, BN_EXT },
{ "bevel"         , BN_STR   , BEV_HDR  , 178,  32,  74,  18, BN_EXT },

{ "flat"          , BN_CHK   , BEV_FLA  , 178,  54,  74,  14, BN_RAD|BN_GRP },
{ "raised"        , BN_CHK   , BEV_RAI  , 178,  70,  74,  14, BN_RAD },
{ "sunken"        , BN_CHK   , BEV_SUN  , 178,  86,  74,  14, BN_RAD },
{ "bevel 2"       , BN_CHK   , BEV_P2   , 178, 106,  74,  14, 0 },
{ "border"        , BN_STR   , BOR_HDR  , 178, 200,  74,  18, BN_EXT },
{ "width"         , BN_UPDN  , BOR_WID  , 178, 222,  74,  14, BN_16 },
{ "margin"        , BN_STR   , MAR_HDR  , 178, 128,  74,  18, BN_EXT },
{ "frame"         , BN_UPDN  , MAR_WID1A, 178, 150,  74,  14, BN_16 },
{ "label"         , BN_UPDN  , MAR_WID1B, 178, 166,  74,  14, BN_16 },
{ "button"        , BN_UPDN  , MAR_WID1C, 178, 182,  74,  14, BN_16 },
{ "title"         , BN_UPDN  , MAR_WID2A, 178, 150,  74,  14, BN_16 },
{ "frame"         , BN_UPDN  , MAR_WID2B, 178, 166,  74,  14, BN_16 },
{ "active"        , BN_UPDN  , MAR_WID2C, 178, 182,  74,  14, BN_16 },
{ "title"         , BN_UPDN  , MAR_WID3A, 178, 150,  74,  14, BN_16 },
{ "label"         , BN_UPDN  , MAR_WID3B, 178, 166,  74,  14, BN_16 },
{ "button"        , BN_UPDN  , MAR_WID3C, 178, 182,  74,  14, BN_16 },
{ "frame"         , BN_UPDN  , MAR_WID4A, 178, 150,  74,  14, BN_16 },
{ "handle"        , BN_STR   , HAN_HDR  , 178, 128,  74,  18, BN_EXT },
{ "height"        , BN_UPDN  , HAN_SIZ  , 178, 150,  74,  14, BN_16 },

{ ""              , BN_RECT  , COL_RCT  , 258, 152,  74,  88, BN_EXT },
{ "colors"        , BN_STR   , COL_HDR  , 258, 152,  74,  18, BN_EXT },

{ "color 1/2"     , BN_CHK   , COL_GRD  , 258, 174,  74,  14, BN_RAD|BN_GRP },
{ "text"          , BN_CHK   , COL_TXT  , 258, 190,  74,  14, BN_RAD },
{ "border"        , BN_CHK   , COL_BOR  , 258, 222,  74,  14, BN_RAD },
{ "disabled"      , BN_CHK   , COL_DIS  , 258, 206,  74,  14, BN_RAD },
{ ""              , BN_SLD   , SLD_R1   , 346,  16,  16, 276, 0 },
{ ""              , BN_SLD   , SLD_G1   , 370,  16,  16, 276, 0 },
{ ""              , BN_SLD   , SLD_B1   , 394,  16,  16, 276, 0 },
{ ""              , BN_SLD   , SLD_R2   , 480,  16,  16, 276, 0 },
{ ""              , BN_SLD   , SLD_G2   , 504,  16,  16, 276, 0 },
{ ""              , BN_SLD   , SLD_B2   , 528,  16,  16, 276, 0 },
{ "grey"          , BN_BTN   , COL_GR1  , 346, 294,  32,  16, BN_EXT },
{ "grey"          , BN_BTN   , COL_GR2  , 482, 294,  32,  16, BN_EXT },
{ "link"          , BN_CHK   , COL_LK1  , 378, 294,  32,  16, BN_CEN },
{ "link"          , BN_CHK   , COL_LK2  , 514, 294,  32,  16, BN_CEN },

{ "rgb"           , BN_CHK   , COL_RGB  , 418, 294,  26,  16, BN_CEN|BN_RAD|BN_GRP },
{ "hsl"           , BN_CHK   , COL_HSL  , 446, 294,  26,  16, BN_CEN|BN_RAD },
{ "X"             , BN_BTN   , COL_SW0  , 436, 274,  18,  16, 0 },
{ "<"             , BN_BTN   , COL_SW1  , 416, 274,  18,  16, 0 },
{ ">"             , BN_BTN   , COL_SW2  , 456, 274,  18,  16, 0 },
{ "42  00  00"    , BN_STR   , COL_HX1  , 336,   4,  82,  16, 0 },
{ "00  00  00"    , BN_STR   , COL_HX2  , 470,   4,  82,  16, 0 },
{ "hue  sat  lum" , BN_STR   , COL_HU1  , 342, 294,  74,  16, 0 },
{ "hue  sat  lum" , BN_STR   , COL_HU2  , 474, 294,  78,  16, 0 },
{ ""              , BN_COLOR , PAL_X    , 420,  18,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_1    , 420,  58,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_2    , 420,  82,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_3    , 420, 106,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_4    , 420, 130,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_5    , 420, 154,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_6    , 420, 178,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_7    , 420, 202,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_8    , 420, 250,  50,  22, 0 },
{ ""              , BN_COLOR , PAL_9    , 420, 226,  50,  22, 0 },

{ ""              , BN_RECT  , FNT_RCT  ,  90, 246, 242,  62, BN_EXT },
{ "toolbar font"  , BN_STR   , FNT_HDR  ,  90, 246, 242,  18, BN_EXT },
{ ""              , BN_EDT   , FNT_NAM  ,  98, 268, 160,  18, 0 },
{ "bold"          , BN_CHK   , FNT_BOL  , 264, 270,  36,  14, BN_CEN },
{ "..."           , BN_BTN   , FNT_CHO  , 304, 269,  24,  16, 0 },
{ "height"        , BN_UPDN  , FNT_HEI  ,  94, 290,  82,  14, 0 },
//{ "fonts"         , BN_UPDN  , FNT_ART  , 258,  86,  74,  14, 0 },

{ "left"          , BN_CHK   , FNT_LEF  , 190, 290,  38,  14, BN_CEN|BN_RAD|BN_GRP },
{ "center"        , BN_CHK   , FNT_CEN  , 232, 290,  54,  14, BN_CEN|BN_RAD },
{ "right"         , BN_CHK   , FNT_RIG  , 290, 290,  38,  14, BN_CEN|BN_RAD },
{ "bullet"        , BN_STR   , BUL_HDR  ,  90,  32,  82,  18, BN_EXT },

{ "empty"         , BN_CHK   , BUL_EMP  ,  90,  54,  82,  14, BN_RAD|BN_GRP },
{ "triangle"      , BN_CHK   , BUL_TRI  ,  90,  78,  82,  14, BN_RAD },
{ "square"        , BN_CHK   , BUL_SQR  ,  90,  94,  82,  14, BN_RAD },
{ "diamond"       , BN_CHK   , BUL_DIA  ,  90, 110,  82,  14, BN_RAD },
{ "circle"        , BN_CHK   , BUL_CIR  ,  90, 126,  82,  14, BN_RAD },

{ "left"          , BN_CHK   , BUL_LEF  ,  90, 150,  82,  14, BN_RAD|BN_GRP },
{ "right"         , BN_CHK   , BUL_RIG  ,  90, 166,  82,  14, BN_RAD },

{ ""              , BN_RECT  , OPT_RCT  , 258,  32,  74,  72, BN_EXT },
{ "option"        , BN_STR   , OPT_HDR  , 258,  32,  74,  18, BN_EXT },
{ "*border"       , BN_CHK   , BOR_DEF  , 258,  54,  74,  14, 0 },
{ "*font"         , BN_BTN   , FNT_DEF  , 258,  70,  74,  14, BN_EXT },
{ "modula"        , BN_STR   , ROT_MHD  , 178, 168,  74,  18, BN_EXT },
{ "mod"           , BN_CHK   , ROT_MOD  , 178, 190,  74,  14, 0 },
{ "x"             , BN_UPDN  , ROT_MDX  , 178, 206,  74,  14, 0 },
{ "y"             , BN_UPDN  , ROT_MDY  , 178, 222,  74,  14, 0 },

{ ""              , BN_RECT  , ROT_ORC  , 258,  32,  74,  72, BN_EXT },
{ "image"         , BN_STR   , ROT_OHD  , 258,  32,  74,  18, BN_EXT },
{ "scale"         , BN_UPDN  , ROT_SCL  , 258,  86,  74,  14, 0 },
{ "sat"           , BN_UPDN  , ROT_SAT  , 258,  54,  74,  14, BN_255 },
{ "hue"           , BN_UPDN  , ROT_HUE  , 258,  70,  74,  14, BN_255 },

{ ""              , BN_RECT  , ROT_IRC  ,  90, 246, 242,  62, BN_EXT },
{ "image file"    , BN_STR   , ROT_IHD  ,  90, 246, 242,  18, BN_EXT },

{ "tile"          , BN_CHK   , ROT_TIL  , 186, 290,  42,  14, BN_CEN|BN_RAD|BN_GRP },
{ "center"        , BN_CHK   , ROT_CEN  , 230, 290,  54,  14, BN_CEN|BN_RAD },
{ "full"          , BN_CHK   , ROT_FUL  , 286, 290,  38,  14, BN_CEN|BN_RAD },
{ ""              , BN_EDT   , ROT_IMG  ,  98, 266, 226,  18, 0 },

{ ""              , BN_RECT  , INF_RCT  , 106,  54, 430, 130, BN_EXT },
{ "style information", BN_STR   , INF_HDR  , 106,  54, 430,  18, BN_EXT },
{ "clear"         , BN_BTN   , INF_CLR  , 474, 198,  58,  16, 0 },
{ "auto"          , BN_BTN   , INF_NAM  , 488,  78,  42,  16, 0 },
{ "auto"          , BN_BTN   , INF_AUT  , 488,  98,  42,  16, 0 },
{ "auto"          , BN_BTN   , INF_DAT  , 488, 118,  42,  16, 0 },
{ "name"          , BN_STR   , INF_ST1  , 114,  78,  74,  18, BN_LFT },
{ "author"        , BN_STR   , INF_ST2  , 114,  98,  74,  18, BN_LFT },
{ "date"          , BN_STR   , INF_ST3  , 114, 118,  74,  18, BN_LFT },
{ "credits"       , BN_STR   , INF_ST4  , 114, 138,  74,  18, BN_LFT },
{ "comment"       , BN_STR   , INF_ST5  , 114, 158,  74,  18, BN_LFT },
{ ""              , BN_EDT   , INF_LN1  , 190,  78, 294,  18, 0 },
{ ""              , BN_EDT   , INF_LN2  , 190,  98, 294,  18, 0 },
{ ""              , BN_EDT   , INF_LN3  , 190, 118, 294,  18, 0 },
{ ""              , BN_EDT   , INF_LN4  , 190, 138, 338,  18, 0 },
{ ""              , BN_EDT   , INF_LN5  , 190, 158, 338,  18, 0 },
{ "edit"          , BN_BTN   , CMD_EDI  ,  12,   8,  40,  16, 0 },
{ "save"          , BN_BTN   , CMD_SAV  ,  56,   8,  44,  16, 0 },
{ "save as"       , BN_BTN   , CMD_SAA  , 104,   8,  62,  16, 0 },
{ "revert"        , BN_BTN   , CMD_REV  , 170,   8,  52,  16, 0 },
{ "quit"          , BN_BTN   , CMD_QUI  , 226,   8,  38,  16, 0 },
{ "?"             , BN_BTN   , CMD_ABO  , 268,   8,  20,  16, 0 },
//{ "r"             , BN_BTN   , CMD_DOC  , 292,   8,  20,  16, 0 },

#endif
{NULL}
};

/*----------------------------------------------------------------------------*/
LRESULT CALLBACK main_dlg_proc (struct dlg *dlg, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void load_guistyle(struct dlg *dlg, const char *filename)
{
    if (NULL == filename)
        filename = ReadString(rcpath, "bbstylemaker.guistyle", "");

    if (0 == filename[0])
        filename = rcpath;

    readstyle(filename, &gui_style, 0);

    dlg->dx = ReadInt(rcpath, "bbstylemaker.widthpercent", 100);
    dlg->dy = ReadInt(rcpath, "bbstylemaker.heightpercent", 100);
    if (dlg->hwnd) {
        fix_dlg(dlg);
        set_dlg_windowpos(dlg);
    }
}

int bbstylemaker_create(void)
{
    struct dlg *dlg = make_dlg(main_buttons, 560, 320);
    int xp, yp;

    xp = ReadInt(rcpath, "bbstylemaker.xpos", -1);
    yp = ReadInt(rcpath, "bbstylemaker.ypos", -1);
    dlg->captionbar = ReadBool(rcpath, "bbstylemaker.captionbar", false);
    dlg->typ = D_DLG;

    load_guistyle(dlg, NULL);
    if (0 == make_dlg_wnd(dlg, NULL, xp, yp, APPNAME, main_dlg_proc))
        return 0;
    return 1;
}

/*----------------------------------------------------------------------------*/
/* conversions StyleStruct <-> bbStyleMaker's private extended NStyleStruct */

void copy_item_n(NStyleItem *to, StyleItem *from)
{
    memcpy(to, from, sizeof *from);
    memset((char*)to + sizeof *from, 0, sizeof *to - sizeof *from);
}

void copy_item_o(StyleItem *to, NStyleItem *from)
{
    memcpy(to, from, sizeof *to);
}

#define COPYITEM_N(to, from, I) copy_item_n(&to->I, &from->I)
#define COPYITEM_O(to, from, I) copy_item_o(&to->I, &from->I)
#define COPYINT(to, from, I) to->I = from->I
#define COPYBOOL(to, from, I) to->I = from->I
#define COPYCOLOR(to, from, I) to->I = from->I
#define COPYCHAR(to, from, I) memcpy(to->I, from->I, sizeof to->I)

void copy_to_N(NStyleStruct *to, StyleStruct *from)
{
    memset(to, 0, sizeof *to);

    COPYITEM_N(to, from, Toolbar);
    COPYITEM_N(to, from, ToolbarButton);
    COPYITEM_N(to, from, ToolbarButtonPressed);
    COPYITEM_N(to, from, ToolbarLabel);
    COPYITEM_N(to, from, ToolbarWindowLabel);
    COPYITEM_N(to, from, ToolbarClock);

    COPYITEM_N(to, from, MenuTitle);
    COPYITEM_N(to, from, MenuFrame);
    COPYITEM_N(to, from, MenuHilite);

    COPYITEM_N(to, from, windowTitleFocus);
    COPYITEM_N(to, from, windowLabelFocus);
    COPYITEM_N(to, from, windowHandleFocus);
    COPYITEM_N(to, from, windowGripFocus);
    COPYITEM_N(to, from, windowButtonFocus);
    COPYITEM_N(to, from, windowButtonPressed);

    COPYITEM_N(to, from, windowTitleUnfocus);
    COPYITEM_N(to, from, windowLabelUnfocus);
    COPYITEM_N(to, from, windowHandleUnfocus);
    COPYITEM_N(to, from, windowGripUnfocus);
    COPYITEM_N(to, from, windowButtonUnfocus);

    COPYITEM_N(to, from, Slit);

    COPYCHAR(to, from, menuBullet);
    COPYCHAR(to, from, menuBulletPosition);
    COPYCHAR(to, from, rootCommand);
    COPYBOOL(to, from, is_070);
    COPYBOOL(to, from, menuTitleLabel);
    COPYBOOL(to, from, menuNoTitle);
    COPYINT(to, from, handleHeight);

    COPYCOLOR(to, from, borderColor);
    COPYINT(to, from, borderWidth);
    COPYINT(to, from, bevelWidth);

    to->windowFrameFocus.borderColor = from->windowFrameFocusColor;
    to->windowFrameUnfocus.borderColor = from->windowFrameUnfocusColor;
    to->windowFrameFocus.borderWidth =
    to->windowFrameUnfocus.borderWidth = from->frameWidth;
    to->windowFrameFocus.parentRelative = true;
    to->windowFrameUnfocus.parentRelative = true;
}

void copy_from_N(StyleStruct *to, NStyleStruct *from)
{
    COPYITEM_O(to, from, Toolbar);
    COPYITEM_O(to, from, ToolbarButton);
    COPYITEM_O(to, from, ToolbarButtonPressed);
    COPYITEM_O(to, from, ToolbarLabel);
    COPYITEM_O(to, from, ToolbarWindowLabel);
    COPYITEM_O(to, from, ToolbarClock);

    COPYITEM_O(to, from, MenuTitle);
    COPYITEM_O(to, from, MenuFrame);
    COPYITEM_O(to, from, MenuHilite);

    COPYITEM_O(to, from, windowTitleFocus);
    COPYITEM_O(to, from, windowLabelFocus);
    COPYITEM_O(to, from, windowHandleFocus);
    COPYITEM_O(to, from, windowGripFocus);
    COPYITEM_O(to, from, windowButtonFocus);
    COPYITEM_O(to, from, windowButtonPressed);

    COPYITEM_O(to, from, windowTitleUnfocus);
    COPYITEM_O(to, from, windowLabelUnfocus);
    COPYITEM_O(to, from, windowHandleUnfocus);
    COPYITEM_O(to, from, windowGripUnfocus);
    COPYITEM_O(to, from, windowButtonUnfocus);

    COPYITEM_O(to, from, Slit);

    COPYCHAR(to, from, menuBullet);
    COPYCHAR(to, from, menuBulletPosition);
    COPYCHAR(to, from, rootCommand);

    COPYBOOL(to, from, is_070);
    COPYBOOL(to, from, menuTitleLabel);
    COPYBOOL(to, from, menuNoTitle);
    COPYINT(to, from, handleHeight);

    COPYCOLOR(to, from, borderColor);
    COPYINT(to, from, borderWidth);
    COPYINT(to, from, bevelWidth);

    to->windowFrameFocusColor = from->windowFrameFocus.borderColor;
    to->windowFrameUnfocusColor = from->windowFrameUnfocus.borderColor;
    to->frameWidth = from->windowFrameFocus.borderWidth;
}

/*----------------------------------------------------------------------------*/
/* file was dropped on the window */

void drop_guistyle(struct dlg *dlg, void* hdrop)
{
    char filename[MAX_PATH];
    int n;

    filename[0]=0;
    if (NULL == hdrop)
        return;

    n = DragQueryFile((HDROP)hdrop, 0, filename, sizeof(filename));
    DragFinish((HDROP)hdrop);
    if (0 == n)
        return;

#if 0
    if (FALSE == CopyFile(filename, rcpath, FALSE))
        return;
    strcpy(filename, rcpath);
#endif

    load_guistyle(dlg, filename);
    WriteString(rcpath, "bbstylemaker.guistyle", filename);
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* screen picker */

COLORREF get_screen_pixel(void)
{
    HWND dw;
    HDC dc;
    POINT pt;
    COLORREF pixel;

    dw = GetDesktopWindow();
    dc = GetWindowDC(dw);
    GetCursorPos(&pt);
    pixel = GetPixel(dc, pt.x, pt.y);
    ReleaseDC(dw, dc);
    return pixel;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* find blackbox window */

HWND GetBBWnd(void)
{
    HWND hwnd;
    hwnd = FindWindow("BlackBoxClass", "BlackBox");
    if (NULL == hwnd)
        hwnd = FindWindow("xoblite", NULL);
    return hwnd;
}

struct getdata_info {
    int msg;
    char *dest;
};

void bbgetdata(HWND hwnd, unsigned msg, void *dest)
{
    struct getdata_info gdi;
    gdi.msg = msg;
    gdi.dest = (char*)dest;
    SendMessage(BBhwnd, msg, (WPARAM)&gdi, (LPARAM)hwnd);
}

bool link_to_BB(HWND hwnd)
{
    char temp[MAX_PATH];
    BBhwnd = GetBBWnd();
    if (NULL == BBhwnd)
        return 0;
    temp[0] = 0;
    bbgetdata(hwnd, BB_GETSTYLE, temp);
    if (temp[0] < 32) {
        BBhwnd = NULL;
        return 0;
    }
    return 1;
}

/*----------------------------------------------------------------------------*/
/* open editor */

void edit_file(const char *file)
{
    char buffer[MAX_PATH];
    const char *e;

    e = ReadString(rcpath, "bbstylemaker.editor", "");
    if (*e) {
        sprintf(buffer, "%s \"%s\"", e, file);
        if (WinExec(buffer, SW_SHOWNORMAL) > 31)
            return;
    }

    if (BBhwnd) {
        SetForegroundWindow(BBhwnd);
        BBSendData(BBhwnd, BB_EDITFILE, (WPARAM)-1, file, -1);
    }
}

/*----------------------------------------------------------------------------*/
/* try to find out whats the style with oldbb/xob */

int get_xobstyle(char *temp)
{
    const char *p, *r;
    char b1[MAX_PATH];
    int lr;

    r = ReadString(rcpath, "bbstylemaker.blackboxrc", "");
    if (0 == *r)
        r = set_my_path(NULL, b1, "blackbox.rc");

    lr = strlen(r);
    while (lr) { --lr; if (IS_SLASH(r[lr])) break; }

    p = ReadString(r, "session.styleFile", "");
    if (0 == *p)
        return 0;

    if (0 == memcmp(p, "$Blackbox$", sizeof "$Blackbox$" - 1)) {
        sprintf(temp, "%.*s%s", lr, r, p + sizeof "$Blackbox$" - 1);
    } else if (p[1] != ':') {
        sprintf(temp, "%.*s\\%s", lr, r, p);
    } else {
        strcpy(temp, p);
    }
    return 1;
}

/*----------------------------------------------------------------------------*/
/* make blackbox set a new style */

void set_bbstyle(HWND hDlg, const char *filename)
{
    char buffer[MAX_PATH];
    COPYDATASTRUCT cds;
    HWND bbwnd;

    if (BBhwnd) {
        BBSendData(BBhwnd, BB_SETSTYLE, 0, filename, -1);
        return;
    }

    // xoblite:
    bbwnd = GetBBWnd();
    if (NULL == bbwnd)
        return;

    cds.dwData = BB_SETSTYLE;
    cds.cbData = 1 + strlen(filename);
    cds.lpData = strcpy(buffer, filename);
    SendMessage(bbwnd, WM_COPYDATA, 0, (LPARAM)&cds);
    PostMessage(hDlg, BB_RECONFIGURE, 0, 0);

    //PostMessage(FindWindow("FreeStyler", NULL), BB_RECONFIGURE, 0, 0);
}

/*----------------------------------------------------------------------------*/

void set_auto_name(void)
{
    char buffer[MAX_PATH];
    strcpy(buffer, fname(work_stylefile));
    *(char*)fextension(buffer) = 0;
    replace_str(&style_info[0], buffer);
}

/*----------------------------------------------------------------------------*/
/* save style to disk */

int save_style(HWND hwnd, int flags)
{
    int is_old;
    int reorder = 0;
    char temp[MAX_PATH+100];
    int answer;
    NStyleStruct nss, *pss = &work_style;
    StyleStruct ss;

    if (0 == flags) // save if changed
    {
        if (0 == is_style_changed())
            return 1;

        sprintf(temp, "Save changes?\n%s", work_stylefile);
        answer = bb_msgbox(hwnd, temp, APPNAME, B_YES|B_NO|B_CANCEL);
        if (IDCANCEL == answer)
            return -1;

        if (IDNO == answer)
            return 1;
    }

    is_old = is_style_old(work_stylefile);
    if (flags == 2 || is_old)
    {
        strcpy(temp, work_stylefile);
        if (is_old && temp[0]) {
            // make name-new
            char *e = (char*)fextension(temp);
            replace_string(
                temp,
                sizeof temp,
                e - temp,
                0,
                write_070 ? "-new" : "-old");
        }

        if (!get_save_filename(hwnd, temp, "Save style as...", "blackbox style\0*.*\0"))
            return 0;

        if (0 != stricmp(temp, work_stylefile))
            flags = 2;

        if (flags == 2)
        {
            answer = bb_msgbox(hwnd, "Preserve formattings?", APPNAME, B_YES|B_NO|B_CANCEL);
            if (IDCANCEL == answer)
                return 0;

            if (IDYES == answer) {
                CopyFile(work_stylefile, temp, FALSE);
                reorder = 2;
            } else {
                DeleteFile(temp);
            }

            strcpy(work_stylefile, temp);
            set_auto_name();
            InvalidateRect(hwnd, NULL, FALSE);
        }
    }

    nss = *pss;
    set_style_defaults(&nss, SSD_OPT_WRITE);

    copy_from_N(&ss, &nss);
    writestyle(work_stylefile, &ss, style_info,
        reorder || write_070 != read_070 ? 2 : 0);
    new_flags();
    set_bbstyle(hwnd, work_stylefile);
    return flags;
}

/*----------------------------------------------------------------------------*/
/* get style name and style structure from BB */

void get_bbstyle(HWND hwnd)
{
    char temp[MAX_PATH];
    const char *p;

    NStyleStruct *pss = &work_style;
    StyleStruct *b = &bb_style;
    StyleStruct ss;

    // get style's filename
    temp[0] = 0;
    if (BBhwnd) {
        bbgetdata(hwnd, BB_GETSTYLE, temp);
    } else {
        get_xobstyle(temp);
    }

    // set style's filename
    strcpy(work_stylefile, temp);

    // get StyleStruct
    if (BBhwnd) {
        bbgetdata(hwnd, BB_GETSTYLESTRUCT, b);
        style_version = imax(1, b->Toolbar.nVersion);
        ss = *b;
    } else {
        style_version = 1;
        memset(b, 0, sizeof *b);
        b->toolbarAlpha = b->menuAlpha = 255;
        b->bulletUnix = b->metricsUnix = false;
    }

    //t1 = GetTickCount();

    readstyle(work_stylefile, &ss, 1);
    copy_to_N(pss, &ss);
    bsetroot_parse(pss, pss->rootCommand);
    read_070 = pss->is_070;

    p = ReadString(rcpath, "bbstylemaker.syntax", "070");
    if (0 == strcmp(p, "065"))
        write_070 = 0;
    else
    if (0 == strcmp(p, "070"))
        write_070 = 1;
    else
        write_070 = read_070;

    pss->is_070 = write_070;
    set_font_default(&pss->Toolbar);
    set_font_default(&pss->MenuFrame);
    set_font_default(&pss->MenuTitle);
    set_font_default(&pss->windowLabelFocus);

    if (false == write_070)
        adjust_style_065(pss);

    set_style_defaults(pss, SSD_OPT_FIRST);
    new_flags();
    bimage_init(true, write_070);

    //t2 = GetTickCount();
    //dbg_printf("time: %d", t2 - t1);
}

/*----------------------------------------------------------------------------*/

void copy_item(NStyleItem *d, NStyleItem *s)
{
    d->parentRelative   = s->parentRelative;
    d->type             = s->type               ;
    d->interlaced       = s->interlaced         ;
    d->bevelstyle       = s->bevelstyle         ;
    d->bevelposition    = s->bevelposition      ;
    d->Color            = s->Color              ;
    d->ColorTo          = s->ColorTo            ;

    d->TextColor        = s->TextColor          ;
    d->foregroundColor  = s->foregroundColor    ;
    d->disabledColor    = s->disabledColor      ;

    d->borderWidth      = s->borderWidth        ;
    d->borderColor      = s->borderColor        ;
    //d->marginWidth      = s->marginWidth        ;

    memcpy(PREF(d), PREF(s), CREF_SIZE);
}

NStyleItem * get_style_item(int v_section, int v_item)
{
    NStyleStruct *pss = &work_style;
    switch (v_section) {

    case 0:
        switch (v_item) {
            case 0: return &pss->Toolbar;
            case 1: return &pss->ToolbarButton;
            case 2: return &pss->ToolbarButtonPressed;
            case 3: return &pss->ToolbarLabel;
            case 4: return &pss->ToolbarWindowLabel;
            case 5: return &pss->ToolbarClock;
        }
        break;

    case 1:
        switch (v_item) {
            case 0: return &pss->MenuTitle;
            case 1: return &pss->MenuFrame;
            case 2: return &pss->MenuHilite;
            case 3: break;
            case 4: return &pss->MenuFrame;
        }
        break;

    case 2:
        switch (v_item) {
            case 0: return &pss->windowTitleFocus;
            case 1: return &pss->windowLabelFocus;
            case 2: return &pss->windowHandleFocus;
            case 3: return &pss->windowGripFocus;
            case 4: return &pss->windowButtonFocus;
            case 5: return &pss->windowButtonPressed;
            case 6: return &pss->windowFrameFocus;
        }
        break;

    case 3:
        switch (v_item) {
            case 0: return &pss->windowTitleUnfocus;
            case 1: return &pss->windowLabelUnfocus;
            case 2: return &pss->windowHandleUnfocus;
            case 3: return &pss->windowGripUnfocus;
            case 4: return &pss->windowButtonUnfocus;
            case 6: return &pss->windowFrameUnfocus;
        }
        break;

    case 4:
        switch (v_item) {
            case 0: return &pss->Slit;
        }
        break;

    case 5:
        switch (v_item) {
            case 0: return &pss->rootStyle;
        }
        break;
    }

    return NULL;
}

NStyleItem *get_margin_item(int msg)
{
    static char a_mar[10] = { 0,3,1,  0,1,2,  0,1,4,  0 };
    int m = msg - MAR_WID1A;
    int v = v_section;

    v -= v == 3;
    if (v < 5 && m >= 0 && m < 10) {
        if (style_version < 3
            && (msg == MAR_WID1C
             || msg == MAR_WID2C
             || msg == MAR_WID3C))
            return NULL;
        if (write_070)
            return get_style_item(v, a_mar[m]);
        if (a_mar[m] == 0)
            return get_style_item(0, 0);
    }
    return NULL;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
void adjust_style_065(NStyleStruct *pss)
{
    NStyleItem *si;

    si = &pss->Toolbar;
    if (pss->borderColor != si->borderColor
     || pss->borderWidth != si->borderWidth
     || pss->bevelWidth != si->marginWidth) {
        pss->borderColor = si->borderColor;
        pss->borderWidth = si->borderWidth;
        pss->bevelWidth = si->marginWidth;
        v_upd_all = true;
    }

    pss->MenuTitle.marginWidth =
    pss->Toolbar.marginWidth =
    pss->windowTitleFocus.marginWidth =
    pss->Slit.marginWidth =
        pss->bevelWidth;

    pss->MenuHilite.marginWidth =
        pss->bevelWidth+2;

    si = &pss->MenuFrame;
    if (BEVEL_SUNKEN == si->bevelstyle || BEVEL2 == si->bevelposition)
        si->marginWidth = si->bevelposition;
    else if (pss->MenuHilite.borderWidth)
        si->marginWidth = 1;
    else
        si->marginWidth = 0;

    pss->windowLabelFocus.marginWidth =
    pss->ToolbarWindowLabel.marginWidth =
    pss->ToolbarClock.marginWidth =
    pss->ToolbarLabel.marginWidth = 2;

    pss->windowButtonFocus.marginWidth =
    pss->ToolbarButton.marginWidth = 1;

    pss->MenuFrame.borderWidth =
    pss->MenuTitle.borderWidth =
    pss->Toolbar.borderWidth =
    pss->Slit.borderWidth =
    pss->windowTitleFocus.borderWidth =
    pss->windowTitleUnfocus.borderWidth =
    pss->windowHandleFocus.borderWidth =
    pss->windowHandleUnfocus.borderWidth =
    pss->windowGripFocus.borderWidth =
    pss->windowGripUnfocus.borderWidth =
    pss->windowFrameFocus.borderWidth =
        pss->borderWidth;

    pss->MenuFrame.borderColor =
    pss->MenuTitle.borderColor =
    pss->Toolbar.borderColor =
    pss->Slit.borderColor =
    pss->windowTitleFocus.borderColor =
    pss->windowTitleUnfocus.borderColor =
    pss->windowHandleFocus.borderColor =
    pss->windowHandleUnfocus.borderColor =
    pss->windowGripFocus.borderColor =
    pss->windowGripUnfocus.borderColor =
    pss->windowFrameFocus.borderColor =
    pss->windowFrameUnfocus.borderColor =
        pss->borderColor;

    pss->MenuFrame.bordered =
    pss->MenuTitle.bordered =
    pss->Toolbar.bordered =
    pss->windowTitleFocus.bordered =
    pss->windowTitleUnfocus.bordered =
    pss->windowHandleFocus.bordered =
    pss->windowHandleUnfocus.bordered =
    pss->windowGripFocus.bordered =
    pss->windowGripUnfocus.bordered =
        0 != pss->borderWidth;
}

/*----------------------------------------------------------------------------*/

void set_item_defaults(NStyleItem *pSI, int flags)
{
    if (pSI->bevelstyle == BEVEL_FLAT)
        pSI->bevelposition = 0;
    else
    if (pSI->bevelposition == 0)
        pSI->bevelposition = BEVEL1;

    pSI->bordered = 0 != pSI->borderWidth;

    pSI->validated = 0
        |V_TEX
        |V_CO1
        |V_CO2
        |V_TXT
        |V_PIC
        |V_JUS
        |V_DIS
        ;

    if (write_070)
        pSI->validated |= 0
            |V_MAR
            |V_BOW
            |V_BOC
            ;

    pSI->nVersion = style_version;
    pSI->is_070 = write_070;

    if (pSI->Font[0])
        pSI->validated |= V_FON|V_FHE|V_FWE;

    if (flags & SSD_OPT_WRITE)
    {
        strcpy(pSI->Font, RFONT(pSI)->Font);
        pSI->FontHeight = RFONT(pSI)->FontHeight;
        pSI->FontWeight = RFONT(pSI)->FontWeight;
        memset(PREF(pSI), 0, PREF_SIZE);
    }

    if ((flags & SSD_OPT_FIRST) && (write_070 != read_070))
    {
        if (pSI->bevelstyle == BEVEL_SUNKEN
            && pSI->type >= B_PIPECROSS && pSI->type <= B_PYRAMID) {
            COLORREF cr;
            cr = pSI->Color, pSI->Color = pSI->ColorTo, pSI->ColorTo = cr;
        }
    }
}

void set_style_defaults(NStyleStruct *pss, int flags)
{
    NStyleItem *pSI;

    pss->windowFrameUnfocus.borderWidth =
        pss->windowFrameFocus.borderWidth;

    for (pSI = &pss->Toolbar; pSI <= &pss->Slit; ++pSI)
        set_item_defaults(pSI, flags);

    if (style_version < 3) {
        pss->windowButtonFocus.validated &= ~V_MAR;
        pss->ToolbarButton.validated &= ~V_MAR;
        pss->MenuHilite.validated &= ~V_MAR;
    }

    if (pss->MenuTitle.parentRelative)
        pss->menuTitleLabel = false;
    else
        pss->menuNoTitle = false;

    if (flags & SSD_OPT_WRITE) {
        make_bsetroot_string(pss, pss->rootCommand, 1);
        if (0 == strcmp(ref_rootCommand, pss->rootCommand))
            strcpy(pss->rootCommand, ref_style.rootCommand);
    } else if (flags & (SSD_OPT_FIRST)) {
        make_bsetroot_string(pss, ref_rootCommand, 1);
    }
}

void set_font_default(NStyleItem *pSI)
{
    strcpy(RFONT(pSI)->Font, pSI->Font);
    RFONT(pSI)->FontHeight = pSI->FontHeight    ;
    RFONT(pSI)->FontWeight = pSI->FontWeight    ;
    parse_font((StyleItem*)pSI, RFONT(pSI)->Font);
#if 0
    dbg_printf("check <%s/%d/%d> <%s/%d/%d>",
        pSI->Font,
        pSI->FontHeight,
        pSI->FontWeight,
        RFONT(pSI)->Font,
        RFONT(pSI)->FontHeight,
        RFONT(pSI)->FontWeight
        );
#endif
}

/*----------------------------------------------------------------------------*/
static const char *artwizfonts[] = {
    "anorexia"   ,
    "aqui"       ,
    "cure"       ,
    "drift"      ,
    "edges"      ,
    "gelly"      ,
    "glisp"      ,
    "glisp-bold" ,
    "lime"      ,
    "mints-mild" ,
    "mints-strong" ,
    "nu"        ,
    "snap"      ,
    NULL
    };

void set_artwiz_font(NStyleItem *pSI, DWORD_PTR *pdata)
{
    int n, m;
    const char *p, **pp;
    pp = artwizfonts;
    m = 0;
    while (*pp) ++pp, ++m;
    n = *pdata = iminmax(*pdata, 0, m-1);
    p = artwizfonts[n];
    strcpy(pSI->Font, p);
}

/*----------------------------------------------------------------------------*/
/* send style data to blackbox */

void upd_bb(void)
{
    static bool sa, sb, sc;
    int flags, mask, all;

    NStyleStruct *pss = &work_style;
    StyleStruct *b = &bb_style;

    if (NULL == BBhwnd)
        return;

    set_style_defaults(pss, SSD_OPT_SENDBB);
    copy_from_N(b, pss);
    BBSendData(BBhwnd, BB_SETSTYLESTRUCT, SN_STYLESTRUCT, b, sizeof(StyleStruct));

    flags = 0;
    all = v_upd_all;
    v_upd_all = false;

    if (sa)
        flags |= BBRG_WINDOW;

    if (sb)
        flags |= BBRG_TOOLBAR;

    if (sc)
        flags |= BBRG_MENU;

    mask = all ? 1|2|4|8|16 : 0;
    if (v_section != 5 || v_item == 0)
        mask |= 1<<v_section;

    sa = sb = sc = false;

    if (mask & 1) {
        flags |= BBRG_TOOLBAR;
        if (v_section == 0 && v_item == 2) {
            flags |= BBRG_PRESSED;
            sb = true;
        }
    }

    if (mask & 2) {
        flags |= BBRG_MENU;
        if (v_section == 1 && v_item == 2) {
            flags |= BBRG_PRESSED;
            sc = true;
        }
    }

    if (mask & 4) {
        flags |= BBRG_WINDOW;
        if (v_section == 2) {
            flags |= BBRG_FOCUS;
            if (v_item == 5)
                flags |= BBRG_PRESSED;
        }
        sa = true;
    }

    if (mask & 8) {
        flags |= BBRG_WINDOW;
    }

    if (mask & 16) {
        flags |= BBRG_SLIT;
        // --------------------------------------
        if (style_version < 3) {
            HWND hSlit = FindWindow("BBSlit", NULL);
            if (hSlit)
                BBSendData(hSlit, BB_SETSTYLESTRUCT, SN_SLIT, &b->Slit, sizeof(b->Slit));
        }
        // --------------------------------------
    }

    if (mask & 32) {
        // --------------------------------------
        if (style_version < 4) {
            char buffer[MAX_PATH + 100];
            sprintf(buffer, "@BBCore.rootCommand %s", b->rootCommand);
            BBSendData(BBhwnd, BB_BROADCAST, 0, buffer, -1);
        } else
        // --------------------------------------
        flags |= BBRG_DESK;
    }

    PostMessage(BBhwnd, BB_REDRAWGUI, flags, 0);
}

/*----------------------------------------------------------------------------*/
NStyleItem *get_font_item(void)
{
    switch (v_section) {
        case 0: return get_style_item(v_section, 0);
        case 1: return get_style_item(v_section, v_item == 0 ? 0 : 1);
        case 2: case 3: return get_style_item(2, 1);
    }
    return NULL;
}

void set_all_font(void)
{
    NStyleItem *s0, *s1, *ai[4];
    NStyleStruct *pss;
    int i;

    s1 = get_font_item();
    if (NULL == s1)
        return;

    pss = &work_style;
    ai[0] = &pss->Toolbar,
    ai[1] = &pss->MenuFrame,
    ai[2] = &pss->MenuTitle,
    ai[3] = &pss->windowLabelFocus;
    for (i = 0; i < 4; ++i)
    {
        s0 = ai[i];
        strcpy(s0->Font, RFONT(s1)->Font);
        s0->FontHeight = RFONT(s1)->FontHeight;
        s0->FontWeight = RFONT(s1)->FontWeight;
        set_font_default(s0);
    }
    v_upd_all = true;
}

/*----------------------------------------------------------------------------*/
void set_all_border(void)
{
    NStyleStruct *pss = &work_style;
    if (v_section == 1 && (v_item == 0 || v_item == 1)) {
        pss->MenuFrame.borderWidth =
        pss->MenuTitle.borderWidth =
            B0->borderWidth;
        pss->MenuFrame.borderColor =
        pss->MenuTitle.borderColor =
            B0->borderColor;
    } else if ((v_section == 2 || v_section == 3)
        && v_item != 1 && v_item != 4 && v_item != 5) {
        if (v_section == 2) {
            pss->windowTitleFocus.borderColor =
            pss->windowHandleFocus.borderColor =
            pss->windowGripFocus.borderColor =
            pss->windowFrameFocus.borderColor =
                B0->borderColor;
        } else {
            pss->windowTitleUnfocus.borderColor =
            pss->windowHandleUnfocus.borderColor =
            pss->windowGripUnfocus.borderColor =
            pss->windowFrameUnfocus.borderColor =
                B0->borderColor;
        }
        pss->windowTitleFocus.borderWidth =
        pss->windowHandleFocus.borderWidth =
        pss->windowGripFocus.borderWidth =
        pss->windowTitleUnfocus.borderWidth =
        pss->windowHandleUnfocus.borderWidth =
        pss->windowGripUnfocus.borderWidth =
        pss->windowFrameFocus.borderWidth =
        pss->windowFrameUnfocus.borderWidth =
            B0->borderWidth;
    }
}

/*----------------------------------------------------------------------------*/
void fix_P0(void)
{
    NStyleStruct *pss = &work_style;
    NStyleItem *pSI = P0;

    if (pSI == &pss->rootStyle) {
        if (0 == v_root_changed) {
            char new_rootCommand[ROOTCOMMAND_SIZE];
            make_bsetroot_string(pss, new_rootCommand, 1);
            v_root_changed = 0 != strcmp(ref_rootCommand, new_rootCommand);
        }
        if (v_root_changed)
            make_bsetroot_string(pss, pss->rootCommand, 0);

    } else if (pSI == &pss->windowFrameUnfocus) {
        pss->windowFrameFocus.borderWidth = pSI->borderWidth;
    }

    if (false == write_070)
        adjust_style_065(pss);
    else
    if (v_all_border)
        set_all_border();
}

void set_P0(void)
{
    NStyleItem *pSI = get_style_item(v_section, v_item);
    P0 = B0 = pSI;
    if (pSI) {
        NStyleStruct *pss = &work_style;
        if (false == write_070) {
            B0 = &pss->Toolbar;
        } else if (pSI == &pss->windowFrameUnfocus) {
            B0->borderWidth = pss->windowFrameFocus.borderWidth;
        }
    }
}

/*----------------------------------------------------------------------------*/

int is_style_changed(void)
{
    NStyleStruct n1, n2;

    if (-1 == v_changed)
        return 0;

    if (1 == v_changed)
        return 1;

    n1 = work_style;
    n2 = ref_style;

    set_style_defaults(&n1, SSD_OPT_WRITE);
    set_style_defaults(&n2, SSD_OPT_WRITE);

    return 0 != memcmp(&n1, &n2, offsetof(NStyleStruct, rootInfo));
}

/*----------------------------------------------------------------------------*/
/* set pointers to the colors that
   currently can be changed with the sliders */

void get_color_pointers(void)
{
    pL1 = pL2 = NULL;
    if (P0) switch (v_colorsel) {
        case 0:
            SETREF(L1, Color);
            if (false == (P0->type == B_SOLID && false == P0->interlaced)) {
                SETREF(L2, ColorTo);
            }
            break;

        case 1:
            if (v_section == 1) {
                if (v_item == 1 || v_item == 2) {
                    SETREF(L2, foregroundColor);
                }
            }
            SETREF(L1, TextColor);
            break;

        case 2:
            if (B0->borderWidth) {
                SETREF(L1, borderColor);
            }
            break;

        case 3:
            SETREF(L1, disabledColor);
            break;
    }

    if (pL1) {
        if (false == pL1->lock)
            *pL1->cref = 0;
        else
        if (*pL1->cref == 0)
            *pL1->cref = *pL1->cptr;

        if (0 == v_hsl || 0 == *pL1->hslptr)
            *pL1->hslptr = RGBtoHSL(*pL1->cptr);

    }
    if (pL2) {
        if (false == pL2->lock)
            *pL2->cref = 0;
        else
        if (*pL2->cref == 0)
            *pL2->cref = *pL2->cptr;

        if (0 == v_hsl || 0 == *pL2->hslptr)
            *pL2->hslptr = RGBtoHSL(*pL2->cptr);
    }
}

void reset_hsl(void)
{
    if (v_hsl)
    {
        if (pL1) *pL1->hslptr = 0;
        if (pL2) *pL2->hslptr = 0;
    }
}

/*----------------------------------------------------------------------------*/
/* get the color from sliders and apply to currently selected elements */

void get_slider_color(struct dlg *dlg, struct c_lock *pL1, int sld_red, int sld)
{
    bool ctrl_key = 0 != (0x8000 & GetAsyncKeyState(VK_CONTROL));
    bool lock_ctrl = false == pL1->lock && ctrl_key;
    int i = 0;
    COLORREF C = 0;

    do {
        C |= (getbutton(dlg, sld_red+i)->data*256/SB_DIV) << (i<<3);
    } while (++i < 3);

    if (v_hsl) {
        *pL1->hslptr = C;
        *pL1->cptr = *pL1->cref = HSLtoRGB(C);
        return;
    }

    if (lock_ctrl && false == pL1->lock_ctrl)
        *pL1->cref = *pL1->cptr;

    if (pL1->lock && ctrl_key)
        *pL1->cref = C;

    *pL1->cptr = C;
    pL1->lock_ctrl = lock_ctrl;

    if (pL1->lock != ctrl_key) {
        COLORREF REF = *pL1->cref;
        int piv = (sld - sld_red) << 3;
        int dx = (C>>piv & 255) - (REF>>piv & 255);
        *pL1->cptr = rgb(
            iminmax((int)GetRValue(REF) + dx, 0, 255),
            iminmax((int)GetGValue(REF) + dx, 0, 255),
            iminmax((int)GetBValue(REF) + dx, 0, 255));
    }
}

/*----------------------------------------------------------------------------*/
void print_colors(struct dlg *dlg)
{
    //char *p;
    COLORREF C1, C2;

    if (v_hsl) {
        C1 = pL1 ? *pL1->hslptr : 0;
        C2 = pL2 ? *pL2->hslptr : 0;
        sprintf(cbuf1, "%03d  %02d  %02d",
            (unsigned)GetRValue(C1) * 360 / 255,
            (unsigned)GetGValue(C1) * 100 / 255,
            (unsigned)GetBValue(C1) * 100 / 255
            );
        sprintf(cbuf2, "%03d  %02d  %02d",
            (unsigned)GetRValue(C2) * 360 / 255,
            (unsigned)GetGValue(C2) * 100 / 255,
            (unsigned)GetBValue(C2) * 100 / 255
            );
    } else {
        C1 = pL1 ? *pL1->cptr : 0;
        C2 = pL2 ? *pL2->cptr : 0;
        sprintf(cbuf1, "%02X  %02X  %02X",
            GetRValue(C1),
            GetGValue(C1),
            GetBValue(C1));

        sprintf(cbuf2, "%02X  %02X  %02X",
            GetRValue(C2),
            GetGValue(C2),
            GetBValue(C2));
    }
/*
    p = NULL;
    if (colorsnap) p = get_literal_color_name(c1,COLOR_SNAP);
    strcpy(lbuf1,p ? p : "");
    p = NULL;
    if (colorsnap) p = get_literal_color_name(c2,COLOR_SNAP);
    strcpy(lbuf2, p ? p : "");
*/
    set_button_text(dlg, COL_HX1, cbuf1);
    set_button_text(dlg, COL_HX2, cbuf2);
    invalidate_item(dlg, PAL_X);
}

/*----------------------------------------------------------------------------*/
/* set the sliders to the values that are currently selected */

void move_sliders(struct dlg *dlg)
{
    int SLD; struct c_lock *pL;
    for (SLD = SLD_R1, pL = pL1; ; SLD = SLD_R2, pL = pL2) {
        int i = 0;

        COLORREF C;
        if (v_hsl) {
            C = pL ? *pL->hslptr : 0;
        } else {
            C = pL ? *pL->cptr : 0;
        }
        do {
            unsigned d = (((C>>(i<<3)) & 255)*SB_DIV+SB_DIV/2)/256;
            struct button *bp = getbutton(dlg, SLD+i);
            if (d != bp->data) {
                bp->data = d;
                invalidate_button(bp);
            }
        } while (++i < 3);
        if (SLD == SLD_R2)
            break;
    }
    print_colors(dlg);
}

/*----------------------------------------------------------------------------*/
/* update blackbox */

void redraw_gui(struct dlg *dlg)
{
    fix_P0();
    SetTimer(dlg->hwnd, UPDATE_TIMER, 10, NULL);
}

/*----------------------------------------------------------------------------*/
/* get the description of the currently selected font */

const char *font_string(void)
{
    switch(v_section) {
        case 0: return "toolbar font";
        case 1: return v_item == 0 ? "menu title font" : "menu frame font";
        case 2: case 3: return "window font";
        default: return "";
    }
}

/*----------------------------------------------------------------------------*/
void configure_interface(struct dlg *dlg)
{
    int s = v_section;
    int i = v_item;
    int n;
    bool f;
    NStyleItem *styF;

    // setup some flags:

    // window is selected
    bool f_win = s == 2 || s == 3;
    // window.frame is selected
    bool f_frame = f_win && i == 6;
    // window.handle is selected
    bool f_handle = f_win && (i == 2 || i == 3);
    // menu bullet selected
    bool f_bullet = s == 1 && i == 3;
    // info page selected
    bool f_info = s == 5 && i == 1;
    // bsetroot page selected
    bool f_root = s == 5 && i == 0;

    // item has text
    bool f_text = s == 0 ? i != 1 && i != 2
            : s == 1 ? i != 3
            : f_win ? i == 1
            : f_root ? 0 != work_style.rootInfo.mod
            : false;
    // item has pic (buttons & menu frame)
    bool f_pic = s == 0 ? i == 1 || i == 2
            : s == 1 ? i == 1 || i == 2
            : f_win ? i == 4 || i == 5
            : false;
    // item has gradient properties
    bool f_grad = false == f_bullet && false == f_frame && false == f_info;
    // item has border
    bool f_border = B0 && B0->borderWidth;
    // item can be parentrelative
    bool can_pr = f_grad
        && (s == 0 ? i != 0 :
            s == 1 ? i != 1 :
            f_win ? i != 0 && i != 2 :
            f_root);
    // item is parentrelative
    bool is_pr = can_pr && P0->parentRelative;
    // enable selection of gradient types
    bool f_gradtypes = f_grad && false == is_pr;


    // check whether color selection is valid
    v_colorsel = v_colorsel_set;
    if (false == (f_text||f_pic||v_colorsel!=1))
        v_colorsel = 0;
    if (v_colorsel == 2 && false == f_border)
        v_colorsel = 0;
    if (v_colorsel == 3 && (s != 1 || i != 1))
        v_colorsel = 1;
    if (is_pr && v_colorsel == 0)
        v_colorsel = (f_text||f_pic) ? 1 : 2;
    if (f_frame)
        v_colorsel = 2;

    P0_dis = s == 1 && i == 1 && v_colorsel == 3;

    get_color_pointers();

    // section
    check_radio(dlg, s + SEC_TOO);

    // display item as needed for section
    show_section(dlg, TOO_BAC, TOO_CLK, s==0);
    show_section(dlg, MEN_TIT, MEN_NTI, s==1);
    show_section(dlg, WIN_TIT, WIN_FRM, f_win);
    show_section(dlg, SLT_FRM, SLT_FRM, s==4);
    show_section(dlg, MIS_ROT, MIS_INF, s==5);

    show_section(dlg, GRD_RCT, INF_RCT-1, false == f_info);
    show_section(dlg, INF_RCT, INF_LN5, f_info);
    show_section(dlg, CMD_CFG, CMD_RST, f_info);
    show_section(dlg, ROT_MHD, ROT_IMG, f_root);

    //show_button(dlg, SEC_MIS, false);

    switch (s) {
        case 0:
            check_radio(dlg, i + TOO_BAC);
            break;
        case 1:
            f = i == 0 && style_version >= 4;
            show_button(dlg, MEN_LBL, f);
            show_button(dlg, MEN_NTI, f);
            if (f) {
                if (is_pr) {
                    enable_button(dlg, MEN_LBL, false);
                    check_button(dlg, MEN_NTI, work_style.menuNoTitle);
                } else {
                    check_button(dlg, MEN_LBL, work_style.menuTitleLabel);
                    enable_button(dlg, MEN_NTI, false);
                }
            }
            check_radio(dlg, i + MEN_TIT);
            break;
        case 2:
        case 3:
            show_button(dlg, WIN_BUP, s == 2);
            show_button(dlg, WIN_FRM, write_070);
            check_radio(dlg, i + WIN_TIT);
            break;

        case 5:
            check_radio(dlg, i + MIS_ROT);
            if (i == 0) {
                struct rootinfo *ri = &work_style.rootInfo;
                check_button(dlg, ROT_MOD, ri->mod);
                enable_button(dlg, ROT_MOD, false == is_pr);
                f_text = false == is_pr && ri->mod;
                enable_section(dlg, ROT_MDX, ROT_MDY, f_text);
                enable_section(dlg, ROT_SCL, ROT_FUL, 0 != ri->wpfile[0]);
                set_button_data(dlg, ROT_MDX, ri->modx);
                set_button_data(dlg, ROT_MDY, ri->mody);
                set_button_data(dlg, ROT_SCL, ri->scale);
                set_button_data(dlg, ROT_SAT, ri->sat);
                set_button_data(dlg, ROT_HUE, ri->hue);
                set_button_text(dlg, ROT_IMG, ri->wpfile);
                if (ri->wpstyle)
                    check_radio(dlg, ROT_TIL + ri->wpstyle-WP_TILE);
                else {
                    check_radio(dlg, ROT_TIL);
                    check_button(dlg, ROT_TIL, false);
                }
                break;
            }
            if (i == 1) {
                for (i = 0; i < 5; ++i)
                    set_button_text(dlg, INF_LN1+i, style_info[i]);
                return;
            }
            break;
    }

    show_section (dlg, OPT_RCT, FNT_DEF, false == f_root);
    show_section (dlg, BOR_HDR, BOR_WID, false == f_root);
    show_section (dlg, FNT_RCT, FNT_RIG, false == f_root);

    // margin
    show_section(dlg, MAR_WID1A, MAR_WID1C, s==0);
    show_section(dlg, MAR_WID2A, MAR_WID2C, s==1);
    show_section(dlg, MAR_WID3A, MAR_WID3C, f_win && false == f_handle);
    show_button(dlg, MAR_WID4A, s==4);
    show_section(dlg, HAN_HDR, HAN_SIZ, f_handle);
    show_button(dlg, MAR_HDR, false == f_handle && false == f_root);

    if (false == f_root) {
        // handle
        if (f_handle)
            set_button_data(dlg, HAN_SIZ, work_style.handleHeight);
        else
            for (n = 0; n < 3; ++n) {
                int m = MAR_WID1A + (s-(s>=3))*3 + n;
                NStyleItem *pSI = get_margin_item(m);
                if (pSI)
                    set_button_data(dlg, m, pSI->marginWidth);
                else
                    show_button(dlg, m, false);
                if (m == MAR_WID4A)
                    break;
            }

        // font
        styF = get_font_item();
        enable_section(dlg, FNT_NAM, FNT_RIG, NULL != styF);
        set_button_text(dlg, FNT_HDR, font_string());
        if (styF) {
            int h, j; bool b;

            strcpy(fontname, RFONT(styF)->Font);
            h = RFONT(styF)->FontHeight;
            b = RFONT(styF)->FontWeight >= FW_BOLD;
            j = styF->Justify;

            set_button_text(dlg, FNT_NAM, fontname);
            set_button_data(dlg, FNT_HEI, h);
            check_button(dlg, FNT_BOL, b);
            check_radio(dlg, j + FNT_LEF);
        }
        else
            set_button_text(dlg, FNT_NAM, "");
    }

    show_section(dlg, BUL_HDR, BUL_RIG, f_bullet);
    if (false == f_bullet) {
    // texture
        show_button(dlg, GRD_HDR, true);
        enable_section(dlg, GRD_SOL, GRD_PYR, f_grad);
        enable_button(dlg, GRD_INL, f_gradtypes);
        enable_button(dlg, GRD_PRR, can_pr);
        check_radio(dlg,
            is_pr ? GRD_PRR
            : P0->type == B_SOLID ? GRD_SOL
            : P0->type + GRD_HOR );
        check_button(dlg, GRD_INL, P0->interlaced);
    } else {
    // menu bullets
        show_section(dlg, GRD_HDR, GRD_INL, false);
        check_radio(dlg, BUL_EMP + get_bulletstyle(work_style.menuBullet));
        check_radio(dlg, BUL_LEF + (0!=stricmp(work_style.menuBulletPosition, "left")));
    }

    // bevel
    enable_section(dlg, BEV_FLA, BEV_P2, f_gradtypes);
    enable_button(dlg, BEV_P2, f_gradtypes && P0->bevelstyle != BEVEL_FLAT);
    if (f_gradtypes) {
        check_radio(dlg, P0->bevelstyle + BEV_FLA);
        if (P0->bevelstyle != BEVEL_FLAT)
            check_button(dlg, BEV_P2, P0->bevelposition == BEVEL2);
    }

    // color
    check_radio(dlg, v_colorsel + COL_GRD);
    enable_button(dlg, COL_GRD, f_gradtypes);
    enable_button(dlg, COL_TXT, f_grad && (f_text||f_pic));
    enable_button(dlg, COL_BOR, f_border);
    show_button(dlg, COL_DIS, s == 1 && i == 1);

    //set_button_text(dlg, COL_GRD, f_gradtypes && P0->type != B_SOLID?"color1/2":"color");
    set_button_text(dlg, COL_TXT,
        f_pic ? (f_text?"text/pic":"pic") : f_root ? "modfg" : "text");

    set_button_text(dlg, COL_GRD,
        f_gradtypes && P0->type == B_SOLID && false == P0->interlaced
        ? "backgnd" : "color 1/2");

    set_button_text(dlg, GRD_PRR, f_root?"(none)":"parentrel");

    if (false == f_root) {
        // border
        enable_button(dlg, BOR_WID, f = f_grad || f_frame);
        if (f) set_button_data(dlg, BOR_WID, B0->borderWidth);

        // *border / *font
        check_button(dlg, BOR_DEF, v_all_border);
    }

    show_section(dlg, COL_GR1, COL_LK2, 0 == v_hsl);
    show_section(dlg, COL_HU1, COL_HU2, 0 != v_hsl);
    check_radio(dlg, COL_RGB + v_hsl);

    // adjust sliders
    move_sliders(dlg);
}

/*----------------------------------------------------------------------------*/

void set_item(struct dlg * dlg, int i)
{
    v_item = i;
    set_P0();
    configure_interface(dlg);
    redraw_gui(dlg);
}

void set_section(struct dlg * dlg, int v)
{
    a_items[v_section] = v_item;

    if (v_section == 2)
        a_items[3] = v_item == 5 ? 4 : v_item ;

    if (v_section == 3 && (v_item != 4 || a_items[2] != 5))
        a_items[2] = v_item;

    v_section = v;
    set_item(dlg, a_items[v_section]);
}

int handle_received_data(HWND hwnd, UINT msg, WPARAM wParam, const void *data, unsigned data_size)
{
    struct getdata_info *gdi;
    if (msg != BB_SENDDATA)
        return 0;
    gdi = (struct getdata_info*)wParam;

    switch(gdi->msg) {
    case BB_GETSTYLESTRUCT:
        memcpy(gdi->dest, data, imin(sizeof (StyleStruct), data_size));
        break;
    case BB_GETSTYLE:
        strcpy(gdi->dest, (const char*)data);
        break;
    }
    return 1;
}

/*----------------------------------------------------------------------------*/

LRESULT CALLBACK main_dlg_proc (struct dlg *dlg, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static const UINT msgs[] = { BB_RECONFIGURE, BB_EXITTYPE, 0};
    static UINT bb_broadcast_msg;

    char buffer[2000];
    struct button *bp;
    NStyleItem *pSI;
    int f, i;
    const char *s;

    switch (msg) {

    // -------------------------------------
    default:
        if (bb_broadcast_msg == msg)
            goto link_new;
        return DefWindowProc (hwnd, msg, wParam, lParam);

    // -------------------------------------
    case WM_CREATE:
        bb_broadcast_msg = RegisterWindowMessage("TaskbarCreated");
    link_new:
        if (link_to_BB(hwnd)) {
            BBSendData(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, msgs, sizeof msgs);
        }
        PostMessage(hwnd, WM_COMMAND, CMD_GETSTYLE, 0);
        break;

    // -------------------------------------
    case WM_DESTROY:
        if (BBhwnd)
        {
            BBSendData(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, msgs, sizeof msgs);
            PostMessage(BBhwnd, BB_RECONFIGURE, 0, 0);
            PostMessage(BBhwnd, BB_REDRAWGUI, BBRG_TOOLBAR|BBRG_MENU|BBRG_WINDOW|BBRG_SLIT|BBRG_DESK, 0);
        }
        WriteInt(rcpath, "bbstylemaker.xpos", dlg->x);
        WriteInt(rcpath, "bbstylemaker.ypos", dlg->y);
        delete_dlg(dlg);
        reset_rcreader();
        PostQuitMessage(0);
        break;

    // -------------------------------------
    case BB_RECONFIGURE:
        PostMessage(hwnd, WM_COMMAND, CMD_GETSTYLE, 1);
        break;

    case BB_EXITTYPE:
        if (B_RESTART != lParam)
            BBhwnd = NULL;
        break;

    // -------------------------------------
    case WM_COPYDATA:
        return BBReceiveData(hwnd, lParam, handle_received_data);

    // -------------------------------------
    case WM_QUERYENDSESSION:
    case WM_CLOSE:
        goto quit;

    // -------------------------------------
    case WM_TIMER:
        KillTimer(hwnd, wParam);

        if (UPDATE_TIMER == wParam) {
            upd_bb();
            break;
        }

        if (SLIDER_TIMER == wParam) {
            move_sliders(dlg);
            break;
        }
        break;

    // -------------------------------------

    case WM_COMMAND:
        f = LOWORD(wParam);
        goto other;

    // -------------------------------------

    case WM_KEYUP:
        if (VK_CONTROL == wParam) {
            if (pL1)
                pL1->lock_ctrl = false;
            if (pL2)
                pL2->lock_ctrl = false;
        }
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            if (GetCapture() == hwnd)
            {
                dlg->bn_act = NULL;
                dlg->tf  = 0;
                SetCursor(LoadCursor(NULL, (LPCSTR)IDC_ARROW));
                if (P0) copy_item(P0, &save_palette);
                redraw_gui(dlg);
                move_sliders(dlg);
                break;
            }
            goto quit;

        case VK_TAB:
            if (v_section == 5 && v_item == 1)
                set_button_focus(dlg, INF_LN1);
            break;
        }
        break;

    // -------------------------------------
    case WM_SYSKEYDOWN:
        f = get_accel_msg(dlg, wParam);
        if (f == IDCANCEL)
            goto quit;
        break;

    case WM_CHAR:
        f = get_accel_msg(dlg, LOWORD(wParam));
        goto other;

    // -------------------------------------
    case WM_DROPFILES:
        drop_guistyle(dlg, (void*)wParam);
        break;

    // -------------------------------------
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        f = dlg_mouse(dlg, msg, wParam, lParam);
        goto other;

    case WM_RBUTTONUP:
        f = dlg_mouse(dlg, msg, wParam, lParam);
        goto other;

    // -------------------------------------
    case WM_LBUTTONUP:
        bp = dlg->bn_act;
        f = dlg_mouse(dlg, msg, wParam, lParam);
        if (bp && bp->msg == PAL_X)
        {
            /* finish color picker */
            redraw_gui(dlg);
            get_color_pointers();
            move_sliders(dlg);

        }
        SetCursor(LoadCursor(NULL, (LPCSTR)IDC_ARROW));
        goto other;

    // -------------------------------------
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
        f = dlg_mouse(dlg, msg, wParam, lParam);
        bp = dlg->bn_act;
        if (f == 0 && bp && bp->msg == PAL_X && P0)
        {
            /* start color picker */
            pSI = &save_palette;
            copy_item(pSI, P0);
            pSI->nVersion = STYLEITEM_VERSION;
            SetCursor(LoadCursor(NULL, (LPCSTR)IDC_CROSS));
            break;
        }
        goto other;

    // -------------------------------------
    case WM_MOUSEMOVE:
        bp = dlg->bn_act;
        f = dlg_mouse(dlg, msg, wParam, lParam);

        if ((wParam & MK_LBUTTON) && f == 0 && bp && bp->msg == PAL_X && P0)
        {
            /* the color picker in action */
            COLORREF pixel, pixel_hsl;
            int mx, my; bool left; struct button *bm;

            mx = (short)LOWORD(lParam);
            my = (short)HIWORD(lParam);
            left = dlg->tx < bp->x + bp->w/2;
            bm = mousebutton(dlg, mx, my);
            pixel = get_screen_pixel();

            if (bm && bm->msg >= PAL_X && bm->msg <= PAL_9)
            {
                NStyleItem *p;
                f = bm->msg - PAL_1;
                if (f < 0)
                    p = &save_palette;
                else
                    p = &Palette[f];

                if (p->nVersion)
                {
                    int d = 6;
                    int v = bm->w / 2;
                    if (p->borderWidth && (mx < bm->x + d || mx >= bm->x + bm->w - d))
                        pixel = p->borderColor;
                    else
                    if (mx >= bm->x + v - d/2  && mx <  bm->x + v + d/2)
                        pixel = P0_dis ? p->disabledColor : p->TextColor;
                    else
                    if (mx < bm->x + v || (p->type == B_SOLID && false == p->interlaced))
                        pixel = p->Color;
                    else
                        pixel = p->ColorTo;
                }
            }

            pixel_hsl = RGBtoHSL(pixel);
            if (pL1 && (left || NULL == pL2))
                *pL1->cptr = *pL1->cref = pixel, *pL1->hslptr = pixel_hsl;
            else
            if (pL2)
                *pL2->cptr = *pL2->cref = pixel, *pL2->hslptr = pixel_hsl;

            redraw_gui(dlg);
            print_colors(dlg);
            SetTimer(hwnd, SLIDER_TIMER, 40, NULL);
            break;
        }
        goto other;

        // -------------------------------------------------------
    other:
        switch (f)
        {
        default:
            break;

        // -------------------------------------------------------
        case CMD_QUI:
        quit:
            if (-1 == save_style(hwnd, 0))
                break;

            if (msg == WM_QUERYENDSESSION)
                return TRUE;

            DestroyWindow(hwnd);
            break;

        case CMD_RST:
            load_guistyle(dlg, NULL);
            break;

        // -------------------------------------------------------
        case CMD_SAV:
        case CMD_SAA:
            i = save_style(hwnd, f == CMD_SAV ? 1 : 2);
            if (2 == i) {
                set_section(dlg, 5);
                set_item(dlg, 1);
            }
            break;

        // -------------------------------------------------------
        case CMD_ABO:
            bb_msgbox(hwnd, HELPMSG, APPNAME, B_OK);
            break;

        // -------------------------------------------------------
        case CMD_EDI:
            edit_file(work_stylefile);
            break;

        case CMD_DOC:
            edit_file(set_my_path(NULL, buffer, "readme.txt"));
            break;

        case CMD_CFG:
            edit_file(rcpath);
            break;

        case CMD_REV:
            if (style_version < 4
              || (v_section == 5 && v_item == 0 && work_style.rootInfo.bmp)) {
                if (BBhwnd) {
                    v_changed = -1;
                    PostMessage(BBhwnd, BB_RECONFIGURE, 0, 0);
                    break;
                }
            }
            goto get_style;

        // -------------------------------------------------------
        case CMD_GETSTYLE:
            if (lParam && is_style_changed())
            {
                static bool in_msg;
                int a;

                if (in_msg)
                    break;

                in_msg = true;
                a = bb_msgbox(hwnd, "Blackbox was reconfigured. Discard changes?", APPNAME, B_YES|B_NO);
                in_msg = false;

                if (a != IDYES)
                    break;
            }

    get_style:
            get_bbstyle(hwnd);

            sprintf(dlg->title, "%s - %s", APPNAME,
                work_stylefile[0] ? work_stylefile : "no style loaded");

            if (dlg->captionbar)
                SetWindowText(hwnd, dlg->title);

            InvalidateRect(hwnd, NULL, FALSE);

            if (CMD_REV == f)
                v_upd_all = true;

            set_section(dlg, v_section);
            break;

        // -------------------------------------------------------
        case SLD_R1  :
        case SLD_G1  :
        case SLD_B1  :
            if (pL1) get_slider_color(dlg, pL1, SLD_R1, f);
            redraw_gui(dlg);
            move_sliders(dlg);
            break;

        case SLD_R2  :
        case SLD_G2  :
        case SLD_B2  :
            if (pL2) get_slider_color(dlg, pL2, SLD_R2, f);
            redraw_gui(dlg);
            move_sliders(dlg);
            break;

        // -------------------------------------------------------
        case PAL_1:
        case PAL_2:
        case PAL_3:
        case PAL_4:
        case PAL_5:
        case PAL_6:
        case PAL_7:
        case PAL_8:
        case PAL_9:
            if (NULL == P0)
                break;

            pSI = &Palette[f - PAL_1];
            if (msg == WM_RBUTTONUP)
            {
                copy_item(pSI, P0);
                pSI->nVersion = STYLEITEM_VERSION;
                invalidate_item(dlg, f);
                break;
            }

            if (0 == pSI->nVersion)
                break;

            switch (v_colorsel) {
                case 0:
                    if (pSI->parentRelative) {
                        P0->TextColor = pSI->TextColor;
                        COPYREF(P0,pSI,TextColor);
                        P0->foregroundColor = pSI->foregroundColor;
                        COPYREF(P0,pSI,foregroundColor);
                        B0->borderColor = pSI->borderColor;
                        COPYREF(B0,pSI,borderColor);
                        B0->borderWidth = pSI->borderWidth;
                    } else {
                        copy_item(P0, pSI);
                    }
                    break;
                case 1:
                    P0->TextColor = pSI->TextColor;
                    COPYREF(P0,pSI,TextColor);
                    P0->foregroundColor = pSI->foregroundColor;
                    COPYREF(P0,pSI,foregroundColor);
                    break;

                case 2:
                    B0->borderColor = pSI->borderColor;
                    COPYREF(B0,pSI,borderColor);
                    B0->borderWidth = pSI->borderWidth;
                    break;

                case 3:
                    P0->disabledColor = pSI->disabledColor;
                    COPYREF(P0,pSI,TextColor);
                    break;
            }

            reset_hsl();
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case COL_GR1 :
            if (pL1)
            {
                i = greyvalue(*pL1->cptr);
                *pL1->cptr = rgb(i,i,i);
            }
            check_button(dlg, COL_LK1, L1.lock = true);
            goto col_lk1;

        case COL_GR2 :
            if (pL2)
            {
                i = greyvalue(*pL2->cptr);
                *pL2->cptr = rgb(i,i,i);
            }
            check_button(dlg, COL_LK2, L2.lock = true);
            goto col_lk2;

        case COL_LK1 :
            L1.lock = 0 != (getbutton(dlg, f)->f & BN_ON);
        col_lk1:
            if (pL1)
            {
                *pL1->cref = *pL1->cptr;
            }
            redraw_gui(dlg);
            move_sliders(dlg);
            break;

        case COL_LK2 :
            L2.lock = 0 != (getbutton(dlg, f)->f & BN_ON);
        col_lk2:
            if (pL2)
            {
                *pL2->cref = *pL2->cptr;
            }
            redraw_gui(dlg);
            move_sliders(dlg);
            break;

        case COL_RGB :
        case COL_HSL :
            v_hsl = f == COL_HSL;
            reset_hsl();
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case COL_SW0 :
            if (pL1 && pL2)
            {
                COLORREF C;
                C = *pL1->cref; *pL1->cref = *pL2->cref; *pL2->cref = C;
                C = *pL1->cptr; *pL1->cptr = *pL2->cptr; *pL2->cptr = C;
                C = *pL1->hslptr; *pL1->hslptr = *pL2->hslptr; *pL2->hslptr = C;
            }
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case COL_SW1 :
            if (pL1 && pL2)
            {
                *pL1->cref = *pL2->cref;
                *pL1->cptr = *pL2->cptr;
                *pL1->hslptr = *pL2->hslptr;
            }
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case COL_SW2 :
            if (pL1 && pL2)
            {
                *pL2->cref = *pL1->cref;
                *pL2->cptr = *pL1->cptr;
                *pL2->hslptr = *pL1->hslptr;
            }
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case SEC_TOO:
        case SEC_MEN:
        case SEC_WIF:
        case SEC_WIU:
        case SEC_SLI:
        case SEC_MIS:
            set_section(dlg, f - SEC_TOO);
            break;

        // -------------------------------------------------------
        case TOO_BAC:
        case TOO_BUT:
        case TOO_BUP:
        case TOO_LAB:
        case TOO_WLB:
        case TOO_CLK:
            set_item(dlg, f-TOO_BAC);
            break;

        // -------------------------------------------------------
        case MEN_TIT:
        case MEN_FRM:
        case MEN_HIL:
        case MEN_BUL:
            set_item(dlg, f-MEN_TIT);
            break;

        case MEN_LBL:
            work_style.menuTitleLabel = false == work_style.menuTitleLabel;
            redraw_gui(dlg);
            break;

        case MEN_NTI:
            work_style.menuNoTitle = false == work_style.menuNoTitle;
            redraw_gui(dlg);
            break;

        // -------------------------------------------------------
        case WIN_TIT:
        case WIN_LAB:
        case WIN_HAN:
        case WIN_GRP:
        case WIN_BUT:
        case WIN_BUP:
        case WIN_FRM:
            set_item(dlg, f-WIN_TIT);
            break;

        // -------------------------------------------------------
        case MIS_ROT:
        case MIS_INF:
            set_item(dlg, f-MIS_ROT);
            break;

        // -------------------------------------------------------
        case GRD_SOL:
        case GRD_HOR:
        case GRD_VER:
        case GRD_DIA:
        case GRD_CDI:
        case GRD_PIP:
        case GRD_ELL:
        case GRD_REC:
        case GRD_PYR:
            f-=GRD_HOR;
            if (f < 0) f = B_SOLID;
            P0->type = f;
            P0->parentRelative = 0;
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case GRD_PRR:
            P0->parentRelative = true;
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case GRD_INL:
            P0->interlaced = 0!=(getbutton(dlg, f)->f & BN_ON);
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case BEV_FLA:
        case BEV_RAI:
        case BEV_SUN:
            P0->bevelstyle = f-BEV_FLA;
            if (P0->bevelstyle == BEVEL_FLAT)
                P0->bevelposition = 0;
            else
            if (P0->bevelposition == 0)
                P0->bevelposition = BEVEL1;
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case BEV_P2 :
            P0->bevelposition = (getbutton(dlg, f)->f & BN_ON) ? BEVEL2 : BEVEL1;
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case COL_GRD:
        case COL_TXT:
        case COL_BOR:
        case COL_DIS:
            v_colorsel_set = f-COL_GRD;
            configure_interface(dlg);
            break;

        // -------------------------------------------------------
        case MAR_WID1A:
        case MAR_WID1B:
        case MAR_WID1C:
        case MAR_WID2A:
        case MAR_WID2B:
        case MAR_WID2C:
        case MAR_WID3A:
        case MAR_WID3B:
        case MAR_WID3C:
        case MAR_WID4A:
            pSI = get_margin_item(f);
            if (pSI) {
                pSI->marginWidth = getbutton(dlg, f)->data;
                if (false == write_070)
                    v_upd_all = true;
            }
            redraw_gui(dlg);
            break;

        case BOR_WID:
            B0->borderWidth = getbutton(dlg, f)->data;
            if (false == write_070)
                v_upd_all = true;
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case BOR_DEF:
            v_all_border = 0 != (getbutton(dlg, f)->f & BN_ON);
            redraw_gui(dlg);
            break;

        case HAN_SIZ:
            work_style.handleHeight = getbutton(dlg, f)->data;
            redraw_gui(dlg);
            break;

        // -------------------------------------------------------
        case FNT_HDR:
            break;

        case FNT_NAM:
        case FNT_HEI:
        case FNT_BOL:
        case FNT_LEF:
        case FNT_CEN:
        case FNT_RIG:
        case FNT_CHO:
            pSI = get_font_item();
            if (pSI)
            {
                if (f >= FNT_LEF && f <= FNT_RIG) {
                    pSI->Justify = f - FNT_LEF;
                } else if (f == FNT_CHO) {
                    if (0 == choose_font(hwnd, pSI))
                        break;
                    set_font_default(pSI);
                    configure_interface(dlg);
                } else {
                    get_button_text(dlg, FNT_NAM, pSI->Font, sizeof pSI->Font);
                    pSI->FontHeight = getbutton(dlg, FNT_HEI)->data;
                    pSI->FontWeight = getbutton(dlg, FNT_BOL)->f&BN_ON ? FW_BOLD:FW_NORMAL;
                    if (FNT_NAM == f) {
                        i = get_string_index(pSI->Font, artwizfonts);
                        if (-1!= i) set_button_data(dlg, FNT_ART, i);
                    }
                    set_font_default(pSI);
                }
            }
            redraw_gui(dlg);
            break;

        case FNT_DEF:
            set_all_font();
            redraw_gui(dlg);
            break;

        case FNT_ART:
            pSI = get_font_item();
            if (pSI) {
                set_artwiz_font(pSI, &getbutton(dlg, f)->data);
                set_font_default(pSI);
                redraw_gui(dlg);
                configure_interface(dlg);
            }
            break;

        // -------------------------------------------------------
        case BUL_EMP:
        case BUL_TRI:
        case BUL_SQR:
        case BUL_DIA:
        case BUL_CIR:
            strcpy(work_style.menuBullet, get_bullet_string(f-BUL_EMP));
            redraw_gui(dlg);
            break;

        // -------------------------------------------------------
        case BUL_LEF:
        case BUL_RIG:
            strcpy(work_style.menuBulletPosition, f==BUL_LEF ? "left" : "right");
            redraw_gui(dlg);
            break;

        case ROT_MOD:
            work_style.rootInfo.mod = 0 != (getbutton(dlg, f)->f & BN_ON);
            redraw_gui(dlg);
            configure_interface(dlg);
            break;

        case ROT_MDX:
            work_style.rootInfo.modx = getbutton(dlg, f)->data;
            redraw_gui(dlg);
            break;

        case ROT_MDY:
            work_style.rootInfo.mody = getbutton(dlg, f)->data;
            redraw_gui(dlg);
            break;

        case ROT_TIL:
        case ROT_CEN:
        case ROT_FUL:
            i = f - ROT_TIL + WP_TILE;
            if (i == work_style.rootInfo.wpstyle)
                work_style.rootInfo.wpstyle = WP_NONE;
            else
                work_style.rootInfo.wpstyle = (char)i;
            configure_interface(dlg);
            fix_P0();
            break;

        case ROT_IMG:
            i = 0 != work_style.rootInfo.wpfile[0];
            get_button_text(dlg, f, work_style.rootInfo.wpfile, MAX_PATH);
            if (i != (0 != work_style.rootInfo.wpfile[0]))
                configure_interface(dlg);
            fix_P0();
            break;

        case ROT_SCL:
            work_style.rootInfo.scale = getbutton(dlg, f)->data;
            fix_P0();
            break;
        case ROT_SAT:
            work_style.rootInfo.sat = getbutton(dlg, f)->data;
            fix_P0();
            break;
        case ROT_HUE:
            work_style.rootInfo.hue = getbutton(dlg, f)->data;
            fix_P0();
            break;

        case INF_LN1:
        case INF_LN2:
        case INF_LN3:
        case INF_LN4:
        case INF_LN5:
            i = f - INF_LN1;
            buffer[0] = 0;
            get_button_text(dlg, f, buffer, sizeof buffer);
            if (strcmp(buffer, style_info[i])) {
                replace_str(&style_info[i], buffer);
                v_changed = 1;
            }
            if (lParam == 4 && i > 0) --i;
            if (lParam == 3 && i < 4) ++i;
            if (lParam == 2 && --i < 0) i = 4;
            if (lParam == 1 && ++i > 4) i = 0;
            if (lParam) set_button_focus(dlg, i + INF_LN1);
            break;

        case INF_CLR:
            for (i = 0; i < 5; ++i)
                replace_str(&style_info[i], "");
            set_button_focus(dlg, INF_LN1);
            v_changed = 1;
            configure_interface(dlg);
            break;

        case INF_NAM:
            set_auto_name();
            s = NULL;
            goto set_inf;

        case INF_AUT:
            s = ReadString(rcpath, "bbstylemaker.style.author", "<unknown>");
            goto set_inf;

        case INF_DAT:
        {
            time_t systemTime;
            time(&systemTime); // get this for the display

            setlocale(LC_TIME, ReadString(rcpath, "bbstylemaker.date.locale", "C"));
            //dbg_printf("locale: <%s>", setlocale(LC_TIME, NULL));

            strftime(buffer, sizeof buffer,
                ReadString(rcpath, "bbstylemaker.date.format", "%#x"),
                localtime(&systemTime)
                );
            s = buffer;
            goto set_inf;
        }
        set_inf:
            i = f - INF_NAM;
            if (s) replace_str(&style_info[i], s);
            set_button_text(dlg, INF_LN1+i, style_info[i]);
            set_button_focus(dlg, INF_LN1+i);
            v_changed = 1;
            break;

    }}
    return 0;
}

/*----------------------------------------------------------------------------*/
