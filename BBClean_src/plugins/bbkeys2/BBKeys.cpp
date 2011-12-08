/*
 ============================================================================
 This file is part of the Blackbox for Windows source code
 Copyright © 2001-2003 The Blackbox for Windows Development Team
 http://desktopian.org/bb/ - #bb4win on irc.freenode.net
 ============================================================================

  Blackbox for Windows is free software, released under the
  GNU General Public License (GPL version 2 or later), with an extension
  that allows linking of proprietary modules under a controlled interface.
  What this means is that plugins etc. are allowed to be released
  under any license the author wishes. Please note, however, that the
  original Blackbox gradient math code used in Blackbox for Windows
  is available under the BSD license.

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface
  http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  For additional license information, please read the included license.html

 ============================================================================
*/

#include "../../blackbox/BBApi.h"
#include "BBKeys.h"

//===========================================================================

const char szKeysName[] = "BBKeys";
LPSTR szVersion = "1.64b4";

BBKeys *pKeys;

LPSTR szInfoVersion = "1.64b4";
LPSTR szInfoAuthor = "Blackbox for Windows Dev Team";
LPSTR szInfoRelDate = "2004-01-13";
LPSTR szInfoLink = "http://www.bb4win.org";
LPSTR szInfoEmail = "";

LPCSTR pluginInfo(int field)
{
	// pluginInfo is used by Blackbox for Windows to fetch information about
	// a particular plugin. At the moment this information is simply displayed
	// in an "About loaded plugins" MessageBox, but later on this will be
	// expanded into a more advanced plugin handling system. Stay tuned! :)

	switch (field)
	{
		case 1:
			return szKeysName; // Plugin name
		case 2:
			return szInfoVersion; // Plugin version
		case 3:
			return szInfoAuthor; // Author
		case 4:
			return szInfoRelDate; // Release date, preferably in yyyy-mm-dd format
		case 5:
			return szInfoLink; // Link to author's website
		case 6:
			return szInfoEmail; // Author's email

		// ==========

		default:
			return szVersion; // Fallback: Plugin name + version, e.g. "MyPlugin 1.0"
	}
}

//===========================================================================

static const VKTable vkTable[] =
{
	{"LMOUSE", VK_LBUTTON},
	{"RMOUSE", VK_RBUTTON},
	{"MMOUSE", VK_MBUTTON},
	{"F1", VK_F1},
	{"F2", VK_F2},
	{"F3", VK_F3},
	{"F4", VK_F4},
	{"F5", VK_F5},
	{"F6", VK_F6},
	{"F7", VK_F7},
	{"F8", VK_F8},
	{"F9", VK_F9},
	{"F10", VK_F10},
	{"F11", VK_F11},
	{"F12", VK_F12},
	{"PrtScn", VK_SNAPSHOT},
	{"Pause", VK_PAUSE},
	{"Insert", VK_INSERT},
	{"Delete", VK_DELETE},
	{"Home", VK_HOME},
	{"End", VK_END},
	{"PageUp", VK_PRIOR},
	{"PageDown", VK_NEXT},
	{"Left", VK_LEFT},
	{"Right", VK_RIGHT},
	{"Down", VK_DOWN},
	{"Up", VK_UP},
	{"Tab", VK_TAB},
	{"Backspace", VK_BACK},
	{"Spacebar", VK_SPACE},
	{"Apps", VK_APPS},
	{"Enter", VK_RETURN},
	{"Num0", VK_NUMPAD0},
	{"Num1", VK_NUMPAD1},
	{"Num2", VK_NUMPAD2},
	{"Num3", VK_NUMPAD3},
	{"Num4", VK_NUMPAD4},
	{"Num5", VK_NUMPAD5},
	{"Num6", VK_NUMPAD6},
	{"Num7", VK_NUMPAD7},
	{"Num8", VK_NUMPAD8},
	{"Num9", VK_NUMPAD9},
	{"Mul", VK_MULTIPLY},
	{"Div", VK_DIVIDE},
	{"Add", VK_ADD},
	{"Sub", VK_SUBTRACT},
	{"Dec", VK_DECIMAL},
	{"Escape", VK_ESCAPE}
};

#define MAX_VKEYS (sizeof(vkTable) / sizeof(VKTable))

//===========================================================================

int beginPlugin(HINSTANCE hPluginInstance)
{
	pKeys = new BBKeys(hPluginInstance);
	return 0;
}

