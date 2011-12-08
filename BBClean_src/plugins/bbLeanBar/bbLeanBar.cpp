/*
 ============================================================================
  This file is part of the bbLeanBar source code.

  bbLeanBar is a plugin for BlackBox for Windows
  Copyright © 2003 grischka

  grischka@users.sourceforge.net

  http://bb4win.sourceforge.net/bblean/

 ============================================================================

  bbLeanBar is free software, released under the GNU General Public License
  (GPL version 2 or later). See for details:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/ // bbLeanBar as plugin

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0500

//#include "BBApi.h"
   // #endif
#include "..\..\blackbox\BBApi.h"
#include "..\..\blackbox\m_alloc.h"
#include "..\BBPlugin\BBPlugin.h"
#include "..\..\blackbox\tinylist.cpp"
#include <shellapi.h>
#include <commctrl.h>
#include <time.h>
#include "bbLeanBar.h"
#include "bartender.h"

#define ST static

HINSTANCE m_hInstance;

const char szVersion     [] = "bbLeanBar 1.16 (BBClean SVN)";
const char szAppName     [] = "bbLeanBar";
const char szInfoVersion [] = "1.16 (BBClean SVN)";
const char szInfoAuthor  [] = "grischka";
const char szInfoRelDate [] = __DATE__;
const char szInfoLink    [] = "http://bb4win.sourceforge.net/bblean";
const char szInfoEmail   [] = "grischka@users.sourceforge.net";

const char aboutmsg[] = "%s - © 2003-2005 grischka@users.sourceforge.net\n";

LPCSTR pluginInfo(int field)
{
	switch (field)
	{
		default:
		case 0: return szVersion;
		case 1: return szAppName;
		case 2: return szInfoVersion;
		case 3: return szInfoAuthor;
		case 4: return szInfoRelDate;
		case 5: return szInfoLink;
		case 6: return szInfoEmail;
	}
}

//===========================================================================

ST void drawIco (int px, int py, int size, HICON IconHop, HDC hDC, bool f, int saturationValue, int hueIntensity);
ST void SetToolTip(HWND hwnd, RECT *tipRect, char *tipText);
ST void ClearToolTips(HWND hwnd);
ST BOOL(WINAPI*pAllowSetForegroundWindow)(DWORD dwProcessId); // or ASFW_ANY for all processes

ST void ShowSysmenu(HWND Window, HWND hBBSystembarWnd);
ST class TinyDropTarget *init_drop_targ(HWND hwnd);
ST void exit_drop_targ(class TinyDropTarget *m_TinyDropTarget);

#define BB_DRAGOVER (WM_USER+100)
#define CLOCK_TIMER 2
#define LABEL_TIMER 3
#define TASK_RISE_TIMER 4
#define FULLSCREEN_CHECK_TIMER 5

//====================
HWND BBhwnd;
ST bool is_bblean;

ST HWND hToolTips;
ST HWND tray_notify_wnd;
ST HWND tray_clock_wnd;

ST int currentScreen;
ST char screenName[80];
ST HWND last_active_task;

ST COLORREF Color_T;
ST COLORREF Color_A;
int SN_A;
int TBJustify;

ST bool pSettings_arrowUnix;
ST bool pSettings_bulletUnix;
ST int styleBevelWidth;
ST int styleBorderWidth;
ST COLORREF styleBorderColor;

ST int ICOCLICKSLEEP;

struct plugin_info *g_PI;

#define NIF_INFO 0x00000010
#define NIN_BALLOONSHOW (WM_USER + 2)
#define NIN_BALLOONHIDE (WM_USER + 3)
#define NIN_BALLOONTIMEOUT (WM_USER + 4)
#define NIN_BALLOONUSERCLICK (WM_USER + 5)

//===========================================================================
struct config { const char *str; char mode; void *def; void *ptr; };
struct pmenu { char *displ; char *msg; char f; void *ptr; };

struct barinfo : plugin_info
{
	void process_broam(const char *temp, int f);
	LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret);
	void show_menu(bool);
	class BuffBmp *pBuff;
	class LeanBar *pLeanBar;
	void set_desktop_margin (void);
	void pos_changed(void);
	void reset_tbinfo(void);

	void set_screen_info(void);
	void set_clock_string (void);
	void update_windowlabel(void);
	void set_fullscreen_timer(void);

	void update_bar(int n);
	void post_trayicon_message(UINT wParam);
	void GetRCSettings();
	void WriteRCSettings();
	void GetStyleSettings();

	struct config *cfg_list;
	struct pmenu *cfg_menu;
	void make_cfg();

	void insert_cfg_menu_item(n_menu *cSub, struct pmenu *cp);

	// runtime temp variables
	int old_place;
	HWND task_over_hwnd;
	bool force_button_pressed;
	char clockTime[80];
	char clockTimeTips[80];
	char windowlabel[256];
	bool ShowingExternalLabel;

	// painting
	HFONT hFont;
	HDC hdcPaint;
	RECT *p_rcPaint;

	// metrics
	bool has_tasks;
	int buttonH;
	int labelH;
	int bbLeanBarLineHeight[8];
	int BarLines;
	int T_ICON_SIZE;
	int S_ICON_SIZE;


	// configuration
	int  widthPercent           ;
	int  TaskStyleCfg           ;
	bool reverseTasks           ;
	bool currentOnlyCfg         ;
	int  iconSaturationValue    ;
	int  iconHueIntensity       ;
	int  iconSize               ;
	int  traySaturationValue    ;
	int  trayHueIntensity       ;
	int  alphaValue             ;
	char strftimeFormat[60]     ;
	char strftimeTipsFormat[60] ;
	bool taskSysmenu            ;
	int  TaskStyle              ;
	bool currentOnly            ;
	bool task_with_border       ;
	bool enable_balloons        ;
	bool hide_in_fullscreen ;

	int  maxTaskItemWidth       ;  // Maximum task item width

	bool on_multimon;

	// other
	class TinyDropTarget *m_TinyDropTarget;
	char instance_name[40];
	int m_index;


	// The string, which describes the outlay of the bar
	char item_string[32];
	// a flag, which says what has to be updated
	int update_flag;
	// an item has captured the mouse
	class baritem *capture_item;

	~barinfo()
	{
		ClearToolTips(hwnd);
		exit_drop_targ(m_TinyDropTarget);
		BBP_Exit_Plugin(this);
		delete (char*)cfg_list;
		delete (char*)cfg_menu;
	}


	// --------------------------------------------------------------
	// tasklist support - wrapper for the GetTask... functions
	// to implement the 'current tasks only' feature

	struct tasklist *TL;
	struct tasklist *GetTaskListPtrEx(void) { return TL; }

	static BOOL task_enum_func(struct tasklist *tl, LPARAM lParam)
	{
		struct barinfo *PI = (struct barinfo *)lParam;

		if (PI->currentOnly)
		{
			if (PI->on_multimon)
			{
				if (PI->hMon != GetMonitorRect(tl->hwnd, NULL, GETMON_FROM_WINDOW))
					return TRUE;
			}

			if (currentScreen != tl->wkspc)
				return TRUE;
		}

		struct tasklist *item = new struct tasklist;
		*item = *tl;
		cons_node(&PI->TL, item);
		if (tl->active) last_active_task = tl->hwnd;
		return TRUE;
	}

	void DelTasklist(void)
	{
		freeall(&TL);
	}

	void NewTasklist(void)
	{
		DelTasklist();
		EnumTasks(task_enum_func, (LPARAM)this);
		reverse_list(&TL);
	}

	struct tasklist * GetTaskPtrEx(int pos)
	{
		struct tasklist *tl = TL; int i = 0;
		while (tl) { if (pos == i) break; i++, tl = tl->next; }
		return tl;
	}

	int GetTaskListSizeEx(void)
	{
		struct tasklist *tl = TL; int i = 0;
		while (tl) { i++, tl = tl->next; }
		return i;
	}

	HWND GetTaskWindowEx(int i)
	{
		struct tasklist *tl = GetTaskPtrEx(i);
		return tl ? tl->hwnd : NULL;
	}

	// --------------------------------------------------------------
#if 1
	static int GetTraySize(void)
	{
		return ::GetTraySize();
	}

	static systemTray *GetTrayIcon(int i)
	{
		return ::GetTrayIcon(i);
	}
#else
	static BOOL gettraysize_fn(systemTray *icon, LPARAM lParam)
	{
		++*(int*)lParam;
		return TRUE;
	}

	static BOOL gettrayicon_fn(systemTray *icon, LPARAM lParam)
	{
		struct is { int n; systemTray * st; } *ip = (is*)lParam;
		if (0 == ip->n -- ) { ip->st = icon; return FALSE; }
		return TRUE;
	}

	int GetTraySize(void)
	{
		int i = 0;
		EnumTray(gettraysize_fn, (LPARAM)&i);
		return i;
	}

	systemTray *GetTrayIcon(int i)
	{
		struct { int n; systemTray * st; } is = { i, NULL };
		EnumTray(gettrayicon_fn, (LPARAM)&is);
		return is.st;
	}
#endif
};

//==========================================================================={

#include "utils.cpp"
#include "bartender.cpp"
#include "tooltips.cpp"
#include "drawico.cpp"
#include "BuffBmp.cpp"
#include "bbLeanClasses.cpp"
#include "sysmenu.cpp"
#include "TinyDropTarg.cpp"

//===========================================================================
// configuration and menu items

//#define TESTF

enum { R_BOL = 0, R_INT, R_STR } ;
ST bool nobool = false         ;
ST bool tbStyle[3]             ;

#define CFG_255 1
#define CFG_INT 2
#define CFG_TASK 4
#define CFG_STR 8

void barinfo::make_cfg()
{
	struct config c [] = {
	{  "widthPercent" ,     R_INT, (void*)72,             &widthPercent },
	{  "tasks.style" ,      R_INT, (void*)2,              &TaskStyleCfg },
	{  "tasks.reverse" ,    R_BOL, (void*)false,          &reverseTasks },
	{  "tasks.current" ,    R_BOL, (void*)false,          &currentOnlyCfg },
	{  "tasks.sysmenu" ,    R_BOL, (void*)false,          &taskSysmenu },
	{  "tasks.drawBorder" , R_BOL, (void*)false,          &task_with_border },
	{  "icon.saturation" ,  R_INT, (void*)0,              &iconSaturationValue },
	{  "icon.hue" ,         R_INT, (void*)60,             &iconHueIntensity },
	{  "tray.saturation" ,  R_INT, (void*)0,              &traySaturationValue },
	{  "tray.hue" ,         R_INT, (void*)60,             &trayHueIntensity },
	{  "alpha.value" ,      R_INT, (void*)255,            &alphaValue },
	{  "icon.Size" ,        R_INT, (void*)0,              &iconSize },
	{  "strftimeFormat" ,   R_STR, (void*)"%H:%M",        &strftimeFormat },
	{  "strftimeTipsFormat",R_STR, (void*)"%A %d %B %Y",  &strftimeTipsFormat },
	{  "balloonTips" ,      R_BOL, (void*)true,           &enable_balloons },
	{  "autoFullscreenHide",R_BOL, (void*)false,          &hide_in_fullscreen },
	{  "tasks.maxWidth",    R_INT, (void*)0,              &maxTaskItemWidth },
// Noccy: Changed this back to 0 to allow for default behaviour
	{  "balloonTimeout" ,   R_INT, (void*)0,              &balloonTimeout },
	{  "balloonRightDismiss", R_BOL, (void*)false,          &balloonRightButtonDismiss },
	{ NULL,0,NULL,NULL }
	};

	cfg_list = (struct config *)memcpy(new char[sizeof c], c,  sizeof c);

	struct pmenu m [] = {
	{ "Width Percent",          "widthPercent",   CFG_INT, &widthPercent  },

	#define MENUITEM_TBSTYLE 1
	{ "Text only",              "tbStyle0",       CFG_TASK, &tbStyle[0] },
	{ "Icons only",             "tbStyle1",       CFG_TASK, &tbStyle[1] },
	{ "Text and Icons",         "tbStyle2",       CFG_TASK, &tbStyle[2] },
	{ "Reversed",               "reversedTasks",  CFG_TASK, &reverseTasks  },
	{ "Max Item Size",          "maxTaskItemWidth",   CFG_INT|CFG_255, &maxTaskItemWidth },
	#define MENUITEM_CURRENT 6
	{ "Current Only",           "currentOnly",    CFG_TASK, &currentOnlyCfg },
	{ "System Menu",            "sysMenu",        CFG_TASK, &taskSysmenu },
	{ "Draw Border",            "drawBorder",     CFG_TASK, &task_with_border },
	#define MENUITEM_TASK_LAST 9
	{ "Size",                   "iconSize",       CFG_INT, &iconSize  },
	{ "Task Saturation",        "iconSaturation", CFG_INT|CFG_255, &iconSaturationValue  },
	{ "Task Hue",               "iconHue",        CFG_INT|CFG_255, &iconHueIntensity  },
	{ "Tray Saturation",        "traySaturation", CFG_INT|CFG_255, &traySaturationValue  },
	{ "Tray Hue",               "trayHue",        CFG_INT|CFG_255, &trayHueIntensity  },
	#define MENUITEM_SPECIAL 14
	{ "Enable Balloon Tips",    "enableBalloonTips", 0, &enable_balloons  },
	{ "Detect Fullscreen App",  "autoFullscreenHide", 0, &hide_in_fullscreen  },
	#define MENUITEM_NORMAL 16
	{ "Clock Format",           "clockFormat",    CFG_STR, &strftimeFormat  },
	{ "Clock Tips Format",      "clockTipsFormat", CFG_STR, &strftimeTipsFormat  },
	#define MENUITEM_LAST 18
	{ NULL,NULL,0,NULL }
	};

	cfg_menu = (struct pmenu *)memcpy(new char [sizeof m], m,  sizeof m);
}


//===========================================================================
// 0.90 emulation code

ST BOOL update_task_flag_fn(struct tasklist *tl, LPARAM lParam)
{
	struct ts { HWND hwnd; LPARAM lParam; } *tp = (ts*)lParam;
	if (tl->hwnd == tp->hwnd)
		tl->flashing = TASKITEM_FLASHED == tp->lParam;

	if (TASKITEM_ACTIVATED == tp->lParam)
		tl->active = tl->hwnd == tp->hwnd;
	return TRUE;
}

ST void set_task_flags(HWND hwnd, UINT lParam)
{
	// skip odd message sent by the toolbar
	if (hwnd && FALSE == IsWindow(hwnd))
		return;

	struct ts { HWND hwnd; LPARAM lParam; } ts = { hwnd, lParam };
	EnumTasks(update_task_flag_fn, (LPARAM)&ts);
}

//===========================================================================
void barinfo::set_fullscreen_timer(void)
{
	if (this->hide_in_fullscreen && false == this->autoHide)
		SetTimer(hwnd, FULLSCREEN_CHECK_TIMER, 5000, NULL);
	else
		KillTimer(hwnd, FULLSCREEN_CHECK_TIMER);

}

void barinfo::set_screen_info(void)
{
	DesktopInfo DI;
	GetDesktopInfo(&DI);
	currentScreen = DI.number;
	strcpy(screenName, DI.name);
}

//===========================================================================
void barinfo::set_clock_string (void)
{
	time_t systemTime;
	time(&systemTime);
	struct tm *ltp = localtime(&systemTime);
	strftime(clockTime, sizeof clockTime, strftimeFormat, ltp);
	strftime(clockTimeTips, sizeof clockTimeTips, strftimeTipsFormat, ltp);

	SYSTEMTIME lt;
	GetLocalTime(&lt);
	bool seconds = strstr(strftimeFormat, "%S") || strstr(strftimeFormat, "%#S");
	SetTimer(this->hwnd, CLOCK_TIMER, seconds ? 1100 - lt.wMilliseconds : 61000 - lt.wSecond * 1000, NULL);
}

//===========================================================================
void barinfo::set_desktop_margin (void)
{
	bool b = this->ypos  < (this->mon_rect.bottom + this->mon_rect.top) / 2;
	int m = 0;
	if (false == this->autoHide && false == this->hidden)
	{
		m = b
			? this->ypos + this->height
			: this->mon_rect.bottom - this->ypos
			;

		if (false == this->alwaysOnTop) m += 4;
	}
	SetDesktopMargin(this->hwnd, b ? BB_DM_TOP : BB_DM_BOTTOM, m);
}

//==========================================================================={

void barinfo::update_bar(int n)
{
	InvalidateRect(this->hwnd, NULL, FALSE);
	update_flag |= n;
}

void barinfo::update_windowlabel(void)
{
	windowlabel[0] = 0;
	struct tasklist *tl;
	dolist (tl, GetTaskListPtrEx())
		if (tl->active)
		{
			strcpy_max(windowlabel, tl->caption, sizeof windowlabel);
			break;
		}
}

//===========================================================================
ST void drop_style(HDROP hdrop)
{
	char filename[MAX_PATH]; filename[0]=0;
	DragQueryFile(hdrop, 0, filename, sizeof(filename));
	DragFinish(hdrop);
	SendMessage(BBhwnd, BB_SETSTYLE, 1, (LPARAM)filename);
}

bool check_fullscreen_window(HMONITOR hMon)
{
	HWND fg_hwnd = GetForegroundWindow();
	LONG style = GetWindowLong(fg_hwnd, GWL_STYLE);
	if (WS_CAPTION == (style & WS_CAPTION))
		return false;
	RECT s;
	if (hMon != GetMonitorRect(fg_hwnd, &s, GETMON_FROM_WINDOW))
		return false;
	// check for just maximized does not work for e.g. Firefox
	RECT r; GetWindowRect(fg_hwnd, &r);
/*
	dbg_printf("%d/%d %d/%d - %d/%d %d/%d - %d",
		r.left, r.top, r.right, r.bottom,
		s.left, s.top, s.right, s.bottom,
		hide_flag
		);
*/
	return r.right - r.left >= s.right - s.left
		&& r.bottom - r.top >= s.bottom - s.top;
}

