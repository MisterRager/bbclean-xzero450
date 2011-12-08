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
*/ // mousehook.cpp
#define BB_MOUSEBTN 856

#define _WIN32_WINNT 0x500
#include "BBAPI.h"

//#define TESTING

#ifdef USE_LLCRT
#define DllMain _DllMainCRTStartup
#pragma comment(linker, "/NODEFAULTLIB")
#ifdef TESTING
#include "../bbLeanSkin/engine/llcrt.cpp"
#endif
#endif

/*
static void dbg_printf (const char *fmt, ...)
{
	char buffer[256]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	OutputDebugString(buffer);
}
*/

//===========================================================================
#ifdef __BORLANDC__
  extern HHOOK g_mMouseHook;
  extern bool otherkey;
#endif

#ifdef _MSC_VER
#pragma data_seg(".hook")
  HHOOK g_mMouseHook = NULL;
  bool otherkey = false;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.hook,RWS") 
#endif

#ifdef __GNUC__
  #define SHARED(X) X __attribute__((section(".shared"), shared))
  HHOOK SHARED(g_mMouseHook) = NULL;
  bool SHARED(otherkey) = false;
#endif

HINSTANCE hInstance;

//===========================================================================
static void PostBB(UINT msg, WPARAM wParam)
{
	HWND BBKeyWnd = FindWindow("BBKeys", NULL);
	if (BBKeyWnd)
		PostMessage(BBKeyWnd, msg, wParam, 0);
}

static int post_mouse(bool key_down, UINT key)
{
	PostBB(BB_MOUSEBTN, key);
	return 0;
}

//===========================================================================
LRESULT CALLBACK LLMouseProc (INT nCode, WPARAM wParam, LPARAM lParam) {
	// By returning a non-zero value from the hook procedure, the
	// message does not get passed to the target window
	if (HC_ACTION == nCode) {
		//dbg_printf("hc_action code %x wp %x", pkbhs->vkCode, wParam);

		////////////
			//dbg_printf("vk_win %08x", flags);
		////////////
	}
	return CallNextHookEx (g_mMouseHook, nCode, wParam, lParam);
}

//===========================================================================

//===========================================================================
extern "C" DLL_EXPORT bool SetKbdHook(bool set) {
	BOOL r;
	if (set) {
		r = NULL == g_mMouseHook && NULL !=
#ifdef TESTING
			(g_mMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, hInstance, 0));
		dbg_printf("mousehook set (%d)", r);
#else
			(g_mMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, hInstance, 0));
#endif
	} else {
		r = NULL != g_mMouseHook && FALSE != UnhookWindowsHookEx(g_mMouseHook);
		g_mMouseHook = NULL;
#ifdef TESTING
		dbg_printf("mousehook removed (%d)", r);
#endif
	}
	return r;
}

//===========================================================================

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (DLL_PROCESS_ATTACH==fdwReason)
	{
		// The DLL is being loaded for the first time by a given process.
		hInstance = hinstDLL;
	}
	return TRUE;
}

//===========================================================================
