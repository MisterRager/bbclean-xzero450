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

// Contextmenu.cpp

#include "../BB.h"
#include "../Pidl.h"
#include "../Settings.h"
#include "Menumaker.h"
#include "Menu.h"

#include <shlobj.h>
#include <shellapi.h>

typedef struct
{
	UINT     cbSize;
	UINT     fMask;
	UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
	UINT     fState;        // used if MIIM_STATE
	UINT     wID;           // used if MIIM_ID
	HMENU    hSubMenu;      // used if MIIM_SUBMENU
	HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
	HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
	DWORD    dwItemData;    // used if MIIM_DATA
	LPSTR    dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
	UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
#if 0 // (WINVER >= 0x0500)
	HBITMAP  hbmpItem;      // used if MIIM_BITMAP
#endif /* WINVER >= 0x0500 */
}   MENUITEMINFO_0400;

//===========================================================================

//                          class ShellContext

//===========================================================================

class ShellContext
{
public:
	friend Menu *GetContextMenu(LPCITEMIDLIST pidl);
	//friend class ContextMenu;

	ShellContext(BOOL *, LPCITEMIDLIST);
	int ShellMenu(void);
	void Invoke(int nCmd);
	void decref(void) { if (0==--refc) delete this; }
	void addref(void) { ++refc; }
	HRESULT HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ((LPCONTEXTMENU2)pContextMenu)->HandleMenuMsg(uMsg, wParam, lParam);
	}