/*ST bool check_mouse(HWND hwnd)
{
	POINT pt; RECT rct;
	GetCursorPos(&pt);
	GetWindowRect(hwnd, &rct);
	return PtInRect(&rct, pt);
}*/

#ifdef XOB
void init_tasks(HWND _LBhwnd, HINSTANCE hInst);
void exit_tasks(void);
#endif

//===========================================================================
LRESULT barinfo::wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret)
{
	static int msgs[] =
	{
		BB_TASKSUPDATE,
		BB_TRAYUPDATE,
		BB_DESKTOPINFO,
		BB_SETTOOLBARLABEL,
		BB_REDRAWGUI,
		BB_TOGGLETRAY,
		0
	};

	//dbg_printf("hwnd %04x  msg %5d  wp %08x  lp %08x", hwnd, message, wParam, lParam);
	if (ret)
	{
		if (WM_EXITSIZEMOVE == message)
		{
			if (this->place != POS_User && this->place < POS_CenterLeft)
				old_place = this->place;
		}
		return *ret;
	}

	switch (message)
	{
		//=============================================
		case WM_ERASEBKGND:
			return TRUE;

		//=============================================
		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			HDC hdc = BeginPaint(hwnd, &ps);
			HDC buf = CreateCompatibleDC(NULL);
			HGDIOBJ other = SelectObject(buf, CreateCompatibleBitmap(hdc, this->width, this->height));

			RECT r = {0, 0, this->width, 0};
			for (int i = 0; i < BarLines; i++)
			{
				int o = bbLeanBarLineHeight[i];
				r.bottom = r.top + o;
				StyleItem *T = (StyleItem *)GetSettingPtr(SN_TOOLBAR);
				this->pBuff->MakeStyleGradient(buf, &r, T, true);
				r.top += o - styleBorderWidth;
			}

			hdcPaint = buf;
			p_rcPaint = &ps.rcPaint;
			this->pLeanBar->draw();
			this->pLeanBar->settip();
			ClearToolTips(hwnd);
			this->pBuff->ClearBitmaps();
			update_flag = 0;

			BitBltRect(hdc, buf, &ps.rcPaint);
			DeleteObject(SelectObject(buf, other));
			DeleteDC(buf);
			EndPaint(hwnd, &ps);
			return 0;
		}

		//=============================================
		case WM_CREATE:
			SendMessage(BBhwnd, BB_REGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
#ifdef XOB
			init_tasks(hwnd, this->hInstance);
#endif
			this->pBuff = new BuffBmp;
			this->pLeanBar = new LeanBar(this);

#ifdef WITH_EXTENDERS
			GetExtenderList();
#endif
			GetRCSettings();
			currentOnly = currentOnlyCfg;
			TaskStyle = TaskStyleCfg;

			GetStyleSettings();
			this->NewTasklist();
			this->update_windowlabel();
			set_screen_info();
			this->set_clock_string();
			this->update_bar(1);
			this->set_fullscreen_timer();
			break;

		//=============================================
		case WM_DESTROY:
			SendMessage(BBhwnd, BB_UNREGISTERMESSAGE, (WPARAM)hwnd, (LPARAM)msgs);
			SetDesktopMargin(hwnd, 0, 0);
			delete this->pLeanBar;
			delete this->pBuff;
			if (hFont) DeleteObject(hFont), hFont = NULL;
			DelTasklist();
			this->reset_tbinfo();
#ifdef XOB
			exit_tasks();
#endif
			break;

		//=============================================
		case BB_TOGGLETRAY:
			BBP_set_hidden(this, false == this->hidden);
			break;

		//=============================================
		case BB_REDRAWGUI:
			if (wParam & BBRG_TOOLBAR)
			{
				force_button_pressed = 0 != (wParam & BBRG_PRESSED);
				GetStyleSettings();
				this->set_clock_string();
				this->update_bar(1);
			}
			break;

		//=============================================
		// If Blackbox sends a reconfigure message, load the new settings and update window...

		case BB_RECONFIGURE:
			GetExtenderList();
			GetRCSettings();
			GetStyleSettings();
			this->NewTasklist();
			this->update_windowlabel();
			set_screen_info();
			this->set_clock_string();
			this->update_bar(1);
			this->set_fullscreen_timer();
			break;

		//=============================================
		// Desktop or Desktop Name changed

		case BB_DESKTOPINFO:
			if (((DesktopInfo *)lParam)->isCurrent)
			{
				set_screen_info();
				this->NewTasklist();
				this->update_bar(2);
			}
			break;

		//=============================================
		// Task was added/removed/activated/has changed text

		case BB_TASKSUPDATE:
			if (false == is_bblean) set_task_flags((HWND)wParam, lParam);

			this->NewTasklist();

			if (false == ShowingExternalLabel)
				this->update_windowlabel();

			this->update_bar(2);

			if (false == this->toggled_hidden)
			{
				bool hide_flag = this->hide_in_fullscreen
					&& false == this->autoHide
					&& check_fullscreen_window(this->hMon);

                if ( this->autoHide == true && lParam == TASKITEM_FLASHED ) {
                    BBP_set_hidden(this, true);
                } else if ( this->autoHide == true && lParam == TASKITEM_ACTIVATED ) {
                    BBP_set_hidden(this, false);
                } else {
                    BBP_set_hidden(this, hide_flag);
                }
			}
			break;

		//=============================================
		// Tray Icon was added/removed/has changed tooltip

		case BB_TRAYUPDATE:
			//dbg_printf("BB_TRAYUPDATE %x %d", wParam, lParam);
			if (TRAYICON_MODIFIED == lParam)
			{
				if (NIF_TIP == wParam || NIF_INFO == wParam)
				{
					this->pLeanBar->settip();
					ClearToolTips(hwnd);
				}
				else
				{
					this->pLeanBar->invalidate_item(M_TRAYLIST);
				}
			}
			else
			{
				this->update_bar(4);
			}
			break;

		//=============================================
		case WM_TIMER:
			if (CLOCK_TIMER == wParam)
			{
				this->set_clock_string();
				//pLeanBar->invalidate_item(M_CLCK);
				this->update_bar(1);
				break;
			}

			if (TASK_RISE_TIMER == wParam)
			{
				handle_task_timer(m_TinyDropTarget);
				break;
			}

			if (FULLSCREEN_CHECK_TIMER == wParam)
			{
				if (false == this->toggled_hidden)
				{
					bool hide_flag = this->hide_in_fullscreen
						&& false == this->autoHide
						&& check_fullscreen_window(this->hMon);

					BBP_set_hidden(this, hide_flag);
				}
				break;
			}

			KillTimer(hwnd, wParam);

			if (LABEL_TIMER == wParam)
			{
				ShowingExternalLabel = false;
				update_windowlabel();
				//pLeanBar->invalidate_item(M_WINL);
				this->update_bar(1);
				break;
			}
			break;

		//=============================================
		case BB_DRAGOVER:
			task_over_hwnd = NULL;
			if (this->auto_hidden)
				BBP_set_autoHide(this, true);
			else
				this->pLeanBar->mouse_event((short)LOWORD(lParam), (short)HIWORD(lParam), message, wParam);
			return (LRESULT)task_over_hwnd;

		//=============================================
		case BB_SETTOOLBARLABEL:
			if (NULL == strchr(item_string, M_WINL))
				break;

			SetTimer(hwnd, LABEL_TIMER, 2000, (TIMERPROC)NULL);
			ShowingExternalLabel = true;
			strcpy_max(windowlabel, (LPCSTR)lParam, sizeof windowlabel);
			//pLeanBar->invalidate_item(M_WINL);
			this->update_bar(1);
			break;

		//=============================================
		// R.S.: please dont touch
		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE;

		//=============================================
		case WM_NCRBUTTONUP:
			this->show_menu(true);
			break;

		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
			//if (0x8000 & GetAsyncKeyState(VK_CONTROL))
			if (wParam & MK_CONTROL)
			{
				if (WM_RBUTTONUP == message)
					this->show_menu(true);
				break;
			}
			goto mouse;

		case WM_LBUTTONDBLCLK:
			if (wParam & MK_CONTROL)
			{
				BBP_set_place(this, old_place);
				break;
			}
			goto mouse;

		case WM_MOUSEWHEEL:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:

		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		mouse:
			this->pLeanBar->mouse_event((short)LOWORD(lParam), (short)HIWORD(lParam), message, wParam);
			break;

		//=============================================
		case WM_DROPFILES:
			drop_style((HDROP)wParam);
			break;

		/*case NIN_BALLOONSHOW:
			((msg_balloon*)lParam)->show();
			break;*/

		//=============================================
		default:
			return DefWindowProc(hwnd,message,wParam,lParam);

	//====================
	}
	return 0;
}

