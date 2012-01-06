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

#include "BB.h"
#include "Toolbar.h"

// the static ToolbarInfo, passed by GetToolbarInfo
ToolbarInfo TBInfo;

#ifndef BBTINY

#include "Settings.h"
#include "MessageManager.h"
#include "Workspaces.h"
#include "Menu/MenuMaker.h"
#include <time.h>
#include <locale.h>
#include <shellapi.h>

#define ST static

static const struct cfgmenu tb_sub_placement[] = {
    { NLS0("Top Left")      , "placement topLeft"       , &Settings_toolbar.placement },
    { NLS0("Top Center")    , "placement topCenter"     , &Settings_toolbar.placement },
    { NLS0("Top Right")     , "placement topRight"      , &Settings_toolbar.placement },
    { NLS0("Bottom Left")   , "placement bottomLeft"    , &Settings_toolbar.placement },
    { NLS0("Bottom Center") , "placement bottomCenter"  , &Settings_toolbar.placement },
    { NLS0("Bottom Right")  , "placement bottomRight"   , &Settings_toolbar.placement },
    { NULL }
};

static const struct cfgmenu tb_sub_settings[] = {
    { NLS0("Width Percent") ,   "widthPercent", &Settings_toolbar.widthPercent },
    { NLS0("Clock Format")  ,   "clockFormat", Settings_toolbar.strftimeFormat },
    { NLS0("Toggle With Plugins"),  "togglePlugins", &Settings_toolbar.pluginToggle },
    { NLS0("Transparency")  ,   "alphaEnabled", &Settings_toolbar.alphaEnabled },
    { NLS0("Alpha Value")   ,   "alphaValue", &Settings_toolbar.alphaValue },
    { NULL }
};

static const struct cfgmenu tb_main[] = {
    { NLS0("Placement")     ,   NULL, tb_sub_placement },
    { NLS0("Settings")      ,   NULL, tb_sub_settings },
    { NLS0("Always On Top") ,   "alwaysOnTop", &Settings_toolbar.onTop },
    { NLS0("Auto Hide")     ,   "autoHide", &Settings_toolbar.autoHide },
    { NLS0("Workspace Names"),  "@BBCore.SetWorkspaceNames", Settings_workspaceNames },
    { NULL }
};

ST const char * const placement_strings[] =
{
    "topLeft"       ,   //#define PLACEMENT_TOP_LEFT     0
    "topCenter"     ,   //#define PLACEMENT_TOP_CENTER   1
    "topRight"      ,   //#define PLACEMENT_TOP_RIGHT    2
    "bottomLeft"    ,   //#define PLACEMENT_BOTTOM_LEFT  3
    "bottomCenter"  ,   //#define PLACEMENT_BOTTOM_CENTER 4
    "bottomRight"   ,   //#define PLACEMENT_BOTTOM_RIGHT 5
    NULL
};

ST const char Toolbar_ClassName[] = "BBToolbar"; // Window class etc.

// window procedure
ST LRESULT CALLBACK Toolbar_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void Toolbar_ShowMenu(bool pop);
void Toolbar_UpdatePosition(void);

ST void Toolbar_AutoHide(bool);
ST void Toolbar_set_pos(void);

ST bool Toolbar_ShowingExternalLabel;
ST char Toolbar_CurrentWindow[100];
ST char Toolbar_CurrentTime[100];
ST char Toolbar_WorkspaceName[100];

ST int Toolbar_HideY;
ST HFONT Toolbar_hFont;
ST bool Toolbar_hidden;
ST bool Toolbar_moving;
ST HWND Toolbar_hwnd;
ST bool Toolbar_force_button_pressed;
ST int tbClockW;

ST int tbButtonWH;
ST int tbLabelH;
ST int tbLabelIndent;
ST int tbMargin;

ST struct button
{
    RECT r;
    bool pressed;
} Toolbar_Button[5];

#define TOOLBAR_AUTOHIDE_TIMER 1
#define TOOLBAR_CLOCK_TIMER 2
#define TOOLBAR_LABEL_TIMER 3

