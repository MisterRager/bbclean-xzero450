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

#ifndef BBSETTING_STYLEREADER_ONLY

#include "BB.h"
#define BBSETTING
#include "Settings.h"
#include "Menu/MenuMaker.h"

// to be used in multimonitor setups (in the future, maybe ...)
int screenNumber = 0;

#endif

//===========================================================================
// check a font if it is available on the system

static int CALLBACK EnumFontFamProc(
    ENUMLOGFONT FAR *lpelf,     // pointer to logical-font data 
    NEWTEXTMETRIC FAR *lpntm,   // pointer to physical-font data 
    int FontType,               // type of font 
    LPARAM lParam               // address of application-defined data  
   )
{
    (*(int*)lParam)++;
    return 0;
}   

static int checkfont (char *face)
{
    int data = 0;
    HDC hdc = CreateCompatibleDC(NULL);
    EnumFontFamilies(hdc, face, (FONTENUMPROC)EnumFontFamProc, (LPARAM)&data);
    DeleteDC(hdc);
    return data;
}

//===========================================================================
/*
    *nix font spec:
    -foundry-family-weight-slant-setwidth-addstyle-pixel-point
    -resx-resy-spacing-width-charset-encoding-
    weight: "-medium-", "-bold-", "-demibold-", "-regular-",
    slant: "-r-", "-i-", "-o-", "-ri-", "-ro-",
*/

static int getweight (const char *p)
{
    static const char *fontweightstrings[] = {
    "thin", "extralight", "light", "normal",
    "medium", "demibold", "bold", "extrabold",
    "heavy", "regular", "semibold", NULL
    };
    int i = get_string_index(p, fontweightstrings) + 1;
    if (i==0 || i==10) i = 4;
    if (i==11) i=6;
    return i*100;
}

static void tokenize_string(char *buffer, char **pp, const char *src, int n, const char *delims)
{
    while (n--)
    {
        NextToken(buffer, &src, delims);
        buffer+=strlen(*pp = buffer) + 1;
        ++pp;
    }
}

static void parse_font(StyleItem *si, const char *font)
{
    static const char *scanlist[] =
    {
        "lucidatypewriter"  ,
        "fixed"             ,
        "lucida"            ,
        "helvetica"         ,
        "calisto mt"        ,
        "8x8\\ system\\ font" ,
        "8x8 system font" ,
        NULL
    };

    static const char *replacelist[] =
    {
        "edges"             ,
        "lucida console"    ,
        "lucida sans"       ,
        "tahoma"            ,
        "verdana"           ,
        "edges" ,
        "edges" ,
    };

    char fontstring[256]; char *p[16]; char *b;
    if ('-' == *font)
    {
        tokenize_string(fontstring, p, font+1, 16, "-");
        int i = get_string_index(b = p[1], scanlist);
        strcpy(si->Font, -1 == i ? b : replacelist[i]);

        if (*(b = p[2]))
        {
            si->FontWeight = getweight(b);
            si->validated |= VALID_FONTWEIGHT;
        }

        if (*(b = p[6])>='1' && *b<='9')
        {
            si->FontHeight = atoi(b) * 12 / 10;
            si->validated |= VALID_FONTHEIGHT;
        }
        else
        if (*(b = p[7])>='1' && *b<='9')
        {
            si->FontHeight = atoi(b) * 12 / 100;
            si->validated |= VALID_FONTHEIGHT;
        }
    }
    else
    if (strchr(font, '/'))
    {
        tokenize_string(fontstring, p, font, 3, "/");
        strcpy(si->Font, p[0]);
        if (*(b = p[1]))
        {
            si->FontHeight = atoi(b);
            si->validated |= VALID_FONTHEIGHT;
        }

        if (*(b = p[2]))
        {
            if (stristr(b, "shadow"))
                si->FontShadow = true;
            if (stristr(b, "bold"))
            {
                si->FontWeight = FW_BOLD;
                si->validated |= VALID_FONTWEIGHT;
            }
        }

        //dbg_printf("<%s> <%s> <%s>", p[0], p[1], p[2]);
        //dbg_printf("<%s> %d %d", si->Font, si->FontHeight, si->FontWeight);
    }
    else
    {
        strcpy(si->Font, font);
        b = strlwr(strcpy(fontstring, font));
        if (strstr(b, "lucidasans"))
        {
            strcpy(si->Font, "gelly");
        }
    }

    if (0 == checkfont(si->Font))
    {
        LOGFONT logFont;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
        strcpy(si->Font, logFont.lfFaceName);
    }
}

//===========================================================================
// API: CreateStyleFont
// Purpose: Create a Font, possible substitutions have been already applied.
//===========================================================================

HFONT CreateStyleFont(StyleItem * si)
{
    return CreateFont(
        si->FontHeight,
        0, 0, 0,
        si->FontWeight,
        false, false, false,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH|FF_DONTCARE,
        si->Font
        );
}

//===========================================================================
enum
{
    C_INT = 1,
    C_BOL,
    C_STR,
    C_COL,
    C_STY,

    C_TEX,
    C_CO1,
    C_CO2,
    C_CO3,
    C_CO4,
    C_CO5,
    C_CO6,

    C_FHEI,
    C_FWEI,
    C_FONT,
	C_SHAX,
	C_SHAY,
    C_JUST,
    C_ALPHA,

    C_MARG,
    C_BOCO,
    C_BOWD,

    C_CO1ST,
    C_CO2ST,
};

struct ShortStyleItem
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
    int validated;
    char Font[16];
};

static ShortStyleItem DefStyleA =
{
    BEVEL_RAISED,  BEVEL1, B_DIAGONAL, FALSE, FALSE,
    0xEEEEEE, 0xCCCCCC, 0x555555, 12, FW_NORMAL, DT_LEFT, 0, "verdana"
};