//===========================================================================
void GetMouseEventSettings(const char *rcpath, const char *rc_key, const char *label, const char **events)
{
	int i;
	for (i = 0; i < NB_MOUSE_EVENT; i++)
	{
		static const char *buttons[]  = { "Left", "Mid", "Right", "Dbl" };
		if (events[i]) m_free(events[i]), events[i] = NULL;
		char rcstring[MAX_PATH];
		sprintf(rcstring, "%s.%s.%sClick:", rc_key, label, buttons[i]);
		const char *event = ReadValue(rcpath, rcstring);
		events[i] = event ? new_str(event) : NULL;
	}
}

//===========================================================================
// Function: GetRCSettings
//===========================================================================
void barinfo::GetRCSettings()
{
	if (false == BBP_read_window_modes(this, szAppName))
		this->place = POS_TopCenter;

	struct config *p = cfg_list;
	do switch (p->mode) {
		case R_BOL:
			*(bool*)p->ptr = BBP_read_bool(this, p->str, (bool)(int)p->def);
			break;

		case R_INT:
			*(int*)p->ptr  = BBP_read_int(this, p->str, (int)p->def);
			break;

		case R_STR:
			BBP_read_string(this, (char*)p->ptr, p->str, (char*)p->def);
			break;

	} while ((++p)->str);

	ICOCLICKSLEEP = BBP_read_int(this, "tray.focusdelay", 60);
	pSettings_arrowUnix = ReadBool(extensionsrcPath(), "blackbox.appearance.arrow.unix:", true);

	// -----------------------------------------------------------------
	// read the bbleanbar.items

	char *ip = item_string; LONG pos = 0;
	char item_key[80];
	sprintf(item_key, "%s.item:", this->rc_key);

	for (;;)
	{
		int i;
		const char *c = ReadValue(this->rcpath, item_key, &pos);
		if (NULL == c) break;

		//dbg_printf("c=<%s> pos=%d", c, pos);

		static const char *baritemstrings[] = {
			"Tasks" ,
			"Tray" ,
			"WorkspaceLabel" ,
			"Clock" ,
			"WindowLabel" ,
			"Space" ,
			"NewLine" ,
			"CurrentOnlyButton" ,
			"TaskStyleButton" ,
			"WorkspaceButtonL" ,
			"WorkspaceButtonR" ,
			"WindowButtonL" ,
			"WindowButtonR" ,
			NULL
		};
		if (0 != (i = 1 + get_string_index(c, baritemstrings)))
			*ip++ = (char)i;
	}
	*ip = 0;
	if (item_string == ip)
	{
		static char default_items[] =
		{
			M_WSPL,
			M_WSPB_L,
			M_WSPB_R,
			M_TASK,
			M_CUOB,
			M_TRAY,
			M_WINB_L,
			M_WINB_R,
			M_CLCK,
			0
		};
		strcpy(ip, default_items);
	}
	// -----------------------------------------------------------------
	update_flag |= 15;

	// -----------------------------------------------------------------
	// Labels click actions
	static const char _SHOWMENU[] = "@BBCore.ShowMenu";
	GetMouseEventSettings(this->rcpath, this->rc_key, "WorkspaceLabel", me_workspace);
	if(NULL == me_workspace[ME_LEFT])  me_workspace[ME_LEFT]  = new_str("@BBCore.RightWorkspace");
	if(NULL == me_workspace[ME_MID])   me_workspace[ME_MID]   = new_str("@BBCore.ShowMenu workspaces");
	if(NULL == me_workspace[ME_RIGHT]) me_workspace[ME_RIGHT] = new_str("@BBCore.LeftWorkspace");

	GetMouseEventSettings(this->rcpath, this->rc_key, "Clock", me_clock);
	if(NULL == me_clock[ME_RIGHT]) me_clock[ME_RIGHT] = new_str(_SHOWMENU);
	if(NULL == me_clock[ME_DBL])   me_clock[ME_DBL]   = new_str("control.exe timedate.cpl");

	GetMouseEventSettings(this->rcpath, this->rc_key, "WindowLabel", me_window);
	if(NULL == me_window[ME_RIGHT]) me_window[ME_RIGHT] = new_str(_SHOWMENU);

	// -----------------------------------------------------------------
	// Buttons click actions
	GetMouseEventSettings(this->rcpath, this->rc_key, "CurrentOnlyButton", me_curOnly);
	if (me_curOnly[ME_LEFT]) m_free(me_curOnly[ME_LEFT]), me_curOnly[ME_LEFT] = NULL;

	GetMouseEventSettings(this->rcpath, this->rc_key, "TaskStyleButton", me_taskSty);
	if (me_taskSty[ME_LEFT]) m_free(me_taskSty[ME_LEFT]), me_taskSty[ME_LEFT] = NULL;

	GetMouseEventSettings(this->rcpath, this->rc_key, "WorkspaceButtonL", me_wspb_l);
	if(NULL == me_wspb_l[ME_LEFT]) me_wspb_l[ME_LEFT] = new_str("@BBCore.LeftWorkspace");

	GetMouseEventSettings(this->rcpath, this->rc_key, "WorkspaceButtonR", me_wspb_r);
	if(NULL == me_wspb_r[ME_LEFT]) me_wspb_r[ME_LEFT] = new_str("@BBCore.RightWorkspace");

	GetMouseEventSettings(this->rcpath, this->rc_key, "WindowButtonL", me_wnd_l);
	if(NULL == me_wnd_l[ME_LEFT]) me_wnd_l[ME_LEFT] = new_str("@BBCore.PrevWindow");

	GetMouseEventSettings(this->rcpath, this->rc_key, "WindowButtonR", me_wnd_r);
	if(NULL == me_wnd_r[ME_LEFT]) me_wnd_r[ME_LEFT] = new_str("@BBCore.NextWindow");
}

