/*===================================================

	PLUGIN MASTER HEADERS

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_StyleMaster_h
#define BBInterface_StyleMaster_h

//Includes
#include "WindowMaster.h"

//Define these constants
#define STYLE_COUNT 9
#define PRESSED_STYLE_COUNT 10

#define STYLETYPE_TOOLBAR 0
#define STYLETYPE_INSET 1
#define STYLETYPE_FLAT 2
#define STYLETYPE_SUNKEN 3
#define STYLETYPE_LABEL 4
#define STYLETYPE_CLOCK 5
#define STYLETYPE_BUTTON 6
#define STYLETYPE_PRESSEDBUTTON 7
#define STYLETYPE_NONE 8
#define STYLETYPE_DEFAULT 9

extern const char * szStyleNames[];
extern const char * szStyleProperties[];
extern const char * szPressedStyleNames[];
extern const char * szPressedStyleProperties[];
extern const char * szBevelTypes[];

const short int STYLE_PROPERTY_COUNT = 10;
const short int STYLE_COLOR_PROPERTY_COUNT = 3;
const short int STYLE_SHADOW_PROPERTY_COUNT = 3;
const short int STYLE_BORDER_PROPERTY_COUNT = 2;

const short int STYLE_BEVEL_TYPE_COUNT = 3;

const short int STYLE_COLOR_INDEX = 0;
const short int STYLE_COLORTO_INDEX = 1;
const short int STYLE_TEXTCOLOR_INDEX = 2;
const short int STYLE_BEVELTYPE_INDEX = 3;
const short int STYLE_BEVELPOS_INDEX = 4;
const short int STYLE_BORDERWIDTH_INDEX = 5;
const short int STYLE_BORDERCOLOR_INDEX = 6;
const short int STYLE_SHADOWCOLOR_INDEX = 7;
const short int STYLE_SHADOWPOSX_INDEX = 8;
const short int STYLE_SHADOWPOSY_INDEX = 9;

//Define these structures
struct styledrawinfo
{
	HWND        hwnd;
	RECT        rect;
	PAINTSTRUCT ps;
	HDC         devicecontext;
	HDC         buffer;
	HGDIOBJ     otherbmp;
	bool        apply_sat_hue;
};

//Define these functions internally
int style_startup();
int style_shutdown();

bool style_draw_begin(control *c, styledrawinfo &d);
void style_draw_end(styledrawinfo &d);
void style_draw_invalidate(control *c);

void style_draw_box(RECT &rect, HDC &buffer, int styletype, bool is_bordered);
void style_draw_box(RECT &rect, HDC &buffer, StyleItem* styleptr, bool is_bordered);
void style_draw_text(RECT &rect, HDC &buffer, StyleItem* styleptr, char *text, UINT settings, HFONT font);
void style_draw_text(RECT &rect, HDC &buffer, int style, char *text, UINT settings, HFONT font);
COLORREF style_make_shadowcolor(StyleItem* style);
void style_draw_image_alpha(HDC &buffer, int x, int y, int width, int height, HANDLE &image);

COLORREF style_get_text_color(int styleidx);
StyleItem style_get_copy(int styleidx);

void style_set_transparency(HWND hwnd, BYTE transvalue, bool transback);
void style_check_transparency_workaround(HWND hwnd);

extern int style_border_width;
extern int style_bevel_width;
extern int style_font_height;
extern HFONT style_font;

int imax(int a, int b);
int imin(int a, int b);
int iminmax(int a, int b, int c);
int get_pressedstyle_index(int styleidx);

#endif
/*=================================================*/
