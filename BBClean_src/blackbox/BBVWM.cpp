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
*/ // VWM

#include "BB.h"
#include "BBVWM.h"

#define ST static
#define SCREEN_DIST 10

ST bool vwm_alt_method;
ST bool vwm_xpfix;
ST struct winlist
{
	struct winlist *next;
	HWND hwnd;
	RECT rect;
	int  desk;
	int  save_desk;

	bool check;
	bool moved;
	bool hidden;
	bool iconic;
	bool sticky;

} *vwm_WL;

//=========================================================
//                  helper functions
//=========================================================

ST BOOL CALLBACK win_enum_proc(HWND hwnd, LPARAM lParam)
{
	if (CheckSticky(hwnd))
		return TRUE;

	bool hidden = FALSE == IsWindowVisible(hwnd);
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);

	if (hidden && (NULL == wl || false == wl->moved))
		return TRUE;

	if (NULL == wl)
	{
		wl = (winlist *)c_alloc (sizeof *wl);
		wl->hwnd = hwnd;
		wl->desk = currentScreen;
		wl->sticky = check_sticky_name(hwnd);
		cons_node (&vwm_WL, wl);
	}

	wl->iconic = IsIconic(hwnd);
	wl->hidden = hidden;
	if (false == wl->moved && false == wl->iconic)
	{
		GetWindowRect(hwnd, &wl->rect);
	}

	wl->check = true;
	return TRUE;
}

//=========================================================
//              update the list
//=========================================================

void vwm_update_winlist(void)
{
	winlist *wl;
	dolist (wl, vwm_WL) wl->check = false;
	EnumWindows(win_enum_proc, 0);

	winlist **wlp = &vwm_WL;
	while (NULL != (wl = *wlp)) // clear not listed windows
	{
		if (false == wl->check)
			*wlp = wl->next, m_free(wl);
		else
		{
			wlp = &wl->next;
		}
	}

#if 0
	dbg_printf("-----------------");
	dolist (wl, vwm_WL)
	{
		char buffer[80];
		GetClassName(wl->hwnd, buffer, sizeof buffer);
		dbg_printf("hwnd:%x desk:%d moved:%d hidden:%d iconic:%d <%s>",
			wl->hwnd, wl->desk, wl->moved, wl->hidden, wl->iconic, buffer);
	}
#endif
}

//=========================================================

ST void center_window(int *left, int *top, int width, int height)
{
	int x0 = VScreenX - SCREEN_DIST;
	int y0 = VScreenY - SCREEN_DIST;
	int dx = VScreenWidth + SCREEN_DIST;
	int dy = VScreenHeight + SCREEN_DIST;
	int w2 = width/2;
	int h2 = height/2;
	while (*left+w2 <  x0) *left += dx;
	while (*left+w2 >= x0+dx) *left -= dx;
	while (*top+h2  <  y0) *top += dy;
	while (*top+h2  >= y0+dy) *top -= dy;
}

ST void fix_iconized_window(winlist *wl)
{
	WINDOWPLACEMENT wp; wp.length = sizeof wp;
	GetWindowPlacement(wl->hwnd, &wp);
	RECT *n = &wp.rcNormalPosition;
/*
	*n = wl->rect;
	RECT m; GetMonitorRect((POINT*)&wl->rect.left, &m, GETMON_WORKAREA|GETMON_FROM_POINT);
	n->top -= m.top; n->bottom -= m.top;
	n->left -= m.left; n->right -= m.left;
*/
	int left = n->left;
	int top = n->top;
	int width = n->right - n->left;
	int height = n->bottom - n->top;
	center_window(&left, &top, width, height);
	n->left = left;
	n->top = top;
	n->right = width + n->left;
	n->bottom = height + n->top;
	SetWindowPlacement(wl->hwnd, &wp);
}

//=========================================================

ST HWND get_root(HWND hwnd)
{
	HWND pw, dt = GetDesktopWindow();
	while ((pw = GetWindow(hwnd, GW_OWNER)) != dt && pw) hwnd = pw;
	return hwnd;
}

ST void check_owned_windows(HWND hwnd)
{
	hwnd = get_root(hwnd);
	winlist *wl;
	dolist (wl, vwm_WL)
		wl->check = hwnd == get_root(wl->hwnd);
}

/// thread wrapper function
void WINAPI EndDeferWindowPosThreadProc(HDWP hWinPosInfo)
{
	EndDeferWindowPos(hWinPosInfo);
}

