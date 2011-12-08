/*
 ============================================================================

  This file is part of the Blackbox for Windows source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team

  http://bb4win.org
  http://sourceforge.net/projects/bb4win

 ============================================================================

  Blackbox for Windows is free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. Please note, however, that the original Blackbox gradient math
  code used in Blackbox for Windows is available under the BSD license.

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface
  http://www.xfree86.org/3.3.6/COPYRIGHT2.html#5

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

/**********************
		ToDo
1. Copy kbdHook for mouseHook
2. Send message for mouse events
3. Parse for all mouse messages
4. Done?
**********************/

#include "BBApi.h"
#include "m_alloc.h"
#include "Tinylist.cpp"

const char szVersion     [] = "bbKeys 1.16";
const char szAppName     [] = "bbKeys";
const char szInfoVersion [] = "1.16";
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = "2005-05-02";
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";

//===========================================================================
extern "C"
{
	DLL_EXPORT int beginPlugin(HINSTANCE hMainInstance);
	DLL_EXPORT void endPlugin(HINSTANCE hMainInstance);
	DLL_EXPORT LPCSTR pluginInfo(int field);
}

LPCSTR pluginInfo(int field)
{
	switch (field)
	{
		default:
		case 0: return szVersion;
		case 1: return szAppName;
		case 2: return szInfoVersion;
		case 3: return szInfoAuthor;
		case 4: return szInfoRelDate;
		case 5: return szInfoLink;
		case 6: return szInfoEmail;
	}
}

//===========================================================================

HWND hKeysWnd;
HWND BBhwnd;
HINSTANCE m_hMainInstance;
bool nolabel;
bool usingNT;
bool hook_is_set;

struct HotkeyType
{
	struct HotkeyType *next;
	unsigned ch, sub, is_ExecCommand;
	char *szAction;

} *m_hotKeys;

//===========================================================================

const char* stristr(const char *aa, const char *bb)
{
	do {
		const char *a, *b; char c, d;
		for (a = aa, b = bb;;++a, ++b)
		{
			if (0 == (c = *b)) return aa;
			if (0 != (d = *a^c))
				if (d != 32 || (c |= 32) < 'a' || c > 'z')
					break;
		}
	} while (*aa++);
	return NULL;
}

//===========================================================================

void set_kbdhook(bool set)
{
	static HINSTANCE hdll;
	bool (*SetKbdHook)(bool set);

	if (hook_is_set == set) return;
	hook_is_set = set;

	if (false == usingNT) return;

	if (set && NULL == hdll)
	{
		char path[MAX_PATH]; int nLen;
		GetModuleFileName(m_hMainInstance, path, sizeof(path));
		nLen = strlen(path);
		while (nLen && path[nLen-1] != '\\') nLen--;
		strcpy(path+nLen, "winkeyhook.dll");
		hdll = LoadLibrary(path);
	}

	if (hdll)
	{
		*(FARPROC*)&SetKbdHook = GetProcAddress(hdll, "SetKbdHook");

		if (SetKbdHook && SetKbdHook(set) && set)
			return;

		FreeLibrary(hdll);
		hdll = NULL;
	}
}

//===========================================================================

int getvalue(char *from, char *token, char *to, bool from_right)
{
	const char *p, *q; int l=0;
	if (NULL!=(p=stristr(from, token)))
	{
		p += strlen(token);
		while (' ' == *p) ++p;
		if ('(' == *p && NULL != (q = (from_right?strrchr:strchr)(++p,')')))
			memcpy(to, p, l = q-p);
	}
	to[l]=0;
	return l;
}