//===========================================================================
int beginToolbar(HINSTANCE hi)
{
    if (Toolbar_hwnd)
        return 1;

    memset(&Toolbar_Button[0], 0, sizeof(Toolbar_Button));
    Toolbar_ShowingExternalLabel = false;
    Toolbar_CurrentWindow[0] =
    Toolbar_CurrentTime[0] =
    Toolbar_WorkspaceName[0] = 0;

    memset(&TBInfo, 0, sizeof TBInfo);
    if (false == Settings_toolbar.enabled)
    {
        Toolbar_UpdatePosition();
        return 0;
    }

    BBRegisterClass(Toolbar_ClassName, Toolbar_WndProc, BBCS_VISIBLE);
    CreateWindowEx(
        WS_EX_TOOLWINDOW,
        Toolbar_ClassName,      // window class name
        NULL, //"BBToolbar",    // window title
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0,0,0,0,
        NULL,                   // parent window
        NULL,                   // no menu
        hMainInstance,          // module handle
        NULL                    // init params
        );
    return 0;
}

//===========================================================================

void endToolbar(HINSTANCE hi)
{
    if (NULL == Toolbar_hwnd) return;

    DestroyWindow(Toolbar_hwnd);
    UnregisterClass(Toolbar_ClassName, hMainInstance); // unregister window class
    Toolbar_UpdatePosition();
}

//===========================================================================
ST void Toolbar_set_pos(void)
{
    if (false == Toolbar_hidden)
        SetTransparency(Toolbar_hwnd, mStyle.toolbarAlpha);

    SetWindowPos(Toolbar_hwnd,
        TBInfo.onTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        TBInfo.xpos,
        TBInfo.autohidden ? Toolbar_HideY : TBInfo.ypos,
        TBInfo.width,
        TBInfo.height,
        Toolbar_hidden
            ? SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOSENDCHANGING
            : SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSENDCHANGING
        );

    if (TBInfo.bbsb_hwnd)
        PostMessage(TBInfo.bbsb_hwnd, BB_TOOLBARUPDATE, 0, 0);
}

//---------------------------------------------------------------------------
// Function:    CheckButton
// Purpose:     ...
//---------------------------------------------------------------------------
static RECT *capture_rect;
static bool *pressed_state;
static void (*MouseButtonEvent)(unsigned message, LPARAM id);
static LPARAM button_id;

