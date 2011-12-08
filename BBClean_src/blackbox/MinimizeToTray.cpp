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
#include "Tinylist.h"
#include "MinimizeToTray.h"

#define ST static

ST nid_list *nidl;
ST hwnd_list *hdnwl;

///////////////////////////////////////////////
// hide hOwner itself or hOwner-owned window
ST BOOL CALLBACK EnumHideWindowsProc(HWND hWnd, LPARAM lParam){
	HWND hOwner = (HWND)lParam;
	if (hOwner == hWnd || hOwner == GetWindow(hWnd, GW_OWNER)){
		if (ShowWindow(hWnd, SW_HIDE))
			cons_node(&hdnwl, new_node(hWnd));
	}
	return TRUE;
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// re-show hidden windows
// ShowWindow -> AddTask@Workspaces.cpp -> RemoveFromTray
ST void reshow_windows(HWND hWnd){
	hwnd_list *p;
	dolist(p, hdnwl){
		if (hWnd == p->hwnd || hWnd == GetWindow(p->hwnd, GW_OWNER)){
			ShowWindow(p->hwnd, SW_SHOW);
			// activate window
			SendMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)p->hwnd);
		}
	}
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// remove system-tray icon, and clean-up hdnwl
ST void remove_tray_icon(nid_list *nl){
	// remove node from nidl, remove icon from tray
	Shell_NotifyIcon(NIM_DELETE, &nl->NID);
	// clean-up hidden windows list
	for (hwnd_list *n, *p = (hwnd_list*)&hdnwl; n = p->next;){
		if (nl->hWnd == n->hwnd || nl->hWnd == GetWindow(n->hwnd, GW_OWNER)){
			p->next = n->next, m_free(n);
		}
		else p = n;
	}
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// add icon to system-tray,
// hide hWnd itself and hWnd-owned windows
bool MinimizeToTray(HWND hWnd){
	// create NOTIFYICONDATA node
	nid_list *nl = (nid_list*)c_alloc(sizeof(nid_list));
	nl->hWnd = hWnd;
	nl->NID.cbSize = sizeof(NOTIFYICONDATA);
	nl->NID.hWnd = BBhwnd;
	nl->NID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nl->NID.uCallbackMessage = WM_TRAYICONMESSAGE;
	nl->NID.hIcon = GetIconFromHWND(nl->hWnd);
	GetWindowText(nl->hWnd, nl->NID.szTip, sizeof(nl->NID.szTip));

	// add icon to tray
	for (nl->NID.uID = (UINT)-1; nl->NID.uID; nl->NID.uID--){
		if (Shell_NotifyIcon(NIM_ADD, &nl->NID)){
			EnumWindows(EnumHideWindowsProc, (LPARAM)nl->hWnd);
			// append node to nidl
			cons_node(&nidl, nl);
			return true;
		}
	}

	return false;
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// restore from system-tray.
// it is called by WM_TRAYICONMESSAGE.
HWND RestoreFromTray(UINT uID, bool bShow){
	HWND hWnd = NULL;
	for (nid_list *n, *p = (nid_list*)&nidl; n = p->next;){
		if(n->NID.uID == uID){
			hWnd = n->hWnd;
			// Restore Window
			if (bShow) reshow_windows(n->hWnd);
			remove_tray_icon(n);
			p->next = n->next, m_free(n);
			break;
		}
		else p = n;
	}
	return hWnd;
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// remove system-tray icon.
// it is called from AddTask@Workspaces.cpp
bool RemoveFromTray(HWND hWnd){
	for (nid_list *n, *p = (nid_list*)&nidl; n = p->next;){
		if(n->hWnd == hWnd){
			remove_tray_icon(n);
			p->next = n->next, m_free(n);
			return true;
		}
		else p = n;
	}
	return false;
}
///////////////////////////////////////////////

///////////////////////////////////////////////
// free nidl, hdnwl.
// and re-show all hidden windows.
void FreeNIDList(){
	// restore hidden window before free list
	for (hwnd_list *n, *p = (hwnd_list*)&hdnwl; n = p->next;){
		ShowWindow(n->hwnd, SW_SHOW);
		p->next = n->next, m_free(n);
	}
	freeall(&nidl);
}
///////////////////////////////////////////////

