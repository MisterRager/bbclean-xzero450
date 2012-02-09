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

#include "../../blackbox/BBApi.h"

static void perform_satnhue(BYTE *pixels, int size_x, int size_y, int saturationValue, int hueIntensity)
{
    unsigned sat = (unsigned char )saturationValue;
    unsigned hue = (unsigned char )hueIntensity;
    unsigned i_sat = 255 - sat;
    unsigned i_hue = 255 - hue;

    BYTE* mask, *icon, *back;
    mask = pixels;
    unsigned next_ico = size_x * size_y * 4;
    int y = size_y;
    do {
        int x = size_x;
        do {
            if (0 == *mask)
            {
                back = (icon = mask + next_ico) + next_ico;
                unsigned r = icon[2];
                unsigned g = icon[1];
                unsigned b = icon[0];
                if (sat<255)
                {
                    unsigned greyval = (r*79 + g*156 + b*21) * i_sat / 256;
                    r = (r * sat + greyval) / 256;
                    g = (g * sat + greyval) / 256;
                    b = (b * sat + greyval) / 256;
                }
                if (hue)
                {
                    r = (r * i_hue + back[2] * hue) / 256;
                    g = (g * i_hue + back[1] * hue) / 256;
                    b = (b * i_hue + back[0] * hue) / 256;
                }
                back[2] = (unsigned char )r;
                back[1] = (unsigned char )g;
                back[0] = (unsigned char )b;
            }
            mask += 4;
        }
        while (--x);
    }
    while (--y);
}

void DrawIconSatnHue (
    HDC hDC,
    int px,
    int py,
    HICON IconHop,
    int size_x,
    int size_y,
    UINT istepIfAniCur, // index of frame in animated cursor
    HBRUSH hbrFlickerFreeDraw,  // handle to background brush
    UINT diFlags, // icon-drawing flags
    int apply_satnhue,
    int saturationValue,
    int hueIntensity
    )
{
    if (0 == apply_satnhue || (saturationValue >= 255 && hueIntensity <= 0)) {
        DrawIconEx(hDC, px, py, IconHop, size_x, size_y, istepIfAniCur, hbrFlickerFreeDraw, diFlags);
        return;
    }

    BITMAPINFOHEADER bv4info;

    ZeroMemory(&bv4info,sizeof(bv4info));
    bv4info.biSize = sizeof(bv4info);
    bv4info.biWidth = size_x;
    bv4info.biHeight = size_y * 3;
    bv4info.biPlanes = 1;
    bv4info.biBitCount = 32;
    bv4info.biCompression = BI_RGB;

    BYTE* pixels;

    HBITMAP bufbmp = CreateDIBSection(NULL, (BITMAPINFO*)&bv4info, DIB_RGB_COLORS, (PVOID*)&pixels, NULL, 0);

    if (NULL == bufbmp)
        return;

    HDC bufdc = CreateCompatibleDC(hDC);
    HGDIOBJ other = SelectObject(bufdc, bufbmp);

    // draw the required three things side by side

    // background for hue
    BitBlt(bufdc,       0, 0,       size_x, size_y, hDC, px, py, SRCCOPY);

    // background for icon
    BitBlt(bufdc,       0, size_y,    size_x, size_y, hDC, px, py, SRCCOPY);

    // icon, in colors
    DrawIconEx(bufdc,   0, size_y,    IconHop, size_x, size_y, 0, NULL, DI_NORMAL);

    // icon mask
    DrawIconEx(bufdc,   0, size_y*2,  IconHop, size_x, size_y, 0, NULL, DI_MASK);

    perform_satnhue(pixels, size_x, size_y, saturationValue, hueIntensity);

    BitBlt(hDC, px, py, size_x, size_y, bufdc, 0, 0, SRCCOPY);

    DeleteObject(SelectObject(bufdc, other));
    DeleteDC(bufdc);
}

//===========================================================================
