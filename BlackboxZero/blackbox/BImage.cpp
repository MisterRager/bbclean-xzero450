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

#ifndef BI_HIBITS
/* byte to fill into bits 24-31 */
#define BI_HIBITS 0
#endif

// -------------------------------------
#define SQF 181  // 181 * 181 * 2   = 65527
#define SQR 256  // sqrt(65527)     = 255.98
#define SQD  16
#define SQR_TAB_SIZE (SQF*SQF*2/SQD) // ~ 4096

static unsigned char _sqrt_table[SQR_TAB_SIZE];

#define DITH_TABLE_SIZE (256+8)
static int bits_per_pixel = -1;
static unsigned char _dith_r_table[DITH_TABLE_SIZE];
static unsigned char _dith_g_table[DITH_TABLE_SIZE];
static unsigned char _dith_b_table[DITH_TABLE_SIZE];
static unsigned char add_r[16], add_g[16], add_b[16];

static bool option_dither;
static bool option_070;
#define BBP 4 // bytes per pixel

// -------------------------------------
struct bimage
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
    unsigned char *xtab;
    unsigned char *ytab;
    unsigned char pixels[1];
};

// -------------------------------------
static unsigned long bsqrt(unsigned long x)
{
    unsigned long q, r;
    if (x < 2) return x;
    for (r = x >> 1; q = x / r, q < r; r = (r + q) >> 1);
    return r;
}

static void init_sqrt(void)
{
    int i;
    for (i = 0; i < SQR_TAB_SIZE; i++)
    {
        // backwards compatible to the elliptic inconsistency in
        // the original gradient code: sqrt(i/2) instead of sqrt(i)
        _sqrt_table[i] = (unsigned char)(SQR - 1 - bsqrt(i*SQD/2));
    }
}

inline static int isgn(int x)
{
    return x>0 ? 1 : x<0 ? -1 : 0;
}

static int _imin(int a, int b)
{
    return a<b?a:b;
}

static void init_dither_tables(void)
{
    HDC hdc;
    int red_bits, green_bits, blue_bits;
    int i, dr, dg, db;

    static const unsigned char dither4[16] =
    {
      7, 3, 6, 2,  // 3 1 3 1
      1, 5, 0, 4,  // 0 2 0 2
      6, 2, 7, 3,  // 3 1 3 1
      0, 4, 1, 5   // 0 2 0 2
    };

    hdc = GetDC(NULL);
    bits_per_pixel = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    //dbg_printf("bits_per_pixel %d", bits_per_pixel);

    if (bits_per_pixel > 16 || bits_per_pixel < 8)
    {
        option_dither = false;
        bits_per_pixel = 0;
        return;
    }

    // hardcoded for 16 bit display: red:5, green:6, blue:5
    red_bits    = 1 << (8 - 5);
    green_bits  = 1 << (8 - 6);
    blue_bits   = 1 << (8 - 5);
    for (i = 0; i < DITH_TABLE_SIZE; i++)
    {
        _dith_r_table[i] = (unsigned char)_imin(255, i & -red_bits    );
        _dith_g_table[i] = (unsigned char)_imin(255, i & -green_bits  );
        _dith_b_table[i] = (unsigned char)_imin(255, i & -blue_bits   );
    }

    dr = 8/red_bits, dg = 8/green_bits, db = 8/blue_bits;
    for (i = 0; i < 16; i++)
    {
      int d = dither4[i];
      add_r[i] = (unsigned char)(d / dr);
      add_g[i] = (unsigned char)(d / dg);
      add_b[i] = (unsigned char)(d / db);
    }
}

//===========================================================================
// ColorDither for 16bit displays.

// Original comment from the authors of bb4*nix:
// "algorithm: ordered dithering... many many thanks to rasterman
//  (raster@rasterman.com) for telling me about this... portions of
//  this code is based off of his code in Imlib"

static void TrueColorDither(struct bimage *bi)
{
  unsigned char *p = bi->pixels;
  int x, y, w = bi->width, h = bi->height;
  for (y = 0; y < h; y++) {
    int oy = 4 * (y & 3);
    for (x = 0; x < w; x++) {
      int ox = oy + (x & 3);
      p[0] = _dith_b_table[p[0] + add_b[ox]];
      p[1] = _dith_g_table[p[1] + add_g[ox]];
      p[2] = _dith_r_table[p[2] + add_r[ox]];
      p+=BBP;
    }
  }
}

