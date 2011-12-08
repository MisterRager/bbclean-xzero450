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
#include "bbWheelHook.h"
// --------------------------------------------------------------------------------
HHOOK (*beginHook)(HWND);
void (*endHook)();

// --------------------------------------------------------------------------------
BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
	return TRUE;
}

// --------------------------------------------------------------------------------
int beginPlugin(HINSTANCE hPluginInstance){

	// Load HookDll
	char szHookDllPath[MAX_PATH];
	set_my_path(hPluginInstance, szHookDllPath, "bbWheelHookHook.dll");

	if (hHookDll = LoadLibrary(szHookDllPath)){
		beginHook = (HHOOK(*)(HWND))GetProcAddress(hHookDll, "beginHook");
		endHook = (void(*)())GetProcAddress(hHookDll, "endHook");
		if (beginHook == NULL || endHook == NULL){
			FreeLibrary(hHookDll);
			return 1;
		}
	}
	else{
		return 1;
	}

	WNDCLASS wc;
	ZeroMemory((void*)&wc, sizeof(wc));
	wc.lpszClassName = szVersion;
	wc.hInstance = hPluginInstance;
	wc.lpfnWndProc = WndProc;
	RegisterClass(&wc);

	hPluginWnd = CreateWindow(
		szVersion,
		NULL,
		WS_POPUP,
		0, 0, 0, 0,
		HWND_MESSAGE,
		NULL,
		hPluginInstance,
		NULL
	);

	HWND (*GetBBWnd)() = (HWND(*)())GetProcAddress((HINSTANCE)GetModuleHandle(NULL), "GetBBWnd");
	BBhWnd = GetBBWnd();
	SendMessage(BBhWnd, BB_REGISTERMESSAGE, (WPARAM)hPluginWnd, (LPARAM)msgs);

	// read exclusions.rc
	set_my_path(hPluginInstance, szExclusionsPath, "exclusions.rc");
	ExclusionsList = read_exclusions(szExclusionsPath, ExclusionsList);

	beginHook(hPluginWnd);

	return 0;
}

// --------------------------------------------------------------------------------
void endPlugin(HINSTANCE hPluginInstance){
	if (hHookDll){
		endHook();
		FreeLibrary(hHookDll);
	}

	SendMessage(BBhWnd, BB_UNREGISTERMESSAGE, (WPARAM)hPluginWnd, (LPARAM)msgs);
	DestroyWindow(hPluginWnd);
	UnregisterClass(szVersion, hPluginInstance);
	freeall(&ExclusionsList);

	SendMessage(HWND_BROADCAST, WM_NULL, 0, 0);
}

// --------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	HWND hWnd = (HWND)lParam;
	switch (message){
		case BB_RECONFIGURE:
			freeall(&ExclusionsList);
			ExclusionsList = read_exclusions(szExclusionsPath, ExclusionsList);
			return DefWindowProc(hwnd, message, wParam, lParam);

		case WM_USER+1:
			char szPath[MAX_PATH*2 + 1];
			GetFileNameFromHwnd(hWnd, szPath, MAX_PATH);
			char szClassName[MAX_PATH];
			GetClassName(hWnd, szClassName, MAX_PATH);

			if (0 == lstrcmpi(szClassName, "DesktopBackgroundClass")) // action @ desktop ?
				return true;
			else
				return find_exclusions(ExclusionsList, szPath, szClassName);

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

// --------------------------------------------------------------------------------
LPCSTR pluginInfo (int field){
	switch (field){
		case 0  : return szVersion;
		case 1  : return szAppName;
		case 2  : return szInfoVersion;
		case 3  : return szInfoAuthor;
		case 4  : return szInfoRelDate;
		default : return "";
	}
}
