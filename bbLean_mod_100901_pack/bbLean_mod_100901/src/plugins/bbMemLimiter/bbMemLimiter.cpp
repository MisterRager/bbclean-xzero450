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

#include "bbMemLimiter.h"

PROCESS_MEMORY_COUNTERS MemInfo;

//-------------------------------------------------------------------------------------

int beginPlugin(HINSTANCE hPluginInstance){
	WNDCLASS wc;
	ZeroMemory((void*)&wc, sizeof(wc));
	wc.lpszClassName = szVersion;
	wc.hInstance = hPluginInstance;
	wc.lpfnWndProc = DefWindowProc;
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

	BBhWnd = (HWND)GetCurrentProcess();
	SetTimer(hPluginWnd, TIMER_ID, TIMER_INTERVAL, TimerProc);
	return 0;
}

//-------------------------------------------------------------------------------------

void endPlugin(HINSTANCE hPluginInstance){
	DestroyWindow(hPluginWnd);
	UnregisterClass(szVersion, hPluginInstance);
	KillTimer(hPluginWnd, TIMER_ID);
}

//-------------------------------------------------------------------------------------
void CALLBACK TimerProc(HWND hwnd , UINT uMsg ,UINT idEvent , DWORD dwTime){
	GetProcessMemoryInfo(BBhWnd, &MemInfo, sizeof(MemInfo));
	if (MemInfo.WorkingSetSize >> 10 > LIMIT_SIZE){
		SetProcessWorkingSetSize(BBhWnd, (SIZE_T)-1, (SIZE_T)-1);
	}
}

//-------------------------------------------------------------------------------------
LPCSTR pluginInfo (int field){
	switch (field){
		case 0: return szVersion;
		case 1: return szAppName;
		case 2: return szInfoVersion;
		case 3: return szInfoAuthor;
		case 4: return szInfoRelDate;
	}
	return "";
}

//-------------------------------------------------------------------------------------
BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
    return TRUE;
}