//===========================================================================
// brightness delta for bevels and interlaced in percent

static int delta_bevel[2]           = { 82, 136 }; // originally 75/150%
static int delta_bevel_corner[2]    = { 70, 160 };
static int delta_interlace[2]       = { 82, 110 }; // originally 75/112%

inline static unsigned char trans_late(int i, int d)
{
    int r = i*d/100;
    return r > 255 ? 255 : r;
}

static void make_delta_table(struct bimage *bi, int *m, bool inv)
{
    int i = 0;
    do {
        bi->dark_table[i] = trans_late(i, m[inv]);
        bi->lite_table[i] = trans_late(i, m[!inv]);
    } while (++i < 256);
}

/* make a single pixel darker or lighter */
static void modify_pixel(unsigned char *pixel, int *m, bool inv)
{
    int n = 2;
    do {
        pixel[n] = trans_late(pixel[n], m[!inv]);
    } while (n--);
}

inline static void lighter(struct bimage *bi, unsigned char *pixel)
{
    pixel[0] = bi->lite_table[pixel[0]];
    pixel[1] = bi->lite_table[pixel[1]];
    pixel[2] = bi->lite_table[pixel[2]];
}

inline static void darker(struct bimage *bi, unsigned char *pixel)
{
    pixel[0] = bi->dark_table[pixel[0]];
    pixel[1] = bi->dark_table[pixel[1]];
    pixel[2] = bi->dark_table[pixel[2]];
}

/* draw the bevel along the edges */
static void bevel(struct bimage *bi, bool sunken, int pos)
{
    int w, h, nx, ny;
    unsigned char *p; int d, e, n;

    if (--pos < 0)
        return;

    w = bi->width, h = bi->height;
    nx = w - 2*pos - 1;
    ny = h - 2*pos - 1;
    if (nx <= 0 || ny <= 0)
        return;

    make_delta_table (bi, delta_bevel, sunken);

    p = bi->pixels + (pos + w * pos) * BBP;

    d = ny * w * BBP; e = BBP; n = nx;
    while (--n) { p += e; darker(bi, p); lighter(bi, p+d); }
    p += e;
    modify_pixel(p, delta_bevel_corner, !sunken);  // bottom-right corner pixel

    d = nx * BBP; e = w * BBP; n = ny;
    while (--n) { p += e; darker(bi, p); lighter(bi, p-d); }
    p += e;
    modify_pixel(p-d, delta_bevel_corner, sunken); // top-left corner pixel
}

inline static void table_fn(struct bimage *bi, unsigned char *p, int length, bool invert)
{
    unsigned char *c = p;
    int i, end, d;

    if (invert)
        i = length-1, d = end = -1;
    else
        i = 0, d = 1, end = length;

    while (i != end) {
        c[0] = (unsigned char)(bi->from_blue  + bi->diff_blue  * i / length);
        c[1] = (unsigned char)(bi->from_green + bi->diff_green * i / length);
        c[2] = (unsigned char)(bi->from_red   + bi->diff_red   * i / length);
        c[3] = (unsigned char)BI_HIBITS;
        c += BBP;
        i += d;
    }
}

/* BlackboxZero 1.5.2012 */
int iminmax(int a, int b, int c)
{
    if (a>c) a=c;
    if (a<b) a=b;
    return a;
}

/* BlackboxZero 1.5.2012 */
inline static void table_fn_mirror(struct bimage *bi, unsigned char *p, int length, bool invert)
{
	unsigned char *c = p;
	int i, end, d, v;

	if (invert)
        i = length-1, d = end = -1;
    else
        i = 0, d = 1, end = (length);

	while (i != end)
	{
		v = ( i <= (length /2)) ? (i * 2) : ((length - i) * 2);
        c[0] = (unsigned char)iminmax((bi->from_blue  + bi->diff_blue  * v / (length)), 0, 255);
        c[1] = (unsigned char)iminmax((bi->from_green + bi->diff_green * v / (length)), 0, 255);
        c[2] = (unsigned char)iminmax((bi->from_red   + bi->diff_red   * v / (length)), 0, 255);
        c[3] = (unsigned char)BI_HIBITS;
        c += BBP;
        i += d;
	}
}