//===========================================================================
// Function: WriteRCSettings
//===========================================================================
void barinfo::WriteRCSettings()
{
	struct config *p = cfg_list;
	do switch (p->mode) {
		case R_BOL:
			BBP_write_bool(this, p->str, *(bool*)p->ptr);
			break;

		case R_INT:
			BBP_write_int(this, p->str, *(int*)p->ptr);
			break;

		case R_STR:
			BBP_write_string(this, p->str, (char*)p->ptr);
			break;

	} while ((++p)->str);
}

//===========================================================================
// Function: GetStyleSettings
//===========================================================================

void barinfo::GetStyleSettings()
{
	StyleItem * T = (StyleItem *)GetSettingPtr(SN_TOOLBAR);
	StyleItem * W = (StyleItem *)GetSettingPtr(SN_TOOLBARWINDOWLABEL);
	StyleItem * L = (StyleItem *)GetSettingPtr(SN_TOOLBARLABEL);
	StyleItem * C = (StyleItem *)GetSettingPtr(SN_TOOLBARCLOCK);
	StyleItem * B = (StyleItem *)GetSettingPtr(SN_TOOLBARBUTTON);
	//StyleItem * BP = (StyleItem *)GetSettingPtr(SN_TOOLBARBUTTONP);
	StyleItem * X;

	if (T->validated & VALID_TEXTCOLOR) X = T;
	else
	if (W->parentRelative)          X = W;
	else
	if (L->parentRelative)          X = L;
	else
	if (B->parentRelative)          X = B;
	else                            X = L;

	Color_T = X->TextColor;

	if (false == W->parentRelative) X = W, SN_A = SN_TOOLBARWINDOWLABEL;
	else
	if (false == L->parentRelative) X = L, SN_A = SN_TOOLBARLABEL;
	else
	if (false == C->parentRelative) X = C, SN_A = SN_TOOLBARCLOCK;
	else                            X = W, SN_A = SN_TOOLBARWINDOWLABEL;

	Color_A = X->TextColor;

	TBJustify = T->Justify | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX;

	if (hFont) DeleteObject(hFont), hFont = NULL;
	hFont = CreateStyleFont(T);

	if (is_bblean && T->nVersion >= 2)
	{
		styleBevelWidth     = T->marginWidth;
		styleBorderWidth    = T->borderWidth;
		styleBorderColor    = T->borderColor;
	}
	else
	{
		styleBevelWidth     = *(int*)GetSettingPtr(SN_BEVELWIDTH);
		styleBorderWidth    = *(int*)GetSettingPtr(SN_BORDERWIDTH);
		styleBorderColor    = *(COLORREF*)GetSettingPtr(SN_BORDERCOLOR);
	}

	if (0==styleBorderWidth) styleBorderColor = T->Color;

	pSettings_bulletUnix = *(bool*)GetSettingPtr(SN_BULLETUNIX);

	bool newMetrics = (bool)GetSettingPtr(SN_NEWMETRICS);

	if (newMetrics || (L->validated & VALID_MARGIN))
	{
		int fontH = get_fontheight(hFont);
		if (fontH < iconSize - 2) fontH = iconSize - 2;
		else if (-1 == iconSize) iconSize = fontH + 2;
		if (L->validated & VALID_MARGIN)
			labelH = fontH + 2 * L->marginWidth;
		else
			labelH = imax(10, fontH) + 2 * 2;
	}
	else
		if(0 == iconSize)
			labelH = imax(8, T->FontHeight) + 2;
		else
			labelH = imax(iconSize, T->FontHeight + 2);
	labelH |= 1;

	if (B->validated & VALID_MARGIN)
	{
		buttonH = 9 + 2 * B->marginWidth;
	}
	else
	{
		buttonH = labelH - 2;
	}

	int minH = imax(labelH, buttonH) - 2;

	if (minH & 1) ++minH;

	int b = (styleBorderWidth + styleBevelWidth);

	int h0 = minH, h1, h2;
	T_ICON_SIZE = S_ICON_SIZE = iconSize ? iconSize : minH;
	h1 = h2 = imax(minH, iconSize);

	h0 += 2;
	h1 += 2;
	h2 += 2;

	// calculate the bar height(s)
	BarLines = 0;
	this->height = 0;
	has_tasks = false;
	char c, *ip = item_string;
	for (;;)
	{
		// check, if a line has an icon
		int h = h0;
		for (;0 != (c = *ip++) && M_NEWLINE != c;)
		{
			if (M_TASK == c)
			{
				 has_tasks = true;
				 if (h < h1) h = h1;
			}
			if (M_TRAY == c)
			{
				 if (h < h2) h = h2;
			}
		}

		h += b + b;
		this->height += h;
		bbLeanBarLineHeight[BarLines] = h;
		BarLines ++;
		if (0 == c) break;
		this->height -= styleBorderWidth;
	}

	// set location on the screen
	this->width = (this->mon_rect.right - this->mon_rect.left) * widthPercent / 100;

	BBP_reconfigure(this);
	on_multimon = GetSystemMetrics(SM_CMONITORS) > 1;

	//dbg_printf("Monitors: %d %d", GetSystemMetrics(SM_CMONITORS), on_multimon);
}

