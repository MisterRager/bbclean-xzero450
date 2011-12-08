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
#include <windows.h>
#include "bbWheelHookHook.h"
// --------------------------------------------------------------------------------
BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
	hHookDll = (HINSTANCE)hModule;
	return TRUE;
}

// --------------------------------------------------------------------------------
HHOOK beginHook(HWND hWnd){
	if (hPluginWnd == NULL)
		hPluginWnd = hWnd;
	return sh_hWHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)WheelProc, hHookDll, 0);
}

// --------------------------------------------------------------------------------
void endHook(){
	UnhookWindowsHookEx(sh_hWHook);
	sh_hWHook = NULL;
}

// --------------------------------------------------------------------------------
LRESULT CALLBACK WheelProc(int nCode, WPARAM wParam, LPARAM lParam){
	if(
			nCode == HC_ACTION &&
			wParam == PM_REMOVE &&
			!(0x8000 & GetAsyncKeyState(VK_ESCAPE))
	){
		MSG *msg = (MSG *)lParam;

		switch (LOWORD(msg->message)){
			case WM_LBUTTONDOWN:
				{
					if (0x8000 & GetKeyState(VK_LWIN)){ // LWIN + Ctrl/Shift
						msg->message = WM_NULL;
						WPARAM wAction = 0;
						if (0x8000 & GetKeyState(VK_CONTROL)) // move window
							wAction = SC_MOVE | 0x02;
						if (0x8000 & GetKeyState(VK_MENU)) // resize window
							wAction = SC_SIZE | 0x08;

						if (wAction) {
							SetCursor(LoadCursor(NULL, IDC_SIZEALL)); // change cursor
							PostMessage(GetAncestor(WindowFromPoint(msg->pt), GA_ROOT), WM_SYSCOMMAND, wAction, 0);
						}
					}
					break;
				}

			case WM_MOUSEWHEEL:
				{
					UINT uWM = WM_VSCROLL;
					int nSV = 3;
					bool bMod = false;
					HWND hWndCur = WindowFromPoint(msg->pt);
					bool bExclusion = SendMessage(hPluginWnd, WM_USER+1, 0, (LPARAM)hWndCur);

					// horizontal scroll
					if (0x8000 & GetAsyncKeyState(VK_SHIFT)){
						uWM = WM_HSCROLL;
						bMod = true;
					}
					// single line scroll
					if (0x8000 & GetAsyncKeyState(VK_CONTROL)){
						nSV = 1;
						bMod = true;
					}

					if (bExclusion){ // for excluded apps
						if (GetAncestor(hWndCur, GA_ROOT) == GetForegroundWindow())
							break;
						SendMessage(GetAncestor(hWndCur, GA_ROOT), WM_MOUSEWHEEL, msg->wParam, msg->lParam);
					}
					else if (bMod){ // single-line scroll, horizontal scroll
						WORD nScrollCode = 0 < (short)HIWORD(msg->wParam) ? SB_LINEUP : SB_LINEDOWN;
						for (int i = 0; i < nSV; i++){
							SendMessage(hWndCur, uWM, MAKEWPARAM(nScrollCode, 0), 0);
						}
						SendMessage(hWndCur, WM_VSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
					}
					else // default hook action
						SendMessage(hWndCur, WM_MOUSEWHEEL, msg->wParam, msg->lParam);

					msg->message = WM_NULL;
					break;
				}

			default:
				break;
		}
	}
	return CallNextHookEx(sh_hWHook, nCode, wParam, lParam);
}