inline unsigned char tobyte(int v) {
    return ((v > 255) ? 255 : ((v < 0) ? 0 : v));
}

/**
 * creates a gradient with 2 colors
 * color1 -> color2 (from top to bottom)
 * @author eliteforce
**/
inline void create_gradient(struct bimage *bi, unsigned char *p, int length, COLORREF color1, COLORREF color2) {
	unsigned char *c = p;
	int r, g, b;
	int dr, dg, db;

	r = GetRValue(color2);
	g = GetGValue(color2);
	b = GetBValue(color2);
	dr = GetRValue(color1) - r;
	dg = GetGValue(color1) - g;
	db = GetBValue(color1) - b;

	int divlength = ((length > 2) ? (length - 1) : length);
	for (int i = 0; i < length; ++i, c += BBP) {
		c[0] = tobyte(b + db * i / divlength);
		c[1] = tobyte(g + dg * i / divlength);
		c[2] = tobyte(r + dr * i / divlength);
		c[3] = (unsigned char)BI_HIBITS;
	}
}

/**
 * creates a gradient with 4 colors
 * color1 -> color2, color3 -> color4 (from top to bottom)
 * @author eliteforce
**/
inline static void table_fn_split(struct bimage *bi, unsigned char *p, int length, COLORREF color1, COLORREF color2, COLORREF color3, COLORREF color4, bool invert) {
    int halfheight = length / 2;
	if (length % 2 != 0) ++halfheight;

	if ( invert ) {
		create_gradient(bi, p, halfheight, color3, color4);
		create_gradient(bi, p + halfheight * 4, length - halfheight, color1, color2);
	} else {
		/* BlackboxZero 1.7.2012
		** Rather that redo the math in create_gradient,
		** we're just going to swap colors. */
		create_gradient(bi, p, halfheight, color2, color1);
		create_gradient(bi, p + halfheight * 4, length - halfheight, color4, color3);
	}
}

inline static void diag_fn(struct bimage *bi, unsigned char *c, int x, int y)
{
    unsigned char *xp = bi->xtab + x*BBP;
    unsigned char *yp = bi->ytab + y*BBP;
    c[0] = (unsigned char)(((unsigned)xp[0] + (unsigned)yp[0]) >> 1);
    c[1] = (unsigned char)(((unsigned)xp[1] + (unsigned)yp[1]) >> 1);
    c[2] = (unsigned char)(((unsigned)xp[2] + (unsigned)yp[2]) >> 1);
    c[3] = BI_HIBITS;
}

inline static void rect_fn(struct bimage *bi, unsigned char *c, int x, int y)
{
    if (bi->alternativ ^ (x*bi->height <= y*bi->width))
        *(unsigned long*)c = ((unsigned long*)bi->xtab)[x];
    else
        *(unsigned long*)c = ((unsigned long*)bi->ytab)[y];
}

inline static void elli_fn(struct bimage *bi, unsigned char *c, int x, int y)
{
    int dx = SQF - 1 - SQF * x / bi->width;
    int dy = SQF - 1 - SQF * y / bi->height;
    int f = _sqrt_table[(dx*dx + dy*dy) / SQD];
    *(unsigned long *)c = ((unsigned long*)bi->xtab)[f];
}

// -------------------------------------
struct bimage *bimage_create(int width, int height,  StyleItem *si)
{
    int byte_size;
    int table_size;
    bool sunken, interlaced;

    unsigned long *s, *d, c, *e, *f;
    int x, y, z, i;
    unsigned char r2, g2, b2;
    unsigned char *p;

    COLORREF color_from, color_to, cr_tmp;
    int type;

    struct bimage *bi = NULL;

    if (width < 2 && ++width < 2)
        return bi;
    if (height < 2 && ++height < 2)
        return bi;

    byte_size = (width * height) * BBP;
    table_size = width + height;
    if (table_size < SQR)
        table_size = SQR;

    bi = (struct bimage *)malloc(
        sizeof(struct bimage)-1 + byte_size + table_size * BBP
        );

