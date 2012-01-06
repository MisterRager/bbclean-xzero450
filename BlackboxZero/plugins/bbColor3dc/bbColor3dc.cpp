/*============================================================================

  bbColor3dc is a plugin for Blackbox for Windows
  copyright © 2004-2009 grischka

  grischka@users.sourceforge.net
  http://bb4win.sourceforge.net/bblean

  bbColor3dc is free software, released under the GNU General Public License
  (GPL version 2) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

*/

#include "BBApi.h"
#include "bblib.h"
#include "bbversion.h"
#include <stdlib.h>

#define VALID_TEXTURE       1 // gradient definition
#define VALID_COLORFROM     2 // Color
#define VALID_COLORTO       4 // ColorTo
#define VALID_TEXTCOLOR     8 // TextColor
#define VALID_PICCOLOR     16 // PicColor

//============================================================================
// info

const char szVersion     [] = "bbColor3dc 1.3";
const char szAppName     [] = "bbColor3dc";
const char szInfoVersion [] = "1.3";
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = BBLEAN_RELDATE;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";
const char szCopyright   [] = "2004-2009";

//===========================================================================
// global variables

HWND hPluginWnd;
HWND BBhwnd;
HINSTANCE hInstance;
void *(*pGetSettingPtr)(int);
void *mGetSettingPtr(int);
bool is_bblean;
char filename_3dc[MAX_PATH];

// declarations
void syscolor_apply(void);
void syscolor_restore(void);
void syscolor_backup(void);

void syscolor_setcolor(const char * colorspec);
void syscolor_getcolor(const char * colorspec);
void syscolor_write(const char *filename);
void syscolor_read(const char *filename);
int syscolor_handle_bbi_message(const char * p);

int n_stricmp(const char **pp, const char *s);

//============================================================================
// plugin interface

extern "C"
{
    DLL_EXPORT int beginPlugin(HINSTANCE);
    DLL_EXPORT void endPlugin(HINSTANCE);
    DLL_EXPORT LPCSTR pluginInfo(int);
};

