/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003-2009 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2) For details see:

    http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BBApi.h"
#include "win0x500.h"
#include "BImage.h"
#include "hookinfo.h"
#include "subclass.h"
#include "drawico.h"

#define array_count(ary) (sizeof(ary) / sizeof(ary[0]))

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

    if (w)
    {
        SystemParametersInfo(SPI_GETWORKAREA, 0, w, 0);
    }

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

void window_set_pos(HWND hwnd, RECT rc)
{
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    SetWindowPos(hwnd, NULL,
        rc.left, rc.top, width, height,
        SWP_NOZORDER|SWP_NOACTIVATE);
}

int get_shade_height(HWND hwnd)
{
    return SendMessage(hwnd, bbSkinMsg, BBLS_GETSHADEHEIGHT, 0);
}

void ShadeWindow(HWND hwnd)
{
    RECT rc; get_rect(hwnd, &rc);
    int height = rc.bottom - rc.top;
    LPARAM prop = (LPARAM)GetProp(hwnd, BBSHADE_PROP);

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

    prop = MAKELPARAM(h1, h2);
    if (0 == prop) 
        RemoveProp(hwnd, BBSHADE_PROP);
    else 
        SetProp(hwnd, BBSHADE_PROP, (PVOID)prop);

    rc.bottom = rc.top + height;
    window_set_pos(hwnd, rc);
}

//===========================================================================
void ToggleShadeWindow(HWND hwnd)
{
    if (BBVERSION_LEAN == mSkin.BBVersion)
        SendMessage(mSkin.BBhwnd, BB_WINDOWSHADE, 0, (LPARAM)hwnd);
    else
        ShadeWindow(hwnd);
}

//-----------------------------------------------------------------
void post_redraw(HWND hwnd)
{
    PostMessage(hwnd, bbSkinMsg, BBLS_REDRAW, 0);
}

//===========================================================================

