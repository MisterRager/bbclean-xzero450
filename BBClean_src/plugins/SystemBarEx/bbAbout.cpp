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
#include "ModuleInfo.h"
#include "bbAbout.h"
//#include <shellapi.h>

//-----------------------------------------------------------------------------

bbAbout::bbAbout(HINSTANCE h_host_instance, bbAbout **p_this) {
    WNDCLASS wc;
    m_WindowName = "_bbAbout";
    m_hostInstance = h_host_instance;
    m_pThis = p_this;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = _WndProc;
    wc.hInstance = m_hostInstance;
    wc.lpszClassName = m_WindowName;
    RegisterClass(&wc);

    m_AboutHwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        m_WindowName,
        NULL,
        WS_POPUP|WS_VISIBLE,
        0,
        0,
        0,
        0,
        NULL,
        NULL,
        m_hostInstance,
        NULL);

    SetWindowLongPtr(m_AboutHwnd, GWLP_USERDATA, (long)this);

    m_hSecondaryBuf = CreateCompatibleDC(NULL);
    m_hBitmapNull = (HBITMAP)SelectObject(m_hSecondaryBuf, CreateCompatibleBitmap(m_hSecondaryBuf, 2, 2));
    m_hFontNull = (HFONT)SelectObject(m_hSecondaryBuf,
        CreateStyleFont((StyleItem*)GetSettingPtr(SN_MENUTITLE)));

    SetBkMode(m_hSecondaryBuf, TRANSPARENT);

    m_Text = new char[300];
    sprintf(m_Text,
        "%s"
        "\n\nCopyright 2004 %s [%s]"
        "\n\n%s"
        "\n\n%s"
		"\n\nUpdated By %s"
		"\n\nCompiled on %s",
        ModInfo.Get(ModInfo.NAME_VERSION),
        ModInfo.Get(ModInfo.AUTHOR),
        ModInfo.Get(ModInfo.EMAIL),
        ModInfo.Get(ModInfo.WEBLINK),
        ModInfo.Get(ModInfo.RELEASEDATE),
		ModInfo.Get(ModInfo.UPDATEDBY),
		ModInfo.Get(ModInfo.COMPILEDATE));

    ZeroMemory(&m_bmpInfo, sizeof(m_bmpInfo));
    m_bmpInfo.biSize = sizeof(m_bmpInfo);
    m_bmpInfo.biPlanes = 1;
    m_bmpInfo.biCompression = BI_RGB;

    Refresh();
}

//-----------------------------------------------------------------------------

bbAbout::~bbAbout() {
    delete[] m_Text;
    DeleteObject((HBITMAP)SelectObject(m_hSecondaryBuf, m_hBitmapNull));
    DeleteObject((HFONT)SelectObject(m_hSecondaryBuf, m_hFontNull));
    DeleteDC(m_hSecondaryBuf);
    DestroyWindow(m_AboutHwnd);
    UnregisterClass(m_WindowName, m_hostInstance);
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK bbAbout::_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    bbAbout *p = (bbAbout*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return (p && p->_Dispatch(message, wParam, lParam)) ? 0: DefWindowProc(hwnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------

inline bool bbAbout::_Dispatch(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(m_AboutHwnd, &ps);
                BitBlt(
                    hdc,
                    ps.rcPaint.left,
                    ps.rcPaint.top,
                    (ps.rcPaint.right - ps.rcPaint.left),
                    (ps.rcPaint.bottom - ps.rcPaint.top),
                    m_hSecondaryBuf,
                    ps.rcPaint.left,
                    ps.rcPaint.top,
                    SRCCOPY);
                EndPaint(m_AboutHwnd, &ps);
            }
            return true;

        case WM_LBUTTONDOWN:
            *m_pThis = NULL;
            delete this;
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

void bbAbout::Refresh() {
    SIZE sz;
    RECT r = {0, 0, 0, 0};
    StyleItem s = *(StyleItem*)GetSettingPtr(SN_MENUTITLE);
    int border_width = *(int*)GetSettingPtr(SN_BORDERWIDTH);

    SetTextColor(m_hSecondaryBuf, s.TextColor);
    DeleteObject((HFONT)SelectObject(m_hSecondaryBuf, CreateStyleFont(&s)));

    GetTextExtentPoint32(m_hSecondaryBuf, "A", 1, &sz);
    DrawText(m_hSecondaryBuf, m_Text, -1, &r, DT_LEFT|DT_NOCLIP|DT_CALCRECT);

    int win_border = border_width + 4;

    m_Width = r.right + 2 * win_border;
    m_Height = r.bottom + 2 * win_border;

    m_PosRect.left = (GetSystemMetrics(SM_CXSCREEN) - m_Width) / 2;
    m_PosRect.top = (GetSystemMetrics(SM_CYSCREEN) - m_Height) / 2;

    SetWindowPos(
        m_AboutHwnd,
        HWND_TOPMOST,
        m_PosRect.left,
        m_PosRect.top,
        m_Width,
        m_Height,
        SWP_NOACTIVATE|SWP_NOSENDCHANGING);

    HDC hdc = GetDC(NULL);
    m_bmpInfo.biBitCount = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    m_bmpInfo.biWidth = m_Width;
    m_bmpInfo.biHeight = m_Height;
    DeleteObject((HBITMAP)SelectObject(m_hSecondaryBuf, CreateDIBSection(
        NULL, (BITMAPINFO*)&m_bmpInfo, DIB_RGB_COLORS, NULL, NULL, 0)));

    r.right = m_Width;
    r.bottom = m_Height;

    CreateBorder(m_hSecondaryBuf, &r, *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR), border_width);

    r.left =
        r.top = border_width;
    r.right = m_Width - border_width;
    r.bottom = m_Height - border_width;

    MakeStyleGradient(m_hSecondaryBuf, &r, &s, false);

    r.left =
        r.top = win_border;
    r.right = m_Width - win_border;

    DrawText(
        m_hSecondaryBuf,
        m_Text,
        -1,
        &r,
        s.Justify|DT_NOCLIP);

    InvalidateRect(m_AboutHwnd, NULL, false);
}

//-----------------------------------------------------------------------------
