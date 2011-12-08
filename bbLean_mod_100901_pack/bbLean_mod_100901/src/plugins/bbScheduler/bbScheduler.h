/*---------------------------------------------------------------------------------
 bbScheduler is a plugin for bbLean
 ----------------------------------------------------------------------------------
 bbScheduler is free software, released under the GNU General Public License,
 version 2, as published by the Free Software Foundation.  It is distributed
 WITHOUT ANY WARRANTY--without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  Please see the GNU General Public
 License for more details:  [http://www.fsf.org/licenses/gpl.html].
 --------------------------------------------------------------------------------*/
#ifndef _BBSCHEDULER_H
#define _BBSCHEDULER_H

typedef struct SCHEDULETASK{
	SCHEDULETASK *next;
	UINT uID;
	tm timeinfo;
	int nIgnore;
	char szCommand[MAX_PATH];
} SCHEDULETASK;

// --------------------------------------------------------------------------------
#define IGN_YEAR  (1 << 0)
#define IGN_MONTH (1 << 1)
#define IGN_MDAY  (1 << 2)
#define IGN_HOUR  (1 << 3)
#define IGN_MIN   (1 << 4)
#define IGN_SEC   (1 << 5)
// --------------------------------------------------------------------------------
#define pszAppName     "bbScheduler"
#define pszInfoVersion "0.0.2"
#define pszInfoAuthor  "nocd5"
#define pszInfoRelDate "2010-02-23"

// --------------------------------------------------------------------------------
static int msgs[] = {BB_RECONFIGURE, 0};

// --------------------------------------------------------------------------------
static HWND g_hPluginWnd = NULL;
static HWND BBhWnd = NULL;

// --------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
time_t MakeReservation(HWND hWnd, SCHEDULETASK *pScheduleTask);
bool ReadSchedules(HINSTANCE hInstance, SCHEDULETASK **ppScheduleTask);
char *set_my_path(HINSTANCE hInstance, char *path, char *fname);

// ================================================================================

#define dbg_printf_time(head,ptrtm) dbg_printf(head"%04d.%02d.%02d %02d.%02d.%02d\n", 1900+(*(ptrtm)).tm_year, 1+(*(ptrtm)).tm_mon, (*(ptrtm)).tm_mday, (*(ptrtm)).tm_hour, (*(ptrtm)).tm_min, (*(ptrtm)).tm_sec);

#endif

