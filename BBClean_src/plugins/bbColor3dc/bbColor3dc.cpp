/*============================================================================

  bbColor3dc is a plugin for Blackbox for Windows
  copyright © 2004 grischka

  grischka@users.sourceforge.net
  http://bb4win.sourceforge.net/bblean

  bbColor3dc is free software, released under the GNU General Public License
  (GPL version 2 or later) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

*/

#include "BBApi.h"
#include <stdlib.h>

// new in BBApi.h
#define VALID_TEXTURE       (1<<0)  // gradient definition
#define VALID_COLORFROM     (1<<1)  // Color
#define VALID_COLORTO       (1<<2)  // ColorTo
#define VALID_TEXTCOLOR     (1<<3)  // TextColor
#define VALID_PICCOLOR      (1<<4)  // PicColor

//============================================================================
// info

char szVersion     [] = "bbColor3dc 1.1";
char szAppName     [] = "bbColor3dc";
char szInfoVersion [] = "1.1";
char szInfoAuthor  [] = "grischka";
char szInfoRelDate [] = "2004-08-20";
char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
char szInfoEmail   [] = "grischka@users.sourceforge.net";

//===========================================================================
// global variables

HWND hPluginWnd;
HWND BBhwnd;
HINSTANCE hInstance;
void *(*pGetSettingPtr)(int);
bool is_bblean;

// declarations
bool in_colors_changing;

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))

void syscolor_apply(void);
void syscolor_restore(void);
void syscolor_backup(void);

void syscolor_setcolor(const char * colorspec);
void syscolor_getcolor(const char * colorspec);
void syscolor_write(const char *filename);
void syscolor_read(const char *filename);

int n_stricmp(const char **pp, const char *s);

//============================================================================
// plugin interface

extern "C"
{
    DLL_EXPORT int beginPlugin(HINSTANCE);
    DLL_EXPORT void endPlugin(HINSTANCE);
    DLL_EXPORT LPCSTR pluginInfo(int);
	DLL_EXPORT LPCTSTR stylePath(LPCSTR);
};

//============================================
int beginPlugin(HINSTANCE h_instance)
{
    LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    hInstance = h_instance;
    BBhwnd = GetBBWnd();
    is_bblean = 0 == memicmp(GetBBVersion(), "bblean", 6);

    if (is_bblean)
        *(FARPROC*)&pGetSettingPtr =
            GetProcAddress(GetModuleHandle(NULL), "GetSettingPtr");

    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = MsgWndProc;
    wc.hInstance = h_instance;
    wc.lpszClassName = szAppName;

    if (FALSE == RegisterClass(&wc) || NULL == CreateWindowEx(
        WS_EX_TOOLWINDOW,
        szAppName,
        NULL,
        WS_POPUP,
        0, 0, 0, 0,
        NULL, //HWND_MESSAGE,
        NULL,
        h_instance,
        NULL
        ))
        return 1;

    return 0;
}

//============================================
void endPlugin(HINSTANCE h_instance)
{
    DestroyWindow(hPluginWnd);
    UnregisterClass(szAppName, hInstance);
}

//============================================
LPCSTR pluginInfo(int field)
{
    static char *infostr[7] =
    {
        szVersion       ,
        szAppName       ,
        szInfoVersion   ,
        szInfoAuthor    ,
        szInfoRelDate   ,
        szInfoLink      ,
        szInfoEmail
    };
    return (field >= 0 && field < 7) ? infostr[field] : "";
}

