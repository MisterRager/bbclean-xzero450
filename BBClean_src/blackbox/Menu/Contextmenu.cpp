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

#define CR_BLACK 0x000000
#define CR_WHITE 0xffffff
#define MAX_ICONMARGE 32

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

class ODTitleItem : public TitleItem
{
public:
	ODTitleItem(int id, DWORD data);
	~ODTitleItem();
	void Measure(HDC hDC, SIZE *size);
	void Paint(HDC hDC);
private:
	void DrawTextFromBitmap(HDC buf, HDC hDC, COLORREF cr_txt, int left, int top, int tw, int h);
	int m_id;
	DWORD m_data;
	HBITMAP m_bmp;
	int m_text_offset;
	int m_bmp_width;
};

ODTitleItem::ODTitleItem(int id, DWORD data) : TitleItem(NULL)
{
	m_id = id;
	m_data = data;
	m_bmp = NULL;
}

ODTitleItem::~ODTitleItem()
{
	if (m_bmp) DeleteObject(m_bmp);
}

void ODTitleItem::Measure(HDC hDC, SIZE *size)
{
	int x, y, h, w;
	ContextMenu* Ctxt=(ContextMenu*)m_pMenu;

	MEASUREITEMSTRUCT mai;
	mai.CtlType     = ODT_MENU; // type of control
	mai.CtlID       = 0;        // combo box, list box, or button identifier
	mai.itemID      = m_id;     // menu item, variable-height list box, or combo box identifier
	mai.itemWidth   = 0;        // width of menu item, in pixels
	mai.itemHeight  = 0;        // height of single item in list box menu, in pixels
	mai.itemData    = m_data;   // application-defined 32-bit value

	Ctxt->wc->HandleMenuMsg(WM_MEASUREITEM, 0, (LPARAM)&mai);

	HGDIOBJ otherfont = SelectObject(hDC, MenuInfo.hTitleFont);
	h = max(16, MenuInfo.nTitleHeight);
	TEXTMETRIC TXM;
	GetTextMetrics(hDC, &TXM);
	w = MAX_ICONMARGE + (mai.itemWidth - 16) * TXM.tmMaxCharWidth / 8;

	if (m_bmp) DeleteObject(m_bmp);

	// create a bitmap
	HDC buf = CreateCompatibleDC(NULL);
	BITMAPINFOHEADER bv4info;
	ZeroMemory(&bv4info,sizeof(bv4info));
	bv4info.biSize = sizeof(bv4info);
	bv4info.biWidth = w;
	bv4info.biHeight = 2 * h;
	bv4info.biPlanes = 1;
	bv4info.biBitCount = 32;
	bv4info.biCompression = BI_RGB;
	BYTE* pixels;
	m_bmp = CreateDIBSection(NULL, (BITMAPINFO*)&bv4info, DIB_RGB_COLORS, (PVOID*)&pixels, NULL, 0);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);

	// draw item to bitmap
	DRAWITEMSTRUCT dis;
	dis.CtlType     = ODT_MENU;
	dis.CtlID       = 0;
	dis.itemID      = m_id;
	dis.itemAction  = ODA_DRAWENTIRE;
	dis.itemState   = 0;
	dis.hwndItem    = Ctxt->m_hwnd;
	dis.hDC         = buf;
	dis.rcItem.left  = 0;
	dis.rcItem.right = w;
	dis.itemData    = m_data;
	SelectObject(buf, MenuInfo.hTitleFont);
	HBRUSH hbr;

	dis.rcItem.top = 0; dis.rcItem.bottom = h;
	SetBkColor(buf, CR_BLACK);
	SetTextColor(buf, CR_WHITE);
	hbr = CreateSolidBrush(CR_BLACK);
	FillRect(buf, &(dis.rcItem), hbr);
	DeleteObject(hbr);
	Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);

	dis.rcItem.top = h; dis.rcItem.bottom = h * 2;
	SetBkColor(buf, CR_BLACK);
	SetTextColor(buf, CR_BLACK);
	hbr = CreateSolidBrush(CR_BLACK);
	FillRect(buf, &(dis.rcItem), hbr);
	DeleteObject(hbr);
	Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);

	bool ok;
	// find bitmap width
	for (ok = true, x = w; ok && --x;)
		for (y = 0; ok && y < h ; ++y)
			ok = CR_BLACK == GetPixel(buf, x, y);
	m_bmp_width = x + 1;

	// find offset of text
	for (ok = true, x = 0; ok && (x < m_bmp_width); ++x)
		for (y = 0; ok && y < h ; ++y)
			ok = GetPixel(buf, x, y) == GetPixel(buf, x, y + h);
	m_text_offset = x - 1;

	SetBkColor(buf, CR_WHITE);
	SetTextColor(buf, CR_BLACK);
	hbr = CreateSolidBrush(CR_WHITE);
	FillRect(buf, &(dis.rcItem), hbr);
	DeleteObject(hbr);
	Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);

	SelectObject(buf, other_bmp);
	DeleteDC(buf);
	SelectObject(hDC, otherfont);

	size->cx = m_bmp_width - m_text_offset;
	size->cy = MenuInfo.nTitleHeight;
}