//===========================================================================
void barinfo::process_broam(const char *temp, int f)
{
	if (f)
	{
		show_menu(false);
		PostMessage(hwnd, BB_RECONFIGURE, 0, 0);
		return;
	}

	if (0 == stricmp(temp, "About"))
	{
		char buff[256];
		sprintf(buff, aboutmsg, pluginInfo(0));
		MessageBox(NULL, buff, szAppName, MB_OK|MB_TOPMOST|MB_SETFOREGROUND);
		return;
	}

	//-----------------------------------
	// check config menu messages
	struct pmenu *cp;

	for (cp = cfg_menu; cp->displ; cp++)
	{
		int n = strlen(cp->msg);
		if (n && 0==memicmp(temp, cp->msg, n))
		{
			if (cp->ptr)
			{
				if (cp->f & CFG_INT)
					*(int*)cp->ptr = atoi(temp+n+1);
				else
				if (cp->f & CFG_STR)
					strcpy((char*)cp->ptr, temp+n+1);
				else
					*(bool*)cp->ptr = false == *(bool*)cp->ptr;
			}

			int f = cp - cfg_menu;

			if (f >= MENUITEM_TBSTYLE && f < MENUITEM_TBSTYLE+3)
				TaskStyle = TaskStyleCfg = f - MENUITEM_TBSTYLE;

			if (f == MENUITEM_CURRENT)
				currentOnly = currentOnlyCfg;

			WriteRCSettings();
			update_flag |= 8;
			show_menu(false);
			PostMessage(hwnd, BB_RECONFIGURE, 0, 0);
			return;
		}
	}

	HWND task_hwnd = NULL;
	int task_msg = 0;

	if (2 == sscanf(temp, "SysCommand <%d> <%d>", (int*)&task_hwnd, (int*)&task_msg))
	{
		//dbg_printf("hwnd %x  msg %x", sub_msg, task_hwnd, task_msg);
		PostMessage(task_hwnd, WM_SYSCOMMAND, (WPARAM)task_msg, 0);
		return;
	}

	char class_name[256]; int uID;
	if (2 == sscanf(temp, "toggleTrayIcon %d %s", &uID, class_name))
	{
		dbg_printf("toggleTrayIcon %d %s", uID, class_name);
		return;
	}
}