/// CyberShadow 2007.05.11: EndDeferWindowPos will block the calling thread if any of the specified windows 
/// belong to a "hung" thread. This causes bbLean to FREEZE if you try to switch workspaces while there's an
/// unresponding application. The following function attempts to do EndDeferWindowPos in an asynchronous manner.
void EndDeferWindowPosAsync(HDWP hWinPosInfo)
{
	if(usingNT)
	{
		// The kernel-mode part of the Windows USER API has a function called NtUserEndDeferWindowPosEx. 
		// This function takes two parameters - the HDWP (the same one used for *DeferWindowPos functions),
		// and a boolean parameter which dictates if the function should work asynchronously.
		// The 2nd parameter is only used internally from Task Manager, so it wouldn't hang while tiling windows.
		// EndDeferWindowPos in user32.dll is simply a wrapper around the NtUserEndDeferWindowPosEx syscall function,
		// similar to:
		// 
		// BOOL WINAPI EndDeferWindowPos(HDWP hWinPosInfo)
		// {
		//     return NtUserEndDeferWindowPosEx(hWinPosInfo, FALSE);
		// }
		// 
		// The following code will get the address of NtUserEndDeferWindowPosEx, and call it directly.

		HMODULE user32 = GetModuleHandle("user32.dll");
		unsigned char* func = (unsigned char*)GetProcAddress(user32, "EndDeferWindowPos");
		if(func && func[10]==0xE8)    // CALL IMM32 opcode
		{
			typedef int WINAPI (*NtUserEndDeferWindowPosEx)(HDWP, BOOL);

			// get address of NtUserEndDeferWindowPosEx, from within EndDeferWindowPos's code
			NtUserEndDeferWindowPosEx func2 = (NtUserEndDeferWindowPosEx) 
				(func + 
				 15   +    // 10+5 (5 is size of the CALL instruction)
				 (func[11] <<  0 |     // CALL's argument is an offset relative to the address of the next instruction
				  func[12] <<  8 |
				  func[13] << 16 |
				  func[14] << 24));
			
			// call it
			func2(hWinPosInfo, TRUE);
		}
		else // something's wrong, do it normally
			EndDeferWindowPos(hWinPosInfo);
	}
	else
	{
		// the above can't apply to Win9x, try to create a simple thread to wrap around the potentially-blocking call
		DWORD tid;
		// this MIGHT be possible to write as CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EndDeferWindowPos, ...) (that is, 
		// create the thread directly with the API), but the way compilers/linkers do DLL bindings could break this
    	CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EndDeferWindowPosThreadProc, (LPVOID)hWinPosInfo, 0, &tid));
	}
}
//=========================================================
ST void defer_windows(int newdesk)
{
	bool gather = newdesk < 0;
	if (gather)
		newdesk = currentScreen;
	else
	if (newdesk >= Settings_workspaces)
		newdesk = Settings_workspaces-1;

	currentScreen = newdesk;

	HDWP dwp = BeginDeferWindowPos(listlen(vwm_WL));
	winlist *wl;
	dolist (wl, vwm_WL)
	{
		if (wl->sticky || gather)
			wl->desk = newdesk;
		else
		if (wl->desk >= Settings_workspaces)
			wl->desk = Settings_workspaces-1;

		int left = wl->rect.left;
		int top = wl->rect.top;
		int width = wl->rect.right - left;
		int height = wl->rect.bottom - top;

		if (height == 0 || width == 0)
			continue;

		if (wl->iconic && false == vwm_alt_method)
		{
			if (wl->moved && newdesk == wl->desk)
			{
				// The window was iconized while on other WS
				fix_iconized_window(wl);
				wl->moved = false;
			}
			continue;
		}

		wl->moved = newdesk != wl->desk;
		UINT flags;

		if (false == vwm_alt_method) // windows on other WS's are hidden
		{
			if (wl->moved)
			{
				int dx = VScreenWidth + SCREEN_DIST;
				if (vwm_xpfix) left += dx * 2;
				else left += dx * (wl->desk - newdesk);
			}
			else
			{
				center_window(&left, &top, width, height);
			}
			flags = SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE;
		}
		else // windows on other WS's are moved
		{
			if (wl->moved)
				flags = SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_HIDEWINDOW;
			else
				flags = SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE|SWP_SHOWWINDOW;
		}
		dwp = DeferWindowPos(dwp, wl->hwnd, NULL, left, top, width, height, flags);
	}
	EndDeferWindowPosAsync(dwp);
	Sleep(1);
}

//=========================================================

