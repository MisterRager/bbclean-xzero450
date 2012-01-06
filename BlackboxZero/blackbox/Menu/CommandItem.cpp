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

#include "../BB.h"
#include "../Settings.h"
#include "Menu.h"

//===========================================================================
//
// CommandItem
//
//===========================================================================

CommandItem::CommandItem(const char* pszCommand, const char* pszTitle, bool bChecked)
    : MenuItem(pszTitle)
{
    m_pszCommand = new_str(pszCommand);
    m_bChecked = bChecked;
    m_ItemID = MENUITEM_ID_CMD;
}

//====================
void CommandItem::Invoke(int button)
{
    LPCITEMIDLIST pidl = GetPidl();
    if (INVOKE_PROP & button)
    {
        show_props(pidl);
        return;
    }

    if (INVOKE_LEFT & button)
    {
        m_pMenu->hide_on_click();
        if (m_pszCommand) {
            if (strstr(m_pszCommand, "%b"))
                post_command_fmt(m_pszCommand, false == m_bChecked);
            else
                post_command(m_pszCommand);
        } else if (pidl)
            BBExecute_pidl(NULL, pidl);
        return;
    }

    if (INVOKE_RIGHT & button)
    {
        if (m_pszRightCommand)
            post_command(m_pszRightCommand);
        else
        if (m_pRightmenu)
            m_pRightmenu->incref(), ShowRightMenu(m_pRightmenu);
        else
            ShowContextMenu(m_pszCommand, pidl);
        return;
    }

    if (INVOKE_DRAG & button)
    {
        m_pMenu->start_drag(m_pszCommand, pidl);
        return;
    }
}

//===========================================================================
// helper for IntegerItem / StringItem

void CommandItem::next_item (WPARAM wParam)
{
    Menu *pm = m_pMenu->m_pParent;
    if (pm) {
        pm->set_focus();
        Active(0);
        PostMessage(pm->m_hwnd, WM_KEYDOWN, wParam, 0);
    }
}

//===========================================================================
//
// IntegerItem
//
//===========================================================================

IntegerItem::IntegerItem(const char* pszCommand, int value, int minval, int maxval)
    : CommandItem(pszCommand, NULL, false)
{
    m_value = value;
    m_min = minval;
    m_max = maxval;
    m_oldsize = 0;
    m_direction = 0;
    m_count = 0;
    m_offvalue = 0x7FFFFFFF;
    m_offstring = NULL;
    m_ItemID = MENUITEM_ID_INT;
}

void IntegerItem::Measure(HDC hDC, SIZE *size)
{
    if (Settings_menu.showBroams)
    {
        MenuItem::Measure(hDC, size);
        m_Justify = MENUITEM_STANDARD_JUSTIFY;
    }
    else
    {
        char buf[100];
        char val[100];
        const char *s;
        if (m_offvalue == m_value && m_offstring)
            s = NLS1(m_offstring);
        else
            s = val, sprintf(val, "%d", m_value);

        sprintf(buf, "%c %s %c",
            m_value == m_min ? ' ': '-',
            s,
            m_value == m_max ? ' ': '+'
            );
        replace_str(&m_pszTitle, buf);

        MenuItem::Measure(hDC, size);
        if (size->cx > m_oldsize+5 || size->cx < m_oldsize-5)
            m_oldsize = size->cx;
        size->cx = m_oldsize + 5;
        m_Justify = DT_CENTER;
    }
}

//====================

void IntegerItem::Mouse(HWND hwnd, UINT uMsg, DWORD wParam, DWORD lParam)
{
    MenuItem::Mouse(hwnd, uMsg, wParam, lParam);
    if (false == Settings_menu.showBroams)
        switch(uMsg) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            {
                int xmouse = (short)LOWORD(lParam);
                int xwidth = m_pMenu->m_width;
                m_direction = (xmouse < xwidth / 2) ? -1 : 1;
                ItemTimer(MENU_INTITEM_TIMER);
                break;
            }
            case WM_LBUTTONUP:
                break;
        }
}

