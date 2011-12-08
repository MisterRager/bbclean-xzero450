#include <windows.h>

//===========================================================================
// Function: drawIco

static void perform_satnhue(BYTE *pixels, int size, int saturationValue, int hueIntensity)
{
	unsigned sat = (unsigned char )saturationValue;
	unsigned hue = (unsigned char )hueIntensity;
	unsigned i_sat = 255 - sat;
	unsigned i_hue = 255 - hue;

	BYTE* mask, *icon, *back;
	mask = pixels;
	unsigned next_ico = size * size * 4;
	int y = size;
	do {
		int x = size;
		do {
			if (0 == *mask)
			{
				back = (icon = mask + next_ico) + next_ico;
				unsigned r = icon[2];
				unsigned g = icon[1];
				unsigned b = icon[0];
				if (sat<255)
				{
					unsigned greyval = (r*79 + g*156 + b*21) * i_sat / 255;
					r = (r * sat + greyval) / 255;
					g = (g * sat + greyval) / 255;
					b = (b * sat + greyval) / 255;
				}
				if (hue)
				{
					r = (r * i_hue + back[2] * hue) / 255;
					g = (g * i_hue + back[1] * hue) / 255;
					b = (b * i_hue + back[0] * hue) / 255;
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

static void drawIco (int px, int py, int size, HICON IconHop, HDC hDC, bool f, int saturationValue, int hueIntensity)
{
	if (false == f || (saturationValue >= 255 && hueIntensity <= 0))
	{
		DrawIconEx(hDC, px, py, IconHop, size, size, 0, NULL, DI_NORMAL);
		return;
	}

	BITMAPINFOHEADER bv4info;

	ZeroMemory(&bv4info,sizeof(bv4info));
	bv4info.biSize = sizeof(bv4info);
	bv4info.biWidth = size;
	bv4info.biHeight = size * 3;
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
	BitBlt(bufdc,       0, 0,       size, size, hDC, px, py, SRCCOPY);

	// background for icon
	BitBlt(bufdc,       0, size,    size, size, hDC, px, py, SRCCOPY);

	// icon, in colors
	DrawIconEx(bufdc,   0, size,    IconHop, size, size, 0, NULL, DI_NORMAL);

	// icon mask
	DrawIconEx(bufdc,   0, size*2,  IconHop, size, size, 0, NULL, DI_MASK);

	perform_satnhue(pixels, size, saturationValue, hueIntensity);

	BitBlt(hDC, px, py, size, size, bufdc, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(bufdc, other));
	DeleteDC(bufdc);
}

//===========================================================================