private:
	enum { MIN_SHELL_ID = 1, MAX_SHELL_ID = 30000 };
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
		hMenu = CreatePopupMenu();
		HRESULT hr = pContextMenu->QueryContextMenu(
			hMenu, 0, MIN_SHELL_ID, MAX_SHELL_ID,
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
	if (hMenu)         DestroyMenu(hMenu);
	if (pContextMenu)  pContextMenu  ->Release();
	if (psfFolder   )  psfFolder     ->Release();
	if (pidlItem    )  m_free(pidlItem);
	if (pidlFull    )  m_free(pidlFull);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ShellContext::Invoke(int nCmd)
{
	HRESULT hr = S_OK;
	if (19 == nCmd) // rename
	{
		char oldName[256]; char newName[256]; WCHAR newNameW[256];
		sh_get_displayname(psfFolder, pidlItem, SHGDN_NORMAL, oldName);
		if (IDOK == EditBox("bbLean", "Enter new name:", oldName, newName))
		{
			MultiByteToWideChar (CP_ACP, 0, newName, -1, newNameW, 256);
			hr = psfFolder->SetNameOf(NULL, pidlItem, newNameW, SHGDN_NORMAL, NULL);
		}
	}
	else
	{
		CMINVOKECOMMANDINFO ici;
		ici.cbSize          = sizeof(ici);
		ici.fMask           = 0;//CMIC_MASK_FLAG_NO_UI;
		ici.hwnd            = NULL;
		ici.lpVerb          = (const char*)(nCmd - MIN_SHELL_ID);
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

int ShellContext::ShellMenu()
{
	POINT point;
	GetCursorPos(&point);
	HWND hwnd = WindowFromPoint(point);
	SetForegroundWindow(hwnd);

	g_pIContext2  = (LPCONTEXTMENU2)pContextMenu;
	g_pOldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);

	int nCmd = TrackPopupMenu (hMenu,
		  TPM_LEFTALIGN
		| TPM_LEFTBUTTON
		| TPM_RIGHTBUTTON
		| TPM_RETURNCMD,
		point.x, point.y, 0, hwnd, NULL);

	SetWindowLong(hwnd, GWLP_WNDPROC, (LONG_PTR)g_pOldWndProc);
	return nCmd;
}

LRESULT CALLBACK ShellContext::HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   { 
	   case WM_DRAWITEM:
	   case WM_MEASUREITEM:
			if(wParam) break; // not menu related
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

ContextMenu::ContextMenu (LPCSTR title, class ShellContext* w, HMENU hm, int m)
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

ContextItem::ContextItem(Menu *m, LPSTR pszTitle, int id, DWORD data, UINT type)
	: FolderItem(m, pszTitle, NULL)
{
	m_id   = id;
	m_data = data;
	m_type = type;
	m_bmp = NULL;
	if (NULL == m)
	{
		m_nSortPriority = M_SORT_NORMAL;
		m_ItemID = MENUITEM_ID_CONTEXT;
	}
}

ContextItem::~ContextItem()
{
	if (m_bmp) DeleteObject(m_bmp);
}

//===========================================================================
void ContextMenu::Copymenu(HMENU hm)
{
	char text_string[256];
	for (int i = 0; i < GetMenuItemCount(hm); i++)
	{
		MENUITEMINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(MENUITEMINFO_0400); // to make this work on win95
		info.fMask  = MIIM_DATA|MIIM_ID|MIIM_SUBMENU|MIIM_TYPE;
		GetMenuItemInfo (hm, i, TRUE, &info);

		text_string[0]=0;
		if (0 == (info.fType & MFT_OWNERDRAW))
			GetMenuString(hm, i, text_string, 128, MF_BYPOSITION);

		//char buffer[256]; sprintf(buffer, "%d %s", info.wID, text_string); strcpy(text_string, buffer);

		Menu *CM = NULL;
		if (info.hSubMenu)
		{
			wc->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)info.hSubMenu, MAKELPARAM(i, FALSE));
			CM = new ContextMenu(text_string, wc, info.hSubMenu, 0);
		}
		else
		if (info.fType & MFT_SEPARATOR)
		{
			//MakeMenuNOP(this, NULL);
			continue;
		}

		MenuItem *CI = new ContextItem(CM, text_string, info.wID, info.dwItemData, info.fType);
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
		if (m_type & MFT_SEPARATOR) return;
		ShellContext *wc = ((ContextMenu*)m_pMenu)->wc;
		wc->addref();
		m_pMenu->hide_on_click();
		wc->Invoke(m_id);
		wc->decref();
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
	if (active) dis.itemState = ODS_SELECTED;

	Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define CRTXT_WIN 0x000000
#define CRBACK    0xffffff

void ContextItem::Measure(HDC hDC, SIZE *size)
{
	if (0==(m_type & MFT_OWNERDRAW))
	{
		MenuItem::Measure(hDC, size);
		return;
	}

	ContextMenu* Ctxt=(ContextMenu*)m_pMenu;

	MEASUREITEMSTRUCT mai;
	mai.CtlType     = ODT_MENU; // type of control
	mai.CtlID       = 0;        // combo box, list box, or button identifier
	mai.itemID      = m_id;     // menu item, variable-height list box, or combo box identifier
	mai.itemWidth   = 0;        // width of menu item, in pixels
	mai.itemHeight  = 0;        // height of single item in list box menu, in pixels
	mai.itemData    = m_data;   // application-defined 32-bit value

	Ctxt->wc->HandleMenuMsg(WM_MEASUREITEM, 0, (LPARAM)&mai);
	// the dumb measure method does not take an HDC,
	// and as such uses the system font as base instead of our's

	int w = mai.itemWidth * 3 / 2;
	int h = MenuInfo.nItemHeight;

	if (m_bmp) DeleteObject(m_bmp);

	// create a monochrome bitmap
	HDC buf = CreateCompatibleDC(NULL);
	m_bmp = CreateBitmap(w, h, 1, 1, NULL);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);

	SetBkColor(buf, CRBACK);
	SetBkMode(buf, TRANSPARENT);
	SetTextColor(buf, CRTXT_WIN);

	HBRUSH hbr = CreateSolidBrush(CRBACK);
	RECT r = { 0, 0, w, h };
	FillRect(buf, &r, hbr);
	DeleteObject(hbr);

	// let the item draw with our font
	HGDIOBJ otherfont = SelectObject(buf, MenuInfo.hFrameFont);
	DrawItem(buf, w, h, false);
	SelectObject(buf, otherfont);

	// trick 17 (get the real width of the item)
	{
		int x, y;
		for (x = w; --x;)
			for (y = 0; y < h; y++)
				if (CRTXT_WIN == GetPixel(buf, x, y))
				{
					w = x+2;
					goto _break;
				}
		_break:;
	}

	SelectObject(buf, other_bmp);
	DeleteDC(buf);

	static int g_contextMenuAdjust[2];
	if (0 == MenuInfo.nAvgFontWidth)
	{
		MenuInfo.nAvgFontWidth = 1;
		const char *p = ReadString(extensionsrcPath(), "blackbox.contextmenu.itemAdjust:",  "21/28");
		g_contextMenuAdjust[0] =
		g_contextMenuAdjust[1] = atoi(p);
		if (NULL != (p = strchr(p, '/')))
		g_contextMenuAdjust[1] = atoi(p+1);
	}

	m_icon_offset = m_id == 29 // 29 = SendTo-Items
		? g_contextMenuAdjust[1]
		: g_contextMenuAdjust[0]
		;

	m_bmp_width = w;
	size->cx = w - m_icon_offset;
	size->cy = MenuInfo.nItemHeight;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ContextItem::Paint(HDC hDC)
{
	if (m_ItemID == MENUITEM_ID_FOLDER)
		FolderItem::Paint(hDC);
	else
		MenuItem::Paint(hDC);

	if (0==(m_type & MFT_OWNERDRAW))
		return;

	RECT r; GetTextRect(&r);
	int w =  r.right  - r.left;
	int h =  r.bottom - r.top;
	// the remaining margin
	int m = imax(0, w - (m_bmp_width - m_icon_offset));
	// text width
	int tw = w - m;

	HDC buf = CreateCompatibleDC(NULL);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);
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
	COLORREF CRTXT_BB = m_bActive ? mStyle.MenuHilite.TextColor : mStyle.MenuFrame.TextColor;
	int x, y;
	for (y = 0; y < h; y++)
		for (x = 0; x < tw; x++)
			if (CRTXT_WIN == GetPixel(buf, x+m_icon_offset, y))
				SetPixel (hDC, r.left+m+x, r.top+y, CRTXT_BB);
#endif

	SelectObject(buf, other_bmp);

	// this let's the handler know which command to invoke eventually
	if (m_bActive)  DrawItem(buf, m_bmp_width, h, true);

	DeleteDC(buf);
}

//===========================================================================

//                      Global access functions

//===========================================================================

Menu *GetContextMenu(LPCITEMIDLIST pidl)
{
	BOOL r;
	ShellContext *wc = new ShellContext(&r, pidl);
	if (FALSE==r)
	{
	   delete wc;
	   return NULL;
	}

	if (Settings_shellContextMenu)
	{
		int nCmd = wc->ShellMenu();
		if (nCmd) wc->Invoke(nCmd);
		delete wc;
		return NULL;
	}

	char buffer[MAX_PATH]; sh_getdisplayname(pidl, buffer);
	return new ContextMenu(buffer, wc, wc->hMenu, 1);
}

void MenuItem::ShowContextMenu(const char *path, const struct _ITEMIDLIST *pidl)
{
	struct _ITEMIDLIST *new_pidl = NULL;
	if (NULL == pidl && path) pidl = new_pidl = sh_getpidl(NULL, path);
	Menu *pSub = GetContextMenu(pidl);
	if (new_pidl) m_free(new_pidl);
	if (pSub)
	{
		m_pMenu->HideChild();
		LinkSubmenu(pSub);
		ShowSubMenu();
	}
}

//===========================================================================