    if (NULL == bi)
        return bi;

    bi->width = width;
    bi->height = height;
    bi->xtab = bi->pixels + byte_size;
    bi->ytab = bi->xtab + width * BBP;

    color_from = si->Color;
    color_to = si->ColorTo;
    type = si->type;
    sunken = si->bevelstyle == BEVEL_SUNKEN;
    interlaced = si->interlaced;

    if (false == option_070) {
        // backwards compatibility stuff
        if (type == B_SOLID && interlaced) {
            color_to = color_from;
            type = B_HORIZONTAL;
        }
        if (sunken && type >= B_PIPECROSS && type <= B_PYRAMID) {
            cr_tmp = color_to, color_to = color_from, color_from = cr_tmp;
        }
    }

    i = (b2 = GetBValue(color_to)) - (bi->from_blue = GetBValue(color_from));
    bi->diff_blue = i + isgn(i);
    i = (g2 = GetGValue(color_to)) - (bi->from_green = GetGValue(color_from));
    bi->diff_green = i + isgn(i);
    i = (r2 = GetRValue(color_to)) - (bi->from_red = GetRValue(color_from));
    bi->diff_red = i + isgn(i);

    if (interlaced && B_SOLID != type)
        make_delta_table(bi, delta_interlace, sunken != (0 == (height & 1)));

    p = bi->pixels;
    bi->alternativ = false;