static ShortStyleItem DefStyleB =
{
    BEVEL_RAISED,  BEVEL1, B_VERTICAL, FALSE, FALSE,
    0xCCCCCC, 0xAAAAAA, 0x333333, 12, FW_NORMAL, DT_CENTER, 0, "verdana"
};

static COLORREF DefBorderColor = 0x777777;

//===========================================================================
struct items { void *v; char * rc_string; void *def; unsigned short id; unsigned flags; };

#define HAS_TEXTURE (VALID_TEXTURE|VALID_COLORFROM|VALID_COLORTO|VALID_BORDER|VALID_BORDERCOLOR|VALID_FROMSPLITTO|VALID_TOSPLITTO)
#define HAS_FONT (VALID_FONT|VALID_FONTHEIGHT|VALID_FONTWEIGHT)
#define HAS_SHADOW (VALID_SHADOWCOLOR|VALID_SHADOWX|VALID_SHADOWY)


static struct items StyleItems[] = {
{   &mStyle.borderWidth         , "borderWidth:",           (void*) 1,                      C_INT, 0 },
{   &mStyle.borderColor         , "borderColor:",           (void*) &DefBorderColor,        C_COL, 0 },
{   &mStyle.bevelWidth          , "bevelWidth:",            (void*) 1,                      C_INT, 0 },
{   &mStyle.handleHeight        , "handleWidth:",           (void*) 5,                      C_INT, 0 },
{   &mStyle.rootCommand         , "rootCommand:",           (void*)"",                      C_STR, sizeof mStyle.rootCommand },

{   &mStyle.Toolbar             , "toolbar",                (void*) &DefStyleA, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_BORDER|DEFAULT_MARGIN },
{   &mStyle.ToolbarButton       , "toolbar.button",         (void*) &DefStyleB, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_PICCOLOR|VALID_MARGIN },
{   &mStyle.ToolbarButtonPressed, "toolbar.button.pressed", (void*) &DefStyleA, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_PICCOLOR },
{   &mStyle.ToolbarLabel        , "toolbar.label",          (void*) &DefStyleB, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_MARGIN },
{   &mStyle.ToolbarWindowLabel  , "toolbar.windowLabel",    (void*) &DefStyleA, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.ToolbarClock        , "toolbar.clock",          (void*) &DefStyleB, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR },

{   &mStyle.MenuTitle           , "menu.title",             (void*) &DefStyleB, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_MARGIN },
{	&mStyle.MenuGrip			, "menu.grip",				(void*) &DefStyleB,	C_STY,	HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_MARGIN },
{   &mStyle.MenuFrame           , "menu.frame",             (void*) &DefStyleA, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_PICCOLOR|VALID_MARGIN|HAS_FONT|VALID_JUSTIFY|DEFAULT_BORDER },
{   &mStyle.MenuHilite          , "menu.hilite",            (void*) &DefStyleB, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_PICCOLOR },
{   &mStyle.MenuTransparentColor          , "menu.transparentColor",            (void*) &DefStyleB, C_STY,  VALID_TEXTCOLOR },

// menu.item.marginWidth:
{   &mStyle.MenuHilite          , "menu.item",              (void*) &DefStyleB, C_STY,  VALID_MARGIN|VALID_TEXTCOLOR|HAS_SHADOW|VALID_OUTLINECOLOR },
{   &mStyle.MenuSepMargin       , "menu.separator.margin:", (void*) 4, C_INT, 0 },
{   &mStyle.MenuSepColor        , "menu.separator.color:",  (void*) &mStyle.MenuFrame.TextColor, C_COL, 0 },
{   &mStyle.MenuSepShadowColor  , "menu.separator.shadowColor:",  (void*) &mStyle.MenuFrame.ShadowColor, C_COL, 0 },
//{   &mStyle.MenuVolume          , "menu.volume",            (void*) &mStyle.MenuHilite, C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|VALID_PICCOLOR },

{   &mStyle.menuBullet          , "menu.bullet:",           (void*) "triangle", C_STR, sizeof mStyle.menuBullet  },
{   &mStyle.menuBulletPosition  , "menu.bullet.position:",  (void*) "right",  C_STR, sizeof mStyle.menuBulletPosition  },

{   &mStyle.MenuFrame.disabledColor , "menu.frame.disableColor:", (void*) &mStyle.MenuFrame.TextColor, C_COL, 0 },

#ifndef BBSETTING_NOWINDOW
// window.font:
{   &mStyle.windowLabelFocus    , "window",                 (void*) &mStyle.Toolbar,                C_STY,  HAS_FONT|HAS_SHADOW|VALID_JUSTIFY },

{   &mStyle.windowTitleFocus    , "window.title.focus",     (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowLabelFocus    , "window.label.focus",     (void*) &mStyle.ToolbarWindowLabel,     C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR|HAS_FONT|VALID_JUSTIFY },
{   &mStyle.windowHandleFocus   , "window.handle.focus",    (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowGripFocus     , "window.grip.focus",      (void*) &mStyle.ToolbarWindowLabel,     C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowButtonFocus   , "window.button.focus",    (void*) &mStyle.ToolbarButton,          C_STY,  HAS_TEXTURE|VALID_PICCOLOR },
{   &mStyle.windowButtonPressed , "window.button.pressed",  (void*) &mStyle.ToolbarButtonPressed,   C_STY,  HAS_TEXTURE|VALID_PICCOLOR },

{   &mStyle.windowTitleUnfocus  , "window.title.unfocus",   (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER  },
{   &mStyle.windowLabelUnfocus  , "window.label.unfocus",   (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|HAS_SHADOW|VALID_TEXTCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.windowHandleUnfocus , "window.handle.unfocus",  (void*) &mStyle.Toolbar,                C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowGripUnfocus   , "window.grip.unfocus",    (void*) &mStyle.ToolbarLabel,           C_STY,  HAS_TEXTURE|DEFAULT_BORDER },
{   &mStyle.windowButtonUnfocus , "window.button.unfocus",  (void*) &mStyle.ToolbarButton,          C_STY,  HAS_TEXTURE|VALID_PICCOLOR },

// new bb4nix 070 style props
{   &mStyle.handleHeight            , "window.handleHeight:"                , (void*) &mStyle.handleHeight, C_INT, 0 },
{   &mStyle.frameWidth              , "window.frame.borderWidth:"           , (void*) &mStyle.borderWidth, C_INT, 0 },
{   &mStyle.windowFrameFocusColor   , "window.frame.focus.borderColor:"     , (void*) &mStyle.borderColor, C_COL, 0 },
{   &mStyle.windowFrameUnfocusColor , "window.frame.unfocus.borderColor:"   , (void*) &mStyle.borderColor, C_COL, 0 },

// bb4nix 0.65 style props, ignored in bb4win
//{   &mStyle.frameWidth              , "frameWidth:"                   , (void*) &mStyle.borderWidth, C_INT, 0 },
//{   &mStyle.windowFrameFocusColor   , "window.frame.focusColor:"      , (void*) &mStyle.borderColor, C_COL, 0 },
//{   &mStyle.windowFrameUnfocusColor , "window.frame.unfocusColor:"    , (void*) &mStyle.borderColor, C_COL, 0 },

// window margins
{   &mStyle.windowTitleFocus    , "window.title",           (void*) &mStyle.Toolbar,                C_STY,  VALID_MARGIN|DEFAULT_MARGIN },
{   &mStyle.windowLabelFocus    , "window.label",           (void*) &mStyle.ToolbarLabel,           C_STY,  VALID_MARGIN },
{   &mStyle.windowButtonFocus   , "window.button",          (void*) &mStyle.ToolbarButton,          C_STY,  VALID_MARGIN },
#endif
{   &mStyle.MenuVolume          , "menu.volume",            (void*) &mStyle.MenuHilite,             C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_PICCOLOR|VALID_JUSTIFY|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },
{   &mStyle.MenuVolumeHilite    , "menu.volume.hilite",     (void*) &mStyle.MenuHilite,             C_STY,  HAS_TEXTURE|VALID_TEXTCOLOR|VALID_PICCOLOR|VALID_SHADOWCOLOR|VALID_OUTLINECOLOR },

{ NULL, NULL, NULL, 0, 0 }
};

//===========================================================================
// API: GetSettingPtr - retrieve a pointer to a setting var/struct

void * GetSettingPtr(int i)
{
    switch (i) {

    case SN_STYLESTRUCT             : return &mStyle;

    case SN_TOOLBAR                 : return &mStyle.Toolbar                ;
    case SN_TOOLBARBUTTON           : return &mStyle.ToolbarButton          ;
    case SN_TOOLBARBUTTONP          : return &mStyle.ToolbarButtonPressed   ;
    case SN_TOOLBARLABEL            : return &mStyle.ToolbarLabel           ;
    case SN_TOOLBARWINDOWLABEL      : return &mStyle.ToolbarWindowLabel     ;
    case SN_TOOLBARCLOCK            : return &mStyle.ToolbarClock           ;
    case SN_MENUTITLE               : return &mStyle.MenuTitle              ;
    case SN_MENUFRAME               : return &mStyle.MenuFrame              ;
    case SN_MENUHILITE              : return &mStyle.MenuHilite             ;
	case SN_MENUGRIP				: return &mStyle.MenuGrip				;

    case SN_MENUBULLET              : return &mStyle.menuBullet             ;
    case SN_MENUBULLETPOS           : return &mStyle.menuBulletPosition     ;

    case SN_BORDERWIDTH             : return &mStyle.borderWidth            ;
    case SN_BORDERCOLOR             : return &mStyle.borderColor            ;
    case SN_BEVELWIDTH              : return &mStyle.bevelWidth             ;
    case SN_FRAMEWIDTH              : return &mStyle.frameWidth             ;
    case SN_HANDLEHEIGHT            : return &mStyle.handleHeight           ;
    case SN_ROOTCOMMAND             : return &mStyle.rootCommand            ;

    case SN_MENUALPHA               : return &Settings_menuAlpha            ;
    case SN_TOOLBARALPHA            : return &Settings_toolbarAlpha         ;
    case SN_METRICSUNIX             : return &mStyle.metricsUnix            ;
    case SN_BULLETUNIX              : return &mStyle.bulletUnix             ;

    case SN_WINFOCUS_TITLE          : return &mStyle.windowTitleFocus       ;
    case SN_WINFOCUS_LABEL          : return &mStyle.windowLabelFocus       ;
    case SN_WINFOCUS_HANDLE         : return &mStyle.windowHandleFocus      ;
    case SN_WINFOCUS_GRIP           : return &mStyle.windowGripFocus        ;
    case SN_WINFOCUS_BUTTON         : return &mStyle.windowButtonFocus      ;
    case SN_WINFOCUS_BUTTONP        : return &mStyle.windowButtonPressed    ;
    case SN_WINUNFOCUS_TITLE        : return &mStyle.windowTitleUnfocus     ;
    case SN_WINUNFOCUS_LABEL        : return &mStyle.windowLabelUnfocus     ;
    case SN_WINUNFOCUS_HANDLE       : return &mStyle.windowHandleUnfocus    ;
    case SN_WINUNFOCUS_GRIP         : return &mStyle.windowGripUnfocus      ;
    case SN_WINUNFOCUS_BUTTON       : return &mStyle.windowButtonUnfocus    ;

    case SN_WINFOCUS_FRAME_COLOR    : return &mStyle.windowFrameFocusColor  ;
    case SN_WINUNFOCUS_FRAME_COLOR  : return &mStyle.windowFrameUnfocusColor;

	case SN_NEWMETRICS              : return (void*)Settings_newMetrics;

	default                         : return NULL;
    }
}

//===========================================================================
int Settings_ItemSize(int i)
{
    switch (i) {

    case SN_STYLESTRUCT             : return sizeof (StyleStruct);

    case SN_TOOLBAR                 :
    case SN_TOOLBARBUTTON           :
    case SN_TOOLBARBUTTONP          :
    case SN_TOOLBARLABEL            :
    case SN_TOOLBARWINDOWLABEL      :
    case SN_TOOLBARCLOCK            :
    case SN_MENUTITLE               :
    case SN_MENUFRAME               :
    case SN_MENUHILITE              : 
	case SN_MENUGRIP				: return sizeof (StyleItem);

    case SN_MENUBULLET              :
    case SN_MENUBULLETPOS           : return -1; // string, have to take strlen

    case SN_BORDERWIDTH             : return sizeof (int);
    case SN_BORDERCOLOR             : return sizeof (COLORREF);
    case SN_BEVELWIDTH              : return sizeof (int);
    case SN_FRAMEWIDTH              : return sizeof (int);
    case SN_HANDLEHEIGHT            : return sizeof (int);
    case SN_ROOTCOMMAND             : return -1; // string, have to take strlen

    case SN_MENUALPHA               : return sizeof (int);
    case SN_TOOLBARALPHA            : return sizeof (int);
    case SN_METRICSUNIX             : return sizeof (bool);
    case SN_BULLETUNIX              : return sizeof (bool);

    case SN_WINFOCUS_TITLE          :
    case SN_WINFOCUS_LABEL          :
    case SN_WINFOCUS_HANDLE         :
    case SN_WINFOCUS_GRIP           :
    case SN_WINFOCUS_BUTTON         :
    case SN_WINFOCUS_BUTTONP        :
    case SN_WINUNFOCUS_TITLE        :
    case SN_WINUNFOCUS_LABEL        :
    case SN_WINUNFOCUS_HANDLE       :
    case SN_WINUNFOCUS_GRIP         :
    case SN_WINUNFOCUS_BUTTON       : return sizeof (StyleItem);

    case SN_WINFOCUS_FRAME_COLOR    :
    case SN_WINUNFOCUS_FRAME_COLOR  : return sizeof (COLORREF);

	default                         : return 0;
    }
}

//===========================================================================

static int ParseJustify (const char *buff)
{
    if (0==stricmp(buff, "center"))   return DT_CENTER;
    if (0==stricmp(buff, "right"))    return DT_RIGHT;
    return DT_LEFT;
}

static const char *check_global_font(const char *p, const char *fullkey)
{
    if (Settings_globalFonts)
    {
        char globalkey[80];
        strcat(strcpy(globalkey, "blackbox.global."), fullkey);
        const char *p2 = ReadValue(extensionsrcPath(), globalkey, NULL);
        //dbg_printf("<%s> <%s>", globalkey, p2);
        if (p2 && p2[0]) return p2;
    }
    return p;
}

//===========================================================================

static void read_style_item (const char * style, StyleItem *si, char *key, int v,  StyleItem *def)
{
    static struct s_prop { char *k; char mode; int v; } s_prop[]= {

    // texture type
    { ":",                  C_TEX   , VALID_TEXTURE  },
    // colors, from, to, text, pics
    { ".color:",            C_CO1   , VALID_COLORFROM  },
    { ".colorTo:",          C_CO2   , VALID_COLORTO  },
    { ".textColor:",        C_CO3   , VALID_TEXTCOLOR },
    { ".picColor:",         C_CO4   , VALID_PICCOLOR },
    { ".shadowColor:",      C_CO5   , VALID_SHADOWCOLOR },
    { ".outlineColor:",     C_CO6   , VALID_OUTLINECOLOR },
    // font settings
    { ".font:",             C_FONT  , VALID_FONT  },
    { ".fontHeight:",       C_FHEI  , VALID_FONTHEIGHT  },
    { ".fontWeight:",       C_FWEI  , VALID_FONTWEIGHT  },
    { ".justify:",          C_JUST  , VALID_JUSTIFY  },
	{ ".shadowX:",          C_SHAX , VALID_SHADOWX },
	{ ".shadowY:",          C_SHAY , VALID_SHADOWY },
	// _new in BBNix 0.70
    { ".borderWidth:",      C_BOWD  , VALID_BORDER  },
    { ".borderColor:",      C_BOCO  , VALID_BORDERCOLOR  },
    { ".marginWidth:",      C_MARG  , VALID_MARGIN   },
    // xoblite
    { ".bulletColor:",      C_CO4   , VALID_PICCOLOR  },
    // OpenBox
    { ".color.splitTo:",    C_CO1ST , VALID_FROMSPLITTO },
    { ".colorTo.splitTo:",  C_CO2ST , VALID_TOSPLITTO },

    { NULL, 0, 0}
    };

    COLORREF cr;
    int l = strlen(key);
    struct s_prop *cp = s_prop;
    char fullkey[80]; memcpy(fullkey, key, l);

    si->nVersion = 2;

    do
    {
        if (cp->v & v)
        {
            strcpy(fullkey + l, cp->k);
            const char *p = ReadValue(style, fullkey, NULL);
            int found = FoundLastValue();
            if ((si->validated & cp->v) && 1 != found)
                continue;

            switch (cp->mode)
            {
            // --- textture ---
            case C_TEX:
                if (p)
                {
                    ParseItem(p, si);
                    si->bordered = NULL != stristr(p, "border");
                }
                else
                {
                    memcpy(si, def, sizeof(ShortStyleItem));
                    if (def->bordered){
                        si->bordered    = def->bordered;
                        si->borderWidth = def->borderWidth;
                        si->borderColor = def->borderColor;
                    }
                    if (def->validated & VALID_SHADOWCOLOR)
                        si->ShadowColor = def->ShadowColor;
                    if (def->validated & VALID_OUTLINECOLOR)
                        si->OutlineColor = def->OutlineColor;
                }
                break;

            // --- colors ---
            case C_CO1:
                cr = ReadColorFromString(p);
				si->Color = ((COLORREF)-1) != cr ? cr : def ->Color;
                break;

            case C_CO2:
                cr = ReadColorFromString(p);
				si->ColorTo = ((COLORREF)-1) != cr ? cr : def ->ColorTo;
                break;

            case C_CO3:
                cr = ReadColorFromString(p);
				si->TextColor = ((COLORREF)-1) != cr ? cr : def ->TextColor;
                break;

            case C_CO4:
                cr = ReadColorFromString(p);
                if (v & VALID_TEXTCOLOR)
					si->foregroundColor = ((COLORREF)-1) != cr ? cr : si->picColor;
                else
					si->picColor = ((COLORREF)-1) != cr ? cr : def->picColor;
                break;

            case C_CO5:
            	si->ShadowColor = ReadColorFromString(p);
                break;

            case C_CO6:
                cr = ReadColorFromString(p);
                si->OutlineColor = ReadColorFromString(p);
                break;

            // --- Border & margin ---
            case C_BOCO:
                cr = ReadColorFromString(p);
				si->borderColor = ((COLORREF)-1) != cr ? cr : mStyle.borderColor;
                break;

            case C_BOWD:
                if ((v & DEFAULT_BORDER) || si->bordered)
                {
                    if (p) si->borderWidth = atoi(p);
                    else si->borderWidth = mStyle.borderWidth;
                }
                else
                {
                    si->borderWidth = 0;
                    continue;
                }
                break;

            case C_MARG:
                if (p)
                    si->marginWidth = atoi(p);
                else
                if (v & DEFAULT_MARGIN)
                    si->marginWidth = mStyle.bevelWidth;
                else
                    si->marginWidth = 0;
                break;

            // --- Font ---
            case C_FONT:
                p = check_global_font(p, fullkey);
                if (p) parse_font(si, p);
                else strcpy(si->Font, def->Font);
                break;

            case C_FHEI:
                p = check_global_font(p, fullkey);
                if (p) si->FontHeight = atoi(p);
                else
                if (si->validated & VALID_FONT) si->FontHeight = 12;
                else si->FontHeight = def->FontHeight;
                break;

            case C_FWEI:
                p = check_global_font(p, fullkey);
                if (p) si->FontWeight = getweight(p);
                else
                if (si->validated & VALID_FONT) si->FontWeight = FW_NORMAL;
                else si->FontWeight = def->FontWeight;
				break;

			case C_SHAX:
				si->ShadowX = p ? atoi(p) : 0;
				break;

			case C_SHAY:
				si->ShadowY = p ? atoi(p) : 0;
                break;

            // --- Alignment ---
            case C_JUST:
                if (p) si->Justify = ParseJustify(p);
                else si->Justify = def->Justify;
                break;

            // --- Split Color ---
            case C_CO1ST:
                si->ColorSplitTo = ReadColorFromString(p);
                break;

            case C_CO2ST:
                si->ColorToSplitTo = ReadColorFromString(p);
                break;
            }

            if (p) si->validated |= cp->v;
        }
    }
    while ((++cp)->k);
}

//===========================================================================
static void ReadStyle(const char *style)
{
    ZeroMemory(&mStyle, (unsigned)&((StyleStruct*)NULL)->bulletUnix);
    mStyle.metricsUnix = true;
    Settings_newMetrics = is_newstyle(style);

    struct items *p = StyleItems;
    do
    {
        void *def = p->def; int n; bool b; COLORREF cr;
        switch (p->id)
        {
            case C_STY:
                    read_style_item (style, (StyleItem*)p->v, p->rc_string, p->flags, (StyleItem*)p->def);
                break;

            case C_INT:
                n = HIWORD(def) ? *(int*)def : (int) def;
                *(int*)p->v = ReadInt(style, p->rc_string, n);
                break;

            case C_BOL:
                b = HIWORD(def) ? *(bool*)def : (bool)def;
                *(bool*)p->v = ReadBool(style, p->rc_string, b);
                break;

            case C_ALPHA:
                n = HIWORD(def) ? *(int*)def : (int) def;
                *(BYTE*)p->v = ReadInt(style, p->rc_string, n);
                break;

            case C_COL:
                cr = ReadColorFromString(ReadValue(style, p->rc_string, NULL));
				*(COLORREF*)p->v = ((COLORREF)-1) != cr ? cr : *(COLORREF*)def;
                break;

            case C_STR:
                strcpy_max((char*)p->v, ReadString(style, p->rc_string, (char*)def), p->flags);
                break;
        }
    } while ((++p)->v);
}

//===========================================================================
#ifndef BBSETTING_STYLEREADER_ONLY
//===========================================================================
struct rccfg { char *key; char mode; void *def; void *ptr; };

static struct rccfg ext_rccfg[] = {

    //{ "blackbox.appearance.metrics.unix:",      C_BOL, (void*)true,     &Settings_newMetrics },

    { "blackbox.appearance.bullet.unix:",       C_BOL, (void*)true,     &mStyle.bulletUnix },
    { "blackbox.appearance.arrow.unix:",        C_BOL, (void*)false,    &Settings_arrowUnix },
    { "blackbox.appearance.cursor.usedefault:", C_BOL, (void*)false,    &Settings_usedefCursor },

    { "blackbox.desktop.marginLeft:",           C_INT, (void*)-1,       &Settings_desktopMargin.left },
    { "blackbox.desktop.marginRight:",          C_INT, (void*)-1,       &Settings_desktopMargin.right },
    { "blackbox.desktop.marginTop:",            C_INT, (void*)-1,       &Settings_desktopMargin.top },
    { "blackbox.desktop.marginBottom:",         C_INT, (void*)-1,       &Settings_desktopMargin.bottom },

    { "blackbox.snap.toPlugins:",               C_BOL, (void*)true,     &Settings_edgeSnapPlugins },
    { "blackbox.snap.padding:",                 C_INT, (void*)2,        &Settings_edgeSnapPadding },
    { "blackbox.snap.threshold:",               C_INT, (void*)7,        &Settings_edgeSnapThreshold },

    { "blackbox.background.enabled:",           C_BOL, (void*)true,     &Settings_background_enabled  },
    { "blackbox.background.smartWallpaper:",    C_BOL, (void*)true,     &Settings_smartWallpaper },

    { "blackbox.workspaces.followActive:",      C_BOL, (void*)true,     &Settings_followActive },
    { "blackbox.workspaces.followMoved:",       C_BOL, (void*)true,     &Settings_followMoved },
    { "blackbox.workspaces.altMethod:",         C_BOL, (void*)false,    &Settings_altMethod },
    { "blackbox.workspaces.restoreToCurrent:",  C_BOL, (void*)true,     &Settings_restoreToCurrent },
    { "blackbox.workspaces.xpfix:",             C_BOL, (void*)false,    &Settings_workspacesPCo },

    { "blackbox.options.shellContextMenu:",     C_BOL, (void*)false,    &Settings_shellContextMenu },
    { "blackbox.options.desktopHook:",          C_BOL, (void*)false,    &Settings_desktopHook   },
    { "blackbox.options.logging:",              C_INT, (void*)0,        &Settings_LogFlag },

    { "blackbox.global.fonts.enabled:",         C_BOL, (void*)false,    &Settings_globalFonts },
    { "blackbox.editor:",                       C_STR, (void*)"notepad.exe", Settings_preferredEditor },

    { "blackbox.menu.volumeWidth:",             C_INT, (void*)0,        &Settings_menuVolumeWidth },
    { "blackbox.menu.volumeHeight:",            C_INT, (void*)18,       &Settings_menuVolumeHeight },
    { "blackbox.menu.volumeHilite:",            C_BOL, (void*)true,     &Settings_menuVolumeHilite },

    { "blackbox.menu.maxHeightRatio:",          C_INT, (void*)80,       &Settings_menuMaxHeightRatio },
    { "blackbox.menu.keepHilite:",              C_BOL, (void*)false,    &Settings_menuKeepHilite },
    { "blackbox.recent.menuFile:",              C_STR, (void*)"",       &Settings_recentMenu },
    { "blackbox.recent.itemKeepSize:",          C_INT, (void*)3,        &Settings_recentItemKeepSize },
    { "blackbox.recent.itemSortSize:",          C_INT, (void*)5,        &Settings_recentItemSortSize },
    { "blackbox.recent.withBeginEnd:",          C_BOL, (void*)true,     &Settings_recentBeginEnd },
	{ "blackbox.tweaks.noOleUnInit:",           C_BOL, (void*)false,    &Settings_noOleUninit },

	{ "blackbox.processPriority:",				C_INT, (void*)-1,		&Settings_processPriority},

	// --------------------------------

    { NULL, 0, NULL, NULL }
};

//===========================================================================
static struct rccfg rccfg[] = {
    { "#toolbar.enabled:",          C_BOL, (void*)true,     &Settings_toolbarEnabled },
    { "#toolbar.placement:",        C_STR, (void*)"TopCenter", Settings_toolbarPlacement },
    { "#toolbar.widthPercent:",     C_INT, (void*)66,       &Settings_toolbarWidthPercent },
    { "#toolbar.onTop:",            C_BOL, (void*)false,    &Settings_toolbarOnTop },
    { "#toolbar.autoHide:",         C_BOL, (void*)false,    &Settings_toolbarAutoHide },
    { "#toolbar.pluginToggle:",     C_BOL, (void*)true ,    &Settings_toolbarPluginToggle },
    { "#toolbar.alpha.enabled:",    C_BOL, (void*)false,    &Settings_toolbarAlphaEnabled },
    { "#toolbar.alpha.value:",      C_INT, (void*)255,      &Settings_toolbarAlphaValue },

    { ".menu.position.x:",          C_INT, (void*)100,      &Settings_menuPositionX },
    { ".menu.position.y:",          C_INT, (void*)100,      &Settings_menuPositionY },
	{ ".menu.positionAdjust.placement",	C_STR,	(void*)"default",	&Settings_menuPositionAdjustPlacement },
	{ ".menu.positionAdjust.x:",	C_INT, (void*)0,		&Settings_menuPositionAdjustX },
	{ ".menu.positionAdjust.y:",	C_INT, (void*)0,		&Settings_menuPositionAdjustY },
    { ".menu.maxWidth:",            C_INT, (void*)240,      &Settings_menuMaxWidth },
	{ ".menu.minWidth:",            C_INT, (void*)50,		&Settings_menuMinWidth },
	{ ".menu.icon.size:",           C_INT, (void*)-1,       &Settings_menuIconSize },
	{ ".menu.icon.saturation:",     C_INT, (void*)0,        &Settings_menuIconSaturation },
	{ ".menu.icon.hue:",            C_INT, (void*)60,       &Settings_menuIconHue },
	{ ".menu.scrollButton.hue:",    C_INT, (void*)60,       &Settings_menuScrollHue },
    { ".menu.popupDelay:",          C_INT, (void*)80,       &Settings_menuPopupDelay },
	{ ".menu.closeDelay:",          C_INT, (void*)80,       &Settings_menuCloseDelay },
    { ".menu.mouseWheelFactor:",    C_INT, (void*)3,        &Settings_menuMousewheelfac },
	//{ ".menu.alpha.enabled:",       C_BOL, (void*)false,    &Settings_menuAlphaEnabled },
	//{ ".menu.shadow:",			C_BOL, (void*)false,	&Settings_menuShadow },
	{ ".menu.alphaMethod:",		C_STR, (void*)"default", &Settings_menuAlphaMethod_cfg },
    { ".menu.alpha.value:",         C_INT, (void*)255,      &Settings_menuAlphaValue },
	{ ".menu.shadows.Enabled:",     C_BOL, (void*)false,    &Settings_menuShadowsEnabled },
    { ".menu.onTop:",               C_BOL, (void*)false,    &Settings_menusOnTop },
    { ".menu.snapWindow:",          C_BOL, (void*)true,     &Settings_menusSnapWindow },
    { ".menu.pluginToggle:",        C_BOL, (void*)true,     &Settings_menuspluginToggle },
    { ".menu.bulletPosition:",      C_STR, (void*)"default", &Settings_menuBulletPosition_cfg },
	{ ".menu.scrollPosition:",      C_STR, (void*)"default", &Settings_menuScrollPosition_cfg },
	{ ".menu.separatorStyle:",      C_STR, (void*)"Gradient", &Settings_menuSeparatorStyle },
	{ ".menu.fullSeparatorWidth:",  C_BOL, (void*)false,    &Settings_menuFullSeparatorWidth },
	{ ".menu.compactSeparators:",   C_BOL, (void*)false,    &Settings_compactSeparators },
    { ".menu.sortbyExtension:",     C_BOL, (void*)false,    &Settings_menusExtensionSort },
	{ ".menu.separateFolders:",     C_BOL, (void*)false,    &Settings_menusSeparateFolders },
	{ ".menu.grip.enabled:",		C_BOL, (void*)false,	&Settings_menusGripEnabled },

    { "#workspaces:",               C_INT, (void*)4,        &Settings_workspaces },
    { "#workspaceNames:",           C_STR, (void*)"alpha,beta,gamma,delta", &Settings_workspaceNames },
    { "#strftimeFormat:",           C_STR, (void*)"%I:%M %p", Settings_toolbarStrftimeFormat },
    { "#fullMaximization:",         C_BOL, (void*)false,    &Settings_fullMaximization },
    { "#focusModel:",               C_STR, (void*)"ClickToFocus", Settings_focusModel },

    { ".imageDither:",              C_BOL, (void*)true,     &Settings_imageDither },
    { ".opaqueMove:",               C_BOL, (void*)true,     &Settings_opaqueMove },
    { ".autoRaiseDelay:",           C_INT, (void*)250,      &Settings_autoRaiseDelay },

    //{ ".changeWorkspaceWithMouseWheel:", C_BOL, (void*)false,    &Settings_desktopWheel   },

    /* *nix settings, not used here
    // ----------------------------------
    { "#edgeSnapThreshold:",        C_INT, (void*)7,        &Settings_edgeSnapThreshold },

    { "#focusLastWindow:",          C_BOL, (void*)false,    &Settings_focusLastWindow },
    { "#focusNewWindows:",          C_BOL, (void*)false,    &Settings_focusNewWindows },
    { "#windowPlacement:",          C_STR, (void*)"RowSmartPlacement", &Settings_windowPlacement },
    { "#colPlacementDirection:",    C_STR, (void*)"TopToBottom", &Settings_colPlacementDirection },
    { "#rowPlacementDirection:",    C_STR, (void*)"LeftToRight", &Settings_rowPlacementDirection },

    { ".colorsPerChannel:",         C_INT, (void*)4,        &Settings_colorsPerChannel },
    { ".doubleClickInterval:",      C_INT, (void*)250,      &Settings_dblClickInterval },
    { ".cacheLife:",                C_INT, (void*)5,        &Settings_cacheLife },
    { ".cacheMax:",                 C_INT, (void*)200,      &Settings_cacheMax },
    // ---------------------------------- */

    { NULL }
};

//===========================================================================
static const char * makekey(char *buff, struct rccfg * cp)
{
    const char *k = cp->key;
    if (k[0]=='.')
        sprintf(buff, "session%s", k);
    else
    if (k[0]=='#')
        sprintf(buff, "session.screen%d.%s", screenNumber, k+1);
    else
        return k;
    return buff;
}

static void Settings_ReadSettings(const char *bbrc, struct rccfg * cp)
{
    do {
        char keystr[80]; const char *key = makekey(keystr, cp);
        switch (cp->mode)
        {
            case C_INT:
                *(int*) cp->ptr = ReadInt(bbrc, key, (int) cp->def);
                break;
            case C_BOL:
                *(bool*) cp->ptr = ReadBool (bbrc, key, (bool) cp->def);
                break;
            case C_STR:
                strcpy((char*)cp->ptr, ReadString (bbrc, key, (char*) cp->def));
                break;
        }
    } while ((++cp)->key);
}

static bool Settings_WriteSetting(const char *bbrc, struct rccfg * cp, const void *v)
{
    do if (NULL == v || cp->ptr == v)
    {
        char keystr[80]; const char *key = makekey(keystr, cp);
        switch (cp->mode)
        {
            case C_INT:
                WriteInt (bbrc, key, *(int*) cp->ptr);
                break;
            case C_BOL:
                WriteBool (bbrc, key, *(bool*) cp->ptr);
                break;
            case C_STR:
                WriteString (bbrc, key, (char*) cp->ptr);
                break;
        }
        if (v) return true;
    } while ((++cp)->key);
    return false;
}

//===========================================================================

//===========================================================================
void Settings_WriteRCSetting(const void *v)
{
    Settings_WriteSetting(bbrcPath(), rccfg, v)
    ||
    Settings_WriteSetting(extensionsrcPath(), ext_rccfg, v);
}

void Settings_ReadRCSettings()
{
    Settings_ReadSettings(bbrcPath(), rccfg);
    Settings_ReadSettings(extensionsrcPath(), ext_rccfg);
    menuPath(ReadString(bbrcPath(), "session.menuFile:", NULL));
    stylePath(ReadString(bbrcPath(), "session.styleFile:", ""));
}

//===========================================================================

void Settings_ReadStyleSettings()
{
    ReadStyle(stylePath());

#if 1
    // ----------------------------------------------------
    // set some defaults for missing style settings

    if (mStyle.Toolbar.validated & VALID_TEXTCOLOR)
    {
        if (0==(mStyle.ToolbarLabel.validated & VALID_TEXTCOLOR))
		{
			mStyle.ToolbarLabel.TextColor   = mStyle.Toolbar.TextColor;
			/*mStyle.ToolbarLabel.ShadowX     = mStyle.Toolbar.ShadowX;
			mStyle.ToolbarLabel.ShadowY     = mStyle.Toolbar.ShadowY;
			mStyle.ToolbarLabel.ShadowColor = mStyle.Toolbar.ShadowColor;*/
		}

		if (0==(mStyle.ToolbarClock.validated & VALID_TEXTCOLOR))
		{
			mStyle.ToolbarClock.TextColor   = mStyle.Toolbar.TextColor;
			/*mStyle.ToolbarClock.ShadowX     = mStyle.Toolbar.ShadowX;
			mStyle.ToolbarClock.ShadowY     = mStyle.Toolbar.ShadowY;
			mStyle.ToolbarClock.ShadowColor = mStyle.Toolbar.ShadowColor;*/
		}

		if (0==(mStyle.ToolbarWindowLabel.validated & VALID_TEXTCOLOR))
		{
			mStyle.ToolbarWindowLabel.TextColor   = mStyle.Toolbar.TextColor;
			/*mStyle.ToolbarWindowLabel.ShadowX     = mStyle.Toolbar.ShadowX;
			mStyle.ToolbarWindowLabel.ShadowY     = mStyle.Toolbar.ShadowY;
			mStyle.ToolbarWindowLabel.ShadowColor = mStyle.Toolbar.ShadowColor;*/
		}
	}
	else
	{
		if (mStyle.ToolbarLabel.parentRelative)
		{
			mStyle.Toolbar.TextColor   = mStyle.ToolbarLabel.TextColor;
			/*mStyle.Toolbar.ShadowX     = mStyle.ToolbarLabel.ShadowX;
			mStyle.Toolbar.ShadowY     = mStyle.ToolbarLabel.ShadowY;
			mStyle.Toolbar.ShadowColor = mStyle.ToolbarLabel.ShadowColor;*/
		}
		else
		if (mStyle.ToolbarClock.parentRelative)
		{
			mStyle.Toolbar.TextColor   = mStyle.ToolbarClock.TextColor;
			/*mStyle.Toolbar.ShadowX     = mStyle.ToolbarClock.ShadowX;
			mStyle.Toolbar.ShadowY     = mStyle.ToolbarClock.ShadowY;
			mStyle.Toolbar.ShadowColor = mStyle.ToolbarClock.ShadowColor;*/
		}
		else
		if (mStyle.ToolbarWindowLabel.parentRelative)
		{
			mStyle.Toolbar.TextColor   = mStyle.ToolbarWindowLabel.TextColor;
			/*mStyle.Toolbar.ShadowX     = mStyle.ToolbarWindowLabel.ShadowX;
			mStyle.Toolbar.ShadowY     = mStyle.ToolbarWindowLabel.ShadowY;
			mStyle.Toolbar.ShadowColor = mStyle.ToolbarWindowLabel.ShadowColor;*/
		}
		else
		{
			mStyle.Toolbar.TextColor   = mStyle.ToolbarLabel.TextColor;
			/*mStyle.Toolbar.ShadowX     = mStyle.ToolbarLabel.ShadowX;
			mStyle.Toolbar.ShadowY     = mStyle.ToolbarLabel.ShadowY;
			mStyle.Toolbar.ShadowColor = mStyle.ToolbarLabel.ShadowColor;*/
		}
    }

    if (0==(mStyle.ToolbarButtonPressed.validated & VALID_PICCOLOR))
        mStyle.ToolbarButtonPressed.picColor = mStyle.ToolbarButton.picColor;

    if (0==(mStyle.Toolbar.validated & VALID_JUSTIFY))
        mStyle.Toolbar.Justify = DT_CENTER;

    if (0==(mStyle.ToolbarWindowLabel.validated & VALID_TEXTURE))
        if (mStyle.ToolbarLabel.validated & VALID_TEXTURE)
            mStyle.ToolbarWindowLabel = mStyle.ToolbarLabel;

    if (0 == (mStyle.MenuFrame.validated & VALID_TEXTURE)
     && 0 == mStyle.rootCommand[0])
        strcpy(mStyle.rootCommand, "bsetroot -mod 4 4 -fg grey55 -bg grey60");
#endif

    // default SplitColor
    struct items *p = StyleItems;
    do{
        StyleItem* pSI = (StyleItem*)(p->v);
        bool is_split = (pSI->type == B_SPLITVERTICAL) || (pSI->type == B_SPLITHORIZONTAL);
        if (p->flags & VALID_FROMSPLITTO){
            if(is_split && !(pSI->validated & VALID_FROMSPLITTO)){
                unsigned int r = GetRValue(pSI->Color);
                unsigned int g = GetGValue(pSI->Color);
                unsigned int b = GetBValue(pSI->Color);
                r = iminmax(r + (r>>2), 0, 255);
                g = iminmax(g + (g>>2), 0, 255);
                b = iminmax(b + (b>>2), 0, 255);
                pSI->ColorSplitTo = RGB(r, g, b);
                pSI->validated |= VALID_FROMSPLITTO;
            }
        }
        if (p->flags & VALID_TOSPLITTO){
            if(is_split && !(pSI->validated & VALID_TOSPLITTO)){
                unsigned int r = GetRValue(pSI->ColorTo);
                unsigned int g = GetGValue(pSI->ColorTo);
                unsigned int b = GetBValue(pSI->ColorTo);
                r = iminmax(r + (r>>4), 0, 255);
                g = iminmax(g + (g>>4), 0, 255);
                b = iminmax(b + (b>>4), 0, 255);
                pSI->ColorToSplitTo = RGB(r, g, b);
                pSI->validated |= VALID_TOSPLITTO;
            }
        }
    } while ((++p)->v);
}

//===========================================================================
#endif // #ifndef BBSETTING_STYLEREADER_ONLY
//===========================================================================
