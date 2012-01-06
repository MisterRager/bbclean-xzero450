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

#include "BBApi.h"
#include "BImage.h"
#include "bbroot.h"
#include "bbrc.h"

#ifdef TINY_IMAGE
const char *szAppName = "bsetroot 2.1-tiny";
#else
const char *szAppName = "bsetroot 2.1";
#endif

#define ST static

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void show_help(void)
{
    char buffer[4000];
    const char *image_getversion(void);

    sprintf(buffer,
    "%s"
    "\n© 2001-2003 The Blackbox for Windows Development Team"
    "\n© 2003-2009 grischka@users.sourceforge.net"
    "\nBased on bsetroot for Blackbox on Linux by Brad Hughes."
#ifdef TINY_IMAGE
    "\n%s supports only '.bmp' type images."
#else
    "\n%s uses %s"
#endif
    "\n"
    "\nSwitches:"
    "\n  -gradient <texture>  \tgradient texture"
    "\n  -from <color>  \t\tgradient start color"
    "\n  -to <color>    \t\tgradient end color"
    "\n"
    "\n  -solid <color> \t\tsolid color"
    "\n"
    "\n  -mod <x> <y>   \t\tmodula pattern"
    "\n  -fg <color>    \t\tmodula foreground color"
    "\n  -bg <color>    \t\tmodula background color"
    "\n"
    "\n  -full <image>  \t\tset image fullscreen"
    "\n  -tile <image>  \t\tset image tiled"
    "\n  -center <image>      \tset image centered"
    "\n"
    "\nMore detailed information can be found in bsetroot.htm."
    ,szAppName
    ,szAppName
    ,image_getversion()
    );
    MessageBox(NULL, buffer, szAppName, MB_OK);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Image interface

typedef void *HIMG;
const char *image_getversion(void);
const char *image_getlasterror(void);

HIMG image_create_fromfile(const char *path);
HIMG image_create_frombmp(HBITMAP bmp);
HIMG image_create_fromraw(int w, int h, void *pixels);
int image_save(HIMG img, const char *path);
void image_destroy(HIMG Img);

int image_getwidth(HIMG img);
int image_getheight(HIMG img);

RGBQUAD image_getpixel(HIMG img, int x, int y);
int image_setpixel(HIMG img, int x, int y, RGBQUAD c);
int image_resample(HIMG *img, int w, int h);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* bimage utils */
HIMG DesktopGradient(struct rootinfo *r, int width, int height);
void Modula(HIMG Img, int x, int y, COLORREF fg);
void copy_img(HIMG Img1, HIMG Img2, int x0, int y0, int xs, int ys, int hueIntensity, int saturationValue);
int load_bmp(const char *filename, HIMG *pImg, string_node *searchpaths, const char *search_base);

/* system wallpaper interface */
int setwallpaper(const char *wpfile, int wpstyle);
void set_background_color (COLORREF color);

/* parsing */
char *make_full_path(char *buffer, const char *filename, const char *search_base);
char *read_line(FILE *fp, char *buffer);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// main

int WINAPI WinMain(
    HINSTANCE hThisInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
    const char *error_msg = NULL;

    char temp[MAX_PATH];
    char buffer[2048];

    HIMG Img = NULL;
    HIMG Back = NULL;

    int bmp_width;
    int bmp_height;
    int screen_width;
    int screen_height;

    struct rootinfo RI;
    struct rootinfo *r = &RI;

    char *p; int n; FILE *fp; MSG msg;

    // stop hourglass cursor:
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));

    // clear the structure
    init_root(r);

    strcpy (buffer, lpCmdLine);
    // replace tabs
    for (p = buffer; *p; p++)
        if (IS_SPC(*p))
            *p = ' ';

    if (0==buffer[0]) {
        show_help();
        goto theend;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Read switches from bsetroot.rc

    fp = fopen(make_full_path(temp, "bsetroot.rc", NULL), "rb");
    if (fp) {
        while (read_line(fp, temp)) {
            if ('-' == temp[0] && !parse_root(r, temp)) {
                sprintf(buffer, "Error: in bsetroot.rc:\n%s", temp);
                error_msg = buffer;
                goto theend;
            }
        }
        fclose(fp);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // parse commandline

    if (!parse_root(r, buffer)) {
        sprintf(buffer, "Error: in commandstring:\n%s", lpCmdLine);
        error_msg = buffer;
        goto theend;
    }

    if (r->help) {
        show_help();
        goto theend;
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // get the screen sizes

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);
    if (r->vdesk) {
        int v_screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int v_screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (v_screen_width && v_screen_height) {
            screen_width = v_screen_width;
            screen_height = v_screen_height;
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // try to load the image

    bmp_width = bmp_height = 0;

    if (r->bmp) {
        n = load_bmp(r->wpfile, &Img, r->paths, r->search_base);

        if (n == 1) {
            sprintf(buffer, "Error: Could not find image:\n%s", r->wpfile);
            error_msg = buffer;

        } else if (n == 2) {
            sprintf(buffer, "Error: Could not load image - %s:\n%s", image_getlasterror(), r->wpfile);
            error_msg = buffer;
        }

        if (Img) {
            bmp_width  = image_getwidth(Img);
            bmp_height = image_getheight(Img);

            if (r->scale && r->scale != 100) {
                int w = bmp_width * r->scale / 100;
                int h = bmp_height * r->scale / 100;
                image_resample(&Img, w, h);
                bmp_width  = image_getwidth(Img);
                bmp_height = image_getheight(Img);
            }

            if (r->convert) {
                screen_width = bmp_width;
                screen_height = bmp_height;
            }

            if (WP_NONE == r->wpstyle)
                r->wpstyle = WP_FULL;

        } else {
            r->wpstyle = WP_NONE;
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // create background texture, if any

    if (r->gradient
        || r->mod
        || (r->solid
            && (r->interlaced || (r->save && NULL == Img)))) {

        Back = DesktopGradient(r, screen_width, screen_height);
        if (r->mod)
            Modula(Back, r->modx, r->mody, r->modfg);

    } else if (0 == r->save) {
        // default bsetbg behaviour: use os wallpaper / SysColor
        if (r->solid)
            set_background_color(r->color1);

        if (Img && (r->sat < 255 || r->hue > 0)) {
            if (false == r->solid)
                r->color1 = GetSysColor(COLOR_DESKTOP);

            Back = DesktopGradient(r, bmp_width, bmp_height);

            copy_img(Back, Img,
                0, 0, bmp_width, bmp_height, r->hue, r->sat);
        }

        goto write_image;
    }

    if (Img)
    {
        if (WP_FULL == r->wpstyle
            && (bmp_width != screen_width || bmp_height != screen_height)) {
            image_resample(&Img, screen_width, screen_height);
            bmp_width  = image_getwidth(Img);
            bmp_height = image_getheight(Img);
        }

        if (NULL == Back) {
            // do we need a background anyway?
            if (r->sat < 255 || r->hue > 0
                || bmp_width != screen_width
                || bmp_height != screen_height
               ) {
                if (false == r->solid)
                    r->color1 = GetSysColor(COLOR_DESKTOP);
                Back = DesktopGradient(r, screen_width, screen_height);
            }
        }

        if (Back) {
            if (WP_TILE == r->wpstyle) {
                int x0, y0;
                for (x0 = 0; x0 < screen_width;  x0+=bmp_width)
                for (y0 = 0; y0 < screen_height; y0+=bmp_height)
                    copy_img(Back, Img,
                        x0, y0, bmp_width, bmp_height, r->hue, r->sat);
            } else {
                int x0 = (screen_width  - bmp_width) / 2;
                int y0 = (screen_height - bmp_height) / 2;
                copy_img(Back, Img,
                    x0, y0, bmp_width, bmp_height, r->hue, r->sat);
            }
        }
    }
    // since we have a fullscreen image now, set tile mode
    r->wpstyle = WP_TILE;

write_image:
    if (0 == r->save) {
        if (r->convert)
        {
            error_msg = "Error: -convert needs -save";
            goto theend;
        }
        if (!GetEnvironmentVariable("APPDATA", temp, sizeof temp))
            GetWindowsDirectory(temp, sizeof temp);
        join_path(r->bsetroot_bmp, temp, "bsetroot.bmp");
    }

    if (Back || Img) {
        if (Back)
            image_save(Back, r->bsetroot_bmp);
        else
            image_save(Img, r->bsetroot_bmp);
    }

    if (0 == r->save) {
        if (!setwallpaper(r->bsetroot_bmp, r->wpstyle))
            error_msg = "Error: Could not open wallpaper registry key.";
    }

theend:
    image_destroy(Back);
    image_destroy(Img);
    n = 0;
    if (error_msg) {
        if (0 == r->quiet)
            MessageBox(NULL, error_msg, szAppName,
                MB_OK|MB_ICONERROR|MB_TOPMOST|MB_SETFOREGROUND);
        n = 1;
    }
    delete_root(r);
    return n;
}

//===========================================================================
void copy_img(HIMG Img1, HIMG Img2,
    int x0, int y0, int xs, int ys, int hueIntensity, int saturationValue)
{
    unsigned ih = 255 - hueIntensity; int x, y;
    for (y = 0; y < ys; y++)
    for (x = 0; x < xs; x++)
    {
        // First we read the original pixel's color...
        RGBQUAD pix1 = image_getpixel(Img2, x, y);
        unsigned r = pix1.rgbRed;
        unsigned g = pix1.rgbGreen;
        unsigned b = pix1.rgbBlue;
        // ...then we apply saturation...
        if (saturationValue<255)
        {
            unsigned greyscale =
                (79*r + 156*g + 21*b) * (255-saturationValue)/256 + 255;

            r = (r*saturationValue + greyscale)>>8;
            g = (g*saturationValue + greyscale)>>8;
            b = (b*saturationValue + greyscale)>>8;
        }
        // ...and hue according to color and intensity...
        if (hueIntensity>0)
        {
            RGBQUAD pix2 = image_getpixel(Img1, x0+x, y0+y);
            r = (ih*r + hueIntensity*pix2.rgbRed   + 255)>>8;
            g = (ih*g + hueIntensity*pix2.rgbGreen + 255)>>8;
            b = (ih*b + hueIntensity*pix2.rgbBlue  + 255)>>8;
        }
        pix1.rgbRed   = r;
        pix1.rgbGreen = g;
        pix1.rgbBlue  = b;
        image_setpixel(Img1, x0+x, y0+y, pix1);
    }
}

//===========================================================================
HIMG DesktopGradient(struct rootinfo *r, int width, int height)
{
    struct bimage *b;
    HIMG h;
    StyleItem si;

    si.type = r->type,
    si.Color = r->color1,
    si.ColorTo = r->color2,
    si.interlaced = !!r->interlaced,
    si.bevelstyle = r->bevelstyle,
    si.bevelposition = r->bevelposition;
    si.parentRelative = false;

    bimage_init(true, true);
    b = bimage_create(width, height, &si);
    h = image_create_fromraw(width, height, bimage_getpixels(b));
    bimage_destroy(b);
    return h;
}

//===========================================================================
void Modula(HIMG Img, int mx, int my, COLORREF fg)
{
    RGBQUAD q; int x, y;
    int width  = image_getwidth(Img);
    int height = image_getheight(Img);
    *(COLORREF*)&q = switch_rgb(fg);
    if (my > 1)
        for (y = height-my; y >= 0; y-=my)
        for (x = 0; x < width; x++)
            image_setpixel(Img, x, y, q);
    if (mx > 1)
        for (y = height; --y >= 0;)
        for (x = mx-1; x < width; x+= mx)
            image_setpixel(Img, x, y, q);
}

//===========================================================================
// API: ParseItem
// Purpose: parses a given string and assigns settings to a StyleItem class

const struct styleprop styleprop_1[] = {
 {"solid"        ,B_SOLID           },
 {"horizontal"   ,B_HORIZONTAL      },
 {"vertical"     ,B_VERTICAL        },
 {"crossdiagonal",B_CROSSDIAGONAL   },
 {"diagonal"     ,B_DIAGONAL        },
 {"pipecross"    ,B_PIPECROSS       },
 {"elliptic"     ,B_ELLIPTIC        },
 {"rectangle"    ,B_RECTANGLE       },
 {"pyramid"      ,B_PYRAMID         },
 {NULL           ,-1                }
 };

const struct styleprop styleprop_2[] = {
 {"flat"        ,BEVEL_FLAT      },
 {"raised"      ,BEVEL_RAISED    },
 {"sunken"      ,BEVEL_SUNKEN    },
 {NULL          ,-1              }
 };

const struct styleprop styleprop_3[] = {
 {"bevel1"      ,BEVEL1  },
 {"bevel2"      ,BEVEL2  },
 {"bevel3"      ,BEVEL2+1},
 {NULL          ,-1      }
 };

int findtex(const char *p, const struct styleprop *s)
{
    do if (strstr(p, s->key)) break; while ((++s)->key);
    return s->val;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// alter display properties

int setwallpaper(const char *wpfile, int wpstyle)
{
    HKEY key;
    char s_tile[2], s_full[2], tmp[MAX_PATH];
    s_tile[0] = s_full[0] = '0';
    tmp[0] = s_tile[1] = s_full[1] = 0;
    if (wpfile && WP_NONE != wpstyle) {
        strcpy(tmp, wpfile);
        if (WP_TILE==wpstyle)
            s_tile[0] = '1';
        if (WP_FULL==wpstyle)
            s_full[0] = '2';
    }
    if (ERROR_SUCCESS != RegOpenKeyEx(
        HKEY_CURRENT_USER, "Control Panel\\Desktop\\", 0, KEY_ALL_ACCESS, &key))
        return 0;
    RegSetValueEx(key, "TileWallpaper",  0, REG_SZ, (BYTE*)s_tile, 2);
    RegSetValueEx(key, "WallpaperStyle", 0, REG_SZ, (BYTE*)s_full, 2);
    RegCloseKey(key);
    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, tmp,
        SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
    return 1;
}

void set_background_color (COLORREF color)
{
    const int lpaElements[1]  = {COLOR_BACKGROUND};
    const COLORREF colours[1] = {color};
    SetSysColors(1, lpaElements, colours);
}

//===========================================================================
bool FileExists(LPCSTR szFileName)
{
    DWORD a = GetFileAttributes(szFileName);
    return (DWORD)-1 != a && 0 == (a & FILE_ATTRIBUTE_DIRECTORY);
}

char *make_full_path(char *buffer, const char *filename, const char *search_base)
{
    char exe_path[MAX_PATH];
    if (search_base && search_base[0]) {
        join_path(buffer, search_base, filename);
        if (FileExists(buffer))
            return buffer;
    }

    get_exe_path(NULL, exe_path, sizeof exe_path);
    return join_path(buffer, exe_path, filename);
}

char *read_line(FILE *fp, char *buffer)
{
    char *s;
    do {
        s = buffer;
        if (NULL==fgets(s, MAX_PATH, fp))
            return NULL;

        while (*s) { if (IS_SPC(*s)) *s = ' '; ++s; }
        while (s > buffer && IS_SPC(s[-1])) s--;
        *s=0;
        s = buffer;
        while (*s && IS_SPC(*s)) ++s;
    } while ('#' == *s || '!' == *s || 0 == *s);
    return strcpy(buffer, s);
}

//===========================================================================
int load_bmp(const char *filename, HIMG *pImg, string_node *searchpaths, const char *search_base)
{
    char path[MAX_PATH];
    char temp[MAX_PATH];
    const char *p;
    int state;

    for (state = 0;;++state) {
        switch (state) {
        case 0: // try original name
            p = filename;
            goto try_it;
        case 1: // try from exe-path
            p = filename;
            break;
        case 2: // try searchpaths listed in "bsetroot.rc"
            if (NULL == searchpaths)
                continue;
            -- state; // might have still more lines
            p = join_path(path, searchpaths->str, file_basename(filename));
            searchpaths = searchpaths->next;
            break;
        case 3:  // try backgrounds/path
            p = join_path(path, "backgrounds", filename);
            break;
        case 4:  // try backgrounds/file
            p = join_path(path, "backgrounds", file_basename(filename));
            break;
        default: // give up
            return 1;
        }

        if (!is_absolute_path(p))
            p = make_full_path(path, strcpy(temp, p), search_base);
try_it:
        if (FileExists(p)) {
            *pImg = image_create_fromfile(p);
            return *pImg ? 0 : 2;
        }
    }
}

//===========================================================================

