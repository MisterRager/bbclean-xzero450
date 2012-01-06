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

// Contextmenu.cpp

#include "../BB.h"
#include "../Settings.h"
#include "Menu.h"

#include <shlobj.h>
#include <shellapi.h>

#ifndef CMF_CANRENAME
#define CMF_CANRENAME 16
#endif
#ifndef CMF_EXTENDEDVERBS
#define CMF_EXTENDEDVERBS 0x00000100 // rarely used verbs
#endif

//===========================================================================

//                          class ShellContext

//===========================================================================

class ShellContext
{
public:
    friend Menu *MakeContextMenu(const char *path, const void *pidl);
    ShellContext(BOOL *, LPCITEMIDLIST);
    int ShellMenu(void);
    void Invoke(int nCmd);
    void decref(void) { if (0==--refc) delete this; }
    void addref(void) { ++refc; }
    HRESULT HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        LPCONTEXTMENU2 p = (LPCONTEXTMENU2)this->pContextMenu;
        return COMCALL3(p, HandleMenuMsg, uMsg, wParam, lParam);
    }

private:
    enum { MIN_SHELL_ID = 1, MAX_SHELL_ID = 0x7FFF };
    virtual ~ShellContext ();
    int refc;

    LPSHELLFOLDER psfFolder       ;
    LPITEMIDLIST  pidlItem        ;
    LPITEMIDLIST  pidlFull        ;
    LPCONTEXTMENU pContextMenu    ;
    HMENU hMenu;

    static WNDPROC         g_pOldWndProc;
    static LPCONTEXTMENU2  g_pIContext2;
    static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

//===========================================================================

ShellContext::ShellContext(BOOL *r, LPCITEMIDLIST pidl)
{
    hMenu   = NULL;
    refc    = 0;
    *r      = FALSE;
    if (sh_get_uiobject(pidl, &pidlFull, &pidlItem, &psfFolder, IID_IContextMenu, (void**)&pContextMenu))
    {
        HRESULT hr;
        hMenu = CreatePopupMenu();
        hr = pContextMenu->QueryContextMenu(
            hMenu, 
            0, MIN_SHELL_ID, MAX_SHELL_ID,
            CMF_EXPLORE|CMF_CANRENAME//|CMF_EXTENDEDVERBS
            );

        if (SUCCEEDED(hr))
        {
            *r=TRUE;
        }
    }
}

ShellContext::~ShellContext ()
{
    if (hMenu) 
        DestroyMenu(hMenu);
    if (pContextMenu) 
        pContextMenu->Release();
    if (psfFolder   ) 
        psfFolder->Release();
    freeIDList(pidlItem);
    freeIDList(pidlFull);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ShellContext::Invoke(int nCmd)
{
    HRESULT hr = S_OK;

    if (19 == nCmd) // rename
    {
        char oldName[100];
        char newName[100];
        WCHAR newNameW[100];
        sh_getnameof(psfFolder, pidlItem, SHGDN_NORMAL, oldName);
        if (IDOK == EditBox(BBAPPNAME, "Enter new name:", oldName, newName))
        {
            MultiByteToWideChar (CP_ACP, 0, newName, -1, newNameW, array_count(newNameW));
            hr = psfFolder->SetNameOf(NULL, (LPCITEMIDLIST)pidlItem, newNameW, SHGDN_NORMAL, NULL);
        }
    }
    else
    if (nCmd >= MIN_SHELL_ID)
    {
        CMINVOKECOMMANDINFO ici;
        ici.cbSize          = sizeof(ici);
        ici.fMask           = 0;//CMIC_MASK_FLAG_NO_UI;
        ici.hwnd            = NULL;
        ici.lpVerb          = (LPCSTR)(ULONG_PTR)(nCmd - MIN_SHELL_ID);
        ici.lpParameters    = NULL;
        ici.lpDirectory     = NULL;
        ici.nShow           = SW_SHOWNORMAL;
        ici.dwHotKey        = 0;
        ici.hIcon           = NULL;
        hr = pContextMenu->InvokeCommand(&ici);
    }

    if (0==SUCCEEDED(hr))
    {
        ;//MessageBeep(MB_OK);
    }

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This below is for the normal windows context menu

WNDPROC         ShellContext::g_pOldWndProc; // regular window proc
LPCONTEXTMENU2  ShellContext::g_pIContext2;  // active shell context menu

int ShellContext::ShellMenu(void)
{
    HWND hwnd = BBhwnd;
    POINT point;
    int nCmd;

    GetCursorPos(&point);

    g_pIContext2  = (LPCONTEXTMENU2)pContextMenu;
    g_pOldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);

    nCmd = TrackPopupMenu (
        hMenu,
        TPM_LEFTALIGN
        | TPM_LEFTBUTTON
        | TPM_RIGHTBUTTON
        | TPM_RETURNCMD,
        point.x, point.y, 0, hwnd, NULL);

    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_pOldWndProc);
    return nCmd;
}

