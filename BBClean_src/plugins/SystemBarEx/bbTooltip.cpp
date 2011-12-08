/*---------------------------------------------------------------------------------
 SystemBarEx (© 2004 Slade Taylor [bladestaylor@yahoo.com])
 ----------------------------------------------------------------------------------
 based on BBSystemBar 1.2 (© 2002 Chris Sutcliffe (ironhead) [ironhead@rogers.com])
 ----------------------------------------------------------------------------------
 SystemBarEx is a plugin for Blackbox for Windows.  For more information,
 please visit [http://bb4win.org] or [http://sourceforge.net/projects/bb4win].
 ----------------------------------------------------------------------------------
 SystemBarEx is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/
#pragma warning(disable: 4996)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define VC_EXTRALEAN

#include "../../blackbox/BBApi.h"
#include "bbTooltip.h"
//#include <shellapi.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//Borrowed from Xoblite
/*
void DrawTextWithShadow(HDC hdc, LPSTR text, RECT r, unsigned int format, COLORREF textColor, COLORREF shadowColor, bool shadow)
{
	if (shadow)
	{
		RECT s;
		s.left = r.left + 1;
		s.top = r.top + 1;
		s.bottom = r.bottom + 1;
		s.right = r.right + 1;

		SetTextColor(hdc, shadowColor);
		DrawText(hdc, text, strlen(text), &s, format);
	}

	SetTextColor(hdc, textColor);
	DrawText(hdc, text, strlen(text), &r, format);
}

COLORREF CreateShadowColor(StyleItem* styleItem, COLORREF color, COLORREF colorTo, COLORREF textColor)
{
        BYTE r = GetRValue(color);
        BYTE g = GetGValue(color);
        BYTE b = GetBValue(color);
        BYTE rto = GetRValue(colorTo);
        BYTE gto = GetGValue(colorTo);
        BYTE bto = GetBValue(colorTo);

        int rav, gav, bav;
        
        if (styleItem->type != B_SOLID)
        {
                rav = (r+rto) / 2;
                gav = (g+gto) / 2;
                bav = (b+bto) / 2;
        }
        else
        {
                rav = r;
                gav = g;
                bav = b;
        }

        if (rav < 0x10) rav = 0;
        else rav -= 0x10;
        if (gav < 0x10) gav = 0;
        else gav -= 0x10;
        if (bav < 0x10) bav = 0;
        else bav -= 0x10;

        return RGB((BYTE)rav, (BYTE)gav, (BYTE)bav);
}*/

void bbTooltip::Start(HINSTANCE h_host_instance, HWND h_host, bbTooltipInfo *p_info)
{
    WNDCLASS wc;
    m_bInitial = true;
    m_WindowName = "_bbTooltip";
    m_hCore = GetBBWnd();

    ZeroMemory((void*)&m_TipList, sizeof(m_TipList));

    m_hostInstance = h_host_instance;
    m_hHost = h_host;
    m_pInfo = p_info;

    ZeroMemory(&wc,sizeof(wc));
    wc.lpfnWndProc = _WndProc;
    wc.hInstance = m_hostInstance;
    wc.lpszClassName = m_WindowName;
    //wc.style = CS_SAVEBITS;
    RegisterClass(&wc);

    m_TipHwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        m_WindowName,
        NULL,
        WS_POPUP,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        m_hostInstance,
        NULL);

    SetWindowLongPtr(m_TipHwnd, GWLP_USERDATA, (long)this);

    m_hSecondaryBuf = CreateCompatibleDC(NULL);
    m_hBitmapNull = (HBITMAP)SelectObject(m_hSecondaryBuf, CreateCompatibleBitmap(m_hSecondaryBuf, 2, 2));
    m_hFontNull = (HFONT)SelectObject(m_hSecondaryBuf, CreateStyleFont(m_pInfo->pStyle));
    SetBkMode(m_hSecondaryBuf, TRANSPARENT);

    ZeroMemory(&m_bmpInfo, sizeof(m_bmpInfo));
    m_bmpInfo.biSize = sizeof(m_bmpInfo);
    m_bmpInfo.biPlanes = 1;
    m_bmpInfo.biCompression = BI_RGB;
    m_bmpInfo.biWidth = _MAX_WIDTH;
    m_bmpInfo.biHeight = _MAX_HEIGHT;

    UpdateSettings();
}

