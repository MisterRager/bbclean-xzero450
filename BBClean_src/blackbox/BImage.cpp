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

#ifndef __BBAPI_H_
#include "BB.h"
#endif

#include "BImage.h"

#ifndef IMAGEDITHER
extern bool Settings_imageDither;
#define IMAGEDITHER Settings_imageDither
#endif

#ifndef ISNEWSTYLE
extern bool Settings_newMetrics;
#define ISNEWSTYLE Settings_newMetrics
#endif

// -------------------------------------
#define SQF 181  // 181 * 181 * 2   = 65527
#define SQR 256  // sqrt(65527)     = 255.98
#define SQD  16
#define SQR_TAB_SIZE (SQF*SQF*2 / SQD) // ~ 4096

static unsigned char _sqrt_table[SQR_TAB_SIZE];

#define DITH_TABLE_SIZE (256+8)
static int bits_per_pixel = -1;
static unsigned char _dith_r_table[DITH_TABLE_SIZE];
static unsigned char _dith_g_table[DITH_TABLE_SIZE];
static unsigned char _dith_b_table[DITH_TABLE_SIZE];
static unsigned char add_r[16], add_g[16], add_b[16];

// bebel drawing exception
#define BEVEL_EXCEPT_LEFT   1
#define BEVEL_EXCEPT_TOP    2
#define BEVEL_EXCEPT_RIGHT  3
#define BEVEL_EXCEPT_BOTTOM 4
// -------------------------------------

