/*---------------------------------------------------------------------------------
 bbScheduler is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbScheduler is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/
#include <time.h>
#include "BBApi.h"
#include "Tinylist.h"
#include "m_alloc.h"
#include "bbScheduler.h"

// ===================================================================================
extern "C" BOOL WINAPI DllMainCRTStartup(HANDLE hModule, DWORD dwFunction, LPVOID lpNot){
	return TRUE;
}

// ===================================================================================
int beginPlugin(HINSTANCE hPluginInstance){
	WNDCLASS wc;
	ZeroMemory((void*)&wc, sizeof(wc));
	wc.lpszClassName = pszAppName;
	wc.hInstance     = hPluginInstance;
	wc.lpfnWndProc   = WndProc;
	RegisterClass(&wc);

	g_hPluginWnd = CreateWindow(
		pszAppName,
		NULL,
		WS_POPUP,
		0, 0, 0, 0,
		HWND_MESSAGE,
		NULL,
		hPluginInstance,
		NULL
	);

	BBhWnd = GetBBWnd();
	SendMessage(BBhWnd, BB_REGISTERMESSAGE, (WPARAM)g_hPluginWnd, (LPARAM)msgs);

	return 0;
}

// -----------------------------------------------------------------------------------
void endPlugin(HINSTANCE hPluginInstance){
	SendMessage(BBhWnd, BB_UNREGISTERMESSAGE, (WPARAM)g_hPluginWnd, (LPARAM)msgs);
	DestroyWindow(g_hPluginWnd);
	UnregisterClass(pszAppName, hPluginInstance);
}

// -----------------------------------------------------------------------------------
LPCSTR pluginInfo (int field){
	switch (field){
		case PLUGIN_NAME    : return pszAppName;
		case PLUGIN_VERSION : return pszInfoVersion;
		case PLUGIN_AUTHOR  : return pszInfoAuthor;
		case PLUGIN_RELEASE : return pszInfoRelDate;
		default             : return "";
	}
}

// ===================================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	static SCHEDULETASK *pScheduleTask = NULL;

	SCHEDULETASK *p;
	switch (uMsg){
		case WM_CREATE:
			{ // Debug Message
				time_t t0 = time(NULL);
				tm *timeinfo = localtime(&t0);
				dbg_printf_time( "CRT : ", timeinfo);
			}
			ReadSchedules(((LPCREATESTRUCT)lParam)->hInstance, &pScheduleTask);
			dolist (p, pScheduleTask){
				{ // Debug Message
					dbg_printf("-------------------------------");
					dbg_printf(p->szCommand);
				}
				MakeReservation(hWnd, p);
			}
			break;

		case WM_TIMER:
			dolist(p, pScheduleTask){
				if ((UINT)wParam == p->uID){
					SendMessage(BBhWnd, BB_EXECUTEASYNC, 0, (LPARAM)p->szCommand);
					{ // Debug Message
						time_t t0 = time(NULL);
						tm *timeinfo = localtime(&t0);
						dbg_printf("-------------------------------");
						dbg_printf(p->szCommand);
						dbg_printf_time( "NOW : ", timeinfo);
					}
					MakeReservation(hWnd, p);
				}
			}
			break;

		case WM_DESTROY:
			freeall(&pScheduleTask);
			break;

		case BB_RECONFIGURE:
			{ // Debug Message
				time_t t0 = time(NULL);
				tm *timeinfo = localtime(&t0);
				dbg_printf_time( "RCF : ", timeinfo);
			}
			freeall(&pScheduleTask);
			ReadSchedules((HINSTANCE)GetWindowLong(g_hPluginWnd, GWL_HINSTANCE), &pScheduleTask);
			dolist (p, pScheduleTask){
				{ // Debug Message
					dbg_printf("-------------------------------");
					dbg_printf(p->szCommand);
				}
				MakeReservation(hWnd, p);
			}
			break;

		default:
			break;
	}
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------------------------------------
time_t MakeReservation(HWND hWnd, SCHEDULETASK *pScheduleTask){
	time_t tnow = time(NULL);
	int tdiff = mktime(&pScheduleTask->timeinfo) - tnow;

	time_t trem;
	if (tdiff > 0){
		trem = tdiff;
	}
	else{
		if ((pScheduleTask->nIgnore & IGN_YEAR)){
			if      (!(pScheduleTask->nIgnore & IGN_MONTH)) pScheduleTask->timeinfo.tm_year += 1;
			else if (!(pScheduleTask->nIgnore & IGN_MDAY )) pScheduleTask->timeinfo.tm_mon  += 1;
			else if (!(pScheduleTask->nIgnore & IGN_HOUR )) pScheduleTask->timeinfo.tm_mday += 1;
			else if (!(pScheduleTask->nIgnore & IGN_MIN  )) pScheduleTask->timeinfo.tm_hour += 1;
			else if (!(pScheduleTask->nIgnore & IGN_SEC  )) pScheduleTask->timeinfo.tm_min  += 1;
		}
		trem = mktime(&pScheduleTask->timeinfo) - tnow;
	}

	{ // Debug Message
		dbg_printf("diff : %d, remain : %d", tdiff, (int)trem);
		dbg_printf_time( "SET : ", &pScheduleTask->timeinfo);
	}

	SetTimer(hWnd, pScheduleTask->uID, trem*1000, NULL);

	return trem;
}

// -----------------------------------------------------------------------------------
bool ReadSchedules(HINSTANCE hInstance, SCHEDULETASK **ppScheduleTask){
	char buf[MAX_PATH];
	if (FILE *fp = FileOpen(set_my_path(hInstance, buf, "Schedules.rc"))){
		dbg_printf(set_my_path(hInstance, buf, "Schedules.rc"));
		UINT uID = 1;
		while(ReadNextCommand(fp, buf, sizeof(buf)-1)){
			dbg_printf(buf);
			char *pszYear   = strtok(buf, ".");
			char *pszMonth  = strtok(NULL, ".");
			char *pszDay    = strtok(NULL, ".");
			char *pszHour   = strtok(NULL, ".");
			char *pszMinute = strtok(NULL, ".");
			char *pszSecond = strtok(NULL, " ");
			time_t t = time(NULL);

			SCHEDULETASK *pST = (SCHEDULETASK*)c_alloc(sizeof(SCHEDULETASK));
			append_node(ppScheduleTask, pST);

			pST->uID = uID++;
			memcpy(&(pST->timeinfo), localtime(&t), sizeof(tm));
			strncpy(pST->szCommand, strtok(NULL, ""), MAX_PATH);
			if (!((pST->nIgnore |= !strcmp(pszYear,   "****") << 0) & IGN_YEAR )) pST->timeinfo.tm_year = atoi(pszYear) - 1900;
			if (!((pST->nIgnore |= !strcmp(pszMonth,  "**")   << 1) & IGN_MONTH)) pST->timeinfo.tm_mon  = atoi(pszMonth) - 1;
			if (!((pST->nIgnore |= !strcmp(pszDay,    "**")   << 2) & IGN_MDAY )) pST->timeinfo.tm_mday = atoi(pszDay);
			if (!((pST->nIgnore |= !strcmp(pszHour,   "**")   << 3) & IGN_HOUR )) pST->timeinfo.tm_hour = atoi(pszHour);
			if (!((pST->nIgnore |= !strcmp(pszMinute, "**")   << 4) & IGN_MIN  )) pST->timeinfo.tm_min  = atoi(pszMinute);
			if (!((pST->nIgnore |= !strcmp(pszSecond, "**")   << 5) & IGN_SEC  )) pST->timeinfo.tm_sec  = atoi(pszSecond);
		}
		FileClose(fp);
		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------------
char *set_my_path(HINSTANCE hInstance, char *path, char *fname){
	int nLen = GetModuleFileName(hInstance, path, MAX_PATH);
	while (nLen && path[nLen-1] != '\\') nLen--;
	strcpy(path+nLen, fname);
	return path;
}

