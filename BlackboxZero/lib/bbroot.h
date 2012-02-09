/* ------------------------------------------------------------------------- */
/*
  This file is part of the bbLean source code
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
*/
/* ------------------------------------------------------------------------- */
/* bbroot.c */

#ifndef _BBROOT_H_
#define _BBROOT_H_

#include "bblib.h"

#define WP_NONE 0
#define WP_TILE 1
#define WP_CENTER 2
#define WP_FULL 3

#ifdef __cplusplus
extern "C" {
#endif

// recognized switches
enum
{
    E_eos = 0   , E_other,

    E_solid     , E_gradient  , E_mod       ,
    E_from      , E_bg        , E_background ,
    E_to        , E_fg        , E_foreground ,
	E_splitFrom , E_splitTo   ,

    Einterlaced,

    E_tile      , E_t         ,
    E_center    , E_c         ,
    E_full      , E_f         ,
    E_bitmap    , E_hue       , E_sat       ,
    Etile       , Ecenter     , Estretch    ,

    E_scale, E_save, E_convert, E_vdesk, E_help, E_quiet,
    E_prefix, E_path,
    E_last
};

#ifdef BBLIB_COMPILING
static const char *switches[] =
{
    "-solid",       "-gradient",    "-mod",
    "-from",        "-bg",          "-background",
    "-to",          "-fg",          "-foreground",
	"-splitFrom",   "-splitTo",

    "interlaced",

    "-tile",        "-t",
    "-center",      "-c",
    "-full",        "-f",
    "-bitmap",      "-hue",         "-sat",
    "tile",         "center",       "stretch",

    "-scale", "-save", "-convert", "-vdesk", "-help", "-quiet",
    "-prefix", "-path",
    NULL
};
#endif

struct rootinfo
{
    char bmp;       // -bitmap
    char wpstyle;   // -center/-tile/-full
    char wpfile[MAX_PATH]; // the image
    int sat;    // -sat
    int hue;    // -hue

    char mod;       // -mod
    char solid; // -solid
    char gradient; // -gradient

    COLORREF color1;
    COLORREF color2;
    COLORREF color_from;
    COLORREF color_to;
    char interlaced;
    int type;
    int bevelstyle;
    int bevelposition;

    int modx;
    int mody;
    COLORREF modfg;

    char save;  // -save
    char vdesk; // -vdesk
    int scale;  // -scale
    char convert; // -convert
    char help;  // -help
    char quiet; // -quiet

    string_node *paths; //-path <...>
    char search_base[MAX_PATH]; // -prefix <...>
    char bsetroot_bmp[MAX_PATH]; // file to save to

    // internal
    char flag;
    const char *cptr;
    char token[MAX_PATH];
};

BBLIB_EXPORT void init_root(struct rootinfo *r);
BBLIB_EXPORT void delete_root(struct rootinfo *r);
BBLIB_EXPORT int parse_root(struct rootinfo *r, const char *command);
BBLIB_EXPORT const char *get_root_switch(int);

#ifdef __cplusplus
}
#endif

#endif