class bImage
{
    unsigned from_red;
    unsigned from_green;
    unsigned from_blue;
    int diff_red;
    int diff_green;
    int diff_blue;
    int width;
    int height;
    bool alternativ;
    unsigned char dark_table[256];
    unsigned char lite_table[256];
    unsigned char *pixels;
    unsigned char *xtab;
    unsigned char *ytab;

// -------------------------------------

static unsigned long bsqrt(unsigned long x)
{
    unsigned long q, r;
    if (x < 2) return x;
    for (r = x >> 1; q = x / r, q < r; r = (r + q) >> 1);
    return r;
}

static void init_sqrt()
{
    for (int i = 0; i < SQR_TAB_SIZE; i++)
    {
        // backwards compatible to the elliptic inconsistency in
        // the original gradient code: sqrt(i/2) instead of sqrt(i)
        int f = bsqrt(i*SQD/2);
        _sqrt_table[i] = (unsigned char)(SQR - 1 - f);
    }
}

static int isgn(int x)
{
    return x>0 ? 1 : x<0 ? -1 : 0;
}

static int imin(int a, int b)
{
	return a<b?a:b;
}

static void init_dither_tables(void)
{
    HDC hdc = GetDC(NULL);
    bits_per_pixel = GetDeviceCaps(hdc, BITSPIXEL);
    if (bits_per_pixel > 16 || bits_per_pixel < 8)
    {
        bits_per_pixel = 0;
    }
    else
    {
        int red_bits, green_bits, blue_bits;

        int i;
        // hardcoded for 16 bit display: red:5, green:6, blue:5
        red_bits    = 1 << (8 - 5);
        green_bits  = 1 << (8 - 6);
        blue_bits   = 1 << (8 - 5);
        for (i = 0; i < DITH_TABLE_SIZE; i++)
        {
            _dith_r_table[i] = imin(255, i & -red_bits    );
            _dith_g_table[i] = imin(255, i & -green_bits  );
            _dith_b_table[i] = imin(255, i & -blue_bits   );
        }

        static const unsigned char dither4[16] =
        {
          7, 3, 6, 2,  // 3 1 3 1
          1, 5, 0, 4,  // 0 2 0 2
          6, 2, 7, 3,  // 3 1 3 1
          0, 4, 1, 5   // 0 2 0 2
        };

        int dr = 8/red_bits, dg = 8/green_bits, db = 8/blue_bits;
        for (i = 0; i < 16; i++)
        {
          int d = dither4[i];
          add_r[i] = d / dr;
          add_g[i] = d / dg;
          add_b[i] = d / db;
        }
    }
    ReleaseDC(NULL, hdc);
}

//===========================================================================
// ColorDither for 16bit displays.

// Original comment from the authors of bb4*nix:
// "algorithm: ordered dithering... many many thanks to rasterman
//  (raster@rasterman.com) for telling me about this... portions of
//  this code is based off of his code in Imlib"

void TrueColorDither(void)
{
  unsigned char *p = pixels; int x, y;
  for (y = 0; y < height; y++) {
    int oy = 4 * (y & 3);
    for (x = 0; x < width; x++) {
      int ox = oy + (x & 3);
      p[0] = _dith_b_table[p[0] + add_b[ox]];
      p[1] = _dith_g_table[p[1] + add_g[ox]];
      p[2] = _dith_r_table[p[2] + add_r[ox]];
      p+=4;
    }
  }
}

//===========================================================================
// brightness delta for bevels and interlaced

#define DELTA_BEVEL 18          // originally 75/150%
#define DELTA_BEVELCORNER 32
#define DELTA_INTERLACE 10      // originally 75/112%

static int trans_late(int i, int d)
{
    if (d == -DELTA_INTERLACE || d >= DELTA_BEVEL) d *= 2;
    int r = i*(100+d)/100;
    return iminmax(r, 0, 255);
}

void make_delta_table(int m)
{
    for (int i = 0; i<256; i++)
    {
        dark_table[i] = trans_late(i, -m);
        lite_table[i] = trans_late(i, m);
    }
}

static void modify(unsigned char *pixel, int delta)
{
    int n = 2;
    do pixel[n] = trans_late(pixel[n], delta); while (n--);
}

inline void lighter(unsigned char *pixel)
{
    pixel[0] = lite_table[pixel[0]];
    pixel[1] = lite_table[pixel[1]];
    pixel[2] = lite_table[pixel[2]];
}

inline void darker(unsigned char *pixel)
{
    pixel[0] = dark_table[pixel[0]];
    pixel[1] = dark_table[pixel[1]];
    pixel[2] = dark_table[pixel[2]];
}

//====================
void bevel(bool sunken, int pos, int exception)
{
    if (--pos < 0) return;
    int w = width, h = height;
    int nx = w - 2*pos - 1;
    int ny = h - 2*pos - 1;
    if (nx <= 0 || ny <= 0) return;

    int f1 = DELTA_BEVEL;
    int f2 = DELTA_BEVELCORNER;

    if (sunken) f1 = -f1, f2 = -f2;
    make_delta_table (f1);

    unsigned char *p; int d, e, n;

	p = pixels + (pos + w * pos) * 4;

    d = ny * w * 4; e = 4; n = nx;
    if (exception == BEVEL_EXCEPT_LEFT) darker(p);
    while (--n) {
        p += e;
        if (exception != BEVEL_EXCEPT_BOTTOM) darker(p);
        if (exception != BEVEL_EXCEPT_TOP)    lighter(p+d);
    }

    p += e;
    if (exception == BEVEL_EXCEPT_BOTTOM)
        darker(p);
    else if (exception == BEVEL_EXCEPT_RIGHT)
        darker(p), lighter(p+d);
    else modify(p,-f2);  // bottom-right corner pixel

    d = nx * 4; e = w * 4; n = ny;
    if (exception == BEVEL_EXCEPT_BOTTOM) lighter(p-d);
    while (--n) {
        p += e;
        if (exception != BEVEL_EXCEPT_RIGHT) darker(p);
        if (exception != BEVEL_EXCEPT_LEFT)  lighter(p-d);
    }

    p += e;
    if (exception == BEVEL_EXCEPT_TOP)
        darker(p), lighter(p-d);
    else if (exception == BEVEL_EXCEPT_LEFT)
        lighter(p-d);
    else
        modify(p-d, f2); // top-left corner pixel
}

//====================
inline unsigned char tobyte(int v) {
    return ((v > 255) ? 255 : ((v < 0) ? 0 : v));
}

void table_fn(unsigned char *p, int length, bool invert)
{
    unsigned char *c = p;
    int i, e, d;
    if (invert) i = length-1, d = e = -1;
    else i = 0, d = 1, e = length;
    while (i != e)
    {
        c[0] = from_blue  + diff_blue  * i / length;
        c[1] = from_green + diff_green * i / length;
        c[2] = from_red   + diff_red   * i / length;
        c[3] = 0;
        c += 4; i += d;
    }
}

inline void diag_fn(unsigned char *c, int x, int y)
{
    unsigned char *xp = xtab + x*4;
    unsigned char *yp = ytab + y*4;
    c[0] = ((unsigned)xp[0] + (unsigned)yp[0]) >> 1;
    c[1] = ((unsigned)xp[1] + (unsigned)yp[1]) >> 1;
    c[2] = ((unsigned)xp[2] + (unsigned)yp[2]) >> 1;
    c[3] = 0;
}

inline void rect_fn(unsigned char *c, int x, int y)
{
    if (alternativ ^ (x*height <= y*width))
        *(unsigned long*)c = ((unsigned long*)xtab)[x];
    else
        *(unsigned long*)c = ((unsigned long*)ytab)[y];
}

inline void elli_fn(unsigned char *c, int x, int y)
{
    int dx = SQF - 1 - SQF * x / width;
    int dy = SQF - 1 - SQF * y / height;
    int f = _sqrt_table[(dx*dx + dy*dy) / SQD];
    *(unsigned long *)c = ((unsigned long*)xtab)[f];
}

// -------------------------------------
public:

bImage(int width, int height, int type, COLORREF colour_from, COLORREF colour_to, bool interlaced, int bevelStyle, int bevelPosition, int bevelExecption)
{
    pixels = NULL;

    if (width<2)  { if (width<=0) return; width = 2; }
    if (height<2) { if (height<=0) return; height = 2; }

    this->width = width;
    this->height = height;

    int byte_size = (width * height) * 4;
    int table_size = width + height;
    if (table_size < SQR) table_size = SQR;
    pixels = new unsigned char [byte_size + table_size * 4];
    xtab = pixels + byte_size;
    ytab = xtab + width * 4;

    bool sunken = bevelStyle == BEVEL_SUNKEN;

    if (sunken && false == ISNEWSTYLE) switch (type) {
    // for historical reasons: invert color gradient for
    // sunken bevels and these types:
    case B_PIPECROSS:
    case B_ELLIPTIC:
    case B_RECTANGLE:
    case B_PYRAMID:
        colour_to^=colour_from^=colour_to^=colour_from;
    }

    int i;

    i = GetBValue(colour_to) - (from_blue = GetBValue(colour_from));
    diff_blue = i + isgn(i);
    i = GetGValue(colour_to) - (from_green = GetGValue(colour_from));
    diff_green = i + isgn(i);
    i = GetRValue(colour_to) - (from_red = GetRValue(colour_from));
    diff_red = i + isgn(i);

    if (interlaced)
        make_delta_table(sunken != (0 == (height & 1)) ? -DELTA_INTERLACE : DELTA_INTERLACE);

    unsigned long *s, *d, c, *e, *f;
    int x, y, z;
    unsigned char *p = pixels;
    alternativ = false;

    switch (type)
    {
        // -------------------------------------
        default:
        case B_SOLID:
            diff_red = diff_green = diff_blue = 0;

        // -------------------------------------
        case B_HORIZONTAL:
            table_fn(xtab, width, false);

            // draw 2 lines, to cover the 'interlaced' case
            y = 0; do { x = 0; s = (unsigned long *)xtab; do {
            *(unsigned long*)p = *s++;
            if (interlaced) if (1 & y) darker(p); else lighter(p);
            p+=4; } while (++x < width); } while (++y < 2);

            // copy down the lines
            d = (unsigned long*)p;
            while (y < height) {
            s = (unsigned long*)pixels + (y&1)*width;
            memcpy(d, s, width*4);
            d += width; y++;
            }
            break;

        // -------------------------------------
        case B_VERTICAL:
            table_fn(ytab, height, true);

            // draw 1 column
            y = 0; s = (unsigned long *)ytab; z = width*4; do {
            *(unsigned long*)p = *s++;
            if (interlaced) if (1 & y) darker(p); else lighter(p);
            p += z; } while (++y < height);

            // copy colums
            s = (unsigned long*)pixels;
            y = 0; do {
            d = s, s += width; c = *d++;
            do *d = c; while (++d<s);
            } while (++y < height);
			break;

        // -------------------------------------
        case B_CROSSDIAGONAL:
            table_fn(xtab, width, true);
            goto diag;
        case B_DIAGONAL:
            table_fn(xtab, width, false);
        diag:
            table_fn(ytab, height, true);
            y = 0; do { x = 0; do {
            diag_fn(p, x, y);
            if (interlaced) if (1 & y) darker(p); else lighter(p);
            p+=4; } while (++x < width); } while (++y < height);
            break;

        // -------------------------------------
        case B_PIPECROSS:
            alternativ = true;
        case B_RECTANGLE:
        case B_PYRAMID:
            table_fn(xtab, width, false);
            table_fn(ytab, height, false);
            goto draw_quadrant;

        case B_ELLIPTIC:
            if (0 == _sqrt_table[0]) init_sqrt();
            table_fn(xtab, SQR, false);
            goto draw_quadrant;

        draw_quadrant:
            // one quadrant is drawn, and mirrored horizontally and vertically
            s = (unsigned long*)p + height*width; z = height;
            y = 0; do {
            d = (unsigned long*)p + width; f = s; s -= width; e = s; z--;
            x = 0; do {

            if (B_ELLIPTIC == type) elli_fn(p, x, y);
            else
            if (B_PYRAMID == type) diag_fn(p, x, y);
            else rect_fn(p, x, y);

            c = *(unsigned long *)p;
            if (interlaced) if (2 & y) darker(p); else lighter(p);
            *--d = *(unsigned long *)p;

            if (e != (unsigned long *)p)
            {
                *e = c;
                if (interlaced) if (1 & z) darker((unsigned char*)e); else lighter((unsigned char*)e);
                *--f = *e;
            }
            e++;

            p+=4; } while ((x+=2) < width);
            p+=(width/2)*4; } while ((y+=2) < height);
            break;
    }

    if (bevelStyle != BEVEL_FLAT) bevel(sunken, bevelPosition, bevelExecption);
    if (-1 == bits_per_pixel) init_dither_tables();
    if (bits_per_pixel >= 8 && IMAGEDITHER) TrueColorDither();
}

~bImage()
{
    if (pixels) delete pixels;
}

HBITMAP create_bmp(void)
{
    if (NULL == pixels) return NULL;
    BITMAPINFO bv4info;
    setup_bv4info(&bv4info);
    unsigned char *p = NULL;
    HBITMAP bmp = CreateDIBSection(NULL, &bv4info, DIB_RGB_COLORS, (void**)&p, NULL, 0);
    if (bmp && p) memcpy(p, pixels, width * height * 4);
    return bmp;
    //return CreateDIBitmap(hdc, (BITMAPINFOHEADER*)&bv4info, CBM_INIT, pixels, &bv4info, DIB_RGB_COLORS);
}

void copy_to_hdc(HDC hDC, int px, int py, int w, int h)
{
    if (NULL == pixels) return;
    BITMAPINFO bv4info;
    setup_bv4info(&bv4info);
    SetDIBitsToDevice(hDC, px, py, w, h, 0, 0, 0, h, pixels, &bv4info, DIB_RGB_COLORS);
}

// -------------------------------------
private:

void setup_bv4info(BITMAPINFO *bv4info)
{
    ZeroMemory(bv4info, sizeof(*bv4info));
    bv4info->bmiHeader.biSize = sizeof(bv4info->bmiHeader);
    bv4info->bmiHeader.biWidth = width;
    bv4info->bmiHeader.biHeight = height;
    bv4info->bmiHeader.biPlanes = 1;
    bv4info->bmiHeader.biBitCount = 32;
    bv4info->bmiHeader.biCompression = BI_RGB;
}

}; // end of bImage