ST bool set_location_helper(HWND hwnd, struct taskinfo *t, UINT flags)
{
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
	if (NULL == wl) return false;

	int addH = t->xpos - wl->rect.left;
	int addV = t->ypos - wl->rect.top;
	int new_desk = t->desk;

	check_owned_windows(hwnd);
	dolist (wl, vwm_WL)
	{
		if (wl->check)
		{
			if (flags & BBTI_SETDESK)
				wl->desk = new_desk;

			if (flags & BBTI_SETPOS)
				_InflateRect(&wl->rect, addH, addV);
		}
	}

	defer_windows(flags & BBTI_SWITCHTO ? new_desk : currentScreen);

	if (flags & BBTI_SETPOS && vwm_alt_method) dolist (wl, vwm_WL)
	{
		if (wl->check && false == wl->iconic)
			SetWindowPos(wl->hwnd, NULL,
				wl->rect.left,
				wl->rect.top,
				wl->rect.right - wl->rect.left,
				wl->rect.bottom - wl->rect.top,
				SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
				);
	}

	return true;
}

//=========================================================
//              set workspace
//=========================================================

void vwm_switch_desk(int newdesk)
{
	vwm_update_winlist();
	defer_windows(newdesk);
}

void vwm_gather(void)
{
	vwm_switch_desk(-1);
	if (vwm_alt_method)
	{
		vwm_alt_method = false;
		vwm_switch_desk(-1);
		vwm_alt_method = true;
	}
}

//=========================================================
//               Set window properties
//=========================================================

bool vwm_set_desk(HWND hwnd, int new_desk, bool switchto)
{
	vwm_update_winlist();
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
	if (wl)
	{
		check_owned_windows(hwnd);
		dolist (wl, vwm_WL) if (wl->check) wl->desk = new_desk;
	}
	defer_windows(switchto ? new_desk : currentScreen);
	return NULL != wl;
}

bool vwm_set_location(HWND hwnd, struct taskinfo *t, UINT flags)
{
	vwm_update_winlist();
	return set_location_helper(hwnd, t, flags);
}

bool vwm_set_workspace(HWND hwnd, int new_desk)
{
	RECT r; GetWindowRect(hwnd, &r);
	center_window((int*)&r.left, (int*)&r.top, r.right - r.left, r.bottom - r.top);
	struct taskinfo t;
	t.desk  = new_desk;
	t.xpos  = r.left;
	t.ypos  = r.top;
	return set_location_helper(hwnd, &t, BBTI_SETDESK|BBTI_SETPOS);
}

bool vwm_make_sticky(HWND hwnd, bool sticky)
{
	vwm_update_winlist();
	check_owned_windows(hwnd);
	winlist *wl; bool r = false;
	dolist (wl, vwm_WL) if (wl->check) wl->sticky = sticky, r = true;
	return r;
}

//=========================================================
//                  retrieve infos
//=========================================================

int vwm_get_desk(HWND hwnd)
{
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
	return wl ? wl->desk : currentScreen;
}

bool vwm_get_location(HWND hwnd, struct taskinfo *t)
{
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
	int desk; RECT r, *rp;

	if (wl && (wl->moved || wl->iconic))
	{
		desk = wl->desk;
		rp = &wl->rect;
	}
	else
	{
		rp = &r;
		if (FALSE == GetWindowRect(hwnd, rp))
			return false;
		desk = currentScreen;
	}
	t->xpos = rp->left;
	t->ypos = rp->top;
	t->width = rp->right - rp->left;
	t->height = rp->bottom - rp->top;
	t->desk = desk;
	return true;
}

int vwm_get_status(HWND hwnd)
{
	int flags = 0;
	winlist *wl = (winlist*)assoc(vwm_WL, hwnd);
	if (wl)
	{
		flags|=VWM_KNOWN;
		if (wl->moved) flags|=VWM_MOVED;
		if (wl->hidden) flags|=VWM_HIDDEN;
		if (wl->iconic) flags|=VWM_ICONIC;
		if (wl->sticky) flags|=VWM_STICKY;
	}
	return flags;
}

//=========================================================
//              Init/exit
//=========================================================

void vwm_init(bool alt_method, bool xpfix)
{
	vwm_alt_method = alt_method;
	vwm_xpfix = xpfix;
	vwm_update_winlist();
}

void vwm_exit(void)
{
	freeall(&vwm_WL);
}

void vwm_reconfig(bool alt_method, bool xpfix)
{
	winlist *wl;
	bool defer = currentScreen >= Settings_workspaces;
	if (vwm_alt_method != alt_method || vwm_xpfix != xpfix)
	{
		dolist (wl, vwm_WL) wl->save_desk = wl->desk;
		vwm_switch_desk(-1);
		dolist (wl, vwm_WL) wl->desk = wl->save_desk;
		vwm_xpfix = xpfix;
		vwm_alt_method = alt_method;
		defer = true;
	}
	vwm_update_winlist();
	dolist (wl, vwm_WL)
	{
		bool sticky = check_sticky_name(wl->hwnd);
		if (wl->sticky != sticky || wl->desk >= Settings_workspaces)
		{ wl->sticky = sticky, defer = true; }
	}
	if (defer) defer_windows(currentScreen);
}

//=========================================================