void BBKeys_LoadHotkeys(HWND hwnd) {
	char rcpath[MAX_PATH]; int i;
	GetModuleFileName(m_hMainInstance, rcpath, sizeof(rcpath));
	for (i=0;;) {
		int nLen = strlen(rcpath);
		while (nLen && rcpath[nLen-1] != '\\') nLen--;
		strcpy(rcpath+nLen, "bbkeysrc");  if (FileExists(rcpath)) break;
		strcpy(rcpath+nLen, "bbkeys.rc"); if (FileExists(rcpath)) break;
		if (2 == ++i) {
			return;
		}
		GetBlackboxPath(rcpath, sizeof(rcpath));
	}

	char buffer[1024], keytograb[120], modifier[120], action[1024];

	nolabel = false;
	int ID  = 0;
	FILE *fp = FileOpen(rcpath);

	HotkeyType **ppHk = &m_hotKeys;

	// KeyToGrab(), WithModifier(), WithAction(), DoThis()
	while (ReadNextCommand(fp, buffer, sizeof (buffer))) {
		if (0==getvalue(buffer, "KEYTOGRAB", keytograb, false)) {
			if (0==stricmp("NOLABEL", buffer))
				nolabel = true;
			continue;
		}
		getvalue(buffer, "WITHMODIFIER", modifier, false);
		if (0==getvalue(buffer, "WITHACTION", action, false))
			continue;

		strupr(keytograb);

		unsigned ch = (unsigned char)keytograb[0];
		unsigned sub = 0;
		bool is_ExecCommand = false;

		if (0 == stricmp(action, "ExecCommand")) {
			is_ExecCommand = true;
			getvalue(buffer, "DOTHIS", action, true);
		}

		//dbg_printf(buffer, "<%s>\n<%s>\n<%s>\n", keytograb, modifier, action);

		if (keytograb[1]) {
			static const struct vkTable { char* key; int vKey; } vkTable[] = {
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
				{"PRTSCN", VK_SNAPSHOT},
				{"PAUSE", VK_PAUSE},
				{"INSERT", VK_INSERT},
				{"DELETE", VK_DELETE},
				{"HOME", VK_HOME},
				{"END", VK_END},
				{"PAGEUP", VK_PRIOR},
				{"PAGEDOWN", VK_NEXT},
				{"LEFT", VK_LEFT},
				{"RIGHT", VK_RIGHT},
				{"DOWN", VK_DOWN},
				{"UP", VK_UP},
				{"TAB", VK_TAB},
				{"BACKSPACE", VK_BACK},
				{"SPACEBAR", VK_SPACE},
				{"APPS", VK_APPS},
				{"ENTER", VK_RETURN},
				{"NUM0", VK_NUMPAD0},
				{"NUM1", VK_NUMPAD1},
				{"NUM2", VK_NUMPAD2},
				{"NUM3", VK_NUMPAD3},
				{"NUM4", VK_NUMPAD4},
				{"NUM5", VK_NUMPAD5},
				{"NUM6", VK_NUMPAD6},
				{"NUM7", VK_NUMPAD7},
				{"NUM8", VK_NUMPAD8},
				{"NUM9", VK_NUMPAD9},
				{"MUL", VK_MULTIPLY},
				{"DIV", VK_DIVIDE},
				{"ADD", VK_ADD},
				{"SUB", VK_SUBTRACT},
				{"DEC", VK_DECIMAL},
				{"ESCAPE", VK_ESCAPE},
				{"LWIN", VK_LWIN},
				{"RWIN", VK_RWIN},
				{ NULL, 0 }

			};
			const struct vkTable *v = vkTable;
			do if (!strcmp(v->key, keytograb)) {
				ch = v->vKey;
				goto found;
			}
			while ((++v)->key);
			if (keytograb[0] == 'V' && keytograb[1] == 'K'
			  && keytograb[2] >= '0' && keytograb[2] <= '9') {
				ch = atoi(keytograb+2);
				goto found;
			}
			continue;
		}

found:
		if (stristr(modifier, "WIN"))   sub |= MOD_WIN;
		if (stristr(modifier, "ALT"))   sub |= MOD_ALT;
		if (stristr(modifier, "CTRL"))  sub |= MOD_CONTROL;
		if (stristr(modifier, "SHIFT")) sub |= MOD_SHIFT;

		if (VK_LWIN == ch || VK_RWIN == ch)
			set_kbdhook(true);
		else
		if (0==RegisterHotKey(hwnd, ID, sub, ch))
			continue;

		//dbg_printf("registered: %02X %02X %s", ch, sub, action);

		HotkeyType *h = (HotkeyType *)m_alloc(sizeof (HotkeyType));
		h->sub      = sub;
		h->ch       = ch;
		h->is_ExecCommand = is_ExecCommand;
		h->szAction = new_str(action);

		*(ppHk=&(*ppHk=h)->next)=NULL;
		ID++;
	}
	FileClose(fp);
}