void IntegerItem::set_next_value(void)
{
    int value, dir, mod, c, d;
    value = m_value;

    dir = m_direction;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        dir *= 10;
    for (c = 15, d = 2; c < m_count; d = d==2?5:2, c += 10 + 4*d)
        dir *= d;

    mod = value % dir;
    if (mod > 0 && dir < 0) 
        mod+=dir;
    value -= mod;

    m_value = iminmax(value + dir, m_min, m_max);
    m_pMenu->Redraw(0);
    ++m_count;
}

void IntegerItem::ItemTimer(UINT nTimer)
{
    HWND hwnd;
    if (MENU_INTITEM_TIMER != nTimer)
    {
        MenuItem::ItemTimer(nTimer);
        return;
    }
    hwnd = m_pMenu->m_hwnd;
    if (0 == m_direction || GetCapture() != hwnd)
    {
        KillTimer(hwnd, nTimer);
        return;
    }
    SetTimer(hwnd, MENU_INTITEM_TIMER, 0 == m_count ? 320 : 80, NULL);
    set_next_value();
}

void IntegerItem::Key(UINT nMsg, WPARAM wParam)
{
    Active(2);

    if (WM_KEYDOWN == nMsg)
    {
        if (VK_LEFT == wParam)
        {
            m_direction = -1;
            set_next_value();
        }
        else
        if (VK_RIGHT == wParam)
        {
            m_direction = 1;
            set_next_value();
        }
        else
        if (VK_UP == wParam || VK_DOWN==wParam || VK_TAB == wParam)
        {
            next_item(wParam);
        }
    }
    else
    if (WM_KEYUP == nMsg)
    {
        if (VK_LEFT == wParam || VK_RIGHT == wParam)
        {
            Invoke(INVOKE_LEFT);
        }
    }
    else
    if (WM_CHAR == nMsg && (VK_ESCAPE == wParam || VK_RETURN==wParam))
    {
        next_item(0);
    }

}

void IntegerItem::Invoke(int button)
{
    if (0 == m_direction)
        return;
    m_direction = 0;
    m_count = 0;
    if (INVOKE_LEFT & button) {
        if (strstr(m_pszCommand, "%d"))
            post_command_fmt(m_pszCommand, m_value);
        else
            post_command_fmt("%s %d", m_pszCommand, m_value);
    }
}

//===========================================================================
//
// StringItem
//
//===========================================================================

StringItem::StringItem(const char* pszCommand, const char *init_string)
    : CommandItem(pszCommand, NULL, false)
{
    if (init_string)
        replace_str(&m_pszTitle, init_string);
    hText = NULL;
    m_ItemID = MENUITEM_ID_STR;
}

StringItem::~StringItem()
{
    if (hText)
        DestroyWindow(hText);
}

void StringItem::Paint(HDC hDC)
{
    RECT r;
    HFONT hFont;
    int x, y, w, h, padd;
    if (Settings_menu.showBroams)
    {
        if (hText)
            DestroyWindow(hText), hText = NULL;
        m_Justify = MENUITEM_STANDARD_JUSTIFY;
        MenuItem::Paint(hDC);
        return;
    }

    m_Justify = MENUITEM_CUSTOMTEXT;
    MenuItem::Paint(hDC);

    GetTextRect(&r);
    if (EqualRect(&m_textrect, &r))
        return;

    m_textrect = r;

    if (NULL == hText)
    {
        hText = CreateWindow(
            TEXT("EDIT"),
            m_pszTitle,
            WS_CHILD
            | WS_VISIBLE
            | ES_AUTOHSCROLL
            | ES_MULTILINE,
            0, 0, 0, 0,
            m_pMenu->m_hwnd,
            (HMENU)1234,
            hMainInstance,
            NULL
            );

        SetWindowLongPtr(hText, GWLP_USERDATA, (LONG_PTR)this);
        wpEditProc = (WNDPROC)SetWindowLongPtr(hText, GWLP_WNDPROC, (LONG_PTR)EditProc);
#if 0
        int n = GetWindowTextLength(hText);
        SendMessage(hText, EM_SETSEL, 0, n);
        SendMessage(hText, EM_SCROLLCARET, 0, 0);
#endif
        m_pMenu->m_hwndChild = hText;
        if (GetFocus() == m_pMenu->m_hwnd)
            SetFocus(hText);
    }

    hFont = MenuInfo.hFrameFont;
    SendMessage(hText, WM_SETFONT, (WPARAM)hFont, 0);

    x = r.left-1;
    y = r.top+2;
    h = r.bottom - r.top - 4;
    w = r.right - r.left + 2;

    SetWindowPos(hText, NULL, x, y, w, h, SWP_NOZORDER);

    padd = imax(0, (h - get_fontheight(hFont)) / 2);
    r.left  = padd+2;
    r.right = w - (padd+2);
    r.top   = padd;
    r.bottom = h - padd;
    SendMessage(hText, EM_SETRECT, 0, (LPARAM)&r);
}