void ODTitleItem::DrawTextFromBitmap(HDC buf, HDC hDC, COLORREF cr_txt, int left, int top, int tw, int h)
{
	int redS   = GetRValue(cr_txt);
	int greenS = GetGValue(cr_txt);
	int blueS  = GetBValue(cr_txt);
	int grayS = redS + greenS + blueS;
	int x, y, y2, h2, z = get_fontheight(MenuInfo.hTitleFont) + 2;
	h2 = max(h, 16);
	y  = (h2 - z) / 2;
	y2 = y + z;
	top = top + (h - z) / 2 - y;
	for (; y < y2; y++)
		for (x = 0; x < tw; x++)
		{
			COLORREF cr_srcB = GetPixel(buf, x + m_text_offset, y);
			COLORREF cr_srcW = GetPixel(buf, x + m_text_offset, y + h2);
			if (CR_BLACK != cr_srcB && CR_WHITE != cr_srcW)
			{
				int leftx = left + x;
				int topy = top + y;
				if(CR_WHITE == cr_srcB && CR_BLACK == cr_srcW)
					SetPixel(hDC, leftx, topy, cr_txt);
				else
				{
					COLORREF cr_des = GetPixel(hDC, leftx, topy);
					int hue, red, green, blue;

					hue = GetRValue(cr_srcW); hue -= (hue + GetRValue(cr_srcB) - 255) * grayS / 0x2FD;
					red = (redS * (255 - hue) + GetRValue(cr_des) * hue) / 255;

					hue = GetGValue(cr_srcW); hue -= (hue + GetGValue(cr_srcB) - 255) * grayS / 0x2FD;
					green = (greenS * (255 - hue) + GetGValue(cr_des) * hue) / 255;

					hue = GetBValue(cr_srcW); hue -= (hue + GetBValue(cr_srcB) - 255) * grayS / 0x2FD;
					blue = (blueS * (255 - hue) + GetBValue(cr_des) * hue) / 255;

					SetPixel(hDC, leftx, topy, RGB(red, green, blue));
				}
			}
		}
}

void ODTitleItem::Paint(HDC hDC)
{
	TitleItem::Paint(hDC);

	RECT r; GetItemRect(&r);
	int w =  r.right  - r.left;
	int h =  r.bottom - r.top;
	// the remaining margin
	int m = imax(0, w - (m_bmp_width - m_text_offset));
	// text width
	int tw = w - m;

	HDC buf = CreateCompatibleDC(NULL);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);
	// adjust offset according to justifications
	if (mStyle.MenuTitle.Justify == DT_CENTER)
		m /= 2;
	else
	if (mStyle.MenuTitle.Justify != DT_RIGHT)
		m = 0;

	if (mStyle.MenuTitle.ShadowXY)
		DrawTextFromBitmap(buf, hDC, mStyle.MenuTitle.ShadowColor, r.left + m + mStyle.MenuTitle.ShadowX, r.top + mStyle.MenuTitle.ShadowY, tw, h);
	DrawTextFromBitmap(buf, hDC, mStyle.MenuTitle.TextColor, r.left + m, r.top, tw, h);

	SelectObject(buf, other_bmp);
	DeleteDC(buf);
}
ContextMenu::ContextMenu (LPCSTR title, class ShellContext* w, HMENU hm, int m, int id, DWORD data)
	: Menu (title)
{
	m_MenuID = MENU_ID_SHCONTEXT;
	(wc=w)->addref();

	if (NULL == title || 0 == *title)
	{
		m_ppAddItem = &m_pMenuItems;
		delete m_pMenuItems;
		AddMenuItem(new ODTitleItem(id, data));
	}
	Copymenu(hm);
}