//===========================================================================
void BBKeys_FreeHotkeys(HWND hwnd)
{
	HotkeyType * h; int ID = 0;
	dolist (h, m_hotKeys)
	{
		UnregisterHotKey(hwnd, ID++);
		free_str(&h->szAction);
	}
	freeall(&m_hotKeys);
}

//===========================================================================
void send_command(HotkeyType *h)
{
	char buffer[1024];
	const char *action = h->szAction;

	if (false == nolabel)
	{
		sprintf(buffer, "BBKeys -> %s", action);
		SendMessage(BBhwnd, BB_SETTOOLBARLABEL, 0, (LPARAM)buffer);
	}

	if (h->is_ExecCommand)
	{
		//replace "@BBCore.ShowMenu ..." by "@BBCore.ShowMenuKBD ..."
		static const char show_menu[] = "@BBCore.ShowMenu";
		if (0 == memicmp(h->szAction, show_menu, sizeof show_menu - 1)
		 && (0 == h->szAction[sizeof show_menu - 1]
			 || ' ' == h->szAction[sizeof show_menu - 1]
			 ))
		{
			sprintf(buffer, "%sKBD%s", show_menu, action + sizeof show_menu - 1);
			action = buffer;
		}
	}
	else
	{
		sprintf(buffer, "@BBCore.%s", action);
		action = buffer;
	}

	SendMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)action);
}

//===========================================================================

LRESULT CALLBACK HotkeyProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int msgs[] = {BB_RECONFIGURE, BB_WINKEY, 0};
	HotkeyType *h; unsigned sub;

	switch (message)
	{
		case WM_CREATE:
			SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			BBKeys_LoadHotkeys(hwnd);
			break;

		case WM_DESTROY:
			SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			BBKeys_FreeHotkeys(hwnd);
			break;

		case BB_RECONFIGURE:
			BBKeys_FreeHotkeys(hwnd);
			BBKeys_LoadHotkeys(hwnd);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);

		case BB_WINKEY:
			sub = 0;
			if (0x8000 & GetAsyncKeyState(VK_SHIFT))    sub |=MOD_SHIFT;
			if (0x8000 & GetAsyncKeyState(VK_CONTROL))  sub |=MOD_CONTROL;
			if (0x8000 & GetAsyncKeyState(VK_MENU))     sub |=MOD_ALT;
			dolist (h, m_hotKeys)
				if (wParam == h->ch && sub == h->sub)
				{
					send_command(h);
					break;
				}
			break;

		case WM_HOTKEY:
			h = (HotkeyType*)nth_node(m_hotKeys, wParam);
			if (h) send_command(h);
			break;
	}
	return 0;
}

//===========================================================================
int beginPlugin(HINSTANCE hPluginInstance)
{
	if (BBhwnd)
	{
		MessageBox(NULL, "Dont load me twice!", szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
		return 1;
	}

	BBhwnd = GetBBWnd();

	if (0 != memicmp(GetBBVersion(), "bbLean", 6))
	{
		MessageBox(NULL, "This bbKeys requires bbLean.", szVersion, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
		return 1;
	}

	m_hMainInstance = hPluginInstance;

	OSVERSIONINFO osInfo;
	ZeroMemory(&osInfo, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	GetVersionEx(&osInfo);
	usingNT = osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = HotkeyProc;            // our window procedure
	wc.hInstance = m_hMainInstance;     
	wc.lpszClassName = szAppName;          // our window class name

	if (!RegisterClass(&wc)) return 1;

	hKeysWnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,       // exstyles
		wc.lpszClassName,       // our window class name
		NULL,                   // use description for a window title
		WS_POPUP,
		0, 0,                   // position
		0, 0,                   // width & height of window
		NULL,                   // parent window
		NULL,                   // no menu
		m_hMainInstance,        // hInstance of DLL
		NULL
		);

	return 0;
}


void endPlugin(HINSTANCE hPluginInstance)
{
	DestroyWindow(hKeysWnd);
	UnregisterClass(szAppName, m_hMainInstance);
	set_kbdhook(false);
}

//===========================================================================