void StringItem::Measure(HDC hDC, SIZE *size)
{
    MenuItem::Measure(hDC, size);
    if (false == Settings_menu.showBroams)
    {
        size->cx = imax(size->cx + 20, 120);
        size->cy += 6;
    }
    SetRectEmpty(&m_textrect);
}

void StringItem::Invoke(int button)
{
    char *buff;
    int len;
    if (Settings_menu.showBroams)
        return;
    len = 1 + GetWindowTextLength(hText);
    buff = (char*)m_alloc(len);
    GetWindowText(hText, buff, len);
    replace_str(&m_pszTitle, buff);
    if (SendMessage(hText, EM_GETMODIFY, 0, 0) || (button & INVOKE_RET)) {
        SendMessage(hText, EM_SETMODIFY, FALSE, 0);
        if (strstr(m_pszCommand, "%s") || strstr(m_pszCommand, "%q"))
            post_command_fmt(m_pszCommand, buff);
        else
            post_command_fmt("%s %s", m_pszCommand, buff);
    }
    m_free(buff);
}

//===========================================================================

LRESULT CALLBACK StringItem::EditProc(HWND hText, UINT msg, WPARAM wParam, LPARAM lParam)
{
    StringItem *pItem = (StringItem*)GetWindowLongPtr(hText, GWLP_USERDATA);
    LRESULT r = 0;
    Menu *pMenu = pItem->m_pMenu;

    pMenu->incref();
    switch(msg)
    {
        // --------------------------------------------------------
        // Send Result

        case WM_MOUSEMOVE:
            PostMessage(pMenu->m_hwnd, WM_MOUSEMOVE, wParam, MAKELPARAM(10, pItem->m_nTop+2));
            break;

        // --------------------------------------------------------
        // Key Intercept

        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_DOWN:
                case VK_UP:
                case VK_TAB:
                    pItem->Invoke(0);
                    pItem->next_item(wParam);
                    goto leave;

                case VK_RETURN:
                    pItem->Invoke(INVOKE_RET);
                    pItem->next_item(0);
                    goto leave;

                case VK_ESCAPE:
                    SetWindowText(hText, pItem->m_pszTitle);
                    pItem->next_item(0);
                    goto leave;
            }
            break;

        case WM_CHAR:
            switch (wParam)
            {
                case 'A' - 0x40: // ctrl-A: select all
                    SendMessage(hText, EM_SETSEL, 0, GetWindowTextLength(hText));
                case 13:
                case 27:
                    goto leave;
            }
            break;

        // --------------------------------------------------------
        // Paint

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT r;
            StyleItem *pSI;

            hdc = BeginPaint(hText, &ps);
            GetClientRect(hText, &r);
            pSI = &mStyle.MenuFrame;
            MakeGradient(hdc, r,
                pSI->type, pSI->Color, pSI->ColorTo,
                pSI->interlaced, BEVEL_SUNKEN, BEVEL1, 0, 0, 0);
            CallWindowProc(pItem->wpEditProc, hText, msg, (WPARAM)hdc, lParam);
            EndPaint(hText, &ps);
            goto leave;
        }

        case WM_ERASEBKGND:
            r = TRUE;
            goto leave;

        case WM_DESTROY:
            pItem->hText = NULL;
            pMenu->m_hwndChild = NULL;
            break;

        case WM_SETFOCUS:
            break;

        case WM_KILLFOCUS:
            pItem->Invoke(0);
            break;
    }
    r = CallWindowProc(pItem->wpEditProc, hText, msg, wParam, lParam);
leave:
    pMenu->decref();
    return r;
}

//===========================================================================
