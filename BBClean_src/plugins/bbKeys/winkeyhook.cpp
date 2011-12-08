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
*/ // winkeyhook.cpp

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
  extern HHOOK g_mHook;
  extern HHOOK g_mMouseHook;
  extern bool otherkey;
  extern bool winkey;
#endif

#ifdef _MSC_VER
#pragma data_seg(".hook")
  HHOOK g_mHook = NULL;
  HHOOK g_mMouseHook = NULL;
  bool otherkey = false;
  bool winkey = false;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.hook,RWS") 
#endif

#ifdef __GNUC__
  #define SHARED(X) X __attribute__((section(".shared"), shared))
  HHOOK SHARED(g_mHook) = NULL;
  HHOOK SHARED(g_mMouseHook) = NULL;
  bool SHARED(otherkey) = false;
  bool SHARED(winkey)   = false;
#endif

HINSTANCE hInstance;

//===========================================================================
static void PostBB(UINT msg, WPARAM wParam)
{
	HWND BBKeyWnd = FindWindow("BBKeys", NULL);
	if (BBKeyWnd)
		PostMessage(BBKeyWnd, msg, wParam, 0);
}

static int post_winkey(bool key_down, UINT key)
{
	if (key_down)
	{
		if (false == winkey)
		{
			winkey = true;
			otherkey = false;
			return 0;
		}
	}
	else
	{
		winkey = false;
		if (false == otherkey)
		{
			PostBB(BB_WINKEY, key);
			return 0;
		}
	}
	return 0;
}

//===========================================================================
LRESULT CALLBACK LLKeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)
{
	// By returning a non-zero value from the hook procedure, the
	// message does not get passed to the target window
	if (HC_ACTION == nCode) {
/*
		// Check to see if the CTRL key is pressed
		BOOL bControlKeyDown = GetAsyncKeyState (VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);

		// Disable CTRL+ESC
		if (pkbhs->vkCode == VK_ESCAPE && bControlKeyDown)
			return 1;

		// Disable ALT+TAB
		if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
			return 1;

		// Disable ALT+ESC
		if (pkbhs->vkCode == VK_ESCAPE && pkbhs->flags & LLKHF_ALTDOWN)
			return 1;
*/
		//dbg_printf("hc_action code %x wp %x", pkbhs->vkCode, wParam);

		int vkCode = ((KBDLLHOOKSTRUCT *)lParam)->vkCode;
		int flags = ((KBDLLHOOKSTRUCT *)lParam)->flags;
		if (VK_LWIN == vkCode || VK_RWIN == vkCode) {
			//dbg_printf("vk_win %08x", flags);
			return post_winkey(0 == (0x80 & flags), vkCode);
		}
		otherkey = true;
	}
	return CallNextHookEx (g_mHook, nCode, wParam, lParam);
}

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
#ifdef TESTING
LRESULT CALLBACK KeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)
{
	// By returning a non-zero value from the hook procedure, the
	// message does not get passed to the target window
	if (HC_ACTION == nCode)
	{
		int vkCode = wParam;
		if (VK_F3 == vkCode) vkCode = VK_LWIN;
		if (VK_F4 == vkCode) vkCode = VK_RWIN;

		dbg_printf("hc_action code = %08x %x", lParam, wParam);
		if (vkCode == VK_LWIN || vkCode == VK_RWIN)
		{
			//dbg_printf("vk_win");
			return post_winkey(0 == (0x80000000 & lParam), vkCode);
		}
		otherkey = true;

	}
	return CallNextHookEx (g_mHook, nCode, wParam, lParam);
}
#endif

//===========================================================================
extern "C" DLL_EXPORT bool SetKbdHook(bool set) {
	BOOL r;
	if (set) {
		r = NULL == g_mHook && NULL !=
#ifdef TESTING
			(g_mHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, 0));
		dbg_printf("winkeyhook set (%d)", r);
#else
			(g_mHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hInstance, 0));
			(g_mMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, hInstance, 0));
#endif
	} else {
		r = NULL != g_mHook && FALSE != UnhookWindowsHookEx(g_mHook);
		g_mHook = NULL;

		r = NULL != g_mMouseHook && FALSE != UnhookWindowsHookEx(g_mMouseHook);
		g_mMouseHook = NULL;
#ifdef TESTING
		dbg_printf("winkeyhook removed (%d)", r);
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