//===========================================================================
// Function: show_menu
// Purpose: ...
// In: void
// Out: void

void barinfo::insert_cfg_menu_item(n_menu *cSub, struct pmenu *cp)
{
	if (cp->f & CFG_TASK && false == has_tasks)
		;
	else
	if (cp->f & CFG_INT)
		n_menuitem_int(cSub, cp->displ, cp->msg, *(int*)cp->ptr, 0, cp->f & CFG_255 ? 255 : 100);
	else
	if (cp->f & CFG_STR)
		n_menuitem_str(cSub, cp->displ, cp->msg, (char*)cp->ptr);
	else
	if (0 == *cp->displ)
		n_menuitem_nop(cSub, NULL);
	else
		n_menuitem_bol(cSub, cp->displ, cp->msg, cp->ptr ? *(bool*)cp->ptr : false);
}

void barinfo::show_menu(bool pop)
{
	n_menu *myMenu, *cSub, *wSub; int i;

	for (i= 0; i<3; i++) tbStyle[i] = i == TaskStyleCfg;
	nobool = false;

	myMenu = n_makemenu(szAppName);

	if (false == this->inSlit) BBP_n_placementmenu(this, myMenu);

	cSub = n_submenu(myMenu, "Configuration");
	struct pmenu *cp = cfg_menu;
	insert_cfg_menu_item(cSub, cp++);
	BBP_n_insertmenu(this, cSub);

	n_menuitem_nop(cSub, NULL);
	n_menu *m = cSub;
	i = 1;
	do {
		if (i == MENUITEM_TBSTYLE) m = n_submenu(cSub, "Tasks");
		insert_cfg_menu_item(m, cp);
		i++;
		if (i == MENUITEM_TASK_LAST) m = n_submenu(cSub, "Icons");
		if (i == MENUITEM_SPECIAL) m = n_submenu(cSub, "Special");
		if (i == MENUITEM_NORMAL) m = cSub;
	} while ((++cp)->displ);

	wSub = n_submenu(myMenu, "Window");
	n_menuitem_cmd(wSub, "Minimize All", "@BBCore.MinimizeAll");
	n_menuitem_cmd(wSub, "Restore All", "@BBCore.RestoreAll");
	n_menuitem_cmd(wSub, "Cascade", "@BBCore.Cascade");
	n_menuitem_cmd(wSub, "Tile Horizontal", "@BBCore.TileHorizontal");
	n_menuitem_cmd(wSub, "Tile Vertical", "@BBCore.TileVertical");
#if 0
	wSub = n_submenu(myMenu, "Hide TrayIcons");
	for (i = 0, s = GetTraySize(); i < s; ++i)
	{
		char class_name[80];
		char buffer[100];
		char command[100];
		systemTray *icon = GetTrayIcon(i);
		GetClassName(icon->hWnd, class_name, sizeof class_name);
		sprintf(buffer, "%s", class_name);
		sprintf(command, "toggleTrayIcon %d %s", icon->uID, class_name);
		n_menuitem_cmd(wSub, buffer, command);
	}
#endif
	n_menuitem_nop(myMenu, NULL);
	n_menuitem_cmd(myMenu, "Edit Workspace Names", "@BBCore.EditWorkspaceNames");
	n_menuitem_cmd(myMenu, "Edit Settings", "EditRc");
	n_menuitem_cmd(myMenu, "About", "About");
	n_showmenu(myMenu, this->broam_key, pop);
}

