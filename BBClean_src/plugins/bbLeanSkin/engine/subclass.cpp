/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2 or later) For details see:

	http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/
//#pragma comment(lib, "msimg32.lib")

#include "..\..\..\blackbox\BBApi.h"
#include "hookinfo.h"
#include "subclass.h"
#include "BImage.cpp"
#include "DrawIco.cpp"
//
//#include <wingdi.h>
//#include <windows.h>

//===========================================================================

bool isWin2kXP();

//===========================================================================

int imax(int a, int b) {
	return a>b?a:b;
}

int imin(int a, int b) {
	return a<b?a:b;
}

//===========================================================================
void get_workarea(HWND hwnd, RECT *w, RECT *s)
{
	static HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
	static BOOL     (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);

	if (NULL == pMonitorFromWindow)
	{
		HMODULE hUserDll = GetModuleHandle("USER32.DLL");
		*(FARPROC*)&pMonitorFromWindow = GetProcAddress(hUserDll, "MonitorFromWindow" );
		*(FARPROC*)&pGetMonitorInfoA = GetProcAddress(hUserDll, "GetMonitorInfoA"   );
		if (NULL == pMonitorFromWindow) *(DWORD*)&pMonitorFromWindow = 1;
	}

	if (*(DWORD*)&pMonitorFromWindow > 1)
	{
		MONITORINFO mi; mi.cbSize = sizeof(mi);
		HMONITOR hMon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		if (hMon && pGetMonitorInfoA(hMon, &mi))
		{
			if (w) *w = mi.rcWork;
			if (s) *s = mi.rcMonitor;
			return;
		}
	}

	if (w) SystemParametersInfo(SPI_GETWORKAREA, 0, w, 0);

	if (s)
	{
		s->left = s->top = 0;
		s->right = GetSystemMetrics(SM_CXSCREEN);
		s->bottom = GetSystemMetrics(SM_CYSCREEN);
	}
}

//===========================================================================
// Function: SnapWindowToEdge
// Purpose:Snaps a given windowpos at a specified distance
// In: WINDOWPOS* = WINDOWPOS recieved from WM_WINDOWPOSCHANGING
// In: int = distance to snap to
// In: bool = use screensize of workspace
// Out: void = none
//===========================================================================

void SnapWindowToEdge(WinInfo *WI, WINDOWPOS* pwPos, int nDist)
{
	RECT workArea, scrnArea; int x; int y; int z; int dx, dy, dz;

	get_workarea(pwPos->hwnd, &workArea, &scrnArea);

	int fx = WI->S.HiddenSide;
	int fy = WI->S.HiddenTop;
	int bo = WI->S.HiddenBottom;

	//if (workArea.bottom < scrnArea.bottom) bo += 4;
	//if (workArea.top > scrnArea.top) fy += 4;
	
	// top/bottom edge
	dy = y = pwPos->y + fy - workArea.top;
	dz = z = pwPos->y + pwPos->cy - bo - workArea.bottom;
	if (dy<0) dy=-dy;
	if (dz<0) dz=-dz;
	if (dz < dy) y = z, dy = dz;

	// left/right edge
	dx = x = pwPos->x + fx - workArea.left;
	dz = z = pwPos->x - fx + pwPos->cx - workArea.right;
	if (dx<0) dx=-dx;
	if (dz<0) dz=-dz;
	if (dz < dx) x = z, dx = dz;

	if(dy < nDist) pwPos->y -= y;
	if(dx < nDist) pwPos->x -= x;
}

//===========================================================================
#if 1
void get_rect(HWND hwnd, RECT *rp)
{
	GetWindowRect(hwnd, rp);
	if (WS_CHILD & GetWindowLong(hwnd, GWL_STYLE))
	{
		HWND pw = GetParent(hwnd);
		ScreenToClient(pw, (LPPOINT)&rp->left);
		ScreenToClient(pw, (LPPOINT)&rp->right);
	}
}

void set_pos(HWND hwnd, RECT rc)
{
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	SetWindowPos(hwnd, NULL,
		rc.left, rc.top, width, height,
		SWP_NOZORDER|SWP_NOACTIVATE);
}

int get_shade_height(HWND hwnd)
{
	return SendMessage(hwnd, bbSkinMsg, MSGID_GETSHADEHEIGHT, 0);
}

void ShadeWindow(HWND hwnd)
{
	RECT rc; get_rect(hwnd, &rc);
	int height = rc.bottom - rc.top;
	DWORD prop = (DWORD)GetProp(hwnd, BBSHADE_PROP);
	int h1 = LOWORD(prop);
	int h2 = HIWORD(prop);
	if (IsZoomed(hwnd))
	{
		if (h2) height = h2, h2 = 0;
		else h2 = height, height = get_shade_height(hwnd);
	}
	else
	{
		if (h1) height = h1, h1 = 0;
		else h1 = height, height = get_shade_height(hwnd);
		h2 = 0;
	}

	prop = MAKELPARAM(h1,h2);
	if (0 == prop)
		RemoveProp(hwnd, BBSHADE_PROP);
	else
		SetProp(hwnd, BBSHADE_PROP, (PVOID)prop);

	rc.bottom = rc.top + height;
	set_pos(hwnd, rc);
}

#else
//===========================================================================
void ShadeWindow(HWND hwnd)
{
	SendMessage(mSkin.BBhwnd, BB_WINDOWSHADE, 0, (LPARAM)hwnd);
}

#endif
//===========================================================================