//===========================================================================
// API: MakeGradient
// Purpose: creates a gradient and fills it with the specified options
// In: HDC = handle of the device context to fill
// In: RECT = rect you wish to fill
// In: int = type of gradient (eg. B_HORIZONTAL)
// In: COLORREF = first colour to use
// In: COLORREF = second colour to use
// In: bool = whether to interlace (darken every other line)
// In: int = style of bevel (eg. BEVEL_FLAT)
// In: int = position of bevel (eg. BEVEL1)
// In: int = not used
// In: COLORREF = border color
// In: int = width of border around bitmap
//===========================================================================

extern "C" void MakeGradient(HDC hDC, RECT rect, int type, COLORREF colour_from, COLORREF colour_to, bool interlaced, int bevelStyle, int bevelPosition, int bevelWidth, COLORREF borderColour, int borderWidth)
{
    MakeGradientEx(hDC, rect, type, colour_from, colour_to, colour_from, colour_to, interlaced, bevelStyle, bevelPosition, bevelWidth, borderColour, borderWidth);
}
extern "C" void MakeGradientEx(HDC hDC, RECT rect, int type, COLORREF colour_from, COLORREF colour_to, COLORREF colour_from_splitto, COLORREF colour_to_splitto, bool interlaced, int bevelStyle, int bevelPosition, int bevelWidth, COLORREF borderColour, int borderWidth)
{
    if (borderWidth)
    {
        HBRUSH  hBrush    = CreateSolidBrush(borderColour);
        HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);
        while (--borderWidth >= 0)
        {
            FrameRect(hDC, &rect, hBrush);
            _InflateRect(&rect, -1, -1);
        }
        DeleteObject(SelectObject(hDC, hOldBrush));
    }

    if (type >= 0)
    {
        bImage *bImg;
        int w = GetRectWidth(&rect);
        int h = GetRectHeight(&rect);

        bool is_vertical = false; 	//(type == B_SPLITVERTICAL) || (B_SPLIT_VERTICAL)   || (type == B_MIRRORVERTICAL) || (B_MIRROR_VERTICAL);
        bool is_horizontal = false; //(type == B_SPLITHORIZONTAL) || (B_SPLIT_HORIZONTAL) || (type == B_MIRRORHORIZONTAL) || (B_MIRROR_HORIZONTAL);
        bool is_split      = false; //(type == B_SPLITVERTICAL) || (B_SPLIT_VERTICAL)   || (type == B_SPLITHORIZONTAL) || (B_SPLIT_HORIZONTAL);
        bool is_mirror     = false; //(type == B_MIRRORVERTICAL) || (B_MIRROR_VERTICAL)  || (type == B_MIRRORHORIZONTAL) || (B_MIRROR_HORIZONTAL);

		switch (type) {
			case B_SPLITVERTICAL:
			case B_SPLIT_VERTICAL:
				is_vertical = is_split = true;
				break;
				
			case B_SPLITHORIZONTAL:
			case B_SPLIT_HORIZONTAL:
				is_horizontal = is_split = true;
				break;
			
			case B_MIRRORVERTICAL:
			case B_MIRROR_VERTICAL:
				is_vertical = is_mirror = true;
				break;
				
			case B_MIRRORHORIZONTAL:
			case B_MIRROR_HORIZONTAL:
				is_horizontal = is_mirror = true;
				break;
				
		}

        if (is_vertical || is_horizontal || is_split || is_mirror){
            // specify params depending on style
            int exception0 = 0;
            int exception1 = 0;
            COLORREF Color0 = colour_from;
            COLORREF Color1 = colour_from;
            COLORREF Color2 = colour_to;
            COLORREF Color3 = colour_to;

            if (is_vertical){
                type = B_VERTICAL;
                h = (h+1)>>1;
                exception0 = BEVEL_EXCEPT_BOTTOM;
                exception1 = BEVEL_EXCEPT_TOP;
            }
            if (is_horizontal){
                type = B_HORIZONTAL;
                w = (w+1)>>1;
                exception0 = BEVEL_EXCEPT_RIGHT;
                exception1 = BEVEL_EXCEPT_LEFT;
            }
            if(is_split){
                Color0 = colour_from_splitto;
                Color3 = colour_to_splitto;
            }
            if(is_mirror){
                Color1 = colour_to;
                Color3 = colour_from;
            }

            bImg = new bImage(w, h, type, Color0, Color1, interlaced, bevelStyle, bevelPosition, exception0);
            bImg->copy_to_hdc(hDC, rect.left,      rect.top,        w, h);
            delete bImg;
            bImg = new bImage(w, h, type, Color2, Color3, interlaced, bevelStyle, bevelPosition, exception1);
            bImg->copy_to_hdc(hDC, rect.right - w, rect.bottom - h, w, h);
            delete bImg;
        }
        else{
            bImg = new bImage(w, h, type, colour_from, colour_to, interlaced, bevelStyle, bevelPosition, 0);
            bImg->copy_to_hdc(hDC, rect.left, rect.top, w, h);
            delete bImg;
        }
    }
}

//===========================================================================
// API: CreateBorder
//===========================================================================

extern "C" void CreateBorder(HDC hdc, RECT *prect, COLORREF borderColour, int borderWidth)
{
    MakeGradient(hdc, *prect, -1, 0, 0, false, 0, 0, 0, borderColour, borderWidth);
}

//===========================================================================
// API: MakeGradientBitmap
//===========================================================================

extern "C" HBITMAP MakeGradientBitmap(int width, int height, int type, COLORREF colour_from, COLORREF colour_to, bool interlaced, int bevelStyle, int bevelPosition)
{
    bImage bImg(width, height, type, colour_from, colour_to, interlaced, bevelStyle, bevelPosition, 0);
    return bImg.create_bmp();
}

//===========================================================================

