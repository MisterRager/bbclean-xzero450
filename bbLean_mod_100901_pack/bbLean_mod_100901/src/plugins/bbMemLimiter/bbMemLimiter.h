/*---------------------------------------------------------------------------------
 bbMemLimiter (© 2006-2009 nocd5)
 ----------------------------------------------------------------------------------
 bbMemLimiter is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbMemLimiter is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/

#define LIMIT_SIZE 5000 // [K byte]
#define TIMER_INTERVAL 5000 // [ms]
#define TIMER_ID 1

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#ifndef DLL_EXPORT
  #define DLL_EXPORT __declspec(dllexport)
#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <psapi.h>
//-------------------------------------------------------------------------------------
#define szVersion     "bbMemLimiter 0.1.0"
#define szAppName     "bbMemLimiter"
#define szInfoVersion "0.1.0"
#define szInfoAuthor  "nocd5"
#define szInfoRelDate "2009-07-23"
#define szInfoLink    ""
#define szInfoEmail   ""

extern "C"{
	DLL_EXPORT void endPlugin(HINSTANCE);
	DLL_EXPORT int beginPlugin(HINSTANCE);
	DLL_EXPORT LPCSTR pluginInfo(int index);
	DLL_EXPORT BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot);
};

//---------------------------------------------------------------------------------
HWND hPluginWnd;
HWND BBhWnd;
void CALLBACK TimerProc(HWND hwnd , UINT uMsg ,UINT idEvent , DWORD dwTime);
//---------------------------------------------------------------------------------