LRESULT CALLBACK ShellContext::HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   { 
       case WM_DRAWITEM:
       case WM_MEASUREITEM:
            if (wParam) break; // not menu related
            g_pIContext2->HandleMenuMsg(msg, wParam, lParam);
            return TRUE; // handled

       case WM_INITMENUPOPUP: // this will fill the "open with" / "send to" menus
            g_pIContext2->HandleMenuMsg(msg, wParam, lParam);
            return 0;
   }
   return CallWindowProc(g_pOldWndProc, hWnd, msg, wParam, lParam);
}

//===========================================================================

//                          class ContextMenu

//===========================================================================
// This below is for the BB-style context menu

ContextMenu::ContextMenu (const char* title, class ShellContext* w, HMENU hm, int m)
    : Menu (title)
{
    m_MenuID = MENU_ID_SHCONTEXT;
    (wc=w)->addref();
    Copymenu(hm);
}

ContextMenu::~ContextMenu()
{
    wc->decref();
}

ContextItem::ContextItem(Menu *m, char* pszTitle, int id, DWORD data, UINT type)
    : FolderItem(m, pszTitle)
{
    m_id   = id;
    m_data = data;
    m_type = type;
    m_bmp = NULL;
    if (NULL == m)
    {
        m_nSortPriority = M_SORT_NORMAL;
        m_ItemID = MENUITEM_ID_NORMAL;
    }
}

ContextItem::~ContextItem()
{
    if (m_bmp)
        DeleteObject(m_bmp);
}

//===========================================================================

void ContextMenu::Copymenu (HMENU hm)
{
    char text_string[128];
    int n, c, id;
    MENUITEMINFO MII;
    Menu *CM;
    MenuItem *CI;
    bool sep = false;

    static int (WINAPI *pGetMenuStringW)(HMENU,UINT,LPWSTR,int,UINT);

    for (c = GetMenuItemCount(hm),n = 0; n < c; n++)
    {
        memset(&MII, 0, sizeof MII);
        if (usingXP)
            MII.cbSize = sizeof MII;
        else
            MII.cbSize = MENUITEMINFO_SIZE_0400; // to make this work on win95

        MII.fMask  = MIIM_DATA|MIIM_ID|MIIM_SUBMENU|MIIM_TYPE;
        GetMenuItemInfo (hm, n, TRUE, &MII);
        id = MII.wID;

        text_string[0] = 0;
        if (0 == (MII.fType & MFT_OWNERDRAW)) {
            if (usingNT
             && load_imp(&pGetMenuStringW, "user32.dll", "GetMenuStringW")) {
                WCHAR wstr[128];
                pGetMenuStringW(hm, n, wstr, array_count(wstr), MF_BYPOSITION);
                bbWC2MB(wstr, text_string, sizeof text_string);
            } else {
                GetMenuStringA(hm, n, text_string, sizeof text_string, MF_BYPOSITION);
            }
        }

        //char buffer[100]; sprintf(buffer, "%d <%s>", id, text_string); strcpy(text_string, buffer);

        CM = NULL;
        if (MII.hSubMenu)
        {
            wc->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)MII.hSubMenu, MAKELPARAM(n, FALSE));
            if (usingWin7)
                BBSleep(10);
            CM = new ContextMenu(text_string, wc, MII.hSubMenu, 0);

        }
        else
        if (MII.fType & MFT_SEPARATOR)
        {
            sep = true;
            continue;
        }
#if 0
        if (sep)
            MakeMenuNOP(this, NULL), sep = false;
#endif
        CI = new ContextItem(CM, text_string, id, MII.dwItemData, MII.fType);
        AddMenuItem(CI);
    }
}

//===========================================================================

//                          class ContextItem

//===========================================================================