//===========================================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int bb_msgs[] = { BB_RECONFIGURE, BB_EXITTYPE, BB_BROADCAST, 0 };

    switch(msg)
    {
        case WM_CREATE:
            hPluginWnd = hwnd;
            SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)bb_msgs);
            syscolor_backup();
            syscolor_apply();
            break;

        case BB_RECONFIGURE:
            syscolor_apply();
            break;

        case WM_DESTROY:
            SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)bb_msgs);
            syscolor_restore();
            break;

        case BB_EXITTYPE:
            if (B_SHUTDOWN == wParam)
                syscolor_restore();
            break;

        case BB_BROADCAST:
            if (0 == memicmp((LPCSTR)lParam, "@BBColor3dc.", 12))
            {
                const char *msg_string = (LPCSTR)lParam + 12;
                if (0 == n_stricmp(&msg_string, "Read"))
                {
                    syscolor_read(msg_string);
                    break;
                }
                if (0 == n_stricmp(&msg_string, "Keep"))
                {
                    syscolor_backup();
                    break;
                }
                if (0 == n_stricmp(&msg_string, "Write"))
                {
                    syscolor_write(msg_string);
                    break;
                }
                if (0 == stricmp(msg_string, "Clear"))
                {
                    syscolor_restore();
                    break;
                }
                if (0 == n_stricmp(&msg_string, "SetColor"))
                {
                    syscolor_setcolor(msg_string);
                    break;
                }
                if (0 == n_stricmp(&msg_string, "GetColor"))
                {
                    syscolor_getcolor(msg_string);
                    break;
                }
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

//===========================================================================
void ReadStyleItem(const char * style, StyleItem *si, const char *rckey)
{
    char buffer[256], temp[256];
    const char *p;
    char *subkey = buffer + strlen(strcpy(buffer, rckey));

    ZeroMemory(si, sizeof *si);

    strcpy(subkey, ":"            );
    p = ReadString(style, buffer, "");
    if (p) ParseItem(strcpy(temp, p), si), si->validated |= VALID_TEXTURE;

    p = "#000000";

    strcpy(subkey, ".color:"      );
    si->Color       = ReadColor(style, buffer, p);

    strcpy(subkey, ".colorTo:"    );
    si->ColorTo     = ReadColor(style, buffer, p);

    strcpy(subkey, ".textColor:"  );
    si->TextColor   = ReadColor(style, buffer, p);
}

void *xGetSettingPtr(int id)
{
    if (pGetSettingPtr)
        return pGetSettingPtr(id);

    const char *style = stylePath();
    const char *key;
    static StyleItem SI;
    static DWORD dw_result;
    switch (id)
    {
        case SN_MENUFRAME           : key = "menu.frame"; break;
        case SN_MENUTITLE           : key = "menu.title"; break;
        case SN_MENUHILITE          : key = "menu.hilite"; break;
        case SN_WINFOCUS_TITLE      : key = "window.title.focus"; break;
        case SN_WINFOCUS_LABEL      : key = "window.label.focus"; break;
        case SN_WINUNFOCUS_TITLE    : key = "window.title.unfocus"; break;
        case SN_WINUNFOCUS_LABEL    : key = "window.label.unfocus"; break;
        case SN_BORDERWIDTH         : dw_result = ReadInt(style, "borderWidth:", 0);
            return &dw_result;
        case SN_BORDERCOLOR         : dw_result = ReadColor(style, "borderColor:", "black");
            return &dw_result;

        default: return NULL;
    }
    ReadStyleItem(style, &SI, key);
    return &SI;
}

//===========================================================================
struct colors_style { int id; COLORREF cr; };

COLORREF mixcolors(COLORREF c1, COLORREF c2)
{
    return RGB(
        (GetRValue(c1)+GetRValue(c2))/2,
        (GetGValue(c1)+GetGValue(c2))/2,
        (GetBValue(c1)+GetBValue(c2))/2
        );
}

void set_gradient_colors(struct colors_style *cs, StyleItem *s1, StyleItem *s2)
{
    if (s1->validated & VALID_TEXTURE)
    {
        StyleItem *s3 = s1->parentRelative ? s2 : s1;
        cs[0].cr = s3->Color;
        cs[1].cr = s3->type == B_SOLID ? s3->Color : s3->ColorTo;
        cs[2].cr = s1->TextColor;
    }
}

void set_flat_colors(struct colors_style *cs, StyleItem *s1)
{
    if (s1->validated & VALID_TEXTURE)
    {
        cs[0].cr = s1->type == B_SOLID ? s1->Color : mixcolors(s1->Color, s1->ColorTo);
        cs[1].cr = s1->TextColor;
    }
}

void get_style_colors (COLORREF *cr_buffer)
{
    StyleItem menu_frame            = *(StyleItem*)xGetSettingPtr(SN_MENUFRAME        );  
    StyleItem menu_title            = *(StyleItem*)xGetSettingPtr(SN_MENUTITLE        );  
    StyleItem menu_hilite           = *(StyleItem*)xGetSettingPtr(SN_MENUHILITE       );  
    StyleItem window_focus_title    = *(StyleItem*)xGetSettingPtr(SN_WINFOCUS_TITLE   );  
    StyleItem window_focus_label    = *(StyleItem*)xGetSettingPtr(SN_WINFOCUS_LABEL   );  
    StyleItem window_unfocus_title  = *(StyleItem*)xGetSettingPtr(SN_WINUNFOCUS_TITLE );  
    StyleItem window_unfocus_label  = *(StyleItem*)xGetSettingPtr(SN_WINUNFOCUS_LABEL );  
    COLORREF border_color           = *(COLORREF*) xGetSettingPtr(SN_BORDERCOLOR      );
    int border_width                = *(int*)      xGetSettingPtr(SN_BORDERWIDTH      );

    static struct colors_style colors_style [] =
    {
        { COLOR_ACTIVECAPTION           , 0 },  // 0
        { COLOR_GRADIENTACTIVECAPTION   , 0 },  
        { COLOR_CAPTIONTEXT             , 0 },  

        { COLOR_INACTIVECAPTION         , 0 },  // 3
        { COLOR_GRADIENTINACTIVECAPTION , 0 },  
        { COLOR_INACTIVECAPTIONTEXT     , 0 },  

        { COLOR_MENU                    , 0 },  // 6
        { COLOR_MENUTEXT                , 0 },  

        { COLOR_HIGHLIGHT               , 0 },  // 8
        { COLOR_HIGHLIGHTTEXT           , 0 },

        { COLOR_INFOBK                  , 0 },  // 10
        { COLOR_INFOTEXT                , 0 },

        { COLOR_ACTIVEBORDER            , 0 },  // 12
        { COLOR_INACTIVEBORDER          , 0 },
        { COLOR_WINDOWFRAME             , 0 }
    };

    const int NSTYLES = array_count(colors_style);

    // -----------------------------------
    int i;
    for (i = 0; i < NSTYLES; i++)
        colors_style[i].cr = (COLORREF)-1;

    set_gradient_colors(colors_style+0, &window_focus_label, &window_focus_title);
    set_gradient_colors(colors_style+3, &window_unfocus_label, &window_unfocus_title);


    //set_flat_colors(colors_style+6, &menu_frame);
    set_flat_colors(colors_style+8, &menu_hilite);
    set_flat_colors(colors_style+10, &menu_frame);

    if (border_width)
    {
        //colors_style[12].cr =
        //colors_style[13].cr =
        colors_style[14].cr = border_color;
    }

    // -----------------------------------
    for (i = 0; i < NSTYLES; i++)
        if ((COLORREF)-1 != colors_style[i].cr)
            cr_buffer[colors_style[i].id] = colors_style[i].cr;
}

//===========================================================================
#define COLOR_3DFACEALT 25 //?? the only missing index ...

static char color_trans_3dcc [] =
{
    COLOR_3DDKSHADOW                , // 21
    COLOR_3DSHADOW                  , // 16
    COLOR_3DFACE                    , // 15     nonclient surfaces
    COLOR_3DFACEALT                 , // 25     ??
    COLOR_3DLIGHT                   , // 22
    COLOR_3DHIGHLIGHT               , // 20

    COLOR_ACTIVECAPTION             , //  2     active caption
    COLOR_GRADIENTACTIVECAPTION     , // 27
    COLOR_CAPTIONTEXT               , //  9
    COLOR_ACTIVEBORDER              , // 10

    COLOR_APPWORKSPACE              , // 12
    COLOR_BTNTEXT                   , // 18
    COLOR_DESKTOP                   , //  1
    COLOR_WINDOWFRAME               , //  6     tooltip border
    COLOR_GRAYTEXT                  , // 17

    COLOR_INACTIVECAPTION           , //  3     in-active caption
    COLOR_GRADIENTINACTIVECAPTION   , // 28
    COLOR_INACTIVECAPTIONTEXT       , // 19
    COLOR_INACTIVEBORDER            , // 11

    COLOR_MENU                      , //  4     menu
    COLOR_MENUTEXT                  , //  7

    COLOR_HOTLIGHT                  , // 26
    COLOR_SCROLLBAR                 , //  0

    COLOR_HIGHLIGHT                 , // 13     hilited items (menu, list control etc...)
    COLOR_HIGHLIGHTTEXT             , // 14

    COLOR_INFOBK                    , // 24     tooltip
    COLOR_INFOTEXT                  , // 23

    COLOR_WINDOW                    , //  5
    COLOR_WINDOWTEXT                  //  8
};

const int NCOLORS = array_count(color_trans_3dcc);

//===========================================================================

bool parse_3dc(FILE *fp, COLORREF *cr_buffer)
{
    char *trans = color_trans_3dcc; int i = 0; char line[256];
    while (fgets(line, sizeof line, fp))
    {
        // trim trailing stuff
        char *se = strchr(line, '\0');
        while (se > line && (unsigned char)se[-1] <= 32) se--;
        *se = 0;

        if (false == (line[0]>='0' && line[0]<='9'))
            break;

        cr_buffer[*trans] = atoi(line);

        if (++i == NCOLORS)
            break;

        ++trans;
    }
    fclose(fp);
    return i >= NCOLORS-5;
}


//===========================================================================
// utilities

COLORREF rgb (unsigned r,unsigned g,unsigned b)
{
    return RGB(r,g,b);
}

COLORREF switch_rgb (COLORREF c)
{
    return (c&0x0000ff)<<16 | (c&0x00ff00) | (c&0xff0000)>>16;
}

int n_stricmp(const char **pp, const char *s)
{
    int n = strlen (s);
    int i = memicmp(*pp, s, n);
    if (i) return i;
    i = (*pp)[n] - ' ';
    if (i > 0) return i;
    *pp += n;
    while (' '== **pp) ++*pp;
    return 0;
}

char* make_full_path(char *buffer, const char *filename)
{
    buffer[0] = 0;
    if (NULL == strchr(filename, ':')) GetBlackboxPath(buffer, MAX_PATH);
    return strcat(buffer, filename);
}

//===========================================================================
// try to read it from the style itself;
// search for a commentline containing "3dc" and "start" or the stylename
// example: "! --- file.3dc - start ---"

bool read_from_style(const char *filename_3dc, COLORREF *cr_buffer)
{
    bool result = false;
    FILE *fp = fopen(stylePath(), "rb");
    if (fp)
    {
        char line[1000];
        while (fgets(line, sizeof line, fp))
        {
            if (('!' == *line || '#' == *line)
                && strstr(strlwr(line), "3dc")
                && (strstr(line, filename_3dc) || strstr(line, "start"))
                && !strstr(line, "rootcommand:")

                && parse_3dc(fp, cr_buffer)
                )
            {
                //dbg_printf("found 3dc in style %s", stylePath());
                result = true;
                break;
            }
        }
        fclose(fp);
    }
    return result;
}

//===========================================================================
// read from separate .3DC file

bool read_from_file(const char *path, const char *filename_3dc, COLORREF *cr_buffer)
{
    char buffer[MAX_PATH];
    buffer[0] = 0;
    if (path && *path) strcat(strcpy(buffer, path), "\\");
    strcat(buffer, filename_3dc);

    bool result = false;

    char full_path[MAX_PATH];
    FILE *fp = fopen(make_full_path(full_path, buffer), "rb");
    //dbg_printf("open %s %s", fp?"success":"failed", full_path);
    if (fp) { result = parse_3dc(fp, cr_buffer); fclose(fp); }
    return result;
}

//===========================================================================

bool read_3dc(COLORREF *cr_buffer)
{
    char style_path[MAX_PATH];
    char filename_3dc[MAX_PATH];

    strlwr(strcpy(style_path, stylePath()));

    char *dot = strrchr(style_path, '.');
    char *bsl = strrchr(style_path, '\\');
    char *fsl = strrchr(style_path, '/');
    char *eos = strchr(style_path, '\0');
    // use '/' when found and more to the right
    if (fsl > bsl) bsl = fsl;
    // when no extension was specified...
    if (dot <= bsl) dot = eos;

    // replace/add extension
    strcpy(dot, ".3dc");

    if (bsl)
    {
        strcpy(filename_3dc, bsl+1);
        *bsl = 0;
    }
    else
    {
        strcpy(filename_3dc, style_path);
        style_path[0] = 0;
    }

    if (read_from_style(filename_3dc, cr_buffer))
        return true;

    if (read_from_file(style_path, filename_3dc, cr_buffer))
        return true;

    strcat(style_path, "\\3dc");

    if (read_from_file(style_path, filename_3dc, cr_buffer))
        return true;

    if (read_from_file("3dc", filename_3dc, cr_buffer))
        return true;


    return false;
}

//===========================================================================
static COLORREF cr_original[NCOLORS];
static int cr_ids[NCOLORS];
static int colors_changed;

void syscolor_backup(void)
{
    int n = 0;
    do cr_original[cr_ids[n] = n] = GetSysColor(n); while (++n<NCOLORS);
    colors_changed = 0;
}

void syscolor_restore(void)
{
    if (0 == colors_changed) return;

    COLORREF cr_buffer[NCOLORS];
    memmove(cr_buffer, cr_original, sizeof cr_buffer);
    in_colors_changing = true;
    SetSysColors(NCOLORS, cr_ids, cr_buffer);
    in_colors_changing = false;
    colors_changed = 0;
}

void syscolor_apply(void)
{
    if (3 == colors_changed) return;

    COLORREF cr_buffer[NCOLORS];

    memmove(cr_buffer, cr_original, sizeof cr_buffer);
    if (read_3dc(cr_buffer)) // set colors from 3DCC file
    {
        colors_changed = 2;
    }
    else
    {
        memmove(cr_buffer, cr_original, sizeof cr_buffer);
        get_style_colors(cr_buffer);  // set colors from style
        colors_changed = 1;
    }
    in_colors_changing = true;
    SetSysColors(NCOLORS, cr_ids, cr_buffer);
    in_colors_changing = false;
}

//===========================================================================

const char *color_item_strings[] =
{
    "3DDKSHADOW",
    "3DSHADOW",
    "3DFACE",
    "3DFACEALT",
    "3DLIGHT",
    "3DHIGHLIGHT",

    "ACTIVECAPTION",
    "GRADIENTACTIVECAPTION",
    "CAPTIONTEXT",
    "ACTIVEBORDER",

    "APPWORKSPACE",
    "BTNTEXT",
    "DESKTOP",
    "WINDOWFRAME",
    "GRAYTEXT",

    "INACTIVECAPTION",
    "GRADIENTINACTIVECAPTION",
    "INACTIVECAPTIONTEXT",
    "INACTIVEBORDER",

    "MENU",
    "MENUTEXT",

    "HOTLIGHT",
    "SCROLLBAR",

    "HIGHLIGHT",
    "HIGHLIGHTTEXT",

    "INFOBK",
    "INFOTEXT",

    "WINDOW",
    "WINDOWTEXT",

    NULL
};


//===========================================================================

void syscolor_setcolor(const char * p)
{
    //dbg_printf("color: %s", p);
    const char **cs = color_item_strings; int n = 0;
    do { if (0 == n_stricmp(&p, *cs)) break; ++n; } while (*++cs);
    if (NULL == *cs) return;

    COLORREF c;
    if (0 == sscanf(p, "#%x", &c)) return;
    c = switch_rgb(c);

    in_colors_changing = true;
    SetSysColors(1, &cr_ids[color_trans_3dcc[n]], &c);
    in_colors_changing = false;
    colors_changed = 2;
}

void syscolor_getcolor(const char * p)
{
    const char **cs = color_item_strings; int n = 0;
    do { if (0 == n_stricmp(&p, *cs)) break; ++n; } while (*++cs);

    if (NULL == *cs || '@' != *p) return;

    char color[40];
    sprintf(color, "#%06x", switch_rgb(GetSysColor(color_trans_3dcc[n])));

    char buffer[256];
    sprintf(buffer, p, color);

    SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)&buffer);
}

void syscolor_write(const char *filename)
{
    char path_to_3dc[MAX_PATH];
    FILE *fp = fopen(make_full_path(path_to_3dc, filename), "wt");
    if (fp)
    {
        int n = 0;
        do {
            COLORREF c = GetSysColor(color_trans_3dcc[n]);
            fprintf(fp, "%d\n", c);
        } while (++n<NCOLORS);
        fclose(fp);
    }
}

void syscolor_read(const char *filename_3dc)
{
    COLORREF cr_buffer[NCOLORS];
    memmove(cr_buffer, cr_original, sizeof cr_buffer);

    if (false == read_from_file(NULL, filename_3dc, cr_buffer))
        return;

    colors_changed = 3;

    in_colors_changing = true;
    SetSysColors(NCOLORS, cr_ids, cr_buffer);
    in_colors_changing = false;
}

//===========================================================================