//===========================================================================

//===========================================================================
void barinfo::pos_changed(void)
{
	set_desktop_margin();
/*
	ToolbarInfo *TBInfo = GetToolbarInfo();
	if (NULL == TBInfo) return;

	if (NULL == TBInfo->hwnd)
		TBInfo->hwnd = hwnd;

	if (TBInfo->hwnd != hwnd)
		return;

	TBInfo->placement        = place - POS_TopLeft + PLACEMENT_TOP_LEFT;
	TBInfo->widthPercent     = widthPercent;
	TBInfo->onTop            = alwaysOnTop;
	TBInfo->autoHide         = autoHide;
	TBInfo->pluginToggle     = pluginToggle;
	TBInfo->disabled         = false;

	TBInfo->alphaValue       = alpha;
	TBInfo->transparency     = alpha < 255;

	TBInfo->autohidden       = auto_hidden;
	TBInfo->hidden           = hidden;

	TBInfo->xpos             = xpos;
	TBInfo->ypos             = ypos;
	TBInfo->width            = width;
	TBInfo->height           = height;
	TBInfo->hwnd             = hwnd;

	if (TBInfo->bbsb_hwnd)
		PostMessage(TBInfo->bbsb_hwnd, BB_TOOLBARUPDATE, 0, 0);
*/
}