ContextMenu::~ContextMenu()
{
	wc->decref();
}

ContextItem::ContextItem(Menu *m, LPSTR pszTitle, int id, DWORD data, UINT type)
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
	if (m_bmp) DeleteObject(m_bmp);
}

//===========================================================================
void ContextMenu::Copymenu(HMENU hm)
{
	char text_string[256]; int n, c = GetMenuItemCount (hm);
	for (n = 0; n < c; n++)
	{
		MENUITEMINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(MENUITEMINFO_0400); // to make this work on win95
		info.fMask  = MIIM_DATA|MIIM_ID|MIIM_SUBMENU|MIIM_TYPE;
		GetMenuItemInfo (hm, n, TRUE, &info);
		int id = info.wID;

		text_string[0]=0;
		if (0 == (info.fType & MFT_OWNERDRAW))
			GetMenuString(hm, n, text_string, 128, MF_BYPOSITION);

		//char buffer[256]; sprintf(buffer, "%d %s", info.wID, text_string); strcpy(text_string, buffer);

		Menu *CM = NULL;
		if (info.hSubMenu)
		{
			wc->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)info.hSubMenu, MAKELPARAM(n, FALSE));
			CM = new ContextMenu(text_string, wc, info.hSubMenu, 0, id, info.dwItemData);
		}
		else
		if (info.fType & MFT_SEPARATOR)
		{
			MakeMenuNOP(this, NULL)->m_isNOP = MI_NOP_SEP | MI_NOP_LINE;
			continue;
		}

		MenuItem *CI = new ContextItem(CM, text_string, id, info.dwItemData, info.fType);
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
		if (m_ItemID == MENUITEM_ID_FOLDER) return;
		if (m_type & MFT_SEPARATOR) return;
		ShellContext *wc = ((ContextMenu*)m_pMenu)->wc;
		wc->addref();
		m_pMenu->hide_on_click();
		wc->Invoke(m_id);
		wc->decref();
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ContextItem::DrawItem(HDC buf, ContextMenu* Ctxt, DRAWITEMSTRUCT *dis, COLORREF cr_back, COLORREF cr_txt)
{
	SetBkColor(buf, cr_back);
	SetTextColor(buf, cr_txt);
	HBRUSH hbr = CreateSolidBrush(cr_back);
	FillRect(buf, &(dis->rcItem), hbr);
	DeleteObject(hbr);
	Ctxt->wc->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)dis);
}