void endPlugin(HINSTANCE hPluginInstance)
{
	delete pKeys;
}

//===========================================================================

BBKeys::BBKeys(HINSTANCE hPluginInstance)
{
	// saving our window
	m_hMainWnd = GetBBWnd();
	// saving our instance
	m_hMainInstance = hPluginInstance;

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = HotkeyProc;			// our window procedure
	wc.hInstance = m_hMainInstance;		
	wc.lpszClassName = szKeysName;			// our window class name

	if (!RegisterClass(&wc))
	{
		MessageBox(0, "Error registering window class", szKeysName, MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		Log("BBKeys: Error registering window class", NULL);
	}
	
	hKeysWnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,					// exstyles
		szKeysName,							// our window class name
		NULL,								// use description for a window title
		0,
		0, 0,								// position
		0, 0,								// width & height of window
		m_hMainWnd,							// parent window
		NULL,								// no menu
		m_hMainInstance,					// hInstance of DLL
		NULL);

	if (!hKeysWnd)
	{
		MessageBox(0, "Error creating window", szKeysName, MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
		Log("BBKeys: Error creating window", NULL);
	}

	int msgs[] = {BB_RECONFIGURE, 0};
	SendMessage(GetBBWnd(), BB_REGISTERMESSAGE, (WPARAM)hKeysWnd, (LPARAM)msgs);
	LoadHotkeys();
}


BBKeys::~BBKeys()
{
	FreeHotkeys();
	int msgs[] = {BB_RECONFIGURE, 0};
	SendMessage(GetBBWnd(), BB_UNREGISTERMESSAGE, (WPARAM)hKeysWnd, (LPARAM)msgs);
	DestroyWindow(hKeysWnd);
	UnregisterClass(szKeysName, m_hMainInstance);
}

//===========================================================================

void BBKeys::LoadHotkeys()
{
	char bbkeysRCPath[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH], path[MAX_LINE_LENGTH];
	int nLen;
	// First we look for the config file in the same folder as the plugin...
	GetModuleFileName(m_hMainInstance, bbkeysRCPath, sizeof(bbkeysRCPath));
	nLen = strlen(bbkeysRCPath) - 1;
	while (nLen >0 && bbkeysRCPath[nLen] != '\\') nLen--;
	bbkeysRCPath[nLen + 1] = 0;
	strcpy(temp, bbkeysRCPath);
	strcpy(path, bbkeysRCPath);
	strcat(temp, "bbkeys.rc");
	strcat(path, "bbkeysrc");
	if (FileExists(temp)) strcpy(bbkeysRCPath, temp);
	else if (FileExists(path)) strcpy(bbkeysRCPath, path);
	// ...if not found, we try the Blackbox directory...
	else
	{
		GetBlackboxPath(bbkeysRCPath, sizeof(bbkeysRCPath));
		strcpy(temp, bbkeysRCPath);
		strcpy(path, bbkeysRCPath);
		strcat(temp, "bbkeys.rc");
		strcat(path, "bbkeysrc");
		if (FileExists(temp)) strcpy(bbkeysRCPath, temp);
		else if (FileExists(path)) strcpy(bbkeysRCPath, path);
	}

	FILE* f = FileOpen(bbkeysRCPath);
	if (f)
	{
		char	buffer[4096];
		char	keytograb[4096]="", modifier[4096]="", action[4096]="", command[4096] = "";
		char*	tokens[3];
		
		tokens[0] = keytograb;
		tokens[1] = modifier;
		tokens[2] = action;
		// KeyToGrab(), WithModifier(), WithAction(), DoThis()
		while (ReadNextCommand(f, buffer, sizeof (buffer)))
		{
			int count, i;
			
			keytograb[0] = modifier[0] = action[0] = command[0] = '\0';
			count = BBTokenize (buffer, tokens, 3, command);

			memmove(keytograb, keytograb+10, strlen(keytograb));
			keytograb[strlen(keytograb)-2] = '\0';
			memmove(modifier, modifier+13, strlen(modifier));
			modifier[strlen(modifier)-2] = '\0';
			memmove(action, action+11, strlen(action));
			if (strlen(command))
			{
				action[strlen(action)-2] = '\0';
				memmove(command, command+7, strlen(command));
				command[strlen(command)-1] = '\0';
			}
			else 
				action[strlen(action)-1] = '\0';
			char *tmp;
			HotkeyType tempHotkey;
			
			tempHotkey.sub = 0;
			tmp = strtok(modifier, "+");
			
			while (tmp)
			{
				if (!stricmp(tmp, "Win"))
					tempHotkey.sub |= MOD_WIN;
				if (!stricmp(tmp, "Alt"))
					tempHotkey.sub |= MOD_ALT;
				if (!stricmp(tmp, "Ctrl"))
					tempHotkey.sub |= MOD_CONTROL;
				if (!stricmp(tmp, "Shift"))
					tempHotkey.sub |= MOD_SHIFT;
				tmp = strtok(NULL, "+");
			}
			
			if (strlen(keytograb) == 1)
				tempHotkey.ch = strupr(keytograb)[0];
			else
			{
				for (i = 0; i < MAX_VKEYS; i++)
				{
					if (!stricmp(vkTable[i].key, keytograb))
					{
						tempHotkey.ch = vkTable[i].vKey;
						break;
					}
				}
			}
			tempHotkey.szAction = action;
			if (strlen(command))
				tempHotkey.szCommand = command;
			hotKeys.push_back(tempHotkey);
			RegisterHotKey(hKeysWnd, hotKeys.size(), tempHotkey.sub, tempHotkey.ch);
		}
	}
	FileClose(f);
}

//===========================================================================

void BBKeys::FreeHotkeys()
{
	for (int i = 0; i < hotKeys.size(); i++)
		UnregisterHotKey(hKeysWnd,i);
	hotKeys.clear();
}

//===========================================================================

LRESULT CALLBACK HotkeyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == BB_RECONFIGURE)
	{
		pKeys->FreeHotkeys();
		pKeys->LoadHotkeys();
		return 0;
	}

	if (message == WM_HOTKEY)
	{
		int num = wParam - 1;
		//POINT p;
		//GetCursorPos(&p);
		HWND oWnd = GetForegroundWindow(); //WindowFromPoint(p);
		while (GetParent(oWnd))
			oWnd = GetParent(oWnd);
		while (GetWindow(oWnd, GW_OWNER))
			oWnd = GetWindow(oWnd, GW_OWNER);
		oWnd = GetLastActivePopup(oWnd);
		
		if (num < pKeys->hotKeys.size())
		{
			for (;;)
			{
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Minimize"))
				{
					if (!IsAppWindow(oWnd)) break;
					PostMessage(oWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Minimize");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "MaximizeWindow") || !stricmp(pKeys->hotKeys[num].szAction.c_str(), "Maximize")) {
					if (!IsAppWindow(oWnd)) break;
					SendMessage(oWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> MaximizeWindow");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "MaximizeVertical")) {
					if (!IsAppWindow(oWnd)) break;
					SendMessage(GetBBWnd(), BB_WINDOWGROWHEIGHT,(WPARAM)oWnd, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> MaximizeVertical");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "MaximizeHorizontal")) {
					if (!IsAppWindow(oWnd)) break;
					SendMessage(GetBBWnd(), BB_WINDOWGROWWIDTH, (WPARAM)oWnd, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> MaximizeHorizontal");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ToggleMaximize")) {
					if (!IsAppWindow(oWnd)) break;
					if (IsZoomed(oWnd))
					{
						SendMessage(oWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
						SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Restore Window");
					}
					else
					{
						SendMessage(oWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
						SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Maximize Window");
					}
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Restore"))
				{
					if (!IsAppWindow(oWnd)) break;
					PostMessage(oWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Restore");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Resize"))
				{
					if (!IsAppWindow(oWnd)) break;
					PostMessage(oWnd, WM_SYSCOMMAND, SC_SIZE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Resize");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Move"))
				{
					if (!IsAppWindow(oWnd)) break;
					PostMessage(oWnd, WM_SYSCOMMAND, SC_MOVE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Move");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Raise"))
				{
					/*if (!IsAppWindow(oWnd)) break;
					HWND hCurrWnd;
					int iMyTID;
					int iCurrTID;
					hCurrWnd = GetForegroundWindow();
					iCurrTID = GetWindowThreadProcessId(hCurrWnd, 0);
					iMyTID = GetCurrentThreadId();
					// Connect to the current process...
					// Now we look like the foreground process, we can set the foreground window...
					SetForegroundWindow(oWnd);
					// Now detach...
					AttachThreadInput (iMyTID, iCurrTID, FALSE);*/

					if (!IsAppWindow(oWnd)) break;					
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Raise");
					SendMessage(GetBBWnd(), BB_WINDOWRAISE, 0, 0);
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Lower")) {
					if (!IsAppWindow(oWnd)) break;
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Lower");
					SendMessage(GetBBWnd(), BB_WINDOWLOWER, /*(WPARAM)oWnd*/0, 0);
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Close")) {
					if (!IsAppWindow(oWnd)) break;
					PostMessage(oWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Close");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace1")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace1");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace2")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 1);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace2");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace3")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 2);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace3");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace4")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 3);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace4");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace5")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 4);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace5");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace6")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 5);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace6");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace7")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 6);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace7");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace8")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 7);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace8");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace9")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 8);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace9");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace10")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 9);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace10");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace11")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 10);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace11");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Workspace12")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 4, 11);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Workspace12");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "MoveWindowLeft")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 6, (LPARAM)GetForegroundWindow());
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> MoveWindowLeft");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "MoveWindowRight")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 7, (LPARAM)GetForegroundWindow());
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> MoveWindowRight");
					break;
				}
				// ================================
				// NC-17: Added 0.0.80 functionality to cycle through windows 
				//		  on current workspace only or all workspaces.
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "PrevWindow")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 8, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> PrevWindow");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "NextWindow")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 9, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> NextWindow");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "PrevWindowAllWorkspaces")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 8, 1);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> PrevWindowAll");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "NextWindowAllWorkspaces")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 9, 1);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> NextWindowAll");
					break;
				}
				// ==================================
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "LeftWorkspace")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> LeftWorkspace");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "RightWorkspace")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 1, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> RightWorkspace");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "PrevWorkspace")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> PrevWorkspace");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "NextWorkspace")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 1, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> NextWorkspace");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "LastWorkspace")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 10, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> LastWorkspace");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "GatherWindows")) {
					SendMessage(GetBBWnd(), BB_WORKSPACE, 5, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> GatherWindows");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ShadeWindow")) {
					if (!IsAppWindow(oWnd)) break;
					SendMessage(GetBBWnd(), BB_WINDOWSHADE, (WPARAM)oWnd, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> ShadeWindow");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "StickWindow")) {

					// NOTE: We can not use IsAppWindow here because that
					//       function checks the magicDWord. However, the code
					//       below is essentially the same as in IsAppWindow. /qwilk

					LONG nStyle;
					// Make sure the window isn't a WS_CHILD or not WS_VISIBLE...
					nStyle = GetWindowLong(oWnd, GWL_STYLE);	
					if((nStyle & WS_CHILD) || !(nStyle & WS_VISIBLE)) break;
					// ...or a WS_EX_TOOLWINDOW...
					nStyle = GetWindowLong(oWnd, GWL_EXSTYLE);
					if(nStyle & WS_EX_TOOLWINDOW) break;
					// If the window has an owner, then accept it only
					// if the parent isn't accepted...
					HWND hOwner = GetWindow(oWnd, GW_OWNER);
					if(hOwner && IsAppWindow(hOwner)) break;
					// Finally, make sure the window does not have a parent...
					if(GetParent(oWnd)) break;

					if (CheckSticky(oWnd))
						RemoveSticky(oWnd);
					else
						MakeSticky(oWnd);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> StickWindow");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Restart")) {
					PostMessage(GetBBWnd(), BB_RESTART, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Restart");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Reconfigure")) {
					// When Blackbox receives a null BB_SETSTYLE message it will refresh
					// the current style's settings and then broadcast BB_RECONFIGURE to
					// make all GUI elements update themselves
					PostMessage(GetBBWnd(), BB_SETSTYLE, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Reconfigure");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Quit")) {
					PostMessage(GetBBWnd(), BB_QUIT, 0, 0);
					//SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Quit");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "EditStyle")) {
					PostMessage(GetBBWnd(), BB_EDITFILE, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> EditStyle");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "EditMenu")) {
					PostMessage(GetBBWnd(), BB_EDITFILE, 1, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> EditMenu");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "EditPlugins")) {
					PostMessage(GetBBWnd(), BB_EDITFILE, 2, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> EditPlugins");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "EditExtensions")) {
					PostMessage(GetBBWnd(), BB_EDITFILE, 3, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> EditExtensions");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "EditBlackbox")) {
					PostMessage(GetBBWnd(), BB_EDITFILE, 4, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> EditBlackbox");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "AboutStyle")) {
					char stylepath[MAX_LINE_LENGTH], temp[MAX_LINE_LENGTH], a1[MAX_LINE_LENGTH], a2[MAX_LINE_LENGTH], a3[MAX_LINE_LENGTH], a4[MAX_LINE_LENGTH], a5[MAX_LINE_LENGTH];
					strcpy(stylepath, stylePath());
					strcpy(a1, ReadString(stylepath, "style.name:", "[Style name not specified]"));
					strcpy(a2, ReadString(stylepath, "style.author:", "[Author not specified]"));
					strcpy(a3, ReadString(stylepath, "style.date:", ""));
					strcpy(a4, ReadString(stylepath, "style.credits:", ""));
					strcpy(a5, ReadString(stylepath, "style.comments:", ""));
					sprintf(temp, "%s\nby %s\n%s\n%s\n%s", a1, a2, a3, a4, a5);
					MessageBox(GetBBWnd(), temp, "Blackbox for Windows: Style information", MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> AboutStyle");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ShowMenu")) {
					PostMessage(GetBBWnd(), BB_MENU, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> ShowMenu");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ShowWorkspaceMenu")) {
					PostMessage(GetBBWnd(), BB_MENU, 1, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> ShowWorkspaceMenu");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ToggleTray")) {
					PostMessage(GetBBWnd(), BB_TOGGLETRAY, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> ToggleTray");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "TogglePlugins")) {
					PostMessage(GetBBWnd(), BB_TOGGLEPLUGINS, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> TogglePlugins");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Shutdown")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Shutdown");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Reboot")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 1, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Reboot");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Logoff")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 2, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Logoff");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Hibernate")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 3, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Hibernate");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Suspend")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 4, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Suspend");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "LockWorkstation")) {
					PostMessage(GetBBWnd(), BB_SHUTDOWN, 5, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> LockWorkstation");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "Run")) {
					SendMessage(GetBBWnd(), BB_RUN, 0, 0);
					SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"BBKeys -> Run");
					break;
				}
				if (!stricmp(pKeys->hotKeys[num].szAction.c_str(), "ExecCommand")) {
					if (pKeys->hotKeys[num].szCommand.length())
					{
						if (!strncmp(pKeys->hotKeys[num].szCommand.c_str(), "@", 1))
						{
							SendMessage(GetBBWnd(), BB_BROADCAST, 0, (LPARAM)pKeys->hotKeys[num].szCommand.c_str());
							char command[MAX_LINE_LENGTH];
							strcpy(command, "BBKeys -> ExecCommand ");
							strcat(command, pKeys->hotKeys[num].szCommand.c_str());
							SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)command);
							break;
						}
						else
						{
							char command[MAX_LINE_LENGTH], arguments[MAX_LINE_LENGTH], workingdir[MAX_LINE_LENGTH];
							LPSTR tokens[1];
							tokens[0] = command;
							command[0] = arguments[0] = workingdir[0] = '\0';
							BBTokenize (pKeys->hotKeys[num].szCommand.c_str(), tokens, 1, arguments);
							if (command[1] == ':')
							{
								strcpy(workingdir, command);
								int nLen = strlen(workingdir) - 1;
								while (nLen >0 && workingdir[nLen] != '\\') nLen--;
								workingdir[nLen + 1] = 0;
							}

							//##### DEBUG TOOL <g> #####
							//char tempmsg[MAX_LINE_LENGTH];
							//sprintf(tempmsg, "Command:\n%s\n\nArgument:\n%s\n\nWorking directory:\n%s", command, arguments, workingdir);
							//MessageBox(0, tempmsg, "BBKeys", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
							// #########################

							BBExecute(GetDesktopWindow(), NULL, command, arguments, workingdir, SW_SHOWNORMAL, false);
							
							strcpy(command, "BBKeys -> ExecCommand ");
							strcat(command, pKeys->hotKeys[num].szCommand.c_str());
							SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)command);
						}
					}
					break;
				}
				SendMessage(GetBBWnd(), BB_SETTOOLBARLABEL, 0, (LPARAM)"*** BBKeys: Invalid hotkey command ***");
				break;
			}
		}
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//===========================================================================
