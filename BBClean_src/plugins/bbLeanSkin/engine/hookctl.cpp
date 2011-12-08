/*
 ============================================================================

  This file is part of the bbLeanSkin source code.
  Copyright © 2003 grischka (grischka@users.sourceforge.net)

  bbLeanSkin is a plugin for Blackbox for Windows

  http://bb4win.sourceforge.net/bblean
  http://bb4win.sourceforge.net/


  bbLeanSkin is free software, released under the GNU General Public License
  (GPL version 2 or later) For details see:

	http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

#include "..\..\..\blackbox\BBApi.h"
#include "hookinfo.h"
#include "subclass.h"

#ifdef USE_LLCRT
#define DllMain _DllMainCRTStartup
#pragma comment(linker, "/NODEFAULTLIB")
#include "llcrt.cpp"
#endif

// ----------------------------------------------
HINSTANCE hInstance;
unsigned bbSkinMsg;
SkinStruct mSkin;

void dbg_printf (const char *fmt, ...)
{
	char buffer[256]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	OutputDebugString(buffer);
}

//===========================================================================
struct shmem
{
	void *lpvMem;
	HANDLE hMapObject;
};

void ReleaseSharedMem(struct shmem *psh)
{
	if (psh->hMapObject)
	{
		if (psh->lpvMem) UnmapViewOfFile(psh->lpvMem);
		CloseHandle(psh->hMapObject);
	}
}

void *GetSharedMem(struct shmem *psh)
{
	psh->hMapObject = OpenFileMapping(FILE_MAP_READ, FALSE, BBLEANSKIN_SHMEMID);
	if (psh->hMapObject)
	{
		psh->lpvMem = MapViewOfFile(
				psh->hMapObject,// object to map view of
				FILE_MAP_READ,  // read access
				0,              // high offset:  map from
				0,              // low offset:   beginning
				0);             // default: map entire file

		if (psh->lpvMem) return psh->lpvMem;
	}
	ReleaseSharedMem(psh);
	return NULL;
}

bool GetSkin(void)
{
	struct shmem sh;
	SkinStruct *pSkin = (SkinStruct *)GetSharedMem(&sh);
	if (NULL == pSkin) return false;
	memcpy(&mSkin, pSkin, offset_exInfo);
	ReleaseSharedMem(&sh);
	return true;
}

//===========================================================================

HWND GetRootWindow(HWND hwnd)
{
	HWND pw; HWND dw = GetDesktopWindow();
	while (NULL != (pw = GetParent(hwnd)) && dw != pw) hwnd = pw;
	return hwnd;
}

int get_module(HWND hwnd, char *buffer, int buffsize)
{
	char sFileName[MAX_PATH]; HINSTANCE hi; int i, r;
	hi = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
	r = GetModuleFileName(hi, sFileName, MAX_PATH);
	if (0 == r) r = GetModuleFileName(NULL, sFileName, MAX_PATH);
	for (i = r; i && sFileName[i-1] != '\\'; i--);
	r -= i; if (r >= buffsize) r = buffsize-1;
	((char*)memcpy(buffer, sFileName + i, r))[r] = 0;
	return r;
}

char *sprint_window(char *buffer, HWND hwnd, char *msg)
{
	char sClassName[200]; sClassName[0] = 0;
	GetClassName(hwnd, sClassName, sizeof sClassName);

	char sFileName[200]; sFileName[0] = 0;
	get_module(hwnd, sFileName, sizeof sFileName);

	char caption[128]; caption[0] = 0;
	GetWindowText(hwnd, caption, sizeof caption);

	sprintf(buffer,
		"%s window with title \"%s\"\r\n\t%s:%s"
		//" - %08x %08x"
		, msg, caption, sFileName, sClassName
		//, GetWindowLong(hwnd, GWL_STYLE), GetWindowLong(hwnd, GWL_EXSTYLE),
		);
	return buffer;
}

void send_log(HWND hwnd, char *msg)
{
	if (false == mSkin.enableLog)
		return;

	char buffer[1000];
	sprint_window(buffer, hwnd, msg);

	COPYDATASTRUCT cds;
	cds.dwData = 201;
	cds.cbData = 1+strlen(buffer);
	cds.lpData = buffer;
	SendMessage (mSkin.loghwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}

//===========================================================================
// simple wildcard pattern matcher

int match (const char *str, const char *pat)
{
	for (;;)
	{   char s = *str, p = *pat;
		if ('*' == p)
		{   if (s && match(str+1, pat))
				return 1;
			++pat;
			continue;
		}
		if (0==s) return 0==p;
		if (s>='A' && s<='Z') s+=32;
		if (p>='A' && p<='Z') p+=32;
		if (p != s && p != '?') return 0;
		++str, ++pat;
	}
}

//#define CS_DROPSHADOW       0x00020000
//#define SPI_GETDROPSHADOW 0x1024

//===========================================================================
int HookWindow(HWND hwnd, int early)
{
	LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);

	// Noccy: Check if the system has menu shadows enabled in the first plcae
	/*long lDropShadowEnabled = 0;
	SystemParametersInfo(SPI_GETDROPSHADOW,0,&lDropShadowEnabled,0);
	bool bDropShadows = !(lDropShadowEnabled == 0);
	if ( mSkin.enableShadows && bDropShadows && !(lStyle & CS_DROPSHADOW) ) {
		SetWindowLong( hwnd, GWL_STYLE, (lStyle | CS_DROPSHADOW) );
		lStyle = GetWindowLong(hwnd, GWL_STYLE);
	}*/

	// if it does not have a caption, there is nothing to skin.
	if (WS_CAPTION != (lStyle & WS_CAPTION))
	{
		//send_log(hwnd, "no caption");
		return 0;
	}

	LONG lExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

	// child windows are excluded unless they have a submenu or are MDI clients
	if ((lStyle & WS_CHILD)
		&& false == (lStyle & WS_SYSMENU)
		&& false == (WS_EX_MDICHILD & lExStyle)
		)
	{
		//send_log(hwnd, "child, no sysmenu, not a MDI");
		return 0;
	}

	// if it is already hooked, dont fall into loop
	if (get_WinInfo(hwnd))
	{
		//send_log(hwnd, "already hooked");
		return 0;
	}

	// being skeptical about windows without sysmenu
	if (false == (lStyle & WS_SYSMENU) && false == (lStyle & WS_VISIBLE))
	{
		//if (0 == early) send_log(hwnd, "invisible without sysmenu");
		return early;
	}

	// check for something like a vertical titlebar, erm...
	if (lExStyle & WS_EX_TOOLWINDOW)
	{
		RECT rc; GetWindowRect(hwnd, &rc);
		ScreenToClient(hwnd, (LPPOINT)&rc.left);
		if (rc.top > -10)
		{
			//if (0 == early) send_log(hwnd, "abnormal caption");
			return early;
		}
	}

	// ------------------------------------------------------
	// now check the exclusion list

	int found = 0;

	struct shmem sh;
	SkinStruct *pSkin = (SkinStruct *)GetSharedMem(&sh);
	if (NULL == pSkin) return 0;

	char sClassName[200]; sClassName[0] = 0;
	GetClassName(hwnd, sClassName, sizeof sClassName);

	char sFileName[200]; sFileName[0] = 0;
	get_module(hwnd, sFileName, sizeof sFileName);

	struct exclusion_item *ei = pSkin->exInfo.ei;
	for (int i = pSkin->exInfo.count; i; --i)
	{
		char *f, *c = (f = ei->buff) + ei->flen;
		//dbg_printf("check %s:%s", f, c);

		// if filename matches and if class matches or is empty...
		if (match(sFileName, f) && (0 == *c || match(sClassName, c)))
		{
			found = 1 == ei->option ? -1 : 1; // check 'hook-early' option
			break;
		}
		ei = (struct exclusion_item *)(c + ei->clen);
	}
	ReleaseSharedMem(&sh);

	// ------------------------------------------------------

	if (early && 0 == found)
	{
		//send_log(hwnd, "Checking later:");
		return 1;
	}

	// send message to log_window
	if (mSkin.enableLog)
	{
		char msg[100];
		sprintf(msg, "%s%s", found > 0 ? "Excluded" : early ? "Hooked early" : "Hooked", IsWindowVisible(hwnd) ? "" : " invisible");
		send_log(hwnd, msg);
	}

	// return when excluded;
	if (found > 0) return 0;

	// skin it.
	subclass_window(hwnd);
	return 2;
}