void ContextItem::DrawItemToBitmap(HDC buf, int w, int h, bool active)
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
	dis.rcItem.left  = 0;
	dis.rcItem.right = w;
	dis.itemData    = m_data;
	if (active) dis.itemState = ODS_SELECTED;

	int y = 0;
	HGDIOBJ otherfont = SelectObject(buf, MenuInfo.hFrameFont);

	dis.rcItem.top = y; dis.rcItem.bottom = y += h;
	DrawItem(buf, Ctxt, &dis, CR_BLACK, CR_WHITE);

	dis.rcItem.top = y; dis.rcItem.bottom = y += h;
	DrawItem(buf, Ctxt, &dis, CR_BLACK, CR_BLACK);

	dis.rcItem.top = y; dis.rcItem.bottom = y += h;
	DrawItem(buf, Ctxt, &dis, CR_WHITE, CR_BLACK);

	dis.rcItem.top = y; dis.rcItem.bottom = y + h;
	DrawItem(buf, Ctxt, &dis, CR_WHITE, CR_WHITE);

	SelectObject(buf, otherfont);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

	int is = m_pMenu->m_iconSize;
	if (-2 == is) is = MenuInfo.nIconSize;
	int h = max(16, MenuInfo.nItemHeight[is]);
	TEXTMETRIC TXM;
	GetTextMetrics(hDC, &TXM);
	int w = MAX_ICONMARGE + max((int)((mai.itemWidth - 16) * TXM.tmMaxCharWidth / 8), is);

	if (m_bmp) DeleteObject(m_bmp);

	// create a bitmap
	HDC buf = CreateCompatibleDC(NULL);
	BITMAPINFOHEADER bv4info;
	ZeroMemory(&bv4info,sizeof(bv4info));
	bv4info.biSize = sizeof(bv4info);
	bv4info.biWidth = w;
	bv4info.biHeight = 4 * h;
	bv4info.biPlanes = 1;
	bv4info.biBitCount = 32;
	bv4info.biCompression = BI_RGB;
	BYTE* pixels;
	m_bmp = CreateDIBSection(NULL, (BITMAPINFO*)&bv4info, DIB_RGB_COLORS, (PVOID*)&pixels, NULL, 0);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);
	DrawItemToBitmap(buf, w, h, false);

	int left, right;
	int x, y, h2 = h + h, h3 = h2 + h;
	bool ok;

	// find bitmap width
	for (ok = true, x = w; ok && --x;)
		for (y = 0; ok && y < h ; ++y)
			ok = CR_BLACK == GetPixel(buf, x, y);
	m_bmp_width = x + 1;

	// find right border of icon
	for (x = MAX_ICONMARGE, ok = true; ok && --x;)
		for (y = h; ok && y < h2 ; ++y)
			ok = CR_BLACK == GetPixel(buf, x, y)
			  && CR_WHITE == GetPixel(buf, x, y + h2);
	right = x + 1;

	// find left border of icon
	for (ok = true, x = 0; ok && (x < right); ++x)
		for (y = h; ok && y < h2 ; ++y)
			ok = CR_BLACK == GetPixel(buf, x, y)
			  && CR_WHITE == GetPixel(buf, x, y + h2);
	left = --x;

	// find offset of text
	for (ok = true; ok && (x < m_bmp_width); ++x)
		for (y = 0; ok && y < h ; ++y)
			ok = GetPixel(buf, x, y) == GetPixel(buf, x, y + h);
	m_text_offset = --x;

	if (is)
	{
		int  top, bottom, height, width;
		for (ok = true, y = h; ok && y < h2; ++y)
			for (x = left; ok && x < right ; ++x)
				ok = CR_BLACK == GetPixel(buf, x, y)
				  && CR_WHITE == GetPixel(buf, x, y + h2);
		top = --y;

		for (ok = true, y = h2 - 1; ok && y > top; --y)
			for (x = left; ok && x < right ; ++x)
				ok = CR_BLACK == GetPixel(buf, x, y)
				  && CR_WHITE == GetPixel(buf, x, y + h2);
		bottom = y + 2;

		height = bottom - top;
		width = right - left;
		int stretchWidth  = width  * is / 16;
		int stretchHeight = height * is / 16;

		m_rIcon.left   = right;
		m_rIcon.top    = h;
		m_rIcon.right  = right + stretchWidth;
		m_rIcon.bottom = h + stretchHeight;
		StretchBlt(buf, right, h, stretchWidth, stretchHeight,
			buf, left, top, width, height, SRCCOPY);
		StretchBlt(buf, right, h3, stretchWidth, stretchHeight,
			buf, left, top + h2, width, height, SRCCOPY);
	}

	SelectObject(buf, other_bmp);
	DeleteDC(buf);

	size->cx = m_bmp_width - m_text_offset;
	size->cy = MenuInfo.nItemHeight[is];
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ContextItem::DrawTextFromBitmap(HDC buf, HDC hDC, COLORREF cr_txt, int left, int top, int tw, int h)
{
	int redS   = GetRValue(cr_txt);
	int greenS = GetGValue(cr_txt);
	int blueS  = GetBValue(cr_txt);
	int grayS = redS + greenS + blueS;
	int x, y, y2, h2, z = get_fontheight(MenuInfo.hFrameFont) + 2;
	h2 = max(h, 16);
	y  = (h2 - z) / 2;
	h2 *= 2;
	y2 = y + z;
	top = top + (h - z) / 2 - y;
	for (; y < y2; y++)
		for (x = 0; x < tw; x++)
		{
			COLORREF cr_srcB = GetPixel(buf, x + m_text_offset, y);
			COLORREF cr_srcW = GetPixel(buf, x + m_text_offset, y + h2);
			if (CR_BLACK != cr_srcB && CR_WHITE != cr_srcW)
			{
				int leftx = left + x;
				int topy = top + y;
				if(CR_WHITE == cr_srcB && CR_BLACK == cr_srcW)
					SetPixel(hDC, leftx, topy, cr_txt);
				else
				{
					COLORREF cr_des = GetPixel(hDC, leftx, topy);
					int hue, red, green, blue;

					hue = GetRValue(cr_srcW); hue -= (hue + GetRValue(cr_srcB) - 255) * grayS / 0x2FD;
					red = (redS * (255 - hue) + GetRValue(cr_des) * hue) / 255;

					hue = GetGValue(cr_srcW); hue -= (hue + GetGValue(cr_srcB) - 255) * grayS / 0x2FD;
					green = (greenS * (255 - hue) + GetGValue(cr_des) * hue) / 255;

					hue = GetBValue(cr_srcW); hue -= (hue + GetBValue(cr_srcB) - 255) * grayS / 0x2FD;
					blue = (blueS * (255 - hue) + GetBValue(cr_des) * hue) / 255;

					SetPixel(hDC, leftx, topy, RGB(red, green, blue));
				}
			}
		}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ContextItem::DrawIconFromBitmap(HDC buf, HDC hDC, int left, int top, int h)
{
	int x, y, h2;
	unsigned sat = m_bActive? 255 : Settings_menuIconSaturation, i_sat = 255 - sat;
	unsigned hue = m_bActive? 0 : Settings_menuIconHue, i_hue = 255 - hue;
	if (16 > h) h = 16;
	h2 = h * 2;

	top  -= m_rIcon.top;
	left -= m_rIcon.left;
	for (y = m_rIcon.top; y < m_rIcon.bottom; ++y)
		for (x = m_rIcon.left; x < m_rIcon.right; ++x)
		{
			COLORREF c1 = GetPixel(buf, x, y + h2);
			COLORREF c2 = GetPixel(buf, x, y);

			// if something to paint
			if (CR_WHITE != c1 || CR_BLACK != c2)
			{
				unsigned cR, cG, cB;
				cR = GetRValue(c2);
				cG = GetGValue(c2);
				cB = GetBValue(c2);
				COLORREF c3 = GetPixel(hDC, left + x, top + y);
				unsigned cNotA = GetRValue(c1) - cR;
				if (i_sat)
				{
					unsigned grey = (cR*79 + cG*155 + cB*21) * i_sat / 255;
					cR = (cR * sat + grey) / 255;
					cG = (cG * sat + grey) / 255;
					cB = (cB * sat + grey) / 255;
				}
				if (cNotA) // pixel with transparence
				{
					cR += GetRValue(c3) * cNotA / 255;
					cG += GetGValue(c3) * cNotA / 255;
					cB += GetBValue(c3) * cNotA / 255;
				}
				if (hue)
				{
					cR = (cR * i_hue + GetRValue(c3) * hue) / 255;
					cG = (cG * i_hue + GetGValue(c3) * hue) / 255;
					cB = (cB * i_hue + GetBValue(c3) * hue) / 255;
				}

				SetPixel(hDC, left + x, top + y, RGB(cR, cG, cB));
			}
		}
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

	int is = m_pMenu->m_iconSize;
	if (-2 == is) is = MenuInfo.nIconSize;
	RECT r; GetTextRect(&r, is);
	int w =  r.right  - r.left;
	int h =  r.bottom - r.top;
	// the remaining margin
	int m = imax(0, w - (m_bmp_width - m_text_offset));
	// text width
	int tw = w - m;

	HDC buf = CreateCompatibleDC(NULL);
	HGDIOBJ other_bmp = SelectObject(buf, m_bmp);
	// adjust offset according to justifications
	if (mStyle.MenuFrame.Justify == DT_CENTER)
		m /= 2;
	else
	if (mStyle.MenuFrame.Justify != DT_RIGHT)
		m = 0;

	// draw icon
	if (is)
		DrawIconFromBitmap(buf, hDC,
			((DT_LEFT == FolderItem::m_nBulletPosition)? r.right : (r.left - MenuInfo.nItemIndent[is])) + (MenuInfo.nItemIndent[is] - m_rIcon.right + m_rIcon.left) / 2,
			r.top + (h - m_rIcon.bottom + m_rIcon.top) / 2,
			h);

	// draw text
	StyleItem *pSI = m_bActive ? &mStyle.MenuHilite : &mStyle.MenuFrame;
	int ShaX = pSI->ShadowX;
	int ShaY = pSI->ShadowY;
	if (ShaX || ShaY)
		DrawTextFromBitmap(buf, hDC, pSI->ShadowColor, r.left + m + ShaX, r.top + ShaY, tw, h);
	DrawTextFromBitmap(buf, hDC, pSI->TextColor, r.left + m, r.top, tw, h);

	SelectObject(buf, other_bmp);

	// this let's the handler know which command to invoke eventually
	if (m_bActive)  DrawItemToBitmap(buf, m_bmp_width, h, true);

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
