/*
 ============================================================================

	Simple Label BarExtender for bbLeanBar 1.16 (bbClean)

 ============================================================================

	This file is part of the bbLean source code.

	Copyright © 2007 noccy
	http://dev.noccy.com/bbclean

	bbClean is free software, released under the GNU General Public License
	(GPL version 2 or later).

	http://www.fsf.org/licenses/gpl.html

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

 ============================================================================
*/

#include "BBApi.h"

#include <windows.h>
#include <wingdi.h>
#include "labelctl.h"
#include "initparser.h"

const char* g_szClassName = "barextender_label";

void GetStyleSettings(void);

//
//  local variables
//
struct style_info
{
	StyleItem Frame;
	int bevelWidth;
	int borderWidth;
	COLORREF borderColor;
} style_info;

struct extender_properties
{
	// settings
	int xpos, ypos;
	int width, height;
	bool drawBorder;

	// our plugin window
	HWND hwnd;

	// GDI objects
	HBITMAP bufbmp;
	HFONT hFont;

	// the text
	char window_text[100];

} my;

//
//  Constructor
//
barLabel::barLabel()
{
	// Create our new empty window here
	this->hWndLabel = this->CreateExtenderWindow();
}

//
//  Destructor
//
barLabel::~barLabel()
{
	// And destroy it here
	if (this->hWndLabel) DestroyWindow(this->hWndLabel);
}

//
//  Initialize method. Accepted initstring options are:
//
//    text='...'		Sets the label caption
//    onclick='...'		Sets the onclick action
//    width=...			Width in pixels (or 'auto')
//	  id='...'			ID of control (for broams)
//	  style='...'		Style to use (eg. 'button')
//
bool barLabel::Initialize(const char* configString)
{
	// It's ok to throw away unknown items. This shouldn't cause a failure
	// unless it's a several mistake. The extenders should be as forgiving
	// as possible.
	argParser parser;
	parser.ParseString(configString);

	// Now pull the data that we want...
	this->barCaption =  parser.GetArgumentEx("text","bbClean");
	this->barOnClick =  parser.GetArgumentEx("onclick","@bbCore.ShowMenu");
	this->barWidth =    parser.GetArgumentEx("width","100");
	this->barID =       parser.GetArgumentEx("id","");
	this->barStyle =    parser.GetArgumentEx("style","toolbar");

	// And return true.
	return(true);
}

HWND barLabel::getLabelHandle()
{
	return(this->hWndLabel);
}

HWND barLabel::CreateExtenderWindow()
{
    WNDCLASSEX wc;
    HWND hwnd;

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = 0;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    if(!RegisterClassEx(&wc)) return(0);

    hwnd = CreateWindowEx(
        0,
        g_szClassName,
        "Extender Label",
        0,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, 0, NULL);
    if(hwnd == NULL) return(0);

	// Window shouldn't be displayed by the plugin, leave this to the leanbar
    /*
    ShowWindow(hwnd, 1);
    UpdateWindow(hwnd);
    */

    return(hwnd);
}

/*
	Window Callback. Couldn't make this work while in the actual class
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	HDC buf;

    switch(msg)
    {
		//
		//  On creation, fetch the style settings
		//
		case WM_CREATE:
			GetStyleSettings();
			break;

		//
		//  Paint routine
		//
        case WM_PAINT:
			PAINTSTRUCT ps;
			hdc = BeginPaint(hwnd, &ps);

			// create a DC for the buffer
			buf = CreateCompatibleDC(hdc);
			HGDIOBJ otherbmp;

			if (NULL == my.bufbmp) // No bitmap yet?
			{
				// Generate it and paint everything
				my.bufbmp = CreateCompatibleBitmap(hdc, my.width, my.height);

				// Select it into the DC, storing the previous default.
				otherbmp = SelectObject(buf, my.bufbmp);

				// Setup the rectangle
				RECT r; r.left = r.top = 0; r.right = my.width; r.bottom =  my.height;

				// and draw the frame
				MakeStyleGradient(buf, &r, &style_info.Frame, my.drawBorder);

				if (my.window_text[0])
				{
					// Set the font, storing the default..
					HGDIOBJ otherfont = SelectObject(buf, my.hFont);

					SetTextColor(buf, style_info.Frame.TextColor);
					SetBkMode(buf, TRANSPARENT);

					// adjust the rectangle
					int margin = style_info.bevelWidth;
					if (my.drawBorder) margin += style_info.borderWidth;
					r.left  += margin;
					r.top   += margin;
					r.right -= margin;
					r.bottom -= margin;

					// draw the text
					DrawText(buf, my.window_text, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

					// Put back the previous default font.
					SelectObject(buf, otherfont);
				}
			}
			else
			{
				// Otherwise it has been painted previously,
				// so just select it into the DC
				otherbmp = SelectObject(buf, my.bufbmp);
			}

			// ... and copy the buffer on the screen:
			BitBlt(hdc,
				ps.rcPaint.left,
				ps.rcPaint.top,
				ps.rcPaint.right  - ps.rcPaint.left,
				ps.rcPaint.bottom - ps.rcPaint.top,
				buf,
				ps.rcPaint.left,
				ps.rcPaint.top,
				SRCCOPY
				);

			// Put back the previous default bitmap
			SelectObject(buf, otherbmp);

			// clean up
			DeleteDC(buf);

			// Done.
			EndPaint(hwnd, &ps);
			break;

		//
		//  OnClick - Do something user defined here ;)
		//
		case WM_LBUTTONUP:
			// Invoke the onclick event here
			break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


void GetStyleSettings(void)
{
	style_info.Frame = *(StyleItem *)GetSettingPtr(SN_TOOLBAR);
	if (false == (style_info.Frame.validated & VALID_TEXTCOLOR))
		style_info.Frame.TextColor = ((StyleItem *)GetSettingPtr(SN_TOOLBARLABEL))->TextColor;

	style_info.bevelWidth   = *(int*)GetSettingPtr(SN_BEVELWIDTH);
	style_info.borderWidth  = *(int*)GetSettingPtr(SN_BORDERWIDTH);
	style_info.borderColor  = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);

	if (my.hFont) DeleteObject(my.hFont);
	my.hFont = CreateStyleFont((StyleItem *)GetSettingPtr(SN_TOOLBAR));
}