void ContextItem::Invoke(int button)
{
    if (INVOKE_LEFT & button)
    {
        if (m_ItemID & MENUITEM_ID_FOLDER) {
            FolderItem::Invoke(button);
        } else {
            ShellContext *wc = ((ContextMenu*)m_pMenu)->wc;
            wc->addref();
            m_pMenu->hide_on_click();
            wc->Invoke(m_id);
            wc->decref();
        }
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ContextItem::DrawItem(HDC buf, int w, int h, bool active)
{
    ContextMenu* Ctxt=(ContextMenu*)m_pMenu;

    DRAWITEMSTRUCT dis;
    dis.CtlType     = ODT_MENU;
    dis.CtlID       = 0;
    dis.itemID      = m_id;
    dis.itemAction  = ODA_DRAWENTIRE;
    dis.itemState   = 0;
    dis.hwndItem    = Ctxt->m_hwnd;
    dis.hDC         = buf;
    dis.rcItem.left     = 0;
    dis.rcItem.top      = 0;
    dis.rcItem.right    = w;
    dis.rcItem.bottom   = h;
    dis.itemData    = m_data;
    if (active)
        dis.itemState = ODS_SELECTED;
    Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ContextItem::Measure(HDC hDC, SIZE *size)
{
    MEASUREITEMSTRUCT mai;
    int w, h;
    int x, y;
    HDC buf, hdc_s;
    HGDIOBJ other_bmp;
    HGDIOBJ otherfont;
    RECT r;
    HBRUSH hbr;
    ContextMenu* Ctxt=(ContextMenu*)m_pMenu;

    if (0==(m_type & MFT_OWNERDRAW))
    {
        MenuItem::Measure(hDC, size);
        return;
    }


    mai.CtlType     = ODT_MENU; // type of control
    mai.CtlID       = 0;        // combo box, list box, or button identifier
    mai.itemID      = m_id;     // menu item, variable-height list box, or combo box identifier
    mai.itemWidth   = 0;        // width of menu item, in pixels
    mai.itemHeight  = 0;        // height of single item in list box menu, in pixels
    mai.itemData    = m_data;   // application-defined 32-bit value

    Ctxt->wc->HandleMenuMsg(WM_MEASUREITEM, 0, (LPARAM)&mai);
    // the dumb measure method does not take an HDC,
    // and as such uses the system font as base instead of our's

    w = mai.itemWidth * 2;
    h = MenuInfo.nItemHeight;

    if (m_bmp)
        DeleteObject(m_bmp);

    buf = CreateCompatibleDC(NULL);

    hdc_s = GetDC(NULL);
    m_bmp = CreateCompatibleBitmap(hdc_s, w, h);
    ReleaseDC(NULL, hdc_s);

    other_bmp = SelectObject(buf, m_bmp);

    hbr = CreateSolidBrush(GetSysColor(COLOR_MENU));
    r.left = r.top = 0, r.right = w, r.bottom = h;
    FillRect(buf, &r, hbr);
    DeleteObject(hbr);

    // get the background color for reference
    cr_back = GetPixel(buf, 0, 0);

    SetBkColor(buf, cr_back);
    SetBkMode(buf, TRANSPARENT);
    SetTextColor(buf, GetSysColor(COLOR_MENUTEXT));

    // let the item draw with our font
    otherfont = SelectObject(buf, MenuInfo.hFrameFont);
    DrawItem(buf, w, h, false);
    SelectObject(buf, otherfont);

    // trick 17 (get the real width of the item)
    for (x = w; --x;)
        for (y = 0; y < h; y++)
            if (cr_back != GetPixel(buf, x, y)) {
                w = x+2;
                goto _break;
            }
_break:
    SelectObject(buf, other_bmp);
    DeleteDC(buf);
    // 29 = SendTo-Items
    m_icon_offset = Settings_contextMenuAdjust[m_id == 29];
    m_bmp_width = w;
    size->cx = w - m_icon_offset;
    size->cy = MenuInfo.nItemHeight;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ContextItem::Paint(HDC hDC)
{
    RECT r;
    int w, h, m, tw, x, y;
    HDC buf;
    HGDIOBJ other_bmp;
    COLORREF crtxt_bb;

    if (m_ItemID & MENUITEM_ID_FOLDER)
        FolderItem::Paint(hDC);
    else
        MenuItem::Paint(hDC);

    if (0==(m_type & MFT_OWNERDRAW))
        return;

    GetTextRect(&r);
    w =  r.right  - r.left;
    h =  r.bottom - r.top;
    // the remaining margin
    m = imax(0, w - (m_bmp_width - m_icon_offset));
    // text width
    tw = w - m;

    buf = CreateCompatibleDC(NULL);
    other_bmp = SelectObject(buf, m_bmp);
#if 0
    BitBlt(hDC, r.left, r.top, tw, h, buf, m_icon_offset, 0, SRCCOPY);
#else
    // adjust offset according to justifications
    if (mStyle.MenuFrame.Justify == DT_CENTER)
        m /= 2;
    else
    if (mStyle.MenuFrame.Justify != DT_RIGHT)
        m = 0;

    // then plot points when they seem to have the textcolor
    // icons on the left are cut off
    crtxt_bb = m_bActive
        ? mStyle.MenuHilite.TextColor : mStyle.MenuFrame.TextColor;

    for (y = 0; y < h; y++)
        for (x = 0; x < tw; x++)
            if (cr_back != GetPixel(buf, x+m_icon_offset, y))
                SetPixel (hDC, r.left+m+x, r.top+y, crtxt_bb);
#endif

    SelectObject(buf, other_bmp);
    // this let's the handler know which command to invoke eventually
    if (m_bActive)
        DrawItem(buf, m_bmp_width, h, true);
    DeleteDC(buf);
}

//===========================================================================

//                      Global access functions

//===========================================================================

Menu *MakeContextMenu(const char *path, const void *pidl)
{
    char buffer[MAX_PATH];
    ShellContext *wc;
    BOOL r = FALSE;
    Menu *m = NULL;
    LPITEMIDLIST path_pidl = NULL;

    if (NULL == pidl && path)
        pidl = path_pidl = get_folder_pidl(path);

    wc = new ShellContext(&r, (LPCITEMIDLIST)pidl);
    if (FALSE == r) {
       delete wc;

    } else if (Settings_shellContextMenu) {
        int nCmd = wc->ShellMenu();
        if (nCmd)
            wc->Invoke(nCmd);
        delete wc;

    } else {
        sh_getdisplayname((LPCITEMIDLIST)pidl, buffer);
        m = new ContextMenu(buffer, wc, wc->hMenu, 1);
    }

    freeIDList(path_pidl);
    return m;
}

//===========================================================================
