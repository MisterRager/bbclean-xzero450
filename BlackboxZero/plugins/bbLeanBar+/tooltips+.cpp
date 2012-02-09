/*
 ============================================================================
  This file is part of the bbLeanBar+ source code.

  bbLeanBar+ is a plugin for BlackBox for Windows
  Copyright © 2003-2009 grischka
  Copyright © 2008-2009 The Blackbox for Windows Development Team

  http://bb4win.sourceforge.net/bblean/

  bbLeanBar+ is free software, released under the GNU General Public License
  (GPL version 2). See for details:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BBApi.h"
#include <commctrl.h>
#include "win0x500.h"
#include "bblib.h"
#include "bbPlugin+.h"

static HWND hToolTips;
static bool usingNT;

struct tt
{
    struct tt *next;
    char used_flg;
    char text[256];
    WCHAR wstr[256];
    TOOLINFOW ti;
} *tt0;

void InitToolTips(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX ic;
    int n;

    usingNT = 0 == (GetVersion() & 0x80000000);

    for (n = 2;;) {
        hToolTips = CreateWindowEx(
            WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            0,0,0,0,
            NULL,
            NULL,
            NULL, //hInstance,
            NULL
            );

        if (hToolTips)
        {
            SendMessage(hToolTips, TTM_SETMAXTIPWIDTH, 0, 200);
            //SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 200);
            SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, 5000);
            SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_INITIAL,  200);
            SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_RESHOW,    40);
            break;
        }

        if (0 == --n)
            break;
        ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
        ic.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&ic);
    }
}

void ExitToolTips()
{
    struct tt **tp, *t;

    DestroyWindow(hToolTips);
    hToolTips = NULL;

    tp=&tt0;
    while (NULL!=(t=*tp))
        *tp = t->next, m_free(t);
}

//===========================================================================
// Function: SetToolTip
// Purpose: To assign a ToolTip to an icon in the system tray
// In:      the position of the icon, the text
// Out:     void
//===========================================================================

void SetToolTip(HWND hwnd, RECT *tipRect, const char *tipText)
{
    struct tt **tp, *t;
    UINT_PTR n = 0;

    if (NULL==hToolTips || 0 == *tipText)
        return;

    for (tp=&tt0; NULL!=(t=*tp); tp=&t->next)
    {
        if (hwnd == t->ti.hwnd && 0==memcmp(&t->ti.rect, tipRect, sizeof(RECT)))
        {
            t->used_flg = 1;
            if (0 != strcmp(t->text, tipText))
            {
                strcpy(t->text, tipText);
                if (usingNT) {
                    bbMB2WC(t->text, t->wstr, array_count(t->wstr));
                    SendMessage(hToolTips, TTM_UPDATETIPTEXTW, 0, (LPARAM)&t->ti);
                } else {
                    SendMessage(hToolTips, TTM_UPDATETIPTEXTA, 0, (LPARAM)&t->ti);
                }
            }
            return;
        }
        if (t->ti.uId > n)
            n = t->ti.uId;
    }

    *tp = t = c_new (struct tt);
    t->used_flg  = 1;

    strcpy(t->text, tipText);

    t->ti.cbSize   = sizeof t->ti;
    t->ti.uFlags   = TTF_SUBCLASS;
    t->ti.hwnd     = hwnd;
    t->ti.uId      = n+1;
    t->ti.hinst    = NULL;
    t->ti.rect = *tipRect;
    if (usingNT) {
        bbMB2WC(t->text, t->wstr, array_count(t->wstr));
        t->ti.lpszText = t->wstr;
        SendMessage(hToolTips, TTM_ADDTOOLW, 0, (LPARAM)&t->ti);
    } else {
        t->ti.lpszText = (WCHAR*)t->text;
        SendMessage(hToolTips, TTM_ADDTOOLA, 0, (LPARAM)&t->ti);
    }
}

//===========================================================================
// Function: ClearToolTips
// Purpose:  clear all tooltips, which are not longer used
//===========================================================================

void ClearToolTips(HWND hwnd)
{
    struct tt **tp, *t;

    tp=&tt0;
    while (NULL!=(t=*tp))
    {
        if (hwnd != t->ti.hwnd)
        {
            tp=&t->next;
        }
        else
        if (0==t->used_flg)
        {
            SendMessage(hToolTips, TTM_DELTOOL, 0, (LPARAM)&t->ti);
            *tp = t->next;
            m_free(t);
        }
        else
        {
            t->used_flg = 0;
            tp=&t->next;
        }
    }
}

//===========================================================================

//===========================================================================

struct plugin_info *g_bbb;

void exit_bb_balloon(void)
{
    while (g_bbb)
        delete g_bbb;
}


class bb_balloon : public plugin_info
{
    systemTray icon;
    int msgtop;
    int padding;
    int x_icon;
    int y_icon;
    int d_icon;
    bool finished;

public:
    bb_balloon(plugin_info * PI, systemTray *picon, RECT *r)
    {
        hInstance   = PI->hInstance;
        mon_rect    = PI->mon_rect;
        hMon        = PI->hMon;
        class_name  = "BBBalloon";
        alwaysOnTop = true;
        place       = POS_User;
        alphaValue  = PI->alphaValue;

        this->icon = *picon;
        this->finished = false;

        x_icon = (r->left+r->right)/2;
        y_icon = (r->top+r->bottom)/2;
        d_icon = (r->right - r->left);

        padding     = 8;
        calculate_size();
        BBP_Init_Plugin(this);
        this -> next = g_bbb;
        g_bbb = this;
    }

    ~bb_balloon()
    {
        this->finished = true;
        BBP_Exit_Plugin(this);
        for (struct plugin_info **pp = &g_bbb; *pp; pp = &(*pp)->next) {
            if (this == *pp) {
                *pp = this->next;
                break;
            }
        }
    }

private:
    void calculate_size(void)
    {
        int maxwidth = 240;

        RECT r1 = {0, 0, maxwidth, 0};
        RECT r2 = {0, 0, maxwidth, 0};
        HDC buf = CreateCompatibleDC(NULL);
        draw_text(buf, &r1, &r2, DT_CALCRECT | DT_WORDBREAK, 0);
        DeleteDC(buf);

        if (icon.balloon.szInfoTitle[0])
            msgtop = r1.bottom + padding;
        else
            msgtop = 0;

        width = imax(r1.right, r2.right) + 2*padding;
        height = msgtop + r2.bottom + 2*padding;

        int screen_center_x = (mon_rect.left + mon_rect.right) /2;
        int screen_center_y = (mon_rect.top + mon_rect.bottom) /2;

        int is = d_icon / 5;
        xpos = x_icon;
        ypos = y_icon;
        if ((xpos < screen_center_x && xpos - width >= mon_rect.left)
            || xpos + width > mon_rect.right
            )
            xpos -= width + is;
        else
            xpos += is;

        if ((ypos < screen_center_y && ypos - height >= mon_rect.top)
            || ypos + height > mon_rect.bottom
            )
            ypos -= height + is;
        else
            ypos += is;
    }

    void draw_text(HDC buf, RECT *r1, RECT *r2, UINT flags, COLORREF TC)
    {
        StyleItem F  = *(StyleItem *)GetSettingPtr(SN_TOOLBAR);
        F.FontWeight = FW_BOLD;
        HFONT hFont1 = CreateStyleFont(&F);
        F.FontWeight = FW_NORMAL;
        HFONT hFont2 = CreateStyleFont(&F);

        HGDIOBJ otherfont = SelectObject(buf, hFont1);
        bbDrawText(buf, icon.balloon.szInfoTitle, r1, flags, TC);
        SelectObject(buf, hFont2);
        bbDrawText(buf, icon.balloon.szInfo, r2, flags, TC);
        SelectObject(buf, otherfont);
        DeleteObject(hFont1);
        DeleteObject(hFont2);
    }

    void paint(HWND hwnd)
    {
        StyleItem *pStyle = (StyleItem *)GetSettingPtr(SN_TOOLBARWINDOWLABEL);
        if (pStyle->parentRelative)
            pStyle = (StyleItem *)GetSettingPtr(SN_TOOLBAR);

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC buf = CreateCompatibleDC(NULL);
        HGDIOBJ other = SelectObject(buf, CreateCompatibleBitmap(hdc, this->width, this->height));
        RECT r1 = {0, 0, this->width, this->height};
        MakeStyleGradient(buf, &r1, pStyle, true);
        r1.top += padding;
        r1.left += padding;
        r1.right -= padding;
        RECT r2 = r1;
        r2.top += msgtop;

        SetBkMode(buf, TRANSPARENT);
        draw_text(buf, &r1, &r2, DT_LEFT | DT_WORDBREAK, pStyle->TextColor);

        if (icon.balloon.szInfoTitle[0])
        {
            int y = r2.top - padding + 1;
            HGDIOBJ oldPen = SelectObject(buf, CreatePen(PS_SOLID, 1, pStyle->TextColor));
            MoveToEx(buf, r1.left, y, NULL);
            LineTo  (buf, r1.right, y);
            DeleteObject(SelectObject(buf, oldPen));
        }

        BitBltRect(hdc, buf, &ps.rcPaint);
        DeleteObject(SelectObject(buf, other));
        DeleteDC(buf);
        EndPaint(hwnd, &ps);
    }


    void Post(LPARAM lParam)
    {
        PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
    }

    LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret)
    {
        if (ret) return *ret;
        switch (message)
        {       
            case WM_PAINT:
                this->paint(hwnd);
                break;

            case WM_CREATE:
                Post(NIN_BALLOONSHOW);
                SetTimer(hwnd, 2, icon.balloon.uInfoTimeout, NULL);
                break;

            case WM_DESTROY:
                if (false == this->finished)
                    Post(NIN_BALLOONHIDE);
                break;

            case WM_TIMER:
            case WM_RBUTTONDOWN:
                Post(NIN_BALLOONTIMEOUT);
                delete this;
                break;

            case WM_LBUTTONDOWN:
                Post(NIN_BALLOONUSERCLICK);
                delete this;
                break;

            case BB_RECONFIGURE:
                calculate_size();
                BBP_reconfigure(this);
                break;

            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
        }
        return 0;
    }
};

void make_bb_balloon(plugin_info * PI, systemTray *picon, RECT *pr)
{
    if (picon && picon->balloon.uInfoTimeout) {
        RECT r = *pr;
        ClientToScreen(PI->hwnd, (POINT*)&r.left);
        ClientToScreen(PI->hwnd, (POINT*)&r.right);
        new bb_balloon(PI, picon, &r);
        picon->balloon.uInfoTimeout = 0;
    }
}


//===========================================================================
#ifndef TTM_SETTITLEA
#define TTM_SETTITLEA (WM_USER+32)  // wParam = TTI_*, lParam = char* szTitle
#endif
#define TTS_BALLOON     0x40
#define NIIF_NONE       0x00000000
#define NIIF_INFO       0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR      0x00000003
#define TTI_NONE        0
#define TTI_INFO        1
#define TTI_WARNING     2
#define TTI_ERROR       3
#ifndef TTF_TRACK
#define TTF_TRACK 0x0020
#endif

class win_balloon
{
    systemTray icon;
    HWND hwndBalloon;
    WNDPROC prev_wndproc;
    bool finished;

public:
    win_balloon(plugin_info * PI, systemTray *picon, RECT *r)
    {
        HWND hwndParent = PI->hwnd;
        HINSTANCE hInstance = PI->hInstance;

        this->icon = *picon;
        this->finished = false;

        int xpos = (r->left+r->right)/2;
        int ypos = (r->top+r->bottom)/2;

        TOOLINFO ti;
        memset(&ti, 0, sizeof ti);
        ti.cbSize   = sizeof(TOOLINFO);
        ti.uFlags   = TTF_TRACK;
        ti.hwnd     = hwndParent;
        ti.uId      = 1;
        ti.lpszText = icon.balloon.szInfo;

        hwndBalloon = CreateWindowEx(
            WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
            0,0,0,0,
            hwndParent,
            NULL,
            hInstance,
            NULL
            );

        SendMessage(hwndBalloon, TTM_SETMAXTIPWIDTH, 0, 270);
        SendMessage(hwndBalloon, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(hwndBalloon, TTM_SETTITLEA, icon.balloon.dwInfoFlags, (LPARAM)icon.balloon.szInfoTitle);
        SendMessage(hwndBalloon, TTM_TRACKPOSITION, 0, MAKELPARAM(xpos, ypos));
        SendMessage(hwndBalloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

        SetWindowLongPtr(hwndBalloon, GWLP_USERDATA, (LONG_PTR)this);
        prev_wndproc = (WNDPROC)SetWindowLongPtr(hwndBalloon, GWLP_WNDPROC, (LONG_PTR)wndproc);
        SetTimer(hwndBalloon, 700, icon.balloon.uInfoTimeout, NULL);
        Post(NIN_BALLOONSHOW);
    }

    ~win_balloon()
    {
        finished = true;
        DestroyWindow(hwndBalloon);
    }

private:
    void Post(LPARAM lParam)
    {
        PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
    }

    static LRESULT wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        win_balloon *p = (win_balloon *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        switch (message)
        {       
            case WM_CREATE:
                break;

            case WM_DESTROY:
                if (false == p->finished)
                    p->Post(NIN_BALLOONHIDE);
                break;

            case WM_TIMER:
                if (700 != wParam)
                    break;
            case WM_RBUTTONDOWN:
                p->Post(NIN_BALLOONTIMEOUT);
                delete p;
                return 0;

            case WM_LBUTTONDOWN:
                p->Post(NIN_BALLOONUSERCLICK);
                delete p;
                return 0;

            default:
                break;
        }
        return CallWindowProc (p->prev_wndproc, hwnd, message, wParam, lParam);
    }
};

//===========================================================================
class msg_balloon
{
    systemTray icon;

public:
    msg_balloon(plugin_info *PI, systemTray *picon, RECT *r)
    {
        this->icon = *picon;
        PostMessage(PI->hwnd, NIN_BALLOONSHOW, 0, (LPARAM)this);
    }

    void show(void)
    {
        char msg[1000];
        static int f [] =
        {
            0,
            MB_ICONINFORMATION,
            MB_ICONWARNING,
            MB_ICONERROR
        };
        if (icon.balloon.szInfoTitle[0])
            sprintf(msg, "%s\n\n%s", icon.balloon.szInfoTitle, icon.balloon.szInfo);
        else
            sprintf(msg, "%s", icon.balloon.szInfo);

        Post(NIN_BALLOONSHOW);
        if (IDOK == MessageBox(NULL, msg, "Balloon Tip (Beta)", f[iminmax(icon.balloon.dwInfoFlags, 0, 3)]|MB_OKCANCEL|MB_TOPMOST|MB_SETFOREGROUND))
            Post(NIN_BALLOONUSERCLICK);
        else
            Post(NIN_BALLOONTIMEOUT);

        delete this;
    }

    ~msg_balloon()
    {
    }

private:
    void Post(LPARAM lParam)
    {
        PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
    }

};