void exec_button_action(WinInfo *WI, int n, LPARAM lParam)
{
	HWND hwnd = WI->hwnd;
	switch(n)
	{
		case btn_Close:
			if ( ( isWin2kXP() ) && (WI->exstyle & WS_EX_LAYERED) ) {
				SetWindowLong(hwnd, GWL_EXSTYLE, WI->exstyle & ~WS_EX_LAYERED);
				WI->is_focustrans = WI->is_unfocustrans = false;
			}
			PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
			break;

		case btn_Min:
			if (WI->style & WS_MINIMIZEBOX)
				if (BBVERSION_LEAN == mSkin.BBVersion)
				PostMessage(mSkin.BBhwnd, IsIconic(hwnd) ? BB_WINDOWRESTORE : BB_WINDOWMINIMIZE, 0, (LPARAM)hwnd);
				else
				PostMessage(hwnd, WM_SYSCOMMAND, IsIconic(hwnd) ? SC_RESTORE : SC_MINIMIZE, 0);
			break;

		case btn_Max:
			if (WI->style & WS_MAXIMIZEBOX)
				if (BBVERSION_LEAN == mSkin.BBVersion)
				PostMessage(mSkin.BBhwnd, IsZoomed(hwnd) ? BB_WINDOWRESTORE : BB_WINDOWMAXIMIZE, 0, (LPARAM)hwnd);
				else
				PostMessage(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
			break;

		case btn_VMax:
			if (WI->style & WS_MAXIMIZEBOX)
				PostMessage(mSkin.BBhwnd, BB_WINDOWGROWHEIGHT, 0, (LPARAM)hwnd);
			break;

		case btn_HMax:
			if (WI->style & WS_MAXIMIZEBOX)
				PostMessage(mSkin.BBhwnd, BB_WINDOWGROWWIDTH, 0, (LPARAM)hwnd);
			break;

		case btn_Lower:
			PostMessage(mSkin.BBhwnd, BB_WINDOWLOWER, 0, (LPARAM)hwnd);
			break;

		case btn_Rollup:
			if (WI->style & WS_SIZEBOX)
				ShadeWindow(hwnd);
			break;

		case btn_Sticky:
			WI->is_sticky = false == WI->is_sticky;
			PostMessage(mSkin.BBhwnd, BB_WORKSPACE,
				WI->is_sticky ? BBWS_MAKESTICKY : BBWS_CLEARSTICKY,
				(LPARAM)hwnd
				);
			break;

		case btn_OnTop:
			SetWindowPos(hwnd,
				WI->is_ontop ? HWND_NOTOPMOST : HWND_TOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			break;

		case btn_Icon:
			POINT  pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
			ClientToScreen(hwnd, &pt);
			PostMessage(hwnd, 0x0313, 0, MAKELPARAM(pt.x, pt.y));
			break;
	}
}

//-----------------------------------------------------------------
void DeleteBitmaps(WinInfo *WI, int a, int n)
{
	HGDIOBJ *pBmp = WI->gdiobjs + a;
	do {
		if (*pBmp) DeleteObject(*pBmp), *pBmp = NULL;
		pBmp++;
	} while (--n);
}

void PutGradient(WinInfo *WI, HDC hdc, RECT *rc, GradientItem *pSI)
{
	if (pSI->parentRelative)
	{
		if (pSI->borderWidth)
			CreateBorder(hdc, rc, pSI->borderColor, pSI->borderWidth);
		return;
	}

	HBITMAP bmp, *pBmp; HGDIOBJ other;
	int width = rc->right-rc->left;
	int height = rc->bottom-rc->top;

	bmp = *(pBmp = (HBITMAP*)&WI->gdiobjs[pSI - &mSkin.windowTitleFocus]);
	if (NULL == bmp)
	{
		bmp = CreateCompatibleBitmap(hdc, width, height);
		if (NULL == bmp) return;
		other = SelectObject(WI->buf, bmp);
		RECT r = { 0, 0, width, height };
        MakeGradientEx(WI->buf,
			r,
			pSI->type,
			pSI->Color,
			pSI->ColorTo,
            pSI->ColorSplitTo,
            pSI->ColorToSplitTo,
			pSI->interlaced,
			pSI->bevelstyle,
			pSI->bevelposition,
			0,
			pSI->borderColor,
			pSI->borderWidth
			);
		*pBmp = bmp;
	}
	else
	{
		other = SelectObject(WI->buf, bmp);
	}

	BitBlt(hdc, rc->left, rc->top, width, height, WI->buf, 0, 0, SRCCOPY);
	SelectObject(WI->buf, other);
}

//-----------------------------------------------------------------

void DrawButton(WinInfo *WI, HDC hdc, RECT rc, int btn, bool state, bool active, bool pressed)
{
	GradientItem *pSI;
	if (false == active) pSI = &mSkin.windowButtonUnfocus;
	else if (pressed) pSI = &mSkin.windowButtonPressed;
	else pSI = &mSkin.windowButtonFocus;

	PutGradient(WI, hdc, &rc, pSI);

	int d, x, y, xa, ya, xe, ye;
	d = (rc.right - rc.left - BUTTON_SIZE) / 2;
	xe = (xa = rc.left + d) + BUTTON_SIZE;
	ye = (ya = rc.top + d) + BUTTON_SIZE;

	COLORREF c = pSI->picColor;
	unsigned char *up = mSkin.button_bmp[btn].data[(unsigned)state];
	unsigned bits = 0;
	y = ya;
	do {
		x = xa;
		do {
			if (bits < 2) bits = 256 | *up++;
			if (bits & 1) SetPixel(hdc, x, y, c);
			bits >>= 1;
		} while (++x < xe);
	} while (++y < ye);
}

void draw_line(HDC hDC, int x1, int x2, int y1, int y2, int w)
{
	while (w)
	{
		MoveToEx(hDC, x1, y1, NULL);
		LineTo  (hDC, x2, y2);
		if (x1 == x2) x2 = ++x1; else y2 = ++y1;
		--w;
	}
}

//-----------------------------------------------------------------

void PaintAll(struct WinInfo* WI)
{
	//dbg_printf("painting %x", WI->hwnd);

	GradientItem *pSI; RECT rc;
	HICON hIcon = NULL;

	HDC hdc_win = GetWindowDC(WI->hwnd);
	WI->buf = CreateCompatibleDC(hdc_win);

	int left    = WI->S.HiddenSide;
	int width   = WI->S.width;
	int right   = width - WI->S.HiddenSide;

	int top     = WI->S.HiddenTop;
	int bottom  = WI->S.height - WI->S.HiddenBottom;
	int title_height = mSkin.ncTop;
	int title_bottom = top + title_height;

	bool active = WI->is_active
		|| GetActiveWindow() == WI->hwnd
		|| WI->bbsm_option >= MSGID_BBSM_SETACTIVE;

	int bw = mSkin.borderWidth;
	COLORREF bc = active ? mSkin.focus_borderColor : mSkin.unfocus_borderColor;

	HDC hdc = CreateCompatibleDC(hdc_win);
	HGDIOBJ hbmpOld = SelectObject(hdc, CreateCompatibleBitmap(hdc_win, width, title_bottom));

	//----------------------------------
	//Titlebar gradient

	rc.top = top;
	rc.left = left;
	rc.right = right;
	rc.bottom = title_bottom;
	pSI = active ? &mSkin.windowTitleFocus : &mSkin.windowTitleUnfocus;
	PutGradient(WI, hdc, &rc, pSI);

	//----------------------------------
	//Titlebar Buttons

	int label_left = left;
	int label_right = right;

	rc.top = top + mSkin.buttonMargin;
	rc.bottom = rc.top + mSkin.buttonSize;

	LONG w_style = WI->style;
	int pos, i;
	WI->right_btn = 15;
	for (pos = i = 0; i < 15; i++)
	{
		if ('-' == mSkin.button_string[i])
		{
			WI->right_btn = i;
			WI->button_set[i] = btn_Caption;
			pos = 14;
			continue;
		}
		int n = i < WI->right_btn ? i : 15 - i + WI->right_btn;
		int b = mSkin.button_string[n] - '1';
		bool pressed = WI->button_down == b;
		bool set, state;
		switch (b)
		{
			case btn_Rollup: state = WI->is_rolled;
				set = 0 != (w_style & WS_SIZEBOX);
				break;
			case btn_Sticky: state = WI->is_sticky;
				set = 0 == (w_style & WS_CHILD);
				break;
			case btn_OnTop: state = WI->is_ontop;
				set = 0 == (w_style & WS_CHILD);
				break;
			case btn_Min: state = WI->is_iconic;
				set = 0 != (w_style & WS_MINIMIZEBOX);
				break;
			case btn_Max: state = WI->is_zoomed;
				set = 0 != (w_style & WS_MAXIMIZEBOX);
				break;
			case btn_Close: state = false, set = true;
				if (MSGID_BBSM_SETPRESSED == WI->bbsm_option)
					pressed = true;
				break;
			case btn_Icon: state = false;
				if (mSkin.iconSize && NULL == hIcon)
				{
					if (mSkin.iconSize <= 16)
					{
						SendMessageTimeout(WI->hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
						if (NULL == hIcon) hIcon = (HICON)GetClassLong(WI->hwnd, GCLP_HICONSM);
					}
					if (NULL == hIcon) SendMessageTimeout(WI->hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
					if (NULL == hIcon) hIcon = (HICON)GetClassLong(WI->hwnd, GCLP_HICON);
					if (NULL == hIcon && mSkin.iconSize > 16)
					{
						SendMessageTimeout(WI->hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 500, (DWORD_PTR*)&hIcon);
						if (NULL == hIcon) hIcon = (HICON)GetClassLong(WI->hwnd, GCLP_HICONSM);
					}
				}

				set = NULL != hIcon;
				break;
			default:
				set = state = false;
		}

		WI->button_set[n] = btn_Caption;
		if (set)
		{
			WI->button_set[pos] = b;
			if (pos < WI->right_btn) // left button
			{
				if (0 == pos) rc.left = label_left + mSkin.buttonMargin;
				else rc.left = label_left + mSkin.buttonSpace;
				label_left = rc.right = rc.left + mSkin.buttonSize;
				pos++;
			}
			else // right button
			{
				if (14 == pos) rc.right = label_right - mSkin.buttonMargin;
				else rc.right = label_right - mSkin.buttonSpace;
				label_right = rc.left = rc.right - mSkin.buttonSize;
				pos--;
			}

			if (btn_Icon == b)
			{
				int iconMargin = (mSkin.iconSize - mSkin.buttonSize) / 2;
				drawIco (rc.left - iconMargin, rc.top - iconMargin, mSkin.iconSize, hIcon, hdc, !active, mSkin.iconSat, mSkin.iconHue);
			}
			else
				DrawButton(WI, hdc, rc, b, state, active, pressed);
		}
	}

	//----------------------------------
	//Titlebar Label gradient
	if (left == label_left) rc.left = label_left + mSkin.labelMargin;
	else rc.left = label_left + mSkin.buttonMargin - bw;
	if (right == label_right) rc.right = label_right - mSkin.labelMargin;
	else rc.right = label_right - mSkin.buttonMargin + bw;

	rc.top = top + mSkin.labelMargin;
	rc.bottom = title_bottom - mSkin.labelMargin;

	pSI = active ? &mSkin.windowLabelFocus : &mSkin.windowLabelUnfocus;
	PutGradient(WI, hdc, &rc, pSI);

	//----------------------------------
	//Titlebar Text
	rc.left    += 3;
	rc.right   -= 3;

	if (NULL == WI->hFont)
	{
		WI->hFont = CreateFont(
			mSkin.windowFont.Height,
			0, 0, 0,
			mSkin.windowFont.Weight,
			false, false, false,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH|FF_DONTCARE,
			mSkin.windowFont.Face
			);
	}

	HGDIOBJ hfontOld = SelectObject(hdc, WI->hFont);
	SetBkMode(hdc, TRANSPARENT);

	char sTitle[128]; sTitle[0] = 0;
	GetWindowText(WI->hwnd, sTitle, sizeof sTitle);

    BBDrawTextAlt(hdc, sTitle, -1, &rc, mSkin.windowFont.Justify | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER, pSI);

	SelectObject(hdc, hfontOld);

	//----------------------------------
	// Blit the title
	BitBlt(hdc_win, left, top, right-left, title_height, hdc, left, top, SRCCOPY);

	if (false == WI->is_rolled || (false == mSkin.nixShadeStyle && mSkin.handleHeight))
	{
		if (bw)
		{
			//----------------------------------
			// Frame left/right(/bottom) border, drawn directly on screen
			rc.top = title_bottom;
			rc.left = left;
			rc.right = right;
			rc.bottom = bottom - mSkin.ncBottom;

			HGDIOBJ oldPen = SelectObject(hdc_win, CreatePen(PS_SOLID, 1, bc));
			draw_line(hdc_win, rc.left, rc.left, rc.top, rc.bottom, bw);
			draw_line(hdc_win, rc.right-bw, rc.right-bw, rc.top, rc.bottom, bw);
			if (0 == mSkin.handleHeight) draw_line(hdc_win, rc.left, rc.right, rc.bottom, rc.bottom, bw);
			DeleteObject(SelectObject(hdc_win, oldPen));
		}

		if (mSkin.handleHeight)
		{
			//----------------------------------
			// Bottom handle border
			rc.top = 0;
			rc.bottom = mSkin.ncBottom;
			rc.left = left;
			rc.right = right;

			//----------------------------------
			// Bottom Handle gradient
			pSI = active ? &mSkin.windowHandleFocus : &mSkin.windowHandleUnfocus;
			PutGradient(WI, hdc, &rc, pSI);

			//----------------------------------
			//Bottom Grips

			pSI = active ? &mSkin.windowGripFocus : &mSkin.windowGripUnfocus;
			if (false == pSI->parentRelative)
			{
				int r = rc.right;
				rc.right = rc.left + mSkin.gripWidth;
				PutGradient(WI, hdc, &rc, pSI);
				rc.left = (rc.right = r) - mSkin.gripWidth;
				PutGradient(WI, hdc, &rc, pSI);
			}

			//----------------------------------
			// Blit the bottom
			BitBlt(hdc_win, left, bottom - rc.bottom, right-left, rc.bottom, hdc, left, 0, SRCCOPY);

		} // has an handle
	} // not iconic

	DeleteObject(SelectObject(hdc, hbmpOld));
	DeleteDC(hdc);
	DeleteDC(WI->buf);
	ReleaseDC(WI->hwnd, hdc_win);
}

//-----------------------------------------------------------------
void post_redraw(HWND hwnd)
{
	PostMessage(hwnd, bbSkinMsg, MSGID_REDRAW, 0);
}

//-----------------------------------------------------------------
bool get_rolled(WinInfo *WI)
{
	DWORD prop = (DWORD)GetProp(WI->hwnd, BBSHADE_PROP);
	bool rolled = IsZoomed(WI->hwnd)
		? 0 != HIWORD(prop)
		: 0 != LOWORD(prop);
	return WI->is_rolled = rolled;
}

//-----------------------------------------------------------------
// cut off left/right sizeborder and adjust title height
bool set_region(WinInfo *WI)
{
	HWND hwnd = WI->hwnd;

	WI->style = GetWindowLong(hwnd, GWL_STYLE);
	WI->exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

	WI->is_ontop = WI->exstyle & WS_EX_TOPMOST;
	WI->is_zoomed = IsZoomed(hwnd);
	WI->is_iconic = IsIconic(hwnd);

	// since the 'SetWindowRgn' throws a WM_WINPOSCHANGED,
	// and WM_WINPOSCHANGED calls set_region, ...
	if (WI->in_set_region) return false;

	// check for fullscreen mode
	if (WS_CAPTION != (WI->style & WS_CAPTION))
	{
		if (false == WI->apply_skin)
			return false;

		WI->apply_skin = false;
		SetWindowRgn(hwnd, NULL, TRUE);
		return true;
	}

	SizeInfo S = WI->S;

	RECT rc; GetWindowRect(hwnd, &rc);
	WI->S.width  = rc.right - rc.left;
	WI->S.height = rc.bottom - rc.top;
	GetClientRect(hwnd, &WI->S.rcClient);

	int c = (WS_EX_TOOLWINDOW & WI->exstyle)
		? mSkin.cySmCaption : mSkin.cyCaption;

	int b = (WS_SIZEBOX & WI->style)
		? mSkin.cxSizeFrame : mSkin.cxFixedFrame;

	WI->S.HiddenTop     = imax(0, b - mSkin.ncTop + c);
	WI->S.HiddenSide    = imax(0, b - mSkin.borderWidth);
	WI->S.HiddenBottom  = imax(0, b - mSkin.ncBottom);
	WI->S.BottomAdjust  = imax(0, mSkin.ncBottom - b);
	if (get_rolled(WI)) WI->S.HiddenBottom = 0;

	if (0 == memcmp(&S, &WI->S, sizeof S) && WI->apply_skin)
	{
		return false; // nothing changed
	}

	if (WI->S.width != S.width)
	{
		// if width has changed, invalidate the variable bitmaps
		DeleteBitmaps(WI, 0, 3);
		DeleteBitmaps(WI, 6, 3);
	}

	WI->apply_skin = true;

	HRGN hrgn = CreateRectRgn(
		WI->S.HiddenSide,
		WI->S.HiddenTop,
		WI->S.width - WI->S.HiddenSide,
		WI->S.height - WI->S.HiddenBottom
		);

	WI->in_set_region = true;
	SetWindowRgn(hwnd, hrgn, TRUE);
	WI->in_set_region = false;

	return true;
}

//-----------------------------------------------------------------
// This is where it all starts from

void subclass_window(HWND hwnd)
{
	WinInfo *WI = new WinInfo;
	memset(WI, 0, sizeof *WI);

	// Keep a reference to WI piece of code to prevent the
	// engine from being unloaded by chance (i.e. if BB crashes)
	WI->hModule = LoadLibrary(BBLEANSKIN_ENGINEDLL);
	WI->hwnd = hwnd;
	WI->is_unicode = IsWindowUnicode(hwnd);
	WI->button_down = WI->capture_button = btn_None;
	WI->pCallWindowProc = WI->is_unicode ? CallWindowProcW : CallWindowProcA;

	set_WinInfo(hwnd, WI);

	WI->wpOrigWindowProc = (WNDPROC)
		(WI->is_unicode ? SetWindowLongW : SetWindowLongA)
			(hwnd, GWL_WNDPROC, (LONG)WindowSubclassProc);

	// some programs dont handle the posted "MSGID_LOAD" msg, so set
	// the region here (is WI causing the 'opera does not start' issue?)

	set_region(WI);

	// Also, the console (class=tty) does not handle posted messages at all.
	// Have to use the "hook-early:" option with such windows.

	// With other windows again that would cause troubles, like with tabbed
	// dialogs, which seem to subclass themselves during creation.

}

// This is where it ends then
void detach_skinner(WinInfo *WI)
{
	HWND hwnd = WI->hwnd;

	// clean up
	WI->apply_skin = false;
	DeleteBitmaps(WI, 0, NUMOFGDIOBJS);

	// the currently set WindowProc
	WNDPROC wpNow = (WNDPROC)(WI->is_unicode ? GetWindowLongW : GetWindowLongA)(hwnd, GWL_WNDPROC);

	// check, if it's still what we have set
	if (WindowSubclassProc == wpNow)
	{
		// if so, set back to the original WindowProc
		(WI->is_unicode ? SetWindowLongW : SetWindowLongA)(hwnd, GWL_WNDPROC, (LONG)WI->wpOrigWindowProc);
		// remove the property
		del_WinInfo(WI->hwnd);
		// send a note to the log window
		send_log(WI->hwnd, "Released");
	}
	else
	{
		// otherwise, the subclassing must not be terminated
		send_log(hwnd, "Subclassed otherwise, cannot release");
	}
}

DWORD WINAPI UnloadThread (void *pv)
{
	FreeLibraryAndExitThread((HMODULE)pv, 0);
	return 0; // never returns
}

// This is where it definitely ends
void release_window(WinInfo *WI)
{
	// save the hModule
	HMODULE hModule = WI->hModule;

	// free the WinInfo structure
	delete WI;

	// dbg_printf("release");
	FreeLibrary(hModule);

	// make shure WI code is left before the library is free'd
	//DWORD tid; CloseHandle(CreateThread(NULL, 0, UnloadThread, hModule, 0, &tid));
}

//-----------------------------------------------------------------
// get button-id from mousepoint
int get_button (struct WinInfo *WI, int x, int y)
{
	int button_top = WI->S.HiddenTop + mSkin.buttonMargin;
	int margin = WI->S.HiddenSide + mSkin.buttonMargin;
	int left = margin;
	int right = WI->S.width - margin;

	RECT rc; GetWindowRect(WI->hwnd, &rc);
	y -= rc.top;
	x -= rc.left;

	int n = btn_Nowhere;

	if (y < button_top)
	{
		if (x < mSkin.ncTop) n = btn_Topleft;
		else
		if (x >= WI->S.width - mSkin.ncTop) n = btn_Topright;
		else n = btn_Top;
	}
	else
	if (y >= WI->S.HiddenTop + mSkin.ncTop && false == WI->is_rolled) n = btn_None;
	else
	if (y >= button_top + mSkin.buttonSize) n = btn_Caption;
	else
	if (x < left) n = btn_Topleft;
	else
	if (x >= right) n = btn_Topright;
	else
	{
		n = btn_Caption;
		int button_next = mSkin.buttonSize + mSkin.buttonSpace;
		for (int i = 0; i < 15; i++)
		{
			if (WI->right_btn == i)
			{
				left = right - (14 - WI->right_btn) * button_next + mSkin.buttonSpace;
				continue;
			}
			if (x < left) break;
			if (x < left + mSkin.buttonSize)
			{
				n = WI->button_set[i];
				break;
			}
			left += button_next;
		}
	}
	if (n >= btn_Topleft)
	{
		if (WI->is_zoomed) n = btn_Caption;
		if (WI->is_iconic) n = btn_None;
	}
	return n;
}

//-----------------------------------------------------------------
// translate button-id to hittest value
LRESULT translate_hittest(WinInfo *WI, int n)
{
	switch(n)
	{
		case btn_Min: return HTMINBUTTON;               //  8
		case btn_Max: return HTMAXBUTTON;               //  9
		case btn_Close: return HTCLOSE;                 // 20
		case btn_Caption: return HTCAPTION;             //  2
		case btn_Nowhere: return HTNOWHERE;             //  0
		case btn_Top: n = HTTOP;  goto s1;              // 12
		case btn_Topleft: n = HTTOPLEFT; goto s1;       // 13
		case btn_Topright: n = HTTOPRIGHT;  goto s1;    // 14
		default: break;
	  s1:
		if (WI->style & WS_SIZEBOX) return n;
		return HTCAPTION;
	}
	return n+100;
}

//-----------------------------------------------------------------
// translate hittest value to button-id
int translate_button(int wParam)
{
	if (HTMINBUTTON == wParam) return btn_Min;
	if (HTMAXBUTTON == wParam) return btn_Max;
	if (HTCLOSE == wParam) return btn_Close;
	return wParam - 100;
}

//-----------------------------------------------------------------
// titlebar click actions
int get_caption_click(WPARAM wParam, char *pCA)
{
	if (HTCAPTION != wParam) return -1;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		return pCA[1];

	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		return pCA[2];

	return pCA[0];
}

//-----------------------------------------------------------------
//#define LOGMSGS

#ifdef LOGMSGS
#include "winmsgs.cpp"
#endif

//===========================================================================

LRESULT APIENTRY WindowSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WinInfo *WI = get_WinInfo(hwnd);
	//LFWinInfo *LFWI;
	LRESULT result = 0;
	int n = 0;
	bool flag_save = false;

#ifdef LOGMSGS
	dbg_printf("hw %08x  msg %s  wP %08x  lp %08x", hwnd, wm_str(uMsg), wParam, lParam);
#endif

	if (WI->apply_skin
	 || uMsg == WM_NCDESTROY
	 || uMsg == WM_STYLECHANGED
	 || uMsg == bbSkinMsg
	 )
	switch (uMsg)
	{

	//----------------------------------
	// Windows draws the caption on WM_SETTEXT/WM_SETICON

	case WM_SETICON:
	case WM_SETTEXT:
		if ((WI->exstyle & WS_EX_MDICHILD) && IsZoomed(hwnd))
		{
			post_redraw(GetRootWindow(hwnd));
			break;
		}

	//if (false == mSkin.drawLocked)
		goto paint_after;
/*
	paint_locked:  // smooth, but slow and dangerous also
	{
		bool flag_save = WI->dont_draw;
		WI->dont_draw = true;
		BOOL locked = LockWindowUpdate(hwnd);
		result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
		if (locked) LockWindowUpdate(NULL);
		WI->dont_draw = flag_save;
		goto paint_now;
	}
*/
	//----------------------------------
	// Windows draws the caption buttons on WM_SETCURSOR (size arrows),
	// which looks completely unsmooth, have to override this

	case WM_SETCURSOR:
	{
		LPCSTR CU;
		switch (LOWORD(lParam))
		{
			case HTLEFT:
			case HTRIGHT: CU = IDC_SIZEWE;  break;

			case HTTOPRIGHT:
			case HTBOTTOMLEFT: CU = IDC_SIZENESW; break;

			case HTTOPLEFT:
			case HTGROWBOX:
			case HTBOTTOMRIGHT: CU = IDC_SIZENWSE; break;

			case HTTOP:
			case HTBOTTOM: CU = IDC_SIZENS; break;

			default: goto paint_after;
		}
		SetCursor(LoadCursor(NULL, CU));
		result = 1;
		goto leave;
	}

	//----------------------------------

	case WM_NCACTIVATE:
		WI->is_active = 0 != wParam;

		if ( WI->is_active ) {
			flag_save = WI->dont_draw;
			WI->dont_draw = true;
			
			//if ( (isWin2kXP()) ) {
			//	applyAlphaMethod ( WI, WINDOW_STATUS_FOCUS, ((255 * (100 - mSkin.focusTransparency)) / 100) );
			//}
			
			if ( (isWin2kXP()) && (GetWindowLong(hwnd,GWL_EXSTYLE) & WS_EX_LAYERED) ) {
				SetWindowLong(hwnd, GWL_EXSTYLE,
				WI->exstyle & ~WS_EX_LAYERED);
				WI->is_focustrans = WI->is_unfocustrans = false;
			}
			if ( (mSkin.focusColorTrans) && (isWin2kXP()) ) {
				if (!(WI->exstyle & WS_EX_LAYERED)) {
					SetProp( hwnd, BBLEANSKIN_UNFOCUSTRANS, (HANDLE)(TRUE) );
					SetWindowLong(hwnd,GWL_EXSTYLE,	WI->exstyle | WS_EX_LAYERED);
					SetLayeredWindowAttributes(hwnd, mSkin.focusColorTrans, 0, LWA_COLORKEY);
					WI->is_focustrans = true;
				}
			} else
			if ((mSkin.focusTransparency > 0) && (isWin2kXP())) {
				if (!(WI->exstyle & WS_EX_LAYERED)) {
					SetProp( hwnd, BBLEANSKIN_FOCUSTRANS, (HANDLE)(TRUE) );
					dbg_printf("Windows does not have WS_EX_LAYERED style set");
					// Window is not layered. Safe to proceed with layering it here
					int setAlpha = (255 * (100 - mSkin.focusTransparency)) / 100;
					SetWindowLong(hwnd,GWL_EXSTYLE,	WI->exstyle | WS_EX_LAYERED);
					SetLayeredWindowAttributes(hwnd, 0, setAlpha, LWA_ALPHA);
					WI->is_focustrans = true;
				}
			}
			WI->dont_draw = flag_save;
		} else if ( !(WI->is_active) ) {
			flag_save = WI->dont_draw;
			WI->dont_draw = true;
			if ( (isWin2kXP()) && (WI->exstyle & WS_EX_LAYERED) ) {
				SetWindowLong(hwnd, GWL_EXSTYLE,
				WI->exstyle & ~WS_EX_LAYERED);
				WI->is_focustrans = WI->is_unfocustrans = false;
			}
			if ( (mSkin.unfocusColorTrans) && (isWin2kXP()) ) {
				if (!(WI->exstyle & WS_EX_LAYERED)) {
					SetProp( hwnd, BBLEANSKIN_UNFOCUSTRANS, (HANDLE)(TRUE) );
					SetWindowLong(hwnd,GWL_EXSTYLE,	WI->exstyle | WS_EX_LAYERED);
					SetLayeredWindowAttributes(hwnd, mSkin.unfocusColorTrans, 0, LWA_COLORKEY);
					WI->is_unfocustrans = true;
				}
			} else 
			if ((mSkin.unfocusTransparency > 0) && (isWin2kXP())) {
				if (!(WI->exstyle & WS_EX_LAYERED)) {
					SetProp( hwnd, BBLEANSKIN_UNFOCUSTRANS, (HANDLE)(TRUE) );
					dbg_printf("Windows does not have WS_EX_LAYERED style set");
					// Window is not layered. Safe to proceed with layering it here
					int setAlpha = (255 * (100 - mSkin.unfocusTransparency)) / 100;
					SetWindowLong(hwnd,GWL_EXSTYLE,	WI->exstyle | WS_EX_LAYERED);
					SetLayeredWindowAttributes(hwnd, 0, setAlpha, LWA_ALPHA);
					WI->is_unfocustrans = true;
				}
			}
			WI->dont_draw = flag_save;
		}
		post_redraw(hwnd);
		goto paint_after;

	//----------------------------------
	case WM_NCHITTEST:
		n = get_button(WI, (short)LOWORD(lParam), (short)HIWORD(lParam));
		if (btn_None == n)
			result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
		else
			result = translate_hittest(WI, n);

		if (WI->is_rolled)
		{
			switch(result)
			{
				case HTBOTTOM: // if this is removed, a rolled window can be opened by dragging
				case HTTOP:
				case HTMENU:
				case HTBOTTOMLEFT:
				case HTBOTTOMRIGHT:
					result = HTCAPTION;
					break;

				case HTTOPLEFT:
					result = HTLEFT;
					break;

				case HTTOPRIGHT:
					result = HTRIGHT;
					break;
			}
		}
		goto leave;

	//----------------------------------
	case WM_MOUSEMOVE:
		if (btn_None == (n = WI->capture_button))
			break;
		{
			POINT pt;
			pt.x = (short)LOWORD(lParam);
			pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwnd, &pt);
			if (get_button(WI, pt.x, pt.y) != n)
				n = btn_None;
		}
		if (WI->button_down != n)
		{
			WI->button_down = n;
			post_redraw(hwnd);
		}
		goto leave;


	//----------------------------------

	set_capture:
		WI->capture_button = WI->button_down = n;
		SetCapture(hwnd);
		post_redraw(hwnd);
		goto leave;

	exec_action:
		WI->capture_button = WI->button_down = btn_None;
		ReleaseCapture();
		exec_button_action(WI, n, lParam);
		post_redraw(hwnd);
		goto leave;

	case WM_CAPTURECHANGED:
		if (btn_None == WI->capture_button)
			break;
		WI->capture_button = WI->button_down = btn_None;
		goto leave;

	//----------------------------------
	case WM_LBUTTONUP:
		if (btn_None == WI->capture_button)
			break;
		n = WI->button_down;
		goto exec_action;

	case WM_RBUTTONUP:
		if (btn_None == WI->capture_button)
			break;
		n = (wParam & MK_SHIFT) ? btn_VMax : btn_HMax;
		goto exec_action;

	case WM_MBUTTONUP:
		if (btn_None == WI->capture_button)
			break;
		n = btn_VMax;
		goto exec_action;

	//----------------------------------
	case WM_NCLBUTTONDBLCLK:
		if (-1 == (n = get_caption_click(wParam, mSkin.captionClicks.Dbl)))
			goto case_WM_NCLBUTTONDOWN;
		exec_button_action(WI, n, lParam);
		goto leave;

	case_WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
		n = translate_button(wParam);
		if (n >= 0 && n < 7) goto set_capture; // clicked in a button
		if (-1 == get_caption_click(wParam, mSkin.captionClicks.Left))
			break;
		goto leave;

	case WM_NCLBUTTONUP:
		if (-1 == (n = get_caption_click(wParam, mSkin.captionClicks.Left)))
			break;
		exec_button_action(WI, n, lParam);
		goto leave;
/*
These don't seem to work unless it's the maximize button.
We may use one of these to customize the transparency.. */
	//----------------------------------
	case WM_NCRBUTTONDOWN:
		n = translate_button(wParam);
		if (btn_Max == n) goto set_capture;
	case WM_NCRBUTTONDBLCLK:
		if (-1 == get_caption_click(wParam, mSkin.captionClicks.Right))
			break;
		goto leave;

	case WM_NCRBUTTONUP:
		if (-1 == (n = get_caption_click(wParam, mSkin.captionClicks.Right)))
			break;
		exec_button_action(WI, n, lParam);
		goto leave;

	//----------------------------------
	case WM_NCMBUTTONDOWN:
		n = translate_button(wParam);
		if (btn_Max == n) goto set_capture;
	case WM_NCMBUTTONDBLCLK:
		if (-1 == get_caption_click(wParam, mSkin.captionClicks.Mid))
			break;
		goto leave;

	case WM_NCMBUTTONUP:
		if (-1 == (n = get_caption_click(wParam, mSkin.captionClicks.Mid)))
			break;
		exec_button_action(WI, n, lParam);
		goto leave;

	//----------------------------------
	case WM_SYSCOMMAND:
		//dbg_printf("SYSCOMMAND: %08x", wParam);
		// ----------
		// these SYSCOMMAND's enter the 'window move/size' modal loop
		if (wParam >= 0xf001 && wParam <= 0xf008 // size
		 || wParam == 0xf012 // move
			)
		{
			// draw the caption before
			PaintAll(WI);
			break;
		}
		// ----------
		// these SYSCOMMAND's draw the caption
		if (wParam == 0xf095 // menu invoked
		 || wParam == 0xf100 // sysmenu invoked
		 || wParam == 0xf165 // menu closed
			)
		{
			// draw over after
			post_redraw(hwnd);
			break;
		}
		// ----------
		// unshade the window on close, just in case it likes to store it's size
		if (wParam == SC_CLOSE && WI->is_rolled)
		{
			ShadeWindow(hwnd);
			break;
		}
		post_redraw(hwnd);
		break;

	//----------------------------------
	case WM_NCPAINT:
		if (WI->dont_draw)
			goto leave;

		if (false == WI->is_rolled)
		{
			// Okay, so let's create an own region and pass that to
			// the original WndProc instead of (HRGN)wParam
			RECT rc; GetWindowRect(hwnd, &rc);
			HRGN hrgn = CreateRectRgn(
				rc.left + WI->S.HiddenSide + mSkin.borderWidth,
				rc.top + WI->S.HiddenTop + mSkin.ncTop,
				rc.right - WI->S.HiddenSide - mSkin.borderWidth,
				rc.bottom - WI->S.HiddenBottom - mSkin.ncBottom
				);
			result = CALLORIGWINDOWPROC(hwnd, uMsg, (WPARAM)hrgn, lParam);
			DeleteObject(hrgn);
		}

	paint_now:
		PaintAll(WI);
		goto leave;

	paint_after:
		{
			flag_save = WI->dont_draw;
			WI->dont_draw = true;
			result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
			WI->dont_draw = flag_save;
		}
		goto paint_now;

	//----------------------------------

	//----------------------------------
	// set flag, whether snapWindows should be applied below
	case WM_ENTERSIZEMOVE:
		WI->is_moving = true;
		/**
			Noccy: Added transparent window dragging
		*/
		WI->is_transparentdrag = false;
		flag_save = WI->dont_draw;
		WI->dont_draw = true;
		if ((mSkin.dragTransparency > 0) && (isWin2kXP())) {
			if (!(GetWindowLong(hwnd,GWL_EXSTYLE) & WS_EX_LAYERED)) {
				dbg_printf("Windows does not have WS_EX_LAYERED style set");
				// Window is not layered. Safe to proceed with layering it here
				int setAlpha = (255 * (100 - mSkin.dragTransparency)) / 100;
				SetWindowLong(hwnd,GWL_EXSTYLE,	GetWindowLong(hwnd,GWL_EXSTYLE) | WS_EX_LAYERED);
				SetLayeredWindowAttributes(hwnd, 0, setAlpha, LWA_ALPHA);
				WI->is_transparentdrag = true;
			}
		}
		WI->dont_draw = flag_save;
		break;

	case WM_EXITSIZEMOVE:
		WI->is_moving = false;
		/**
			Noccy: Added transparent window dragging
		*/
		flag_save = WI->dont_draw;
		WI->dont_draw = true;
		if (WI->is_transparentdrag == true) {
			SetWindowLong(hwnd, GWL_EXSTYLE, WI->exstyle & ~WS_EX_LAYERED);
			WI->is_transparentdrag = false;
		}
		WI->dont_draw = flag_save;
		break;

	//----------------------------------
	// If moved, snap to screen edges...
	case WM_WINDOWPOSCHANGING:
		if (WI->is_moving
		 && mSkin.snapWindows
		 && 0 == (WS_CHILD & WI->style)
		 && ((((WINDOWPOS*)lParam)->flags & SWP_NOSIZE)
			 || (WI->S.width == ((WINDOWPOS*)lParam)->cx
				&& WI->S.height == ((WINDOWPOS*)lParam)->cy
					)))
			SnapWindowToEdge(WI, (WINDOWPOS*)lParam, 7);

		if (get_rolled(WI))
		{
			// prevent app from possibly setting a minimum size
			((LPWINDOWPOS)lParam)->cy = mSkin.rollupHeight + WI->S.HiddenTop;
			goto leave;
		}
		break;

	//----------------------------------
	case WM_WINDOWPOSCHANGED:
		result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
		// adjust the windowregion
		set_region(WI);
		// MDI childs repaint their buttons at each odd occasion
		if (WS_CHILD & WI->style)
			goto paint_now;
		goto leave;

	//----------------------------------
	case WM_STYLECHANGED:
		set_region(WI);
		break;

	//----------------------------------
	// adjust for the bottom border (handleHeight)

	case WM_NCCALCSIZE:
		if (wParam)
		{
			result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
			if (WI->S.BottomAdjust)
				((NCCALCSIZE_PARAMS*)lParam)->rgrc[0].bottom -= WI->S.BottomAdjust;
			goto leave;
		}
		break;

	//----------------------------------
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMinTrackSize.x = 12 * mSkin.buttonSize;
		lpmmi->ptMinTrackSize.y = mSkin.ncTop + WI->S.HiddenTop + mSkin.ncBottom - WI->S.HiddenBottom - mSkin.borderWidth;
		break;
	}

	//----------------------------------
	// Terminate subclassing

	case WM_NCDESTROY:
		detach_skinner(WI);
		result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
		release_window(WI);
		return result;

	//----------------------------------
	default:
		if (uMsg == bbSkinMsg) // our registered message
		{
			switch (wParam)
			{
				// initialisation message
				case MSGID_LOAD:
					break;

				// detach the skinner
				case MSGID_UNLOAD:
					detach_skinner(WI);
					SetWindowRgn(hwnd, NULL, TRUE);
					result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
					release_window(WI);
					break;

				// repaint the caption
				case MSGID_REDRAW:
					if (WI->apply_skin) goto paint_now;
					break;

				// changed Skin
				case MSGID_REFRESH:
					GetSkin();
					DeleteBitmaps(WI, 0, NUMOFGDIOBJS);
					if (WI->apply_skin && false == set_region(WI))
						goto paint_now;
					break;

				// options for bbStyleMaker
				case MSGID_BBSM_RESET:
				case MSGID_BBSM_SETACTIVE:
				case MSGID_BBSM_SETPRESSED:
					WI->bbsm_option = wParam;
					break;

				// set sticky button state, sent from BB
				case MSGID_BB_SETSTICKY:
					WI->is_sticky = lParam;
					if (WI->apply_skin) goto paint_now;
					break;

				case MSGID_GETSHADEHEIGHT:
					result = mSkin.rollupHeight + WI->S.HiddenTop;
					break;

				//----------------------------------
			}
			goto leave;
		}
		break;

	//----------------------------------
	} //switch

	result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
leave:
	return result;
}

bool isWin2kXP() {

	OSVERSIONINFO osInfo;
	bool usingNT;

	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx(&osInfo);
	usingNT = (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
	return(usingNT && (osInfo.dwMajorVersion >= 5));

}

//===========================================================================
//===========================================================================
// Function: BBDrawText
// Purpose: draw text with shadow and/or outline
// In:
// Out:
//===========================================================================
int BBDrawTextAlt(HDC hDC, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, GradientItem* pSI){
    if (pSI->validated & VALID_SHADOWCOLOR){ // draw shadow
        RECT rcShadow;
        SetTextColor(hDC, pSI->ShadowColor);
        if (pSI->validated & VALID_OUTLINECOLOR){ // draw shadow with outline
            _CopyOffsetRect(&rcShadow, lpRect, 2, 0);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow,  0, 1);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow,  0, 1);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow, -1, 0);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat); _OffsetRect(&rcShadow, -1, 0);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat);
        }
        else{
            _CopyOffsetRect(&rcShadow, lpRect, 1, 1);
            DrawText(hDC, lpString, nCount, &rcShadow, uFormat);
        }
    }
    if (pSI->validated & VALID_OUTLINECOLOR){ // draw outline
        RECT rcOutline;
        SetTextColor(hDC, pSI->OutlineColor);
        _CopyOffsetRect(&rcOutline, lpRect, 1, 0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0,  1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,  -1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   0, -1);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat); _OffsetRect(&rcOutline,   1,  0);
        DrawText(hDC, lpString, nCount, &rcOutline, uFormat);
    }
    // draw text
    SetTextColor(hDC, pSI->TextColor);
    return DrawText(hDC, lpString, nCount, lpRect, uFormat);
}
