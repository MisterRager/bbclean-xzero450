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
#ifndef _BBWHEELHOOKHOOK_H
#define _BBWHEELHOOKHOOK_H
// ================================================================================
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0500
#endif

#ifndef DLL_EXPORT
	#define DLL_EXPORT __declspec(dllexport)
#endif

#ifdef __GNUC__
	#define SHARED(X) X __attribute__((section(".shared"), shared))
#endif

#ifdef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

// --------------------------------------------------------------------------------
extern "C"{
	DLL_EXPORT BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot);
	DLL_EXPORT HHOOK beginHook(HWND hWnd);
	DLL_EXPORT void endHook();
};
// --------------------------------------------------------------------------------
HWND SHARED(hPluginWnd) = NULL;
HINSTANCE hHookDll = NULL;
LRESULT CALLBACK WheelProc(int nCode, WPARAM wParam, LPARAM lParam);
HHOOK SHARED(sh_hWHook) = NULL;
// ================================================================================
#endif