void exec_button_action(WinInfo *WI, int n)
{
    HWND hwnd = WI->hwnd;
    WPARAM SC_xxx;

    switch(n)
    {
        case btn_Close:
            SC_xxx = SC_CLOSE;
        Post_SC:
            {
                POINT pt;
                GetCursorPos(&pt);
                PostMessage(hwnd, WM_SYSCOMMAND, SC_xxx, MAKELPARAM(pt.x, pt.y));
            }
            break;

        case btn_Min:
            if (WI->style & WS_MINIMIZEBOX) {
                if (BBVERSION_LEAN == mSkin.BBVersion) {
                    PostMessage(mSkin.BBhwnd, IsIconic(hwnd) ? BB_WINDOWRESTORE : BB_WINDOWMINIMIZE, 0, (LPARAM)hwnd);
                } else {
                    SC_xxx = IsIconic(hwnd) ? SC_RESTORE : SC_MINIMIZE;
                    goto Post_SC;
                }
            }
            break;

        case btn_Max:
            if (WI->style & WS_MAXIMIZEBOX) {
                if (BBVERSION_LEAN == mSkin.BBVersion) {
                    PostMessage(mSkin.BBhwnd, IsZoomed(hwnd) ? BB_WINDOWRESTORE : BB_WINDOWMAXIMIZE, 0, (LPARAM)hwnd);
                } else {
                    SC_xxx = IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE;
                    goto Post_SC;
                }
            }
            break;

        case btn_VMax:
            if (WI->style & WS_MAXIMIZEBOX)
                PostMessage(mSkin.BBhwnd, BB_WINDOWGROWHEIGHT, 0, (LPARAM)hwnd);
            break;

        case btn_HMax:
            if (WI->style & WS_MAXIMIZEBOX)
                PostMessage(mSkin.BBhwnd, BB_WINDOWGROWWIDTH, 0, (LPARAM)hwnd);
            break;

        case btn_TMin:
            if (WI->style & WS_MINIMIZEBOX)
                PostMessage(mSkin.BBhwnd, BB_WINDOWMINIMIZETOTRAY, 0, (LPARAM)hwnd);
            break;

        case btn_Lower:
            PostMessage(mSkin.BBhwnd, BB_WINDOWLOWER, 0, (LPARAM)hwnd);
            break;

        case btn_Rollup:
            if (WI->style & WS_SIZEBOX)
                ToggleShadeWindow(hwnd);
            break;

        case btn_Sticky:
            WI->is_sticky = false == WI->is_sticky;
            PostMessage(mSkin.BBhwnd, BB_WORKSPACE,
                WI->is_sticky ? BBWS_MAKESTICKY : BBWS_CLEARSTICKY,
                (LPARAM)hwnd
                );
            break;

        case btn_OnTop:
            if (BBVERSION_LEAN == mSkin.BBVersion)
                PostMessage(mSkin.BBhwnd, BB_WORKSPACE, BBWS_TOGGLEONTOP, (LPARAM)hwnd);
            else
                SetWindowPos(hwnd,
                    WI->is_ontop ? HWND_NOTOPMOST : HWND_TOPMOST,
                    0, 0, 0, 0,
                    SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            break;

        case btn_SysMenu:
        case btn_Icon:
            // PostMessage(hwnd, 0x0313, 0, MAKELPARAM(30, 30));
            if (0 == (WI->style & WS_CHILD))
            {
                COPYDATASTRUCT cds;
                struct sysmenu_info s;
                int b = mSkin.F.Title.borderWidth;

                GetWindowRect(hwnd, &s.rect);
                s.rect.bottom = s.rect.top + WI->S.HiddenTop + mSkin.ncTop - b;
                s.rect.right = s.rect.left + WI->S.width - WI->S.HiddenSide;
                s.rect.top += WI->S.HiddenTop + b;
                s.rect.left += WI->S.HiddenSide;
                if (btn_SysMenu == n)
                    s.rect.right = s.rect.left;
                s.hwnd = hwnd;
#ifndef _WIN64
                s._pad32 = 0;
#endif
                cds.dwData = 202;
                cds.cbData = sizeof s;
                cds.lpData = &s;
                SendMessage(mSkin.loghwnd, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&cds);
                post_redraw(hwnd);
            }
            break;
    }
}

//-----------------------------------------------------------------
void DeleteBitmaps(WinInfo *WI)
{
    int n = NUMOFGDIOBJS;
    GdiInfo *pGdi = WI->gdiobjs;
    do {
        if (pGdi->hObj)
            DeleteObject(pGdi->hObj), pGdi->hObj = NULL;
        pGdi++;
    } while (--n);
}

void PutGradient(WinInfo *WI, HDC hdc, RECT *rc, GradientItem *pG)
{
    if (pG->parentRelative) {
        if (pG->borderWidth)
            CreateBorder(hdc, rc, pG->borderColor, pG->borderWidth);
        return;
    }

    int width = rc->right - rc->left;
    int height = rc->bottom - rc->top;
    int i = pG >= &mSkin.F.Title
        ? pG - &mSkin.F.Title + 6
        : pG - &mSkin.U.Title;

    GdiInfo *pGdi = WI->gdiobjs + i;
    HBITMAP bmp = (HBITMAP)pGdi->hObj;
    HGDIOBJ other;

    if (bmp && width == pGdi->cx && height == pGdi->cy) {
        other = SelectObject(WI->buf, bmp);

    } else {
        StyleItem si;
        RECT r;

        r.left = r.top = 0;
        r.right = width, r.bottom = height;

        if (bmp)
            DeleteObject(bmp);

        pGdi->cx = width;
        pGdi->cx = height;
        pGdi->hObj = bmp = CreateCompatibleBitmap(hdc, width, height);

        if (NULL == bmp)
            return;

        other = SelectObject(WI->buf, bmp);

        copy_GradientItem(&si, pG);
        MakeStyleGradient(WI->buf, &r, &si, true);
    }

    BitBlt(hdc, rc->left, rc->top, width, height, WI->buf, 0, 0, SRCCOPY);
    SelectObject(WI->buf, other);
}

//-----------------------------------------------------------------

void DrawButton(WinInfo *WI, HDC hdc, RECT rc, int btn, int state, GradientItem *pG)
{
    int x, y, xa, ya, xe, ye;
    unsigned char *up;
    COLORREF c;
    unsigned bits;

    PutGradient(WI, hdc, &rc, pG);

    c = pG->picColor;
    up = mSkin.glyphmap;
    x = up[0];
    y = up[1];
    up = up+2+(btn*2+state)*ONE_GLYPH(x,y);

    xe = (xa = (rc.left + rc.right - x)/2) + x;
    ye = (ya = (rc.top + rc.bottom - y)/2) + y;

    bits = 0;
    y = ya;
    do {
        x = xa;
        do {
            if (bits < 2)
                bits = 256 | *up++;
            if (bits & 1)
                SetPixel(hdc, x, y, c);
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
#if 1
int get_window_icon(HWND hwnd, HICON *picon)
{
    HICON hIco = NULL;
    SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL,
        0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 1000, (DWORD_PTR*)&hIco);
    if (NULL==hIco) {
    SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG,
        0, SMTO_ABORTIFHUNG|SMTO_NORMAL, 1000, (DWORD_PTR*)&hIco);
    if (NULL==hIco) {
        hIco = (HICON)GetClassLongPtr(hwnd, GCLP_HICONSM);
    if (NULL==hIco) {
        hIco = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    if (NULL==hIco) {
        return 0;
    }}}}
    *picon = hIco;
    return 1;
}

#else
#define DrawIconSatnHue(hDC, px, py, m_hIcon, sizex, sizey, anistep, hbr, flags, apply, sat, hue)
#define get_ico(hwnd) NULL
#endif

//-----------------------------------------------------------------

void PaintAll(struct WinInfo* WI)
{
    int left    = WI->S.HiddenSide;
    int width   = WI->S.width;
    int right   = width - WI->S.HiddenSide;

    int top     = WI->S.HiddenTop;
    int bottom  = WI->S.height - WI->S.HiddenBottom;
    int title_height = mSkin.ncTop;
    int title_bottom = top + title_height;

    HWND focus;
    int active;

    RECT rc;
    HDC hdc, hdc_win;
    HGDIOBJ hbmpOld;
    GradientItem *pG;
    windowGradients *wG;

    //dbg_printf("painting %x", WI->hwnd);

    active =
        (WI->is_active_app
            && (WI->is_active
                || WI->hwnd == (focus = GetFocus())
                || IsChild(WI->hwnd, focus)
                ))
        || (mSkin.bbsm_option & 1);

    hdc_win = GetWindowDC(WI->hwnd);
    hdc = CreateCompatibleDC(hdc_win);
    WI->buf = CreateCompatibleDC(hdc_win);
    hbmpOld = SelectObject(hdc, CreateCompatibleBitmap(hdc_win, width, title_bottom));

    wG = &mSkin.U + active;

    //----------------------------------
    // Titlebar gradient

    rc.top = top;
    rc.left = left;
    rc.right = right;
    rc.bottom = title_bottom;
    pG = &wG->Title;
    PutGradient(WI, hdc, &rc, pG);

    //----------------------------------
    // Titlebar Buttons

    rc.top = top + mSkin.buttonMargin;
    rc.bottom = rc.top + mSkin.buttonSize;

    LONG w_style = WI->style;
    HICON hico = NULL;
    pG = &wG->Button;

    int label_left = left;
    int label_right = right;
    int space = mSkin.buttonMargin;
    int d, i;

    for (d = 1, WI->button_count = i = 0; ; i += d)
    {
        bool state;
        int b;
        struct button_set *p;

        b = mSkin.button_string[i] - '0';
        switch (b) {
            case 0 - '0':
                goto _break;
            case '-' - '0':
                if (d < 0)
                    goto _break;
                d = -1;
                i = strlen(mSkin.button_string);
                space = mSkin.buttonMargin;
                continue;
            case btn_Rollup:
                if (0 == (w_style & WS_SIZEBOX))
                    continue;
                state = WI->is_rolled;
                break;
            case btn_Sticky:
                if ((w_style & WS_CHILD) || 0 == WI->has_sticky)
                    continue;
                state = WI->is_sticky;
                break;
            case btn_OnTop:
                if (w_style & WS_CHILD)
                    continue;
                state = WI->is_ontop;
                break;
            case btn_Min:
                if (0 == (w_style & WS_MINIMIZEBOX))
                    continue;
                state = WI->is_iconic;
                break;
            case btn_Max:
                if (0 == (w_style & WS_MAXIMIZEBOX))
                    continue;
                state = WI->is_zoomed;
                break;
            case btn_Close:
                state = false;
                break;
            case btn_Icon:
                if (!get_window_icon(WI->hwnd, &hico))
                    continue;
                state = !active;
                break;
            default:
                continue;
        }

        if (d > 0) // left button
        {
            rc.left = label_left + space;
            label_left = rc.right = rc.left + mSkin.buttonSize;
        }
        else // right button
        {
            rc.right = label_right - space;
            label_right = rc.left = rc.right - mSkin.buttonSize;
        }

        p = &WI->button_set[(int)WI->button_count];
        p->set = b;
        p->pos = rc.left;
        space = mSkin.buttonSpace;

        if (btn_Icon == b) {
            int s = mSkin.buttonSize;
            int o = (s - 16)/2;
            if (o < 0)
                o = 0;
            else
                s = 16;
            DrawIconSatnHue(hdc, rc.left+o, rc.top+o, hico, s, s, 0, NULL, DI_NORMAL, state, mSkin.iconSat, mSkin.iconHue);
        } else {
            int pressed = WI->button_down == b
                || ((2 & mSkin.bbsm_option) && btn_Close == b);
            DrawButton(WI, hdc, rc, b-1, state, pG + pressed);
        }

        if (++WI->button_count == BUTTON_COUNT)
            _break: break;
    }

    //----------------------------------
    // Titlebar Label gradient

    rc.left = label_left + (left == label_left ? mSkin.labelMargin : mSkin.buttonInnerMargin);
    rc.right = label_right - (right == label_right ? mSkin.labelMargin : mSkin.buttonInnerMargin);
    rc.top = top + mSkin.labelMargin;
    rc.bottom = title_bottom - mSkin.labelMargin;

    pG = &wG->Label;
    PutGradient(WI, hdc, &rc, pG);

    //----------------------------------
    // Titlebar Text

    rc.left += mSkin.labelIndent;
    rc.right -= mSkin.labelIndent;

    if (NULL == WI->hFont)
        WI->hFont = CreateFontIndirect(&mSkin.Font);
    HGDIOBJ hfontOld = SelectObject(hdc, WI->hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, pG->TextColor);

    WCHAR wTitle[200];
    wTitle[0] = 0;

    if (WI->is_unicode) {
        GetWindowTextW(WI->hwnd, wTitle, array_count(wTitle));
        DrawTextW(hdc, wTitle, -1, &rc,
            mSkin.Justify
            | DT_SINGLELINE
            | DT_NOPREFIX
            | DT_END_ELLIPSIS
            | DT_VCENTER
            );
    } else {
        GetWindowText(WI->hwnd, (char*)wTitle, array_count(wTitle));
        DrawText(hdc, (char*)wTitle, -1, &rc,
            mSkin.Justify
            | DT_SINGLELINE
            | DT_NOPREFIX
            | DT_END_ELLIPSIS
            | DT_VCENTER
            );
    }

    SelectObject(hdc, hfontOld);    

    //----------------------------------
    // Blit the title

    BitBlt(hdc_win, left, top, right-left, title_height, hdc, left, top, SRCCOPY);

    //----------------------------------
    // Frame and Bottom

    if (false == WI->is_iconic
      && (false == WI->is_rolled
            || (false == mSkin.nixShadeStyle && mSkin.handleHeight)))
    {
        int fw = mSkin.frameWidth;

        if (fw)
        {
            //----------------------------------
            // Frame left/right(/bottom) border, drawn directly on screen

            rc.top = title_bottom;
            rc.left = left;
            rc.right = right;
            rc.bottom = bottom - WI->S.BottomHeight;

            COLORREF bc = wG->FrameColor;
            HGDIOBJ oldPen = SelectObject(hdc_win, CreatePen(PS_SOLID, 1, bc));
            draw_line(hdc_win, rc.left, rc.left, rc.top, rc.bottom, fw);
            draw_line(hdc_win, rc.right-fw, rc.right-fw, rc.top, rc.bottom, fw);
            if (WI->S.BottomHeight == fw)
                draw_line(hdc_win, rc.left, rc.right, rc.bottom, rc.bottom, fw);
            DeleteObject(SelectObject(hdc_win, oldPen));
        }

        if (WI->S.BottomHeight > fw)
        {
            int gw = mSkin.gripWidth;
            GradientItem *pG2;
            pG = &wG->Handle;
            pG2 = &wG->Grip;
            int bw = imin(pG->borderWidth, pG2->borderWidth);

            //----------------------------------
            // Bottom Handle gradient
            rc.top = 0;
            rc.bottom = WI->S.BottomHeight;
            rc.left = left;
            rc.right = right;
            if (false == pG2->parentRelative) {
                rc.left += gw - bw;
                rc.right -= gw - bw;
            }
            PutGradient(WI, hdc, &rc, pG);

            //----------------------------------
            //Bottom Grips
            if (false == pG2->parentRelative) {
                rc.right = (rc.left = left) + gw;
                PutGradient(WI, hdc, &rc, pG2);
                rc.left = (rc.right = right) - gw;
                PutGradient(WI, hdc, &rc, pG2);
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
bool get_rolled(WinInfo *WI)
{
    DWORD prop = (DWORD)(DWORD_PTR)GetProp(WI->hwnd, BBSHADE_PROP);
    bool rolled = 0 != (IsZoomed(WI->hwnd) 
        ? HIWORD(prop) 
        : LOWORD(prop));
    return WI->is_rolled = rolled;
}

//-----------------------------------------------------------------
// cut off left/right sizeborder and adjust title height
bool set_region(WinInfo *WI)
{
    HWND hwnd = WI->hwnd;

    WI->style = GetWindowLong(hwnd, GWL_STYLE);
    WI->exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    WI->is_ontop = 0 != (WI->exstyle & WS_EX_TOPMOST);
    WI->is_zoomed = FALSE != IsZoomed(hwnd);
    WI->is_iconic = FALSE != IsIconic(hwnd);

    // since the 'SetWindowRgn' throws a WM_WINPOSCHANGED,
    // and WM_WINPOSCHANGED calls set_region, ...
    if (WI->in_set_region)
        return false;

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

    int c = mSkin.cyCaption;
    int b = mSkin.cxSizeFrame;
    int bh = mSkin.ncBottom;

    if (WS_EX_TOOLWINDOW & WI->exstyle)
        c = mSkin.cySmCaption;

    if (0 == (WS_SIZEBOX & WI->style)) {
        b = mSkin.cxFixedFrame;
        bh = mSkin.frameWidth;
    }
    else
    if (WI->is_zoomed && false == WI->is_rolled) {
        bh = mSkin.frameWidth;
    }

    WI->S.HiddenTop = imax(0, b - mSkin.ncTop + c);
    WI->S.HiddenSide = imax(0, b - mSkin.frameWidth);
    WI->S.HiddenBottom = imax(0, b - bh);
    WI->S.BottomAdjust = imax(0, bh - b);
    WI->S.BottomHeight = bh;

    if (WI->is_rolled)
        WI->S.HiddenBottom = 0;

    if (0 == memcmp(&S, &WI->S, sizeof S) && WI->apply_skin)
        return false; // nothing changed

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
    char dllpath[MAX_PATH];
    WinInfo *WI;
    WI = (WinInfo *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof *WI);

    // Keep a reference to this piece of code to prevent the
    // engine from being unloaded by chance (i.e. if BB crashes)
    GetModuleFileName(hInstance, dllpath, sizeof dllpath);
    WI->hModule = LoadLibrary(dllpath);

    WI->hwnd = hwnd;
    WI->is_unicode = FALSE != IsWindowUnicode(hwnd);
    WI->pCallWindowProc = WI->is_unicode ? CallWindowProcW : CallWindowProcA;
    WI->is_active_app = true;

    set_WinInfo(hwnd, WI);
    WI->wpOrigWindowProc = (WNDPROC)
        (WI->is_unicode ? SetWindowLongPtrW : SetWindowLongPtrA)
            (hwnd, GWLP_WNDPROC, (LONG_PTR)WindowSubclassProc);

    // some programs dont handle the posted "BBLS_LOAD" msg, so set
    // the region here (is this causing the 'opera does not start' issue?)
    // Also, the console (class=tty) does not handle posted messages at all.
    // Have to use the "hook-early:" option with such windows.
    // With other windows again that would cause troubles, like with tabbed
    // dialogs, which seem to subclass themselves during creation.
    set_region(WI);
}

// This is where it ends then
void detach_skinner(WinInfo *WI)
{
    HWND hwnd = WI->hwnd;
    HMODULE hModule = WI->hModule;
    // clean up
    DeleteBitmaps(WI);
    // the currently set WindowProc
    WNDPROC wpNow = (WNDPROC)(WI->is_unicode ? GetWindowLongPtrW : GetWindowLongPtrA)(hwnd, GWLP_WNDPROC);
    // check, if it's still what we have set
    if (WindowSubclassProc == wpNow) {
        // if so, set back to the original WindowProc
        SetLastError(0);
        (WI->is_unicode ? SetWindowLongPtrW : SetWindowLongPtrA)
            (hwnd, GWLP_WNDPROC, (LONG_PTR)WI->wpOrigWindowProc);
        if (0 == GetLastError()) {
            // remove the property
            del_WinInfo(hwnd);
            // free the WinInfo structure
            GlobalFree (WI);
            // if blackbox is still there, release this dll
            if (hModule && IsWindow(mSkin.loghwnd))
                FreeLibrary(hModule);
            // send a note to the log window
            send_log(hwnd, "Released");
            return;
        }
    }
    // otherwise, the subclassing must not be terminated
    send_log(hwnd, "Subclassed otherwise, cannot release");
}

//-----------------------------------------------------------------
// get button-id from mousepoint
int get_button (struct WinInfo *WI, int x, int y)
{
    int button_top = WI->S.HiddenTop + mSkin.buttonMargin;
    int margin = WI->S.HiddenSide + mSkin.buttonMargin;
    RECT rc; GetWindowRect(WI->hwnd, &rc);
    y -= rc.top;
    x -= rc.left;
    int n;

    if (y < button_top)
    {
        if (x < mSkin.ncTop)
            n = btn_Topleft;
        else
        if (x >= WI->S.width - mSkin.ncTop)
            n = btn_Topright;
        else
            n = btn_Top;
    }
    else
    if (y >= WI->S.HiddenTop + mSkin.ncTop && false == WI->is_rolled)
        n = btn_None;
    else
    if (y >= button_top + mSkin.buttonSize)
        n = btn_Caption;
    else
    if (x < margin)
        n = btn_Topleft;
    else
    if (x >= WI->S.width - margin)
        n = btn_Topright;
    else
    {
        int i, c = WI->button_count;
        struct button_set *p = WI->button_set;
        for (i = 0; i < c; i++, p++)
            if (x >= p->pos && x < p->pos + mSkin.buttonSize)
                break;
        n = i < c ? p->set : btn_Caption;
    }
    if (n >= btn_Topleft)
    {
        if (WI->is_zoomed)
            n = btn_Caption;
        if (WI->is_iconic)
            n = btn_None;
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
        if (WI->style & WS_SIZEBOX)
            return n;
        return HTCAPTION;
    }
    return n+100;
}

//-----------------------------------------------------------------
// translate hittest value to button-id
int translate_button(WPARAM wParam)
{
    if (HTMINBUTTON == wParam)
        return btn_Min;
    if (HTMAXBUTTON == wParam)
        return btn_Max;
    if (HTCLOSE == wParam)
        return btn_Close;
    return wParam - 100;
}

//-----------------------------------------------------------------
// titlebar click actions
int get_caption_click(WPARAM wParam, char *pCA)
{
    if (HTCAPTION != wParam)
        return btn_None;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        return pCA[1];
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        return pCA[2];
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
        return pCA[3];
    return pCA[0];
}

//-----------------------------------------------------------------
//#define LOGMSGS

#ifdef LOGMSGS
#include "../winmsgs.cpp"
#endif

//===========================================================================

LRESULT APIENTRY WindowSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WinInfo *WI;
    LRESULT result;
    int n;

    result = 0;
    WI = get_WinInfo(hwnd);

#ifdef LOGMSGS
    dbg_printf("hw %08x  msg %s  wP %08x  lp %08x", hwnd, wm_str(uMsg), wParam, lParam);
#endif

    if (NULL == WI)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    if (WI->apply_skin
        || uMsg == WM_NCDESTROY
        || uMsg == WM_STYLECHANGED
        || uMsg == bbSkinMsg)
    switch (uMsg)
    {
    //----------------------------------
    case WM_NCPAINT:
    {
        RECT rc;
        HRGN hrgn;
        bool flag_save;
        BOOL locked;

        if (WI->dont_paint)
            goto leave;

        if (WI->is_rolled)
            goto paint_now;

        if (WI->sync_paint)
            goto paint_after;

        // Okay, so let's create an own region and pass that to
        // the original WndProc instead of (HRGN)wParam
        GetWindowRect(hwnd, &rc);
        hrgn = CreateRectRgn(
            rc.left + WI->S.HiddenSide + mSkin.frameWidth,
            rc.top + WI->S.HiddenTop + mSkin.ncTop,
            rc.right - WI->S.HiddenSide - mSkin.frameWidth,
            rc.bottom - WI->S.HiddenBottom - WI->S.BottomHeight
            );
        result = CALLORIGWINDOWPROC(hwnd, uMsg, (WPARAM)hrgn, lParam);
        DeleteObject(hrgn);

    paint_now:
        PaintAll(WI);
        goto leave;

    paint_after:
        flag_save = WI->dont_paint;
        WI->dont_paint = true;
        result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
        WI->dont_paint = flag_save;
        goto paint_now;

    paint_locked:
        // smooth, but slow and dangerous also
        if (false == mSkin.drawLocked)
            goto paint_after;

        flag_save = WI->dont_paint;
        WI->dont_paint = true;
        locked = LockWindowUpdate(hwnd);
        result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
        if (locked) LockWindowUpdate(NULL);
        WI->dont_paint = flag_save;
        goto paint_now;
    }

    case WM_SYNCPAINT:
        // this is when the user moves other windows over our window. The
        // DefWindowProc will call WM_NCPAINT, WM_ERASEBKGND and WM_PAINT
        WI->sync_paint = true;
        result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
        WI->sync_paint = false;
        goto leave;

    //----------------------------------

    //----------------------------------
    // Windows draws the caption on WM_SETTEXT/WM_SETICON

    case WM_SETICON:
    case WM_SETTEXT:
        if ((WI->exstyle & WS_EX_MDICHILD) && IsZoomed(hwnd))
        {
            post_redraw(GetRootWindow(hwnd));
            break;
        }
        goto paint_locked;

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

            default: goto paint_locked;
        }
        SetCursor(LoadCursor(NULL, CU));
        result = 1;
        goto leave;
    }

    //----------------------------------

    case WM_ACTIVATEAPP:
        //dbg_printf("WM_ACTIVATEAPP %d", wParam);
        WI->is_active_app = 0 != wParam;
        post_redraw(hwnd);
        break;

    case WM_NCACTIVATE:
        //dbg_printf("WM_NCACTIVATE %d", wParam);
        WI->is_active = 0 != wParam;
        post_redraw(hwnd);
        goto paint_after;

    //----------------------------------
    case WM_NCHITTEST:
        n = get_button(WI, (short)LOWORD(lParam), (short)HIWORD(lParam));
        if (btn_None == n) {
            result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
        } else {
            result = translate_hittest(WI, n);
        }

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
            WI->button_down = (char)n;
            post_redraw(hwnd);
        }
        goto leave;


    //----------------------------------

    set_capture:
        WI->capture_button = WI->button_down = (char)n;
        SetCapture(hwnd);
        post_redraw(hwnd);
        goto leave;

    exec_action:
        WI->capture_button = WI->button_down = btn_None;
        ReleaseCapture();
        exec_button_action(WI, n);
        post_redraw(hwnd);
        goto leave;

    case WM_CAPTURECHANGED:
        if (btn_None == WI->capture_button)
            break;
        if (0 == lParam)
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
        n = WI->button_down;
        if (btn_Max == n) {
            n = (wParam & MK_SHIFT) ? btn_VMax : btn_HMax;
        } else if (btn_Min == n) {
            n = btn_TMin;
        } else {
            n = btn_None;
        }
        goto exec_action;

    case WM_MBUTTONUP:
        if (btn_None == WI->capture_button)
            break;
        n = WI->button_down;
        if (btn_Max == n) {
            n = btn_VMax;
        } else {
            n = btn_None;
        }
        goto exec_action;

    //----------------------------------
    case WM_NCLBUTTONDBLCLK:
        n = get_caption_click(wParam, mSkin.captionClicks.Dbl);
        if (btn_None == n)
            goto case_WM_NCLBUTTONDOWN;
        exec_button_action(WI, n);
        goto leave;

    case_WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
        n = translate_button(wParam);
        if (n > btn_None && n <= btn_Last)
            goto set_capture; // clicked in a button
        if (btn_None == get_caption_click(wParam, mSkin.captionClicks.Left))
            break;
        goto leave;

    case WM_NCLBUTTONUP:
        n = get_caption_click(wParam, mSkin.captionClicks.Left);
        if (btn_None == n)
            break;
        exec_button_action(WI, n);
        goto leave;

    //----------------------------------
    case WM_NCRBUTTONDOWN:
        n = translate_button(wParam);
        if (btn_Max == n || btn_Min == n)
            goto set_capture;
    case WM_NCRBUTTONDBLCLK:
        if (btn_None == get_caption_click(wParam, mSkin.captionClicks.Right))
            break;
        goto leave;

    case WM_NCRBUTTONUP:
        n = get_caption_click(wParam, mSkin.captionClicks.Right);
        if (btn_None == n)
            break;
        exec_button_action(WI, n);
        goto leave;

    //----------------------------------
    case WM_NCMBUTTONDOWN:
        n = translate_button(wParam);
        if (btn_Max == n)
            goto set_capture;
    case WM_NCMBUTTONDBLCLK:
        if (btn_None == get_caption_click(wParam, mSkin.captionClicks.Mid))
            break;
        goto leave;

    case WM_NCMBUTTONUP:
        n = get_caption_click(wParam, mSkin.captionClicks.Mid);
        if (btn_None == n)
            break;
        exec_button_action(WI, n);
        goto leave;

    //----------------------------------
    case WM_SYSCOMMAND:
        // dbg_printf("WM_SYSCOMMAND: %08x %08x", wParam, lParam);
        // ----------
        // these SYSCOMMAND's enter the 'window move/size' modal loop
        if ((wParam >= 0xf001 && wParam <= 0xf008) // size
            || wParam == 0xf012 // move
            )
        {
            // draw the caption before
            PaintAll(WI);
            break;
        }
#if 1
        if (wParam == 0xF100 && lParam == ' ') // SC_KEYMENU + spacebar
        {
            exec_button_action(WI, btn_SysMenu);
            goto leave;
        }
#endif
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
        if (wParam == SC_CLOSE && WI->is_rolled)
        {
            // unshade the window on close, just in case it wants
            // to store it's size
            ToggleShadeWindow(hwnd);
            break;
        }

        post_redraw(hwnd);
        break;

    //----------------------------------
    // set flag, whether snapWindows should be applied below
    case WM_ENTERSIZEMOVE:
        WI->is_moving = true;
        break;

    case WM_EXITSIZEMOVE:
        WI->is_moving = false;
        break;

    //----------------------------------
    // If moved, snap to screen edges...
    case WM_WINDOWPOSCHANGING:
    {
        WINDOWPOS *wp = (WINDOWPOS*)lParam;
        if (WI->is_moving
         && mSkin.snapWindows
         && 0 == (WS_CHILD & WI->style)
         && ((wp->flags & SWP_NOSIZE)
             || (WI->S.width == wp->cx 
                && WI->S.height == wp->cy)
            ))
            SnapWindowToEdge(WI, (WINDOWPOS*)lParam, 7);

        if (get_rolled(WI)) {
            wp->cy = mSkin.rollupHeight + WI->S.HiddenTop;
            // prevent app from possibly setting a minimum size
            goto leave;
        }
        break;
    }

    //----------------------------------
    case WM_WINDOWPOSCHANGED:
        // we do not let the app know about rolled state
        if (false == WI->is_rolled)
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
            ((NCCALCSIZE_PARAMS*)lParam)->rgrc[0].bottom -= WI->S.BottomAdjust;
            goto leave;
        }
        break;

    //----------------------------------
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
        lpmmi->ptMinTrackSize.x = 12 * mSkin.buttonSize;
        lpmmi->ptMinTrackSize.y = 
            mSkin.ncTop 
            + WI->S.HiddenTop 
            + WI->S.BottomHeight 
            - WI->S.HiddenBottom 
            - mSkin.frameWidth;
        break;
    }

    //----------------------------------
    // Terminate subclassing

    case WM_NCDESTROY:
        WI->apply_skin = false;
        result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
        detach_skinner(WI);
        goto leave;

    //----------------------------------
    default:
        if (uMsg == bbSkinMsg) { // our registered message
            switch (wParam)
            {
                // initialisation message
                case BBLS_LOAD:
                    break;

                // detach the skinner
                case BBLS_UNLOAD:
                    if (WI->is_rolled)
                        ToggleShadeWindow(hwnd);
                    WI->apply_skin = false;
                    SetWindowRgn(hwnd, NULL, TRUE);
                    detach_skinner(WI);
                    break;

                // repaint the caption
                case BBLS_REDRAW:
                    if (WI->apply_skin)
                        goto paint_now;
                    break;

                // changed Skin
                case BBLS_REFRESH:
                    GetSkin();
                    DeleteBitmaps(WI);
                    if (WI->apply_skin && false == set_region(WI))
                        goto paint_now;
                    break;

                // set sticky button state, sent from BB
                case BBLS_SETSTICKY:
                    WI->has_sticky = true;
                    WI->is_sticky = 0 != lParam;
                    if (WI->apply_skin)
                        goto paint_now;
                    break;

                case BBLS_GETSHADEHEIGHT:
                    result = mSkin.rollupHeight + WI->S.HiddenTop;
                    break;
            }
            goto leave;
        }

    //----------------------------------
    } //switch

    result = CALLORIGWINDOWPROC(hwnd, uMsg, wParam, lParam);
leave:
    return result;
}

//===========================================================================
