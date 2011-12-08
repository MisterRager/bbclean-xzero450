/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "BB.h"
#include "Settings.h"
#include "Toolbar.h"

// the static ToolbarInfo, passed by GetToolbarInfo
ToolbarInfo TBInfo;

#if 1
// -------------------------
#include "MessageManager.h"
#include "Workspaces.h"
#include "Menu/MenuMaker.h"

#include <time.h>
#include <locale.h>
#include <shellapi.h>
#ifdef __GNUC__
#include <shlwapi.h>
#endif

#define ST static

static const struct cfgmenu tb_sub_placement[] = {
    { NLS0("Top Left")      , "@Toolbar TopLeft"       , &Settings_toolbarPlacement },
    { NLS0("Top Center")    , "@Toolbar TopCenter"     , &Settings_toolbarPlacement },
    { NLS0("Top Right")     , "@Toolbar TopRight"      , &Settings_toolbarPlacement },
    { NLS0("Bottom Left")   , "@Toolbar BottomLeft"    , &Settings_toolbarPlacement },
    { NLS0("Bottom Center") , "@Toolbar BottomCenter"  , &Settings_toolbarPlacement },
    { NLS0("Bottom Right")  , "@Toolbar BottomRight"   , &Settings_toolbarPlacement },
    { NULL }
};

static const struct cfgmenu tb_sub_settings[] = {
    { NLS0("Width Percent"),      "@ToolbarWidthPercent", &Settings_toolbarWidthPercent },
    { NLS0("Clock Format"),       "@ToolbarClockFormat", Settings_toolbarStrftimeFormat },
    { NLS0("Toggle With Plugins"), "@ToolbarTogglePlugins", &Settings_toolbarPluginToggle },
    { NLS0("Transparency"),       "@ToolbarAlphaEnabled", &Settings_toolbarAlphaEnabled },
    { NLS0("Alpha Value"),        "@ToolbarAlphaValue", &Settings_toolbarAlphaValue },
    { NULL }
};

static const struct cfgmenu tb_main[] = {
    { NLS0("Placement"),      NULL, tb_sub_placement },
    { NLS0("Settings"),       NULL, tb_sub_settings },
    { NLS0("Always On Top"),  "@ToolbarAlwaysOnTop", &Settings_toolbarOnTop },
    { NLS0("Auto Hide"),      "@ToolbarAutoHide", &Settings_toolbarAutoHide },
    { NLS0("Edit Workspace Names"), "@BBCore.EditWorkspaceNames", NULL },
    { NULL }
};

ST const char *placement_strings[] =
{
    "TopLeft"       ,   //#define PLACEMENT_TOP_LEFT     0
    "TopCenter"     ,   //#define PLACEMENT_TOP_CENTER   1
    "TopRight"      ,   //#define PLACEMENT_TOP_RIGHT    2
    "BottomLeft"    ,   //#define PLACEMENT_BOTTOM_LEFT  3
    "BottomCenter"  ,   //#define PLACEMENT_BOTTOM_CENTER 4
    "BottomRight"   ,   //#define PLACEMENT_BOTTOM_RIGHT 5
    NULL
};

ST const char Toolbar_ClassName[] = "BBToolbar"; // Window class etc.

// window procedure
ST LRESULT CALLBACK Toolbar_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

void Toolbar_ShowMenu(bool pop);
void Toolbar_UpdatePosition();

ST void Toolbar_AutoHide(bool);
ST void Toolbar_set_pos(void);

ST bool Toolbar_ShowingExternalLabel;
ST char Toolbar_CurrentWindow[256];
ST char Toolbar_CurrentTime[80];
ST char Toolbar_WorkspaceName[80];

ST int Toolbar_HideY;
ST HFONT Toolbar_hFont;
ST bool Toolbar_hidden;
ST bool Toolbar_moving;
ST HWND Toolbar_hwnd;
ST bool Toolbar_force_button_pressed;
ST int tbClockW;

ST int tbButtonWH;
ST int tbLabelH;
ST int tbMargin;

ST struct button
{
    RECT r;
    bool pressed;
} Toolbar_Button[5];

#define TOOLBAR_AUTOHIDE_TIMER 1
#define TOOLBAR_UPDATE_TIMER 2
#define TOOLBAR_SETLABEL_TIMER 3