void barinfo::reset_tbinfo(void)
{
/*
	ToolbarInfo *TBInfo = GetToolbarInfo();
	if (TBInfo && TBInfo->hwnd == hwnd)
	{
		TBInfo->hwnd = NULL;
		TBInfo->disabled = TBInfo->hidden = true;
		PostMessage(BBhwnd, BB_TOOLBARUPDATE, 0, 0);
	}
*/
}

//===========================================================================

int beginPluginEx(HINSTANCE hPluginInstance, HWND hSlit)
{
	if (NULL == BBhwnd)
	{
		BBhwnd = GetBBWnd();
		*(FARPROC*)&pAllowSetForegroundWindow = GetProcAddress(GetModuleHandle("USER32"), "AllowSetForegroundWindow");
		is_bblean = 0 == memicmp(GetBBVersion(), "bblean", 6);

		HWND traywnd = NULL;
		for (;;)
		{
			traywnd = FindWindowEx(NULL, traywnd, "Shell_TrayWnd", NULL);
			if (NULL == traywnd) break;
			if (GetWindowThreadProcessId(traywnd, NULL) == GetCurrentThreadId())
			{
				tray_notify_wnd = FindWindowEx(traywnd, NULL, "TrayNotifyWnd", NULL);
				tray_clock_wnd = FindWindowEx(tray_notify_wnd, NULL, "TrayClockWClass", NULL);
			/*
				dbg_printf("systray_wnd %x", traywnd);
				dbg_printf("tray_notify_wnd %x", tray_notify_wnd);
				dbg_printf("tray_clock_wnd %x",  tray_clock_wnd );
			*/
				break;
			}
		}
	}

	m_hInstance = hPluginInstance;

	struct plugin_info *p = g_PI;
	int instance_index = 0;
	while (p) ++instance_index, p = p->next;

	struct barinfo *PI = new barinfo;
	BBP_clear(PI);

	sprintf(PI->instance_name,
		instance_index ? "%s.%d" : "%s",
		szAppName,
		1+instance_index
		);

	//dbg_printf("instance_name: <%s>", PI->instance_name);

	PI->hSlit       = hSlit;
	PI->hInstance   = hPluginInstance;
	PI->class_name  = szAppName;
	PI->rc_key      = PI->instance_name;
	PI->broam_key   = PI->instance_name;
	PI->is_bar      = true;
	PI->m_index     = instance_index;

	PI->make_cfg();

	if (0 == BBP_Init_Plugin(PI))
	{
		delete PI;
		return 1;
	}

	if (NULL == g_PI)
	{
		if (false == is_bblean) OleInitialize(NULL);
		init_tooltips();
	}

	PI->old_place = PI->place;
	PI->m_TinyDropTarget = init_drop_targ(PI->hwnd);

	PI->next = g_PI;
	g_PI = PI;

	return 0;
}

int beginSlitPlugin(HINSTANCE hPluginInstance, HWND hSlit)
{
	return beginPluginEx(hPluginInstance, hSlit);
}

int beginPlugin(HINSTANCE hPluginInstance)
{
	return beginPluginEx(hPluginInstance, NULL);
}

void endPlugin(HINSTANCE hPluginInstance)
{
	struct plugin_info *PI = g_PI;
	int i;
	for(i = 0; i < NB_MOUSE_EVENT; ++i)
	{
		m_free(me_workspace[i]);
		m_free(me_window[i]);
		m_free(me_clock[i]);
		m_free(me_wspb_l[i]);
		m_free(me_wspb_r[i]);
		m_free(me_wnd_l[i]);
		m_free(me_wnd_r[i]);
	}

	if (PI)
	{
		g_PI = PI->next;
		delete PI;
		if (NULL == g_PI)
		{
			DestroyWindow(hToolTips);
			if (false == is_bblean) OleUninitialize();
		}
	}
}

//===========================================================================