    switch (type)
    {
        // -------------------------------------
        default:
        case B_SOLID:
        {
            union {
                unsigned char b[4];
                unsigned long c;
            } v;
            unsigned long c1, c2;
            v.b[0] = (unsigned char)bi->from_blue ,
            v.b[1] = (unsigned char)bi->from_green,
            v.b[2] = (unsigned char)bi->from_red  ,
            v.b[3] = (unsigned char)BI_HIBITS;
            c1 = c2 = v.c;
            if (interlaced) {
                v.b[0] = b2, v.b[1] = g2, v.b[2] = r2;
                c2 = v.c;;
            }

            // draw 2 lines, to cover the 'interlaced' case
            y = 0; do {
                c = (height & 1) == y ? c2 : c1;
                x = 0; do {
                    *(unsigned long*)p = c;
                    p+=BBP;
                } while (++x < width);
            } while (++y < 2);
            goto copy_lines;
        }

		case B_MIRROR_HORIZONTAL:
		case B_MIRRORHORIZONTAL:
			table_fn_mirror(bi, bi->xtab, width, false);

			// draw 2 lines, to cover the 'interlaced' case
            y = 0;
			do {
                x = 0;
				s = (unsigned long *)bi->xtab;
				do {
                    *(unsigned long*)p = *s++;
                    if (interlaced) {
                        if (1 & y)
							darker(bi, p);
                        else
							lighter(bi, p);
                    }
                    p+=BBP;
                } while (++x < width);
            } while (++y < 2);

            // copy down the lines
        //copy_lines:
            d = (unsigned long*)p;
            while (y < height) {
                s = (unsigned long*)bi->pixels + (y&1)*width;
                memcpy(d, s, width*BBP);
                d += width; y++;
            }
            break;

		case B_SPLITHORIZONTAL:
		case B_SPLIT_HORIZONTAL:
			// Vertical gradient. Color1 | Color2
			table_fn_split(bi, bi->xtab, width, si->ColorSplitTo, si->Color, si->ColorTo, si->ColorToSplitTo, false);
			//table_fn_split(ytab, height, false);

			// draw 2 lines, to cover the 'interlaced' case
            y = 0;
			do {
                x = 0;
				s = (unsigned long *)bi->xtab;
				do {
                    *(unsigned long*)p = *s++;
                    if (interlaced) {
                        if (1 & y)
							darker(bi, p);
                        else
							lighter(bi, p);
                    }
                    p+=BBP;
                } while (++x < width);
            } while (++y < 2);

            // copy down the lines
        //copy_lines:
            d = (unsigned long*)p;
            while (y < height) {
                s = (unsigned long*)bi->pixels + (y&1)*width;
                memcpy(d, s, width*BBP);
                d += width; y++;
            }
            break;

        case B_HORIZONTAL:
            table_fn(bi, bi->xtab, width, false);
            // draw 2 lines, to cover the 'interlaced' case
            y = 0;
			do {
                x = 0;
				s = (unsigned long *)bi->xtab;
				do {
                    *(unsigned long*)p = *s++;
                    if (interlaced) {
                        if (1 & y)
							darker(bi, p);
                        else
							lighter(bi, p);
                    }
                    p+=BBP;
                } while (++x < width);
            } while (++y < 2);

            // copy down the lines
        copy_lines:
            d = (unsigned long*)p;
            while (y < height) {
                s = (unsigned long*)bi->pixels + (y&1)*width;
                memcpy(d, s, width*BBP);
                d += width; y++;
            }
            break;

        // -------------------------------------
		case B_MIRRORVERTICAL:
		case B_MIRROR_VERTICAL:
			table_fn_mirror(bi, bi->ytab, height, true);
			// draw 1 column
            y = 0;
			s = (unsigned long *)bi->ytab;
			z = width*BBP;
            do {
                *(unsigned long*)p = *s++;
                if (interlaced) {
                    if (1 & y)
						darker(bi, p);
                    else
						lighter(bi, p);
                }
                p += z;
            } while (++y < height);

            // copy colums
            s = (unsigned long*)bi->pixels;
            y = 0;
			do {
                d = s, s += width;
				c = *d++;
                do *d = c;
				while (++d<s);
            } while (++y < height);
            break;

		case B_SPLITVERTICAL:
		case B_SPLIT_VERTICAL:
			// Vertical gradient. Color1 | Color2
			table_fn_split(bi, bi->ytab, height, si->ColorSplitTo, si->Color, si->ColorTo, si->ColorToSplitTo, true);

			// draw 1 column
			y = 0;
			s = (unsigned long *)bi->ytab;
			z = width*BBP;
			do {
				*(unsigned long*)p = *s++;
				if (interlaced) {
					if (1 & y)
						darker(bi, p);
					else
						lighter(bi, p);
				}
				p += z;
			} while (++y < height);

			// copy colums
			s = (unsigned long*)bi->pixels;
			y = 0;
			do {
				d = s, s += width;
				c = *d++;
				do *d = c;
				while (++d<s);
			} while (++y < height);
			break;

        case B_VERTICAL:
            table_fn(bi, bi->ytab, height, true);

            // draw 1 column
            y = 0; s = (unsigned long *)bi->ytab; z = width*BBP;
            do {
                *(unsigned long*)p = *s++;
                if (interlaced) {
                    if (1 & y) darker(bi, p);
                    else lighter(bi, p);
                }
                p += z;
            } while (++y < height);

            // copy colums
            s = (unsigned long*)bi->pixels;
            y = 0; do {
                d = s, s += width; c = *d++;
                do *d = c; while (++d<s);
            } while (++y < height);
            break;

        // -------------------------------------
        case B_CROSSDIAGONAL:
            table_fn(bi, bi->xtab, width, true);
            goto diag;
        case B_DIAGONAL:
            table_fn(bi, bi->xtab, width, false);
        diag:
            table_fn(bi, bi->ytab, height, true);
            y = 0; do {
                x = 0; do {
                    diag_fn(bi, p, x, y);
                    if (interlaced) {
                        if (1 & y) darker(bi, p);
                        else lighter(bi, p);
                    }
                    p+=BBP;
                } while (++x < width);
            } while (++y < height);
            break;

        // -------------------------------------
        case B_PIPECROSS:
            bi->alternativ = true;
        case B_RECTANGLE:
        case B_PYRAMID:
            table_fn(bi, bi->xtab, width, false);
            table_fn(bi, bi->ytab, height, false);
            goto draw_quadrant;

        case B_ELLIPTIC:
            if (0 == _sqrt_table[0])
                init_sqrt();
            table_fn(bi, bi->xtab, SQR, false);
            goto draw_quadrant;

        draw_quadrant:
            // one quadrant is drawn, and mirrored horizontally and vertically
            y = 0;
            s = (unsigned long*)p + height*width;
            z = height;
            do {
                x = 0;
                d = (unsigned long*)p + width;
                f = s; s -= width; e = s; z--;
                do {
                    if (B_ELLIPTIC == type)
                        elli_fn(bi, p, x, y);
                    else
                    if (B_PYRAMID == type)
                        diag_fn(bi, p, x, y);
                    else
                        rect_fn(bi, p, x, y);

                    c = *(unsigned long *)p;
                    if (interlaced) {
                        if (2 & y) darker(bi, p);
                        else lighter(bi, p);
                    }
                    *--d = *(unsigned long *)p;

                    if (e != (unsigned long *)p) {
                        *e = c;
                        if (interlaced) {
                            if (1 & z) darker(bi, (unsigned char*)e);
                            else lighter(bi, (unsigned char*)e);
                        }
                        *--f = *e;
                    }
                    e++;
                    p+=BBP;
                } while ((x+=2) < width);
                p+=(width/2)*BBP;
            } while ((y+=2) < height);
            break;
    }
    if (si->bevelstyle != BEVEL_FLAT)
        bevel(bi, sunken, si->bevelposition);
    if (option_dither)
        TrueColorDither(bi);
    return bi;
}