//-----------------------------------------------------------------------------

void bbTooltip::End()
{
    ClearList();
    DeleteObject((HBITMAP)SelectObject(m_hSecondaryBuf, m_hBitmapNull));
    DeleteObject((HFONT)SelectObject(m_hSecondaryBuf, m_hFontNull));
    DeleteDC(m_hSecondaryBuf);
    DestroyWindow(m_TipHwnd);
    UnregisterClass(m_WindowName, m_hostInstance);
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK bbTooltip::_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bbTooltip *p = (bbTooltip*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return (p && p->_Dispatch(message, wParam, lParam)) ? 0:
        DefWindowProc(hwnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------

inline bool bbTooltip::_Dispatch(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
            _Paint();
            return true;

        case WM_TIMER:
            _Timer(wParam);
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

inline void bbTooltip::_Timer(WPARAM wParam)
{
    switch (wParam)
    {
        case _INITIAL_TIMER:
            KillTimer(m_TipHwnd, _INITIAL_TIMER);
            _Show();
            break;

        case _UPDATE_TIMER:
            GetCursorPos(&m_timer_point);
            GetWindowRect(m_hHost, &m_timer_rect);
            if (!PtInRect(&m_timer_rect, m_timer_point)) {
                ScreenToClient(m_hHost, &m_timer_point);
                MouseEvent(m_timer_point, WM_MOUSEMOVE);
            }
    }
}

//-----------------------------------------------------------------------------

inline void bbTooltip::_Paint()
{
    m_hPrimaryBuf = BeginPaint(m_TipHwnd, &m_PaintStruct);

    m_rect.left = m_rect.top = 0;
	m_rect.right = m_Width;
    m_rect.bottom = m_Height;

    MakeStyleGradient(m_hSecondaryBuf, &m_rect, m_pInfo->pStyle, m_pInfo->pStyle->bordered);

    m_rect.left =
        m_rect.top = 0;
    m_rect.right = m_Width;
    m_rect.bottom = m_Height;

   // CreateBorder(m_hSecondaryBuf, &m_rect, m_BorderColor, m_BorderWidth);

    m_rect.left = m_rect.top = m_tipBorder;

	//bool	shadow_ = m_pInfo->Tooltips_Shadow ? ( shadow_ = true ) : ( shadow_ = false );
	//COLORREF Shadow = CreateShadowColor(m_pInfo->pStyle, m_pInfo->pStyle->Color, m_pInfo->pStyle->ColorTo, m_pInfo->pStyle->TextColor);
	//DrawTextWithShadow( m_hSecondaryBuf, m_Text, m_rect, DT_LEFT|DT_WORDBREAK|DT_END_ELLIPSIS, m_pInfo->pStyle->TextColor, Shadow, shadow_ );
    
	DrawText(
        m_hSecondaryBuf,
        m_Text,
        -1,
        &m_rect,
        DT_LEFT|DT_WORDBREAK|DT_END_ELLIPSIS);

    m_pPaintRect = &m_PaintStruct.rcPaint;

    BitBlt(
        m_hPrimaryBuf,
        m_pPaintRect->left,
        m_pPaintRect->top,
        (m_pPaintRect->right - m_pPaintRect->left),
        (m_pPaintRect->bottom - m_pPaintRect->top),
        m_hSecondaryBuf,
        m_pPaintRect->left,
        m_pPaintRect->top,
        SRCCOPY);

    EndPaint(m_TipHwnd, &m_PaintStruct);
}

//-----------------------------------------------------------------------------

void bbTooltip::_Activate(RECT *rect, char *text)
{
    strcpy(m_Text, text);

    for (char *p = &m_Text[0]; *p; ++p)
        if (*p == '^')
            *p = 10;

    m_ActivationRect = *rect;

    if (m_bInitial && m_pInfo->delay > 0)
        SetTimer(m_TipHwnd, _INITIAL_TIMER, m_pInfo->delay, NULL);
    else
        _Show();

    SetTimer(m_TipHwnd, _UPDATE_TIMER, 50, NULL);
}

//-----------------------------------------------------------------------------

void bbTooltip::_Show()
{
    RECT parent_rect;
    POINT pos;

    m_bInitial = false;

    GetWindowRect(m_hHost, &parent_rect);

    RECT r = {0, 0, m_pInfo->max_width, _MAX_HEIGHT};
	
	//if (m_pInfo->pStyle->ShadowXY && !m_pInfo->pStyle->parentRelative) {
	if (m_pInfo->pStyle->validated & VALID_SHADOWCOLOR) {
		RECT Rs;
		int i;
		COLORREF cr0;
		i = m_pInfo->pStyle->ShadowY;
		Rs.top = r.top + i;
		Rs.bottom = r.bottom + i;
		i = m_pInfo->pStyle->ShadowX;
		Rs.left = r.left + i;
		Rs.right = r.right + i;
		cr0 = SetTextColor(m_hSecondaryBuf, m_pInfo->pStyle->ShadowColor);
		DrawText(m_hSecondaryBuf, m_Text, -1, &Rs, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
		SetTextColor(m_hSecondaryBuf, cr0);
	}

	if (m_pInfo->pStyle->validated & VALID_OUTLINECOLOR) {	
		COLORREF cr0;
			RECT rcOutline;
			//_CopyRect(&rcOutline, r);
			rcOutline.bottom = r.bottom;
			rcOutline.top = r.top;
			rcOutline.left = r.left+1;
			rcOutline.right = r.right+1;
			//cr0 = SetTextColor(bufDC, StyleItemArray[STYLE_TOOLBARLABEL].OutlineColor);
			SetTextColor(m_hSecondaryBuf, m_pInfo->pStyle->OutlineColor);
			//_CopyOffsetRect(&rcOutline, lpRect, 1, 0);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0,  1);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,  -1,  0);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   0, -1);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
			_OffsetRect(&rcOutline,   1,  0);
			DrawText(m_hSecondaryBuf, m_Text, -1, &rcOutline, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);
	}
	//bool	shadow_ = m_pInfo->Tooltips_Shadow ? ( shadow_ = true ) : ( shadow_ = false );
	//COLORREF Shadow = CreateShadowColor(m_pInfo->pStyle, m_pInfo->pStyle->Color, m_pInfo->pStyle->ColorTo, m_pInfo->pStyle->TextColor);
	//DrawTextWithShadow( m_hSecondaryBuf, m_Text, r, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS, m_pInfo->pStyle->TextColor, Shadow, shadow_ );
    DrawText(m_hSecondaryBuf, m_Text, -1, &r, DT_LEFT|DT_WORDBREAK|DT_CALCRECT|DT_END_ELLIPSIS);

    m_Width = r.right - r.left + 2 * m_tipBorder;
    m_Height = r.bottom - r.top + 2 * m_tipBorder;

    pos.x = parent_rect.left + m_ActivationRect.left +
        (m_pInfo->bCenterTip ? ((m_ActivationRect.right - m_ActivationRect.left) / 2 - (m_Width / 2)): 0);

    if (pos.x < 0)
        pos.x = 0;
    else if ((pos.x + m_Width) > m_ScreenWidth)
        pos.x = m_ScreenWidth - m_Width;

    pos.y =
        ((m_pInfo->bAbove && ((m_Height + m_pInfo->distance) < parent_rect.top)) ||
        (!m_pInfo->bAbove && ((m_Height + m_pInfo->distance) > (m_ScreenHeight - parent_rect.bottom)))) ?
        parent_rect.top - m_Height - (m_pInfo->bDocked ? -m_BorderWidth: m_pInfo->distance):
        parent_rect.bottom + (m_pInfo->bDocked ? -m_BorderWidth: m_pInfo->distance);

    SetWindowPos(
        m_TipHwnd,
        HWND_TOPMOST,
        pos.x,
        pos.y,
        m_Width,
        m_Height,
        SWP_SHOWWINDOW|SWP_NOACTIVATE);

    InvalidateRect(m_TipHwnd, NULL, false);
}

//-----------------------------------------------------------------------------

void bbTooltip::_Hide()
{
    KillTimer(m_TipHwnd, _INITIAL_TIMER);
    KillTimer(m_TipHwnd, _UPDATE_TIMER);
    ShowWindow(m_TipHwnd, SW_HIDE);
    m_bInitial = true;
}

//-----------------------------------------------------------------------------

void bbTooltip::UpdateSettings()
{
    m_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    m_BorderColor = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);
    m_BorderWidth = *(int*)GetSettingPtr(SN_BORDERWIDTH);

    m_tipBorder = m_BorderWidth + ((m_pInfo->pStyle->bevelstyle == BEVEL_SUNKEN) ? 3: 2);

    DeleteObject((HFONT)SelectObject(m_hSecondaryBuf, CreateStyleFont(m_pInfo->pStyleFont)));
    SetTextColor(m_hSecondaryBuf, m_pInfo->pStyle->TextColor);

    if (m_pInfo->max_width > _MAX_WIDTH)
        m_pInfo->max_width = _MAX_WIDTH;

    HDC hdc = GetDC(NULL);
    m_bmpInfo.biBitCount = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    DeleteObject((HBITMAP)SelectObject(m_hSecondaryBuf, CreateDIBSection(
        NULL, (BITMAPINFO*)&m_bmpInfo, DIB_RGB_COLORS, NULL, NULL, 0)));
}

//-----------------------------------------------------------------------------

void bbTooltip::ClearList()  // grischka
{
    TipStruct *t, **tp = &m_TipList;  // search the list for unused tips and remove them
    while (t = *tp)
    {
        if (t->used_flg)
        {
            t->used_flg = 0;
            tp = &t->next;
        }
        else
        {
            if (t->shown)
                _Hide();
            *tp = t->next;
            delete t;
        }
    }
}

//-----------------------------------------------------------------------------

void bbTooltip::Set(RECT *r, char *tipText, bool bShow) { // grischka
    TipStruct **tp, *t;

    if (!*tipText) return; //never set an empty tip

    for (tp = &m_TipList; (t = *tp); tp = &t->next) { // search the list
        if (!memcmp(&t->rect, r, sizeof(RECT))) { // if there's a tip with an identical rect
            t->used_flg = 1;  // then reuse it, set flag
            t->showTip = bShow;
            if (strcmp(t->text, tipText)) {
                strcpy(t->text, tipText); // update text, if needed
				if (t->shown && !m_bInitial) {
                    _Activate(&t->rect, t->text);
				}
            }
            return;
        }
    }
    // no tip was found...append new tip to list
    *tp = t = new TipStruct;
    t->next  = NULL;
    t->shown = t->clicked = 0;
    t->used_flg = 1;
    memcpy(&t->rect, r, sizeof(RECT));
    strcpy(t->text, tipText);
    t->showTip = bShow;
}

//-----------------------------------------------------------------------------

void bbTooltip::MouseEvent(POINT pos, UINT message) { // grischka

    TipStruct *t = m_TipList, *tx = NULL, *ty = NULL;

    while (t) {
		if ( PtInRect(&t->rect, pos)) {
            tx = t;                         // there is the mouse over
		} else if (t->shown) {
            ty = t;                         // there's not, but it is currently displayed
            t->shown = 0;
		} else if (t->clicked) {
            t->clicked = 0;                 // none of above, reset clicked flag
		}
        t = t->next;
    }

    if (message == WM_MOUSEMOVE) {
        if (tx) {                           // show the tip under the mouse, if necessary
			if (m_pInfo->bSetLabel) {
                PostMessage(m_hCore, BB_SETTOOLBARLABEL, 0, (LPARAM)tx->text);
			}

            if (tx->showTip) {
                if (!tx->clicked && !tx->shown) {
                    _Activate(&tx->rect, tx->text);
                    tx->shown = 1;
                }
                return;
            }
        }
        if (ty) {
            RECT r = {
                ty->rect.left - m_pInfo->activation_rect_pad - 1,  // prevent tip from hiding between rects
                ty->rect.top,
                ty->rect.right + m_pInfo->activation_rect_pad + 1,
                ty->rect.bottom
            };

            if (PtInRect(&r, pos)) {
                ty->shown = 1;
                return;
            }
        }
    }

    if (ty)
        goto ty_true;
    if (tx) {
        tx->clicked = 1;
        tx->shown   = 0;
ty_true:
        _Hide();
    }
}

//-----------------------------------------------------------------------------