bool CheckButton(HWND hwnd, UINT message, POINT MousePoint, RECT *pRect, bool *pState, void (*handler)(UINT, LPARAM), LPARAM id)
{
    if (PtInRect(pRect, MousePoint))
    {
        switch (message)
        {
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
                capture_rect = pRect;
                pressed_state = pState;
                MouseButtonEvent = handler;
                if (pressed_state)
                {
                    *pressed_state = true;
                    InvalidateRect(hwnd, capture_rect, FALSE);
                }
                SetCapture(hwnd);
                MouseButtonEvent(message, button_id = id);
        }
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------
// Function:    ReleaseButton
// Purpose:     ...
//---------------------------------------------------------------------------

void ReleaseButton(RECT *r)
{
    if (capture_rect == r)
    {
        capture_rect = NULL;
        ReleaseCapture();
    }
}

//---------------------------------------------------------------------------
// Function:    HandleCapture
// Purpose:     ...
//---------------------------------------------------------------------------

bool HandleCapture(HWND hwnd, UINT message, POINT MousePoint)
{
    if (capture_rect)
    {
        bool inside, hilite, release = false;
        inside = hilite = (PtInRect(capture_rect, MousePoint) != 0);

        if (message == WM_LBUTTONUP || 
            message == WM_MBUTTONUP || 
            message == WM_RBUTTONUP || 
            GetCapture() != hwnd)
        {
            release = true;
            hilite  = false;
        }

        if (pressed_state && (hilite != *pressed_state))
        {
            *pressed_state = hilite;
            InvalidateRect(hwnd, capture_rect, FALSE);
        }

        if (release)
            ReleaseButton(capture_rect);

        if (inside)
            MouseButtonEvent(message, button_id);

        return true;
    }
    return false;
}


//===========================================================================
void Toolbar_button_event(UINT message, LPARAM i)
{
    if (4 == i) {
        if (WM_LBUTTONDBLCLK == message) {
            BBExecute_string("control.exe timedate.cpl", RUN_SHOWERRORS);
            return;
        }
        return;
    }

    if (WM_LBUTTONUP == message) {
        bool co = false, rt = false;
        if (TBInfo.bbsb_hwnd)
            co = TBInfo.bbsb_currentOnly, rt = TBInfo.bbsb_reverseTasks;

        co ^= (0 == (GetAsyncKeyState(VK_SHIFT) & 0x8000));
        switch(i)
        {
            case 0:
                PostMessage(BBhwnd, BB_WORKSPACE, BBWS_DESKLEFT, 0);
                break;
            case 1:
                PostMessage(BBhwnd, BB_WORKSPACE, BBWS_DESKRIGHT, 0);
                break;
            case 2:
                PostMessage(BBhwnd, BB_WORKSPACE, (int)rt^BBWS_PREVWINDOW, co);
                break;
            case 3:
                PostMessage(BBhwnd, BB_WORKSPACE, (int)rt^BBWS_NEXTWINDOW, co);
                break;
        }
    }
}

//===========================================================================

ST void Toolbar_setlabel(void)
{
    // Get current workspace name...
    DesktopInfo DI;
    HWND hwnd;

    GetDesktopInfo (& DI);
    strcpy(Toolbar_WorkspaceName, DI.name);
    if (Toolbar_ShowingExternalLabel)
        return;

#ifdef BBOPT_MEMCHECK
    // Display some statistics.
    if (m_alloc_size()) {
        struct menu_stats st;
        Menu_Stats(&st);
        sprintf(Toolbar_CurrentWindow,"Menus %d  MenuItems %d  Memory %d", st.menu_count, st.item_count, m_alloc_size());
        return;
    }
#endif
    hwnd = GetForegroundWindow();
    if (NULL == hwnd || is_bbwindow(hwnd)) {
        if (GetCapture())
            return;
        hwnd = BBhwnd;
    }
    get_window_text(hwnd, Toolbar_CurrentWindow, sizeof Toolbar_CurrentWindow);
}

ST void Toolbar_setclock(void)
{
    time_t systemTime;
    struct tm *ltp;
    char *format;

    format = Settings_toolbar.strftimeFormat;

    time(&systemTime); // get this for the display
    ltp = localtime(&systemTime);
    strftime(Toolbar_CurrentTime, sizeof Toolbar_CurrentTime, format, ltp);
    if (Toolbar_hidden) {
        KillTimer(Toolbar_hwnd, TOOLBAR_CLOCK_TIMER);
    } else {
        SYSTEMTIME lt;
        bool seconds;
        GetLocalTime(&lt); // get this for the milliseconds to set the timer

#ifdef BBOPT_MEMCHECK
        Toolbar_setlabel();
        seconds = true;
#else
        seconds = strstr(format, "%S") || strstr(format, "%#S");
#endif
        SetTimer(Toolbar_hwnd, TOOLBAR_CLOCK_TIMER, seconds ? 1100 - lt.wMilliseconds : 61000 - lt.wSecond * 1000, NULL);

    }
}

//===========================================================================

ST bool check_mouse(HWND hwnd)
{
    POINT pt; RECT rct;
    GetCursorPos(&pt);
    GetWindowRect(hwnd, &rct);
    return FALSE != PtInRect(&rct, pt);
}

ST int get_text_extend(HDC hdc, const char *cp)
{
    RECT s = {0,0,0,0};
    bbDrawText(hdc, cp, &s, DT_CALCRECT|DT_NOPREFIX, 0);
    return s.right;
}

//===========================================================================
ST void PaintToolbar(HDC hdc, RECT *rcPaint)
{
    RECT r;
    StyleItem *pSI;
    struct button *btn;

    HDC buf;
    HGDIOBJ bufother, other_font;
    int size;

    int margin, border, border_margin, button_padding, middle_padding, two_buttons;
    int tbW, tbH, tbLabelW, tbLabelX, tbClockX, tbWinLabelX, tbWinLabelW;
    int i, justify;

    tbW = TBInfo.width;
    tbH = TBInfo.height;
    buf = CreateCompatibleDC(NULL);
    bufother = SelectObject(buf, CreateCompatibleBitmap(hdc, tbW, tbH));

    if (NULL==Toolbar_hFont)
        Toolbar_hFont = CreateStyleFont(&mStyle.Toolbar);
    other_font = SelectObject(buf, Toolbar_hFont);

    size = 6 + get_text_extend(buf, Toolbar_CurrentTime);
    if (tbClockW < size)
        tbClockW = size + 2*tbLabelIndent;

    size = get_text_extend(buf, Toolbar_WorkspaceName);
    tbLabelW = size + 2*tbLabelIndent;

    // The widest sets the width!
    tbLabelW = tbClockW = imax(tbH * 2, imax(tbLabelW, tbClockW));

    margin = tbMargin;
    border = mStyle.Toolbar.borderWidth;
    border_margin = margin + border;
    button_padding = (tbH - tbButtonWH) / 2 - border;
    middle_padding = button_padding;
    if (0 == button_padding)
        middle_padding -= mStyle.ToolbarButton.borderWidth;

    tbLabelX = border_margin;
    tbClockX = tbW - tbClockW - border_margin;
    two_buttons = 2*tbButtonWH + 2*button_padding + middle_padding;
    tbWinLabelX = tbLabelX + tbLabelW + two_buttons;
    tbWinLabelW = tbClockX - tbWinLabelX - two_buttons;
    if (tbWinLabelW < 0) tbWinLabelW = 0;

    btn = Toolbar_Button;
    btn[0].r.left = tbLabelX + tbLabelW + button_padding;
    btn[1].r.left = btn[0].r.left + tbButtonWH + middle_padding;
    btn[2].r.left = tbClockX - 2*tbButtonWH - button_padding - middle_padding;
    btn[3].r.left = btn[2].r.left + tbButtonWH + middle_padding;
    btn[4].r.left = tbClockX;
    for (i = 0; i<5; i++) {
        btn[i].r.top    = (tbH - tbButtonWH) / 2;
        btn[i].r.bottom = btn[i].r.top + tbButtonWH;
        btn[i].r.right  = btn[i].r.left + tbButtonWH;
    }
    btn[4].r.right = tbClockX + tbClockW;

    //====================

    // Paint toolbar Style
    r.left = r.top = 0;
    r.right = tbW;
    r.bottom = tbH;
    pSI = &mStyle.Toolbar;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);

    //====================
    // Paint unpressed workspace/task buttons...

    r.left = r.top = 0;
    r.right = r.bottom = tbButtonWH;
    {
        HDC src;
        HGDIOBJ srcother;
        int x, y, f2, f1 = -1;
        src = CreateCompatibleDC(NULL);
        srcother = SelectObject(src, CreateCompatibleBitmap(hdc, tbButtonWH, tbButtonWH));
        for (i = 0; i < 4; i++)
        {
            btn = Toolbar_Button + i;
            f2 = btn->pressed || (Toolbar_force_button_pressed && (i&1));
            x = btn->r.left, y = btn->r.top;
            pSI = f2 ? &mStyle.ToolbarButtonPressed : &mStyle.ToolbarButton;
            if (pSI->parentRelative) {
                RECT b;
                b.left = x, b.top = y, b.right = x+r.right, b.bottom = y+r.bottom;
                CreateBorder(buf, &b, pSI->borderColor, pSI->borderWidth);
            } else {
                if (f1 != f2) {
                    MakeStyleGradient(src, &r, pSI, pSI->bordered);
                    f1 = f2;
                }
                BitBlt(buf, x, y, tbButtonWH, tbButtonWH, src, 0, 0, SRCCOPY);
            }
            bbDrawPix(buf, &btn->r, pSI->picColor, (i&1) ? BS_TRIANGLE : -BS_TRIANGLE);
        }

        DeleteObject(SelectObject(src, srcother));
        DeleteDC(src);
    }

    //====================

    r.top = (tbH - tbLabelH)/2;
    r.bottom = r.top + tbLabelH;
    SetBkMode(buf, TRANSPARENT);
    justify = mStyle.Toolbar.Justify | (DT_VCENTER|DT_SINGLELINE|DT_WORD_ELLIPSIS|DT_NOPREFIX);

    // Paint workspaces background...
    r.right = (r.left = tbLabelX) + tbLabelW;
    pSI = &mStyle.ToolbarLabel;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    r.left  += tbLabelIndent;
    r.right -= tbLabelIndent;
    //bbDrawText(buf, Toolbar_WorkspaceName, &r, justify, pSI->TextColor);
	/* BlackboxZero 1.5.2012 */
	BBDrawText(buf, Toolbar_WorkspaceName, -1, &r, justify, pSI);

    // Paint window label background...
    r.right = (r.left = tbWinLabelX) + tbWinLabelW;
    pSI = &mStyle.ToolbarWindowLabel;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    r.left  += tbLabelIndent;
    r.right -= tbLabelIndent;
    //bbDrawText(buf, Toolbar_CurrentWindow, &r, justify, pSI->TextColor);
	/* BlackboxZero 1.5.2012 */
	BBDrawText(buf, Toolbar_CurrentWindow, -1, &r, justify, pSI);

    // Paint clock background...
    r.right = (r.left = tbClockX) + tbClockW;
    pSI = &mStyle.ToolbarClock;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    r.left  += tbLabelIndent;
    r.right -= tbLabelIndent;
    //bbDrawText(buf, Toolbar_CurrentTime, &r, justify, pSI->TextColor);
	/* BlackboxZero 1.5.2012 */
	BBDrawText(buf, Toolbar_CurrentTime, -1, &r, justify, pSI);

    //====================

    BitBltRect(hdc, buf, rcPaint);

    SelectObject(buf, other_font);
    DeleteObject(SelectObject(buf, bufother));
    DeleteDC(buf);
}

//===========================================================================
ST LRESULT CALLBACK Toolbar_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const UINT msgs[] =
    {
        BB_RECONFIGURE,
        BB_TASKSUPDATE,
        BB_SETTOOLBARLABEL,
        BB_BROADCAST,
        BB_DESKTOPINFO,
        BB_REDRAWGUI,
        0
    };

    switch (message)
    {
        //====================
        case WM_CREATE:
            TBInfo.hwnd = Toolbar_hwnd = hwnd;
            MessageManager_Register (hwnd, msgs, true);
            MakeSticky(hwnd);
            Toolbar_UpdatePosition();
            break;

        //====================
        case WM_DESTROY:
            RemoveSticky(hwnd);
            MessageManager_Register (hwnd, msgs, false);
            SetDesktopMargin(Toolbar_hwnd, 0, 0);
            if (Toolbar_hFont)
                DeleteObject(Toolbar_hFont), Toolbar_hFont = NULL;
            TBInfo.hwnd = Toolbar_hwnd = NULL;
            break;

        //====================
        case BB_RECONFIGURE:
        tbreconfig:
            Toolbar_UpdatePosition();
            Toolbar_ShowMenu(false);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        //====================
        case BB_REDRAWGUI:
            if (wParam & BBRG_TOOLBAR)
            {
                Toolbar_force_button_pressed = 0 != (wParam & BBRG_PRESSED);
                Toolbar_UpdatePosition();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        //====================
        case BB_TASKSUPDATE:
        showlabel:
            Toolbar_setlabel();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case BB_DESKTOPINFO:
            Toolbar_setlabel();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        //====================
        case WM_ACTIVATEAPP:
            if (wParam) SetOnTop(hwnd);
            break;

        //====================

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintToolbar(hdc, &ps.rcPaint);
            EndPaint(hwnd, &ps);
            break;
        }

        //====================

        case BB_SETTOOLBARLABEL:
            SetTimer(hwnd, TOOLBAR_LABEL_TIMER, 2000, (TIMERPROC)NULL);
            Toolbar_ShowingExternalLabel = true;
            strcpy_max(Toolbar_CurrentWindow, (const char*)lParam, sizeof Toolbar_CurrentWindow);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        //====================

        case WM_TIMER:
        if (wParam == TOOLBAR_AUTOHIDE_TIMER)
        {
            if (TBInfo.autoHide)
            {
                if (check_mouse(hwnd) || (TBInfo.bbsb_hwnd && check_mouse(TBInfo.bbsb_hwnd)))
                    break;

                Toolbar_AutoHide(true);
            }
            KillTimer(hwnd, wParam);
            break;
        }

        if (wParam == TOOLBAR_CLOCK_TIMER)
        {
            Toolbar_setclock();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        KillTimer(hwnd, wParam);

        if (wParam == TOOLBAR_LABEL_TIMER)
        {
            Toolbar_ShowingExternalLabel = false;
            goto showlabel;
        }

        break;

        //====================

        case WM_LBUTTONDOWN:
            if (wParam & MK_CONTROL)
            {
                // Allow window to move if control key is being held down,
                UpdateWindow(hwnd);
                SendMessage(hwnd, WM_SYSCOMMAND, 0xf012, 0);
                break;
            }
            goto left_mouse;

        case WM_LBUTTONDBLCLK:
            if (wParam & MK_CONTROL)
            {
                // double click moves the window back to the default position
                Toolbar_set_pos();
                break;
            }
            goto left_mouse;

        case WM_MOUSEMOVE:
            if (TBInfo.autohidden)
            {
                // bring back from autohide
                SetTimer(hwnd, TOOLBAR_AUTOHIDE_TIMER, 250, NULL);
                Toolbar_AutoHide(false);
                break;
            }
            goto left_mouse;

        case WM_LBUTTONUP:
        left_mouse:
        {
            POINT MouseEventPoint; int i;
            MouseEventPoint.x = (short)LOWORD(lParam);
            MouseEventPoint.y = (short)HIWORD(lParam);
            if (HandleCapture(hwnd, message, MouseEventPoint))
                break;

            for (i=0; i<5; i++)
            if (CheckButton(
                    hwnd,
                    message,
                    MouseEventPoint,
                    &Toolbar_Button[i].r,
                    &Toolbar_Button[i].pressed,
                    Toolbar_button_event,
                    i
                    )) goto _break;

            if (message == WM_LBUTTONDOWN)
                SetActiveWindow(hwnd);
        }
        _break:
        break;

        //====================
        // show menus

        case WM_RBUTTONUP:
        {
            int x = (short)LOWORD(lParam);
            if (x < tbClockW)
                PostMessage(BBhwnd, BB_MENU, BB_MENU_TASKS, 0);
            else
            if (x >= TBInfo.width - tbClockW)
                PostMessage(BBhwnd, BB_MENU, BB_MENU_ROOT, 0);
            else
                Toolbar_ShowMenu(true);
            break;
        }

        //====================

        case WM_MBUTTONUP:
            // Is shift key held down?
            if (wParam & MK_SHIFT)
                PostMessage(BBhwnd, BB_TOGGLEPLUGINS, 0, 0);
            else
                PostMessage(BBhwnd, BB_TOGGLETRAY, 0, 0);
        break;

        //====================
        // If moved, snap window to screen edges...

        case WM_WINDOWPOSCHANGING:
            if (Toolbar_moving)
                SnapWindowToEdge((WINDOWPOS*)lParam, 0, SNAP_FULLSCREEN);
            break;

        case WM_ENTERSIZEMOVE:
            Toolbar_moving = true;
            break;

        case WM_EXITSIZEMOVE:
            Toolbar_moving = false;
            break;

        //====================

        case BB_BROADCAST:
        {
            const char *broam = (const char*)lParam;

            if (!stricmp(broam, "@BBShowPlugins"))
            {
                Toolbar_hidden = false;
                Toolbar_UpdatePosition();
                break;
            }

            if (!stricmp(broam, "@BBHidePlugins"))
            {
                if (Settings_toolbar.pluginToggle)
                {
                    Toolbar_hidden = true;
                    Toolbar_UpdatePosition();
                }
                break;
            }

            if (0 == memcmp(broam, "@Toolbar.", 9))
            {
                const struct cfgmenu *p_menu, *p_item;
                exec_internal_broam(broam+9, tb_main, &p_menu, &p_item);
                goto tbreconfig;
            }

            break;
        }

        case WM_CLOSE:
            break;

        default:
            return DefWindowProc(hwnd,message,wParam,lParam);

    //====================
    }
    return 0;
}

//===========================================================================

void Toolbar_UpdatePosition(void)
{
    int ScreenWidth;
    int ScreenHeight;
    int labelH, buttonH, tbheight, tbwidth, labelBorder;
    int place, fontH;
    HFONT hf;

    struct toolbar_setting *tbs = &Settings_toolbar;

    // Another Toolbar has set the info:
    if (NULL == Toolbar_hwnd && TBInfo.hwnd)
        return;

    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    place = get_string_index(tbs->placement, placement_strings);
    if (-1 == place)
        place = PLACEMENT_BOTTOM_CENTER;

    // --- toolbar metrics ----------------------

    hf = CreateStyleFont(&mStyle.Toolbar);
    fontH = get_fontheight(hf);
    DeleteObject(hf);

    // xxx old behaviour xxx
    if (false == mStyle.is_070
        && 0 == (mStyle.ToolbarLabel.validated & V_MAR))
        fontH = mStyle.Toolbar.FontHeight-2;
    //xxxxxxxxxxxxxxxxxxxxxx

    labelBorder = imax(mStyle.ToolbarLabel.borderWidth,
            imax(mStyle.ToolbarWindowLabel.borderWidth,
                mStyle.ToolbarClock.borderWidth));

    labelH = (fontH|1) + 2*(mStyle.ToolbarLabel.marginWidth/*+labelBorder*/);
    buttonH = labelH + 2*(mStyle.ToolbarButton.marginWidth-mStyle.ToolbarLabel.marginWidth);
    tbMargin    = mStyle.Toolbar.marginWidth;
    tbButtonWH  = buttonH;
    tbLabelH    = labelH;
    tbLabelIndent = imax(2+labelBorder, (labelH-fontH)/2);

    tbheight    = imax(labelH, buttonH) +
        2*(mStyle.Toolbar.borderWidth + mStyle.Toolbar.marginWidth);
    tbwidth     = imax (300, ScreenWidth * tbs->widthPercent / 100);

    // ------------------------------------------

    TBInfo.placement        = place;
    TBInfo.widthPercent     = tbs->widthPercent;
    TBInfo.height           = tbheight;
    TBInfo.width            = tbwidth;
    TBInfo.ypos             = (place<3) ? 0 : ScreenHeight - TBInfo.height;
    TBInfo.xpos             = (place%3) * (ScreenWidth - TBInfo.width) / 2;

    TBInfo.onTop            = tbs->onTop;
    TBInfo.autoHide         = tbs->autoHide;
    TBInfo.pluginToggle     = tbs->pluginToggle;
    TBInfo.autohidden       = TBInfo.autoHide;
    TBInfo.alphaValue       = (BYTE)tbs->alphaValue;
    TBInfo.transparency     = tbs->alphaEnabled;
    TBInfo.disabled         = false == tbs->enabled;
    TBInfo.hidden           = false == tbs->enabled;

    if (Toolbar_hwnd)
    {
        int d = TBInfo.height - 1;
        Toolbar_HideY = TBInfo.ypos + (place < 3 ? -d : d);

        SetDesktopMargin(
            Toolbar_hwnd,
            place < 3 ? BB_DM_TOP : BB_DM_BOTTOM,
            TBInfo.autoHide || Toolbar_hidden ? 0 : TBInfo.height);

        tbClockW = 0;
        Toolbar_setlabel();
        Toolbar_setclock();
        if (Toolbar_hFont) DeleteObject(Toolbar_hFont), Toolbar_hFont = NULL;
        Toolbar_set_pos();
    }
}

//===========================================================================

ST void Toolbar_AutoHide(bool hidden)
{
    if (TBInfo.autohidden != hidden)
    {
        TBInfo.autohidden = hidden;
        Toolbar_set_pos();
    }
}

//===========================================================================
// API: GetToolbarInfo
//===========================================================================

ToolbarInfo *GetToolbarInfo(void)
{
    return &TBInfo;
}

//===========================================================================

void Toolbar_ShowMenu(bool popup)
{
    char menu_id[200];
    strcpy(menu_id, "Core_configuration_TB");
    ShowMenu(CfgMenuMaker(NLS0("Toolbar"), "@Toolbar.", tb_main, popup, menu_id));
}

//===========================================================================
#else
//===========================================================================
// No internal toolbar - provide some nop's:

ToolbarInfo * GetToolbarInfo(void)
{
    return &TBInfo;
}

int beginToolbar(HINSTANCE hi)
{
    return 0;
}

void endToolbar(HINSTANCE hi)
{
}

void Toolbar_UpdatePosition(void)
{
}

void Toolbar_ShowMenu(bool pop)
{
}

//===========================================================================
#endif
