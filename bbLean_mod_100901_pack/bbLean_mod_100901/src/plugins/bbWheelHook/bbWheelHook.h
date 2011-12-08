/*---------------------------------------------------------------------------------
 bbWheelHook (© 2006-2008 nocd5)
 ----------------------------------------------------------------------------------
 bbWheelHook is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbWheelHook is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/
#ifndef _BBWHEELHOOK_H
#define _BBWHEELHOOK_H
// ================================================================================
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#ifndef DLL_EXPORT
	#define DLL_EXPORT __declspec(dllexport)
#endif

#ifdef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

// Blackbox messages
#define BB_REGISTERMESSAGE   10001
#define BB_UNREGISTERMESSAGE 10002
#define BB_RECONFIGURE       10103
// #define BB_EXECUTEASYNC      10882
// #define BB_POSTSTRING        10899
// #define BB_EXECUTE           10202
// #define BB_BROADCAST         10901

#include "Utils.h"
// --------------------------------------------------------------------------------
#define szVersion     "bbWheelHook 0.1.7"
#define szAppName     "bbWheelHook"
#define szInfoVersion "0.1.7"
#define szInfoAuthor  "nocd5"
#define szInfoRelDate "2008-11-23"

extern "C"{
	DLL_EXPORT BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot);
	DLL_EXPORT void endPlugin(HINSTANCE);
	DLL_EXPORT int beginPlugin(HINSTANCE);
	DLL_EXPORT LPCSTR pluginInfo(int index);
};

// --------------------------------------------------------------------------------
static int msgs[] = {BB_RECONFIGURE, 0};
// --------------------------------------------------------------------------------
HWND hPluginWnd = NULL;
HWND BBhWnd = NULL;
static HINSTANCE hHookDll = NULL;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LPCSTR pluginInfo(int field);
string_node *ExclusionsList;
static char szExclusionsPath[MAX_PATH];
// ================================================================================
#endif
