//===========================================================================
// Function: ShowSysmenu
//===========================================================================

#define COPY_MENU

#ifdef COPY_MENU
#define MENUITEMINFO_SIZE_0400 ((int)&((MENUITEMINFO*)NULL)->cch + sizeof ((MENUITEMINFO*)NULL)->cch)
static Menu *copymenu (HWND hwnd, HMENU hm, const char *title)
{
	Menu *m = MakeMenu(title);
	int c = GetMenuItemCount (hm);
	int n;
	for (n = 0; n<c; n++)
	{
		MENUITEMINFO info;
		ZeroMemory(&info,sizeof(info));
		info.cbSize = MENUITEMINFO_SIZE_0400; // to make this work on win95
		info.fMask  = MIIM_DATA|MIIM_ID|MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE;
		GetMenuItemInfo (hm, n, TRUE, &info);

		char item_string[128];
		GetMenuString(hm, n, item_string, sizeof item_string, MF_BYPOSITION);
		//dbg_printf("string: <%s>", item_string);
		char *p = item_string; while (*p) { if ('\t' == *p) *p = ' '; p++; }


		if (info.hSubMenu)
		{
			SendMessage(hwnd, WM_INITMENUPOPUP, (WPARAM)info.hSubMenu, MAKELPARAM(n, TRUE));
			Menu *s = copymenu(hwnd, info.hSubMenu, item_string);
			MakeSubmenu(m, s, item_string);
		}
		else
		if (info.fType & MFT_SEPARATOR)
		{
			MakeMenuNOP(m, NULL);
		}
		else
		if (!(info.fState & MFS_DISABLED) && !(info.fState & MFS_GRAYED))
		{
			char broam[256];
			sprintf(broam, "@BBLeanbar.SysCommand <%d> <%d>", (int)hwnd, info.wID);
			MakeMenuItem(m, item_string, broam, false);
		}
	}
	return m;
}
#endif

//===========================================================================

//===========================================================================

void ShowSysmenu(HWND Window, HWND Owner)
{
	BOOL iconic = IsIconic(Window);
	BOOL zoomed = IsZoomed(Window);
	LONG style = GetWindowLong(Window, GWL_STYLE);

	HMENU systemMenu = GetSystemMenu(Window, FALSE);
	if (NULL == GetSystemMenu) return;

	// restore is enabled only if minimized or maximized (not normal)
	EnableMenuItem(systemMenu, SC_RESTORE, MF_BYCOMMAND |
		(iconic || zoomed ? MF_ENABLED : MF_GRAYED));

	// move is enabled only if normal
	EnableMenuItem(systemMenu, SC_MOVE, MF_BYCOMMAND |
		(!(iconic || zoomed) ? MF_ENABLED : MF_GRAYED));

	// size is enabled only if normal
	EnableMenuItem(systemMenu, SC_SIZE, MF_BYCOMMAND |
		(!(iconic || zoomed) && (style & WS_SIZEBOX) ? MF_ENABLED : MF_GRAYED));

	// minimize is enabled only if not minimized
	EnableMenuItem(systemMenu, SC_MINIMIZE, MF_BYCOMMAND |
		(!iconic && (style & WS_MINIMIZEBOX)? MF_ENABLED : MF_GRAYED));

	// maximize is enabled only if not maximized
	EnableMenuItem(systemMenu, SC_MAXIMIZE, MF_BYCOMMAND |
		(!zoomed && (style & WS_MAXIMIZEBOX) ? MF_ENABLED : MF_GRAYED));

	// let application modify menu
	SendMessage(Window, WM_INITMENU, (WPARAM)systemMenu, 0);

#ifndef COPY_MENU
	SendMessage(Window, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, TRUE));

	SetActiveWindow(Owner);
	if (!iconic)
	{
		SendMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)Window);
		DWORD ThreadID1 = GetWindowThreadProcessId(Window, NULL);
		DWORD ThreadID2 = GetCurrentThreadId();
		AttachThreadInput(ThreadID1, ThreadID2, TRUE);
		SetActiveWindow(Owner);
		AttachThreadInput(ThreadID1, ThreadID2, FALSE);
	}

	// display the menu
	POINT pt; GetCursorPos(&pt);
	int command =
		TrackPopupMenu(
			systemMenu,
			TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_CENTERALIGN,
			pt.x, pt.y,
			0,
			Owner,
			NULL
			);

	if (command)
	{
		if (!iconic && command != SC_MINIMIZE)
			SendMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)Window);

		PostMessage(Window, WM_SYSCOMMAND, (WPARAM)command, 0);
	}

#else
	SetActiveWindow(Owner);
	if (!iconic) SendMessage(BBhwnd, BB_BRINGTOFRONT, 0, (LPARAM)Window);

	char buff[36];
	int i = GetWindowText(Window, buff, sizeof buff);
	if (i >= (int)sizeof buff - 1) memset(buff + sizeof buff - 4, '.', 3);
	Menu *m = copymenu(Window, systemMenu, buff);
	ShowMenu(m);
#endif
}

//===========================================================================