//===========================================================================

LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (WM_NCCREATE == ((CWPSTRUCT*)lParam)->message)
		{
			//dbg_printf("create hwnd %x hk %x", ((CWPSTRUCT*)lParam)->hwnd, mSkin.hCallWndHook);
			//if (1 == HookWindow(((CWPSTRUCT*)lParam)->hwnd, 1))
				PostMessage(((CWPSTRUCT*)lParam)->hwnd, bbSkinMsg, MSGID_LOAD, 0);
		}
	}
	return CallNextHookEx(mSkin.hCallWndHook, nCode, wParam, lParam);
}

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (bbSkinMsg == ((MSG*)lParam)->message && MSGID_LOAD == ((MSG*)lParam)->wParam)
		{
			//dbg_printf("getmsg hwnd %x hk %x", ((MSG*)lParam)->hwnd, mSkin.hGetMsgHook);
			HookWindow(((MSG*)lParam)->hwnd, 0);
		}
	}
	return CallNextHookEx(mSkin.hGetMsgHook, nCode, wParam, lParam);
}

// this is called for process 'blackbox' only
extern "C" DLL_EXPORT int EntryFunc(int option, SkinStruct *pSkin)
{
	if (ENGINE_GETVERSION == option)
	{
		return ENGINE_THISVERSION;
	}

	if (ENGINE_SETHOOKS == option)
	{
		pSkin->hCallWndHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hInstance, 0);
		pSkin->hGetMsgHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, hInstance, 0);
		GetSkin(); // copy HHOOKs
		//dbg_printf("Hooks set %x %x", mSkin.hCallWndHook, mSkin.hGetMsgHook);
		return NULL != mSkin.hCallWndHook && NULL != mSkin.hGetMsgHook;
	}

	if (ENGINE_UNSETHOOKS == option)
	{
		//dbg_printf("Hooks removed %x %x", mSkin.hCallWndHook, mSkin.hGetMsgHook);
		BOOL r1 = UnhookWindowsHookEx(pSkin->hGetMsgHook);
		BOOL r2 = UnhookWindowsHookEx(pSkin->hCallWndHook);
		return r1 && r2;
	}

	if (ENGINE_SKINWINDOW == option) // skin one specified window (no hook)
	{
		memcpy(&mSkin, pSkin, offset_hooks);
		HWND hwnd = pSkin->skinwnd;
		if (get_WinInfo(hwnd))
			PostMessage(hwnd, bbSkinMsg, MSGID_REFRESH, 0);
		else
			subclass_window(hwnd);
		return 1;
	}
	return 0;
}