//===========================================================================
int beginToolbar(HINSTANCE hi)
{
    if (Toolbar_hwnd) return 1;

    ZeroMemory(&Toolbar_Button[0], sizeof(Toolbar_Button));
    Toolbar_ShowingExternalLabel = false;
    Toolbar_CurrentWindow[0] =
    Toolbar_CurrentTime[0] =
    Toolbar_WorkspaceName[0] = 0;

    ZeroMemory(&TBInfo, sizeof TBInfo);
    if (false == Settings_toolbarEnabled)
    {
        Toolbar_UpdatePosition();
        return 0;
    }

    WNDCLASS wc;
    // Register our window class
    ZeroMemory(&wc,sizeof(wc));
    wc.lpfnWndProc      = Toolbar_WndProc;
    wc.hInstance        = hMainInstance;
    wc.lpszClassName    = Toolbar_ClassName;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.style            = CS_DBLCLKS;

    if (!BBRegisterClass(&wc))
        return 1;

    CreateWindowEx(
        WS_EX_TOOLWINDOW,//|WS_EX_ACCEPTFILES, // exstyles
        Toolbar_ClassName,              // window class name
        NULL, //"BBToolbar",                    // window title
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0,0,0,0,
        NULL,                           // parent window
        NULL,                           // no menu
        hMainInstance,                  // module handle
        NULL                            // init params
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
ST void Toolbar_set_pos()
{
    if (false == Toolbar_hidden)
        SetTransparency(Toolbar_hwnd, Settings_toolbarAlpha);

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

    if (TBInfo.bbsb_hwnd) PostMessage(TBInfo.bbsb_hwnd, BB_TOOLBARUPDATE, 0, 0);
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
    if (4 == i)
    {
        if (WM_LBUTTONDBLCLK == message)
        {
            BBExecute_string("control.exe timedate.cpl", false);
        }
        return;
    }

    if (WM_LBUTTONUP == message)
    {
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
    GetDesktopInfo (& DI);
    strcpy(Toolbar_WorkspaceName, DI.name);
    if (Toolbar_ShowingExternalLabel) return;
    Toolbar_CurrentWindow[0] = 0;
    struct tasklist* tl;
    dolist (tl, GetTaskListPtr()) if (tl->active) break;
    if (tl) strcpy_max(Toolbar_CurrentWindow, tl->caption, sizeof Toolbar_CurrentWindow);
    //else strcpy(Toolbar_CurrentWindow, "Blackbox");
}

ST void Toolbar_setclock(void)
{
    time_t systemTime;
    time(&systemTime); // get this for the display
    struct tm *ltp = localtime(&systemTime);
    strftime(Toolbar_CurrentTime, sizeof Toolbar_CurrentTime, Settings_toolbarStrftimeFormat, ltp);
    if (Toolbar_hidden)
    {
        KillTimer(Toolbar_hwnd, TOOLBAR_UPDATE_TIMER);
    }
    else
    {
        SYSTEMTIME lt;
        GetLocalTime(&lt); // get this for the milliseconds to set the timer
        bool seconds = strstr(Settings_toolbarStrftimeFormat, "%S") || strstr(Settings_toolbarStrftimeFormat, "%#S");
#ifdef BBOPT_MEMCHECK
        seconds = true;
#endif
        SetTimer(Toolbar_hwnd, TOOLBAR_UPDATE_TIMER, seconds ? 1100 - lt.wMilliseconds : 61000 - lt.wSecond * 1000, NULL);
    }
}

//===========================================================================

bool check_mouse(HWND hwnd)
{
    POINT pt; RECT rct;
    GetCursorPos(&pt);
    GetWindowRect(hwnd, &rct);
    return PtInRect(&rct, pt);
}

/*
void drop_style(HDROP hdrop)
{
    char filename[MAX_PATH]; filename[0]=0;
    DragQueryFile(hdrop, 0, filename, sizeof(filename));
    DragFinish(hdrop);
    SendMessage(BBhwnd, BB_SETSTYLE, 1, (LPARAM)filename);
}
*/
 
//===========================================================================
ST void PaintToolbar(HDC hdc, RECT *rcPaint)
{
    RECT r; int i; const char *label = Toolbar_CurrentWindow; StyleItem *pSI;

#ifdef BBOPT_MEMCHECK
    // Display some statistics.
    #pragma message("\n"__FILE__ "(397) : warning 0: MEMCHECK enabled.\n")
/*
    if (NULL==Toolbar_hFont)
    {
        LOGFONT logFont;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
        Toolbar_hFont = CreateFontIndirect(&logFont);
    }
*/
    extern int g_menu_count;
    extern int g_menu_item_count;
    char temp[256];
    if (alloc_size && false == Toolbar_ShowingExternalLabel)
    {
        sprintf(temp,"Menus %d  MenuItems %d  Memory %d", g_menu_count, g_menu_item_count, alloc_size);
        label = temp;

    }
#endif

    int tbW = TBInfo.width;
    int tbH = TBInfo.height;
    HDC buf = CreateCompatibleDC(NULL);
    HGDIOBJ bufother = SelectObject(buf, CreateCompatibleBitmap(hdc, tbW, tbH));

    if (NULL==Toolbar_hFont) Toolbar_hFont = CreateStyleFont(&mStyle.Toolbar);
    HGDIOBJ other_font = SelectObject(buf, Toolbar_hFont);

    // Get width of clock...
    SIZE size;

    GetTextExtentPoint32(buf, Toolbar_CurrentTime, strlen(Toolbar_CurrentTime), &size);
    size.cx += 6;
    if (tbClockW < size.cx) tbClockW = size.cx + 6;

    GetTextExtentPoint32(buf, Toolbar_WorkspaceName, strlen(Toolbar_WorkspaceName), &size);
    int tbLabelW = size.cx + 6;

    // The widest sets the width!
    tbLabelW = tbClockW = imax(tbH * 2, imax(tbLabelW, tbClockW));

    int margin = tbMargin;
    int border = mStyle.Toolbar.borderWidth;
    int border_margin = margin + border;
    int button_padding = (tbH - tbButtonWH) / 2 - border;

    int tbLabelX = border_margin;
    int tbClockX = tbW - tbClockW - border_margin;
    int two_buttons = 2*tbButtonWH + 3*button_padding;
    int tbWinLabelX = tbLabelX + tbLabelW + two_buttons;
    int tbWinLabelW = tbClockX - tbWinLabelX - two_buttons;
    if (tbWinLabelW < 0) tbWinLabelW = 0;

    Toolbar_Button[0].r.left = tbLabelX + tbLabelW + button_padding;
    Toolbar_Button[1].r.left = Toolbar_Button[0].r.left + tbButtonWH + button_padding;
    Toolbar_Button[2].r.left = tbClockX - 2*tbButtonWH - 2*button_padding;
    Toolbar_Button[3].r.left = Toolbar_Button[2].r.left + tbButtonWH + button_padding;

    Toolbar_Button[4].r.left = tbClockX;
    for (i = 0; i<5; i++)
    {
        Toolbar_Button[i].r.top    = (tbH - tbButtonWH) / 2;
        Toolbar_Button[i].r.bottom = Toolbar_Button[i].r.top + tbButtonWH;
        Toolbar_Button[i].r.right  = Toolbar_Button[i].r.left + tbButtonWH;
    }
    Toolbar_Button[4].r.right = tbClockX + tbClockW;

    //====================

    // Paint toolbar Style
    _SetRect(&r, 0, 0, tbW, tbH); pSI = &mStyle.Toolbar;
    MakeStyleGradient(buf, &r, pSI, true);

    //====================
    // Paint unpressed workspace/task buttons...

    r.left = r.top = 0; r.right = r.bottom = tbButtonWH;
    {
        HPEN activePen   = CreatePen(PS_SOLID, 1, mStyle.ToolbarButtonPressed.picColor);
        HPEN inactivePen = CreatePen(PS_SOLID, 1, mStyle.ToolbarButton.picColor);
        HDC src = CreateCompatibleDC(NULL);
        HGDIOBJ srcother = SelectObject(src, CreateCompatibleBitmap(hdc, tbButtonWH, tbButtonWH));
        int yOffset = tbH / 2; int xOffset; int f1 = -1;
        for (i=0; i<4; i++)
        {
            int f2 = Toolbar_Button[i].pressed || (Toolbar_force_button_pressed && i&1);
            pSI = f2 ? &mStyle.ToolbarButtonPressed : &mStyle.ToolbarButton;

            if (pSI->parentRelative)
            {
                CreateBorder(buf, &r, pSI->borderColor, pSI->borderWidth);
            }
            else
            {
                if (f1 != f2)
                    MakeStyleGradient(src, &r, pSI, pSI->bordered), f1 = f2;

                BitBlt(buf,
                    Toolbar_Button[i].r.left,
                    Toolbar_Button[i].r.top,
                    tbButtonWH, tbButtonWH, src, 0, 0, SRCCOPY
                    );
            }

            xOffset = Toolbar_Button[i].r.left + (tbButtonWH / 2);
            HGDIOBJ penother = SelectObject(buf, f2 ? activePen : inactivePen);
            arrow_bullet(buf, xOffset, yOffset, (i&1)*2-1);
            SelectObject(buf, penother);
        }

        DeleteObject(SelectObject(src, srcother));
        DeleteDC(src);
        DeleteObject(inactivePen);
        DeleteObject(activePen);
    }

    //====================

    r.top = (tbH - tbLabelH)/2;
    r.bottom = r.top + tbLabelH;
    SetBkMode(buf, TRANSPARENT);
    int justify = mStyle.Toolbar.Justify | (DT_VCENTER|DT_SINGLELINE|DT_WORD_ELLIPSIS|DT_NOPREFIX);

    // Paint workspaces background...
    r.right = (r.left = tbLabelX) + tbLabelW; pSI = &mStyle.ToolbarLabel;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    _InflateRect(&r, -3, 0);
    BBDrawText(buf, Toolbar_WorkspaceName, -1, &r, justify, pSI);

    // Paint window label background...
    r.right = (r.left = tbWinLabelX) + tbWinLabelW; pSI = &mStyle.ToolbarWindowLabel;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    _InflateRect(&r, -3, 0);
    BBDrawText(buf, label, -1, &r, justify, pSI);

    // Paint clock background...
    r.right = (r.left = tbClockX) + tbClockW; pSI = &mStyle.ToolbarClock;
    MakeStyleGradient(buf, &r, pSI, pSI->bordered);
    _InflateRect(&r, -3, 0);
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
    static UINT msgs[] =
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
            MessageManager_AddMessages (hwnd, msgs);
            MakeSticky(hwnd);
            Toolbar_UpdatePosition();
            break;

        //====================
        case WM_DESTROY:
            RemoveSticky(hwnd);
            MessageManager_RemoveMessages (hwnd, msgs);
            SetDesktopMargin(Toolbar_hwnd, 0, 0);
            if (Toolbar_hFont) DeleteObject(Toolbar_hFont), Toolbar_hFont = NULL;
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
            SetTimer(hwnd, TOOLBAR_SETLABEL_TIMER, 2000, (TIMERPROC)NULL);
            Toolbar_ShowingExternalLabel = true;
            strcpy_max(Toolbar_CurrentWindow, (LPCSTR)lParam, sizeof Toolbar_CurrentWindow);
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

        if (wParam == TOOLBAR_UPDATE_TIMER)
        {
            Toolbar_setclock();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        KillTimer(hwnd, wParam);

        if (wParam == TOOLBAR_SETLABEL_TIMER)
        {
            Toolbar_ShowingExternalLabel = false;
            goto showlabel;
        }

        break;

        //====================
    /*
        case WM_DROPFILES:
            drop_style((HDROP)wParam);
            break;
    */
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
            POINT MouseEventPoint;
            MouseEventPoint.x = (short)LOWORD(lParam);
            MouseEventPoint.y = (short)HIWORD(lParam);
            if (HandleCapture(hwnd, message, MouseEventPoint))
                break;

            for (int i=0; i<5; i++)
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
            if (0 == memcmp((LPSTR)lParam, "@Toolbar", 8))
            {
                const char *argument = (LPSTR)lParam;
                const struct cfgmenu *p_menu, *p_item;
                const void *v = exec_internal_broam(argument, tb_main, &p_menu, &p_item);
                if (v) goto tbreconfig;
                break;
            }
            if (!stricmp((LPCSTR)lParam, "@BBShowPlugins"))
            {
                Toolbar_hidden = false;
                Toolbar_UpdatePosition();
                break;
            }
            if (!stricmp((LPCSTR)lParam, "@BBHidePlugins"))
            {
                if (Settings_toolbarPluginToggle)
                {
                    Toolbar_hidden = true;
                    Toolbar_UpdatePosition();
                }
                break;
            }
            break;

        case WM_CLOSE:
            break;

        default:
            return DefWindowProc(hwnd,message,wParam,lParam);

    //====================
    }
    return 0;
}

//===========================================================================

void Toolbar_UpdatePosition()
{
    Settings_toolbarAlpha = Settings_toolbarAlphaEnabled ? Settings_toolbarAlphaValue : 255;

    // Another Toolbar has set the info:
    if (NULL == Toolbar_hwnd && TBInfo.hwnd) return;

    int place = get_string_index(Settings_toolbarPlacement, placement_strings);
    if (-1 == place) place = PLACEMENT_BOTTOM_CENTER;

    // --- toolbar metrics ----------------------

    int labelH, buttonH;

    if (Settings_newMetrics || (mStyle.ToolbarLabel.validated & VALID_MARGIN))
    {
        HFONT hf = CreateStyleFont(&mStyle.Toolbar);
        int lfh = get_fontheight(hf);
        DeleteObject(hf);

        if (mStyle.ToolbarLabel.validated & VALID_MARGIN)
            labelH = lfh + 2 * mStyle.ToolbarLabel.marginWidth;
        else
            labelH = imax(10, lfh) + 2*2;
    }
    else
    {
        labelH = imax(8, mStyle.Toolbar.FontHeight) + 2;
    }

    labelH |= 1;

    if (mStyle.ToolbarButton.validated & VALID_MARGIN)
    {
        buttonH = 9 + 2 * mStyle.ToolbarButton.marginWidth;
    }
    else
    {
        buttonH = labelH - 2;
    }

    tbMargin    = mStyle.Toolbar.marginWidth;
    tbButtonWH  = buttonH;
    tbLabelH    = labelH;

    int tbheight = imax(labelH, buttonH) +
        2 * (mStyle.Toolbar.borderWidth + mStyle.Toolbar.marginWidth);

    int tbwidth = imax (300, ScreenWidth * Settings_toolbarWidthPercent / 100);

    // ------------------------------------------
    //int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    //int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    TBInfo.placement        = place;
    TBInfo.widthPercent     = Settings_toolbarWidthPercent;
    TBInfo.height           = tbheight;
    TBInfo.width            = tbwidth;
    TBInfo.ypos             = (place<3) ? 0 : ScreenHeight - TBInfo.height;
    TBInfo.xpos             = (place%3) * (ScreenWidth - TBInfo.width) / 2;

    TBInfo.onTop            = Settings_toolbarOnTop;
    TBInfo.autoHide         = Settings_toolbarAutoHide;
    TBInfo.pluginToggle     = Settings_toolbarPluginToggle;
    TBInfo.autohidden       = TBInfo.autoHide;
    TBInfo.alphaValue       = Settings_toolbarAlphaValue;
    TBInfo.transparency     = Settings_toolbarAlphaEnabled;
    TBInfo.disabled         = false == Settings_toolbarEnabled;
    TBInfo.hidden           = false == Settings_toolbarEnabled;

    if (Toolbar_hwnd)
    {
        int d = TBInfo.height - 1;
        Toolbar_HideY = TBInfo.ypos + (place < 3 ? -d : d);

        SetDesktopMargin(
            Toolbar_hwnd,
            place < 3 ? BB_DM_TOP : BB_DM_BOTTOM,
            TBInfo.autoHide || Toolbar_hidden ? 0 : TBInfo.height + (TBInfo.onTop ? 0 : 4)
            );

        tbClockW = 0;
        Toolbar_setlabel();
        Toolbar_setclock();
        if (Toolbar_hFont) DeleteObject(Toolbar_hFont), Toolbar_hFont = NULL;
        Toolbar_set_pos();
    }
}

//===========================================================================

ST void Toolbar_AutoHide(bool hide)
{
    if (TBInfo.autohidden !=hide)
    {
        TBInfo.autohidden = hide;
        Toolbar_set_pos();
    }
}

//===========================================================================
// API: GetToolbarInfo
//===========================================================================

ToolbarInfo * GetToolbarInfo(void)
{
    return &TBInfo;
}

//===========================================================================

void Toolbar_ShowMenu(bool popup)
{
    char menu_id[200]; strcpy(menu_id, "IDRoot_configuration_TB");
    ShowMenu(CfgMenuMaker(NLS0("Toolbar"), tb_main, popup, menu_id));
}

//===========================================================================

//===========================================================================
#else
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

void Toolbar_UpdatePosition()
{
}

void Toolbar_ShowMenu(bool pop)
{
}

//===========================================================================
#endif