//============================================
int beginPlugin(HINSTANCE h_instance)
{
    LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    if (BBhwnd)
    {
        MessageBox(NULL, "Dont load me twice!", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
        return 1;
    }

    hInstance = h_instance;
    BBhwnd = GetBBWnd();
    is_bblean = 0 == memicmp(GetBBVersion(), "bblean", 6);

    if (is_bblean)
        *(FARPROC*)&pGetSettingPtr =
            GetProcAddress(GetModuleHandle(NULL), "GetSettingPtr");

    if (NULL == pGetSettingPtr)
        pGetSettingPtr = mGetSettingPtr;

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

LPCSTR pluginInfo(int field)
{
    switch (field)
    {
        default:
        case 0: return szVersion;
        case 1: return szAppName;
        case 2: return szInfoVersion;
        case 3: return szInfoAuthor;
        case 4: return szInfoRelDate;
        case 5: return szInfoLink;
        case 6: return szInfoEmail;
    }
}

//===========================================================================

int set_param(WPARAM wParam, LPARAM lParam, const char *p, const char *arg)
{
    int l = p - (LPCSTR)lParam;
    memcpy((char *)wParam, (LPCSTR)lParam, l);
    strcpy((char *)wParam + l, arg);
    return 0;
}

//===========================================================================
LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int bb_msgs[] = { BB_RECONFIGURE, BB_EXITTYPE, BB_BROADCAST, BB_GETBOOL, 0 };

    switch(msg)
    {
        case WM_CREATE:
            hPluginWnd = hwnd;
            SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)bb_msgs);
            syscolor_backup();
            syscolor_apply();
            break;

        case BB_RECONFIGURE:
            //dbg_printf("BB_RECONFIGURE");
            filename_3dc[0] = 0;
            syscolor_apply();
            break;

        case WM_SYSCOLORCHANGE:
            //dbg_printf("WM_SYSCOLORCHANGE");
            break;

        case WM_DESTROY:
            SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)bb_msgs);
            syscolor_restore();
            break;

        case BB_EXITTYPE:
            if (0 == wParam)
                syscolor_restore();
            break;

        case BB_GETBOOL:
            if (0 == memicmp((LPCSTR)lParam, "@BBColor3dc.", 12))
            {
                const char *msg_string = (LPCSTR)lParam + 12;
                if (0 == n_stricmp(&msg_string, "Read"))
                {
                    set_param(wParam, lParam, msg_string, filename_3dc);
                    return 1;
                }
            }
            break;

        case BB_BROADCAST:
            if (0 == memicmp((LPCSTR)lParam, "@BBColor3dc.", 12))
            {
                const char *msg_string = (LPCSTR)lParam + 12;
                if (0 == n_stricmp(&msg_string, "Read"))
                {
                    strcpy(filename_3dc, msg_string);
                    syscolor_read(msg_string);
                    PostMessage(BBhwnd, BB_MENU, BB_MENU_UPDATE, 0);
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

                if (syscolor_handle_bbi_message(msg_string))
                {
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
    if (p)
    {
        ParseItem(strcpy(temp, p), si);
        si->validated |= VALID_TEXTURE;
    }

    p = "#000000";

    strcpy(subkey, ".color:"      );
    si->Color       = ReadColor(style, buffer, p);

    strcpy(subkey, ".colorTo:"    );
    si->ColorTo     = ReadColor(style, buffer, p);

    strcpy(subkey, ".textColor:"  );
    si->TextColor   = ReadColor(style, buffer, p);
}

void *mGetSettingPtr(int id)
{
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
        case SN_WINFOCUS_HANDLE     : key = "window.handle.focus"; break;
        case SN_WINFOCUS_GRIP       : key = "window.grip.focus"; break;
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
#define COLOR_3DFACEALT 25 //?? the only missing index ...

#ifndef COLOR_HOTLIGHT
#define COLOR_HOTLIGHT 26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28
#endif

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

static COLORREF cr_original[NCOLORS];
static int cr_ids[NCOLORS];
static int colors_changed;

//===========================================================================
struct colors_style { int id; COLORREF cr; };

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

COLORREF get_flat_color(StyleItem *s)
{
    return s->type == B_SOLID ? s->Color : mixcolors(s->Color, s->ColorTo, 128);
}

void set_flat_color(struct colors_style *cs, StyleItem *s1)
{
    if (s1->validated & VALID_TEXTURE)
    {
        cs[0].cr = get_flat_color(s1);
        cs[1].cr = s1->TextColor;
    }
}

void get_style_colors (COLORREF *cr_buffer)
{
//    StyleItem menu_title            = *(StyleItem*)pGetSettingPtr(SN_MENUTITLE        );  
    StyleItem menu_frame            = *(StyleItem*)pGetSettingPtr(SN_MENUFRAME        );  
    StyleItem menu_hilite           = *(StyleItem*)pGetSettingPtr(SN_MENUHILITE       );  
    StyleItem window_focus_title    = *(StyleItem*)pGetSettingPtr(SN_WINFOCUS_TITLE   );  
    StyleItem window_focus_label    = *(StyleItem*)pGetSettingPtr(SN_WINFOCUS_LABEL   );  
    StyleItem window_unfocus_title  = *(StyleItem*)pGetSettingPtr(SN_WINUNFOCUS_TITLE );  
    StyleItem window_unfocus_label  = *(StyleItem*)pGetSettingPtr(SN_WINUNFOCUS_LABEL );  
//    StyleItem window_focus_handle   = *(StyleItem*)pGetSettingPtr(SN_WINFOCUS_HANDLE  );
//    StyleItem window_focus_grip     = *(StyleItem*)pGetSettingPtr(SN_WINFOCUS_GRIP    );

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
        { COLOR_WINDOWFRAME             , 0 },

        { COLOR_3DDKSHADOW              , 0 },  // 15
        { COLOR_3DSHADOW                , 0 },  // 16
        { COLOR_3DFACE                  , 0 },  // 17
        { COLOR_3DLIGHT                 , 0 },  // 18
        { COLOR_3DHIGHLIGHT             , 0 }   // 19
    };

    const int NSTYLES = array_count(colors_style);

    // -----------------------------------
    int i;
    for (i = 0; i < NSTYLES; i++)
        colors_style[i].cr = (COLORREF)-1;

    set_gradient_colors(colors_style+0, &window_focus_label, &window_focus_title);
    set_gradient_colors(colors_style+3, &window_unfocus_label, &window_unfocus_title);

    //set_flat_color(colors_style+8, &menu_hilite);
    set_flat_color(colors_style+10, &menu_frame);

    if (menu_frame.borderWidth) {
        colors_style[14].cr = menu_frame.borderColor;
    }

#if 0
    set_flat_colors(colors_style+6, &menu_frame);
    colors_style[15].cr = shade_color(colors_style[6].cr, -60);
    colors_style[16].cr = shade_color(colors_style[6].cr, -30);
    colors_style[17].cr = shade_color(colors_style[6].cr,   0);
    colors_style[18].cr = shade_color(colors_style[6].cr,  40);
    colors_style[19].cr = shade_color(colors_style[6].cr,  80);

    COLORREF c1 = get_flat_color(&window_focus_handle);
    COLORREF c2 = window_focus_grip.parentRelative ? c1 : get_flat_color(&window_focus_grip);
    colors_style[15].cr = shade_color(c1, -40);
    colors_style[16].cr = shade_color(c1, -20);
    colors_style[17].cr = shade_color(c1,   0);
    colors_style[18].cr = shade_color(c2,  20);
    colors_style[19].cr = shade_color(c2,  40);
#endif

    // -----------------------------------
    for (i = 0; i < NSTYLES; i++)
        if ((COLORREF)-1 != colors_style[i].cr)
        {
            for (int n = 0; n < NCOLORS; ++n)
                if (cr_ids[n] == colors_style[i].id)
                {
                    cr_buffer[n] = colors_style[i].cr;
                    break;
                }
        }
}

//===========================================================================

bool parse_3dc(FILE *fp, COLORREF *cr_buffer)
{
    int i = 0, n = 0; char line[256];
    while (fgets(line, sizeof line, fp))
    {
        // trim trailing stuff
        char *se = strchr(line, '\0');
        while (se > line && (unsigned char)se[-1] <= 32) se--;
        *se = 0;

        se = line;
        if ('!' == *se || '#' == *se) while (' ' == *++se);
        if (false == (*se>='0' && *se<='9'))
        {
            if (n) break;
            continue;
        }

        cr_buffer[i] = atoi(se);
#if 0
        if (COLOR_DESKTOP == color_trans_3dcc[n])
            cr_buffer[i] = cr_original[i];
#endif
        if (++n == NCOLORS)
            break;
        ++i;
    }
    fclose(fp);
    return n >= NCOLORS-5;
}


//===========================================================================
// utilities

int n_stricmp(const char **pp, const char *s)
{
    int n = strlen (s);
    int i = memicmp(*pp, s, n);
    if (i)
        return i;
    i = (*pp)[n] - ' ';
    if (i > 0)
        return i;
    *pp += n;
    while (' '== **pp)
        ++*pp;
    return 0;
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
    FILE *fp = fopen(set_my_path(NULL, full_path, buffer), "rb");
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

struct color_info {
    HMODULE hDll;
    int cElements;
    int Elements[40];
    COLORREF Values[40];
};

DWORD WINAPI  mSetSysColors_thread(void *pv)
{
    struct color_info *ci = (struct color_info*)pv;
    HMODULE hDll = ci->hDll;
    dbg_printf("enter thread %x", hDll);

    SetSysColors(ci->cElements, ci->Elements, ci->Values);

    free(ci);
    dbg_printf("leave thread %x", hDll);
    FreeLibraryAndExitThread(hDll, 0);
    return 0;
}

BOOL WINAPI mSetSysColors(
    int cElements,  // number of elements to change 
    CONST INT *lpaElements, // address of array of elements 
    CONST COLORREF *lpaRgbValues    // address of array of RGB values  
   ) {

    struct color_info *ci;
    DWORD threadid;
    ci = (struct color_info*)malloc(sizeof *ci);
    ci->cElements = cElements;
    memcpy(ci->Elements, lpaElements, cElements * sizeof lpaElements[0]);
    memcpy(ci->Values, lpaRgbValues, cElements * sizeof lpaRgbValues[0]);

    ci->hDll = LoadLibrary("bbColor3dc.dll");
    CloseHandle(CreateThread(NULL, 0, mSetSysColors_thread, ci, 0, &threadid));
    return TRUE;
}   

//===========================================================================

void syscolor_backup(void)
{
    int n = 0, i = 0;
    do {
        int t = color_trans_3dcc[n];
        cr_ids[i] = t;
        cr_original[i] = GetSysColor(t);
        ++i;
    }
    while (++n<NCOLORS);
    colors_changed = 0;
}

void syscolor_restore(void)
{
    if (0 == colors_changed)
        return;
    COLORREF cr_buffer[NCOLORS];
    memmove(cr_buffer, cr_original, sizeof cr_buffer);
    SetSysColors(NCOLORS, cr_ids, cr_buffer);
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
   SetSysColors(NCOLORS, cr_ids, cr_buffer);
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
void syscolor_getcolor(const char * p)
{
    const char **cs = color_item_strings; int n = 0;
    do { if (0 == n_stricmp(&p, *cs)) break; ++n; } while (*++cs);

    if (NULL == *cs || '@' != *p) return;

    char color[40];
    sprintf(color, "#%06lx", switch_rgb(GetSysColor(color_trans_3dcc[n])));

    char buffer[256];
    sprintf(buffer, p, color);

    SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)&buffer);
}

void syscolor_write(const char *filename)
{
    char path_to_3dc[MAX_PATH];
    FILE *fp = fopen(set_my_path(NULL, path_to_3dc, filename), "wt");
    if (fp)
    {
        int n = 0;
        do {
            COLORREF c = GetSysColor(color_trans_3dcc[n]);
        #if 1
            fprintf(fp, "%lu\n", (unsigned long) c);
        #else
            fprintf(fp, "%-12u ;#%06X ;%s\n",
                c, switch_rgb(c), color_item_strings[n]);
        #endif
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
    SetSysColors(NCOLORS, cr_ids, cr_buffer);
}

void syscolor_setcolor(const char * p)
{
    const char **cs = color_item_strings;
    int n = 0;
    COLORREF c;
    int id;

    do { if (0 == n_stricmp(&p, *cs)) break; ++n; } while (*++cs);
    if (NULL == *cs) return;
    if (0 == sscanf(p, "#%lx", &c)) return;
    c = switch_rgb(c);
    id = color_trans_3dcc[n];
    SetSysColors(1, &id, &c);
    colors_changed = 2;
}

//===========================================================================

//===========================================================================
void set_bbi_slider1 (const char *color, unsigned val)
{
    char buffer[100];
    sprintf(buffer,
        "@BBInterface Control SetControlProperty "
        "3DCSlider_%s Value %d.%03d", color, val/255, val%255*1000/255);
    SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)buffer);
}

void set_bbi_sliders(COLORREF c)
{
    set_bbi_slider1("Red", GetRValue(c));
    set_bbi_slider1("Green", GetGValue(c));
    set_bbi_slider1("Blue", GetBValue(c));
}

void set_bbi_label (int item, COLORREF c)
{
    char buffer[100];
    sprintf(buffer,
        "@BBInterface Control SetAgent 3DC_Label Caption StaticText "
        "\"#%06lX %s\"",
        switch_rgb(c),
        item < 0 ? "" : color_item_strings[item]
        );
    SendMessage(BBhwnd, BB_BROADCAST, 0, (LPARAM)buffer);
}

int syscolor_handle_bbi_message(const char * p)
{
    static int item_set;
    static COLORREF item_color;
    static COLORREF mem_color;

    COLORREF c; int n, id;

    if (0 == n_stricmp(&p, "SetRed")) {
        n = 0;
        goto setrgb;
    }
    else
    if (0 == n_stricmp(&p, "SetGreen")) {
        n = 8;
        goto setrgb;
    }
    else
    if (0 == n_stricmp(&p, "SetBlue")) {
        n = 16;
        goto setrgb;
    }
    else
    if (0 == n_stricmp(&p, "SetItem")) {
        item_set = 0;

        const char **cs = color_item_strings;
        n = 0;
        do {
            ++n;
            if (0 == stricmp(p, *cs)) {
                item_set = n;
                break;
            }
        } while (*++cs);

        if (item_set > 0)
        {
            item_color = c = GetSysColor(color_trans_3dcc[item_set-1]);
            set_bbi_sliders(c);
            set_bbi_label(item_set-1, c);
        }
        return 1;
    }
    else
    if (0 == n_stricmp(&p, "MSto")) {
        mem_color = item_color;
        return 1;
    }
    else
    if (0 == n_stricmp(&p, "MGet")) {
        item_color = mem_color;
        set_bbi_sliders(item_color);
        goto setrgb_2;
    }
    else
    if (0 == n_stricmp(&p, "Load")) {
        syscolor_read(p);
        colors_changed = 2;
        if (item_set > 0)
        {
            item_color = c = GetSysColor(color_trans_3dcc[item_set-1]);
            set_bbi_sliders(c);
            set_bbi_label(item_set-1, c);
        }
        return 1;
    }
    else
        return 0;


setrgb:
    c = atoi(p);
    c = (item_color & ~(255<<n)) | c<<n;
    if (item_color == c)
        return 1;
    item_color = c;

setrgb_2:
    if (item_set > 0) {
        id = color_trans_3dcc[item_set-1];
        SetSysColors(1, &id, &item_color);
        colors_changed = 2;
    }
    set_bbi_label(item_set-1, item_color);

    return 1;
}

//===========================================================================