//===========================================================================
extern "C" BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// The DLL is being loaded for the first time by a given process.
		// Perform per-process initialization here.  If the initialization
		// is successful, return TRUE; if unsuccessful, return FALSE.
#if 0
		char buffer[MAX_PATH], *p;
		GetModuleFileName(NULL, buffer, sizeof buffer);
		p = strrchr(buffer, '\\');
		if (p && 0 == stricmp(p+1, "KERNEL32.DLL"))
		{
			//dbg_printf("refused: %s", buffer);
			return FALSE;
		}
#endif
		//dbg_printf("attached to %s", buffer);
		hInstance = hDLLInst;
		DisableThreadLibraryCalls(hDLLInst);
		bbSkinMsg = RegisterWindowMessage(BBLEANSKIN_WINDOWMSG);
		GetSkin(); // need to get the HHOOK's from the shared mem

		//dbg_printf("Attached to %x HHOOKs: CW %x GM %x", hDLLInst, mSkin.hCallWndHook, mSkin.hGetMsgHook);
		break;

	case DLL_PROCESS_DETACH:
		// The DLL is being unloaded by a given process.  Do any
		// per-process clean up here, such as undoing what was done in
		// DLL_PROCESS_ATTACH.  The return value is ignored.

		//dbg_printf("unloaded.");
		break;

	case DLL_THREAD_ATTACH:
		// A thread is being created in a process that has already loaded
		// this DLL.  Perform any per-thread initialization here.  The
		// return value is ignored.

		//dbg_printf("new thread");
		break;

	case DLL_THREAD_DETACH:
		// A thread is exiting cleanly in a process that has already
		// loaded this DLL.  Perform any per-thread clean up here.  The
		// return value is ignored.
		
		//dbg_printf("end thread");
		break;
	}
	return TRUE;
}

//===========================================================================