static void setup_bmiHeader(struct bimage *bi, BITMAPINFOHEADER *bmiHeader)
{
    memset(bmiHeader, 0, sizeof(*bmiHeader));
    bmiHeader->biSize = sizeof(*bmiHeader);
    bmiHeader->biWidth = bi->width;
    bmiHeader->biHeight = bi->height;
    bmiHeader->biPlanes = 1;
    bmiHeader->biBitCount = 32;
    bmiHeader->biCompression = BI_RGB;
}

static HBITMAP create_bmp(struct bimage *bi)
{
    BITMAPINFOHEADER bmiHeader;
    HBITMAP bmp;
    void *p = NULL;
    if (NULL == bi)
        return NULL;
    setup_bmiHeader(bi, &bmiHeader);
    bmp = CreateDIBSection(NULL, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS, (void**)&p, NULL, 0);
    if (bmp && p)
        memcpy(p, bi->pixels, bi->width * bi->height * BBP);
    return bmp;
}

static void copy_to_hdc(struct bimage *bi, HDC hdc, int px, int py, int w, int h)
{
    BITMAPINFOHEADER bmiHeader;
    if (NULL == bi)
        return;
    setup_bmiHeader(bi, &bmiHeader);
    SetDIBitsToDevice(hdc, px, py, w, h, 0, 0, 0, h, bi->pixels, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS);
}

BYTE *bimage_getpixels(struct bimage *bi)
{
    return bi ? bi->pixels : NULL;
}

void bimage_destroy(struct bimage *bi)
{
    free(bi);
}

void bimage_init(bool dither, bool is_070)
{
    option_dither = dither;
    option_070 = is_070;
    init_dither_tables();
}

//===========================================================================
// API: CreateBorder
//===========================================================================

void CreateBorder(HDC hdc, RECT *rp, COLORREF borderColor, int borderWidth)
{
    if (borderWidth) {
        int x = rp->left;
        int y = rp->top;
        int w = rp->right;
        int h = rp->bottom;
        HGDIOBJ otherPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, borderColor));
        do {
            w--; h--;
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, w, y );
            LineTo(hdc, w, h);
            LineTo(hdc, x, h);
            LineTo(hdc, x, y);
            x++; y++;
        } while (--borderWidth);
        DeleteObject(SelectObject(hdc, otherPen));
    }
}

//===========================================================================
// API: MakeStyleGradient
// Purpose:  Make a gradient from style Item
//===========================================================================
void MakeStyleGradient(HDC hdc, RECT *rp, StyleItem *pSI, bool withBorder)
{
    int x, y, w, h, b;
    struct bimage *bi;

    x = rp->left;
    y = rp->top;
    w = rp->right;
    h = rp->bottom;
    b = pSI->borderWidth;

    if (b && withBorder) {
        CreateBorder(hdc, rp, pSI->borderColor, b);
        x += b, y += b, w -= b, h -= b;
    }

    if (false == pSI->parentRelative) {
        w -= x;
        h -= y;
        bi = bimage_create(w, h, pSI);
        copy_to_hdc(bi, hdc, x, y, w, h);
        bimage_destroy(bi);
    }
}

//===========================================================================
// API: MakeGradient
// Purpose: creates a gradient and fills it with the specified options
// In: HDC = handle of the device context to fill
// In: RECT = rect you wish to fill
// In: int = type of gradient (eg. B_HORIZONTAL)
// In: COLORREF = first color to use
// In: COLORREF = second color to use
// In: bool = whether to interlace (darken every other line)
// In: int = style of bevel (eg. BEVEL_FLAT)
// In: int = position of bevel (eg. BEVEL1)
// In: int = not used
// In: COLORREF = border color
// In: int = width of border around bitmap
//===========================================================================

void MakeGradient(HDC hdc, RECT rect, int type, COLORREF Color, COLORREF ColorTo,
    bool interlaced, int bevelstyle,
    int bevelposition, int bevelWidth, COLORREF borderColor, int borderWidth)
{
    StyleItem si;

    si.bevelstyle = bevelstyle;
    si.bevelposition = bevelposition;
    si.type = type;
    si.parentRelative = false;
    si.interlaced = interlaced;
    si.Color = Color;
    si.ColorTo = ColorTo;
    si.borderColor = borderColor;
    si.borderWidth = borderWidth;

    MakeStyleGradient(hdc, &rect, &si, true);
}

/* BlackboxZero 1.5.2012 */
void MakeGradientEx(HDC hDC, RECT rect, int type, COLORREF colour_from, COLORREF colour_to, 
					COLORREF colour_from_splitto, COLORREF colour_to_splitto, bool interlaced, int bevelStyle,
					int bevelPosition, int bevelWidth, COLORREF borderColor, int borderWidth)
{

    if (type >= 0)
    {
        bool is_vertical	= false;
        bool is_horizontal	= false;
        bool is_split		= false;
		COLORREF color0 = colour_from;
		COLORREF color1 = colour_from;
		COLORREF color2 = colour_to;
		COLORREF color3 = colour_to;

		int	rect_diff = 0;
		RECT rect2;

		rect2.top = rect.top;
		rect2.bottom = rect.bottom;
		rect2.left = rect.left;
		rect2.right = rect.right;

		switch (type) {
			case B_SPLITVERTICAL:
			case B_SPLIT_VERTICAL:
				is_vertical = is_split = true;
				break;
				
			case B_SPLITHORIZONTAL:
			case B_SPLIT_HORIZONTAL:
				is_horizontal = is_split = true;
				break;	
		}

        if ( is_vertical || is_horizontal || is_split ){

            if (is_vertical){
                type = B_VERTICAL;
            }
            if (is_horizontal){
                type = B_HORIZONTAL;
            }

            //if ( is_split ) {//Should always be split to be here...
                color0 = colour_from_splitto;
                color3 = colour_to_splitto;
            //}

			//Split the rect
			if ( is_vertical ) {
				rect_diff = (rect.bottom - rect.top) / 2;
				rect2.top = rect.bottom - rect_diff;
				rect.bottom = rect.top + rect_diff;
			} else { //Has to be horizontal
				rect_diff = (rect.right - rect.left) / 2;
				rect2.right = rect.left - rect_diff;
				rect.left = rect.right + rect_diff;
			}
			//Draw first half
			MakeGradient(hDC, rect, type, color0, color1, interlaced, bevelStyle, bevelPosition, bevelWidth, borderColor, borderWidth);
			//Draw second half
			MakeGradient(hDC, rect2, type, color2, color3, interlaced, bevelStyle, bevelPosition, bevelWidth, borderColor, borderWidth);
        } else{
			/* BlackboxZero 1.6.2012 */
			//Just pass it along to the normal routine since we don't need 4 colors.
            MakeGradient(hDC, rect, type, colour_from, colour_to, interlaced, bevelStyle, bevelPosition, bevelWidth, borderColor, borderWidth);
        }
    }
}

//===========================================================================
// API: MakeGradientBitmap
//===========================================================================

HBITMAP MakeGradientBitmap(int width, int height, StyleItem *pSI)
{
    struct bimage *bi;
    HBITMAP bmp;

    bi = bimage_create(width, height, pSI);
    bmp = create_bmp(bi);
    bimage_destroy(bi);
    return bmp;
}

//===========================================================================
