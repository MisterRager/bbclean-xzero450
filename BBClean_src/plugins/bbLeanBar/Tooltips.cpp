//===========================================================================
// Tooltips

#include "bbLeanBar.h"

void debugPrint(const char *fmt, ...)
{
	char buffer[256]; va_list arg;
	va_start(arg, fmt);
	vsprintf (buffer, fmt, arg);
	strcat(buffer, "\n");
	OutputDebugString(buffer);
}


ST void init_tooltips(void)
{
	debugPrint("init_tooltips invoked");

	INITCOMMONCONTROLSEX ic;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_BAR_CLASSES;

	InitCommonControlsEx(&ic);

	hToolTips = CreateWindowEx(
		WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL);

	SendMessage(hToolTips, TTM_SETMAXTIPWIDTH, 0, 300);
	SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOMATIC, 300);
	SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, 5000);
	SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_INITIAL,  120);
	SendMessage(hToolTips, TTM_SETDELAYTIME, TTDT_RESHOW,    60);
}

//===========================================================================
// Function: SetToolTip
// Purpose: To assign a ToolTip to an icon in the system tray
// In:      the position of the icon, the text
// Out:     void
//===========================================================================

struct tt
{
	struct tt *next;
	char used_flg;
	char text[256];
	TOOLINFO ti;
} *tt0;

void SetToolTip(HWND hwnd, RECT *tipRect, char *tipText)
{
	if (NULL==hToolTips || 0 == *tipText) return;

	struct tt **tp, *t; unsigned n=0;
	for (tp=&tt0; NULL!=(t=*tp); tp=&t->next)
	{
		if (hwnd == t->ti.hwnd && 0==memcmp(&t->ti.rect, tipRect, sizeof(RECT)))
		{
			t->used_flg = 1;
			if (0!=strcmp(t->ti.lpszText, tipText))
			{
				strcpy(t->text, tipText);
				SendMessage(hToolTips, TTM_UPDATETIPTEXT, 0, (LPARAM)&t->ti);
			}
			return;
		}
		if (t->ti.uId > n)
			n = t->ti.uId;
	}

	t = new struct tt;
	t->used_flg  = 1;
	t->next = NULL;
	strcpy(t->text, tipText);
	*tp = t;

	memset(&t->ti, 0, sizeof(TOOLINFO));

	t->ti.cbSize   = sizeof(TOOLINFO);
	t->ti.uFlags   = TTF_SUBCLASS;
	t->ti.hwnd     = hwnd;
	t->ti.uId      = n+1;
	//t->ti.hinst    = NULL;
	t->ti.lpszText = t->text;
	t->ti.rect = *tipRect;
	SendMessage(hToolTips, TTM_ADDTOOL, 0, (LPARAM)&t->ti);
}

//===========================================================================
// Function: ClearToolTips
// Purpose:  clear all tooltips, which are not longer used
//===========================================================================

void ClearToolTips(HWND hwnd)
{
	struct tt **tp, *t;
	tp=&tt0; while (NULL!=(t=*tp))
	{
		if (hwnd != t->ti.hwnd)
		{
			tp=&t->next;
		}
		else
		if (0==t->used_flg)
		{
			SendMessage(hToolTips, TTM_DELTOOL, 0, (LPARAM)&t->ti);
			*tp=t->next;
			delete t;
		}
		else
		{
			t->used_flg = 0;
			tp=&t->next;
		}
	}
}

//===========================================================================

//===========================================================================
class bb_balloon : public plugin_info
{
	systemTray icon;
	systemTrayBalloon balloon;
	int msgtop;
	int padding;
	int x_icon;
	int y_icon;
	int d_icon;
	int balloon_timer;
	bool finished;


public:
	bb_balloon(plugin_info * mPI, systemTray *pIcon, RECT r)
	{
		debugPrint("bb_balloon:bb_balloon invoked. balloonTimeout=%i.",balloonTimeout);

		BBP_clear(this);

		hInstance   = mPI->hInstance;
		mon_rect    = mPI->mon_rect;
		hMon        = mPI->hMon;
		class_name  = "BBLeanbar_Balloon";
		rc_key      = "none";
		broam_key   = "none";
		alwaysOnTop = true;
		place       = POS_User;
		transparency = mPI->transparency;
		alpha       = mPI->alpha;

		this->icon  = *pIcon;
		this->balloon = *pIcon->pBalloon;
		this->finished = false;

		x_icon = (r.left+r.right)/2;
		y_icon = (r.top+r.bottom)/2;
		d_icon = (r.right - r.left);

		padding     = 8;
		calculate_size();

		BBP_Init_Plugin(this);
	}

private:
	~bb_balloon()
	{
		finished = true;
		BBP_Exit_Plugin(this);
	}

	void calculate_size(void)
	{
		int maxwidth = 240;

		RECT r1 = {0, 0, maxwidth, 0};
		RECT r2 = {0, 0, maxwidth, 0};
		HDC buf = CreateCompatibleDC(NULL);
		draw_text(buf, &r1, &r2, DT_CALCRECT | DT_WORDBREAK);
		DeleteDC(buf);

		if (balloon.szInfoTitle[0]) msgtop = r1.bottom + padding;
		else msgtop = 0;

		width = imax(r1.right, r2.right) + 2*padding;
		height = msgtop + r2.bottom + 2*padding;

		int screen_center_x = (mon_rect.left + mon_rect.right) /2;
		int screen_center_y = (mon_rect.top + mon_rect.bottom) /2;

		int is = d_icon / 5;
		xpos = x_icon;
		ypos = y_icon;
		if (xpos < screen_center_x && xpos - width >= mon_rect.left || xpos + width > mon_rect.right)
			xpos -= width + is;
		else
			xpos += is;

		if (ypos < screen_center_y && ypos - height >= mon_rect.top || ypos + height > mon_rect.bottom)
			ypos -= height + is;
		else
			ypos += is;
	}

	void draw_text(HDC buf, RECT *r1, RECT *r2, UINT flags)
	{
		StyleItem F  = *(StyleItem *)GetSettingPtr(SN_TOOLBAR);
		F.FontWeight = FW_BOLD;
		HFONT hFont1 = CreateStyleFont(&F);
		F.FontWeight = FW_NORMAL;
		HFONT hFont2 = CreateStyleFont(&F);

		HGDIOBJ otherfont = SelectObject(buf, hFont1);
		BBDrawText(buf, balloon.szInfoTitle, -1, r1, flags, &F);
		SelectObject(buf, hFont2);
		BBDrawText(buf, balloon.szInfo, -1, r2, flags, &F);
		SelectObject(buf, otherfont);
		DeleteObject(hFont1);
		DeleteObject(hFont2);
	}

	void paint(HWND hwnd)
	{
		StyleItem *pStyle = (StyleItem *)GetSettingPtr(SN_A);
		COLORREF TextColor = Color_A;
		if (pStyle->parentRelative)
		{
			pStyle = (StyleItem *)GetSettingPtr(SN_TOOLBAR);
			TextColor = Color_T;
		}

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		HDC buf = CreateCompatibleDC(NULL);
		HGDIOBJ other = SelectObject(buf, CreateCompatibleBitmap(hdc, this->width, this->height));
		RECT r1 = {0, 0, this->width, this->height};
		MakeStyleGradient(buf, &r1, pStyle, true);
		r1.top += padding;
		r1.left += padding;
		r1.right -= padding;
		RECT r2 = r1;
		r2.top += msgtop;
		SetBkMode(buf, TRANSPARENT);
		SetTextColor(buf, TextColor);
		draw_text(buf, &r1, &r2, DT_LEFT | DT_WORDBREAK);
		if (balloon.szInfoTitle[0])
		{
			int y = r2.top - padding + 1;
			HGDIOBJ oldPen = SelectObject(buf, CreatePen(PS_SOLID, 1, TextColor));
			MoveToEx(buf, r1.left, y, NULL);
			LineTo  (buf, r1.right, y);
			DeleteObject(SelectObject(buf, oldPen));
		}

		BitBltRect(hdc, buf, &ps.rcPaint);
		DeleteObject(SelectObject(buf, other));
		DeleteDC(buf);
		EndPaint(hwnd, &ps);
	}


	void Post(LPARAM lParam)
	{
		PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
	}

	LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *ret)
	{
		if (ret) return *ret;
		switch (message)
		{
			case WM_PAINT:
				this->paint(hwnd);
				break;


			case WM_CREATE:
				Post(NIN_BALLOONSHOW);
				// we do it like this because if the value is unset (=0) the old code should
				// become active and do things like they used to be done. 8 seconds or more.
				if (balloonTimeout == 0) {
					SetTimer(hwnd, iBalloonTimerId, imax(8000, balloon.uInfoTimeout), NULL);
				} else {
					// If a value is set, we use it rather than the default value
					SetTimer(hwnd, iBalloonTimerId, balloonTimeout, NULL);
				}
				break;

			case WM_DESTROY:
				KillTimer(hwnd,iBalloonTimerId);
				if (false == this->finished) Post(NIN_BALLOONHIDE);
				break;

			case WM_TIMER:
				// Kill the timer and destroy the balloon.
				Post(NIN_BALLOONTIMEOUT);
				KillTimer(hwnd,iBalloonTimerId);
				delete this;
				break;

			case WM_RBUTTONDOWN:
				if (balloonRightButtonDismiss) {
					debugPrint("Dismissing balloon popup!");
					KillTimer(hwnd,iBalloonTimerId);
					Post(NIN_BALLOONTIMEOUT);
					delete this;
					break;
				}
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
				KillTimer(hwnd,iBalloonTimerId);
				Post(NIN_BALLOONUSERCLICK);
				delete this;
				break;

			case BB_RECONFIGURE:
				calculate_size();
				BBP_reconfigure(this);
				break;

			default:
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
		return 0;
	}
};

//===========================================================================
#ifndef TTM_SETTITLEA
#define TTM_SETTITLEA (WM_USER+32)  // wParam = TTI_*, lParam = char* szTitle
#endif

#define TTS_BALLOON     0x40

#define NIIF_NONE       0x00000000
#define NIIF_INFO       0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR      0x00000003

#define TTI_NONE        0
#define TTI_INFO        1
#define TTI_WARNING     2
#define TTI_ERROR       3

class win_balloon
{
	systemTray icon;
	systemTrayBalloon balloon;
	HWND hwndBalloon;
	WNDPROC prev_wndproc;
	bool finished;

public:
	win_balloon(plugin_info * mPI, systemTray *pIcon, RECT r)
	{
		HWND hwndParent = mPI->hwnd;
		HINSTANCE hInstance = mPI->hInstance;

		this->icon  = *pIcon;
		this->balloon = *pIcon->pBalloon;
		this->finished = false;

		int xpos = (r.left+r.right)/2;
		int ypos = (r.top+r.bottom)/2;

		TOOLINFO ti;
		memset(&ti, 0, sizeof ti);
		ti.cbSize   = sizeof(TOOLINFO);
		ti.uFlags   = TTF_TRACK;
		ti.hwnd     = hwndParent;
		ti.uId      = 1;
		ti.lpszText = balloon.szInfo;

		hwndBalloon = CreateWindowEx(
			WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
			TOOLTIPS_CLASS,
			NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
			0,0,0,0,
			hwndParent,
			NULL,
			hInstance,
			NULL
			);

		SendMessage(hwndBalloon, TTM_SETMAXTIPWIDTH, 0, 270);
		SendMessage(hwndBalloon, TTM_ADDTOOL, 0, (LPARAM)&ti);
		SendMessage(hwndBalloon, TTM_SETTITLEA, balloon.dwInfoFlags, (LPARAM)balloon.szInfoTitle);
		SendMessage(hwndBalloon, TTM_TRACKPOSITION, 0, MAKELPARAM(xpos, ypos));
		SendMessage(hwndBalloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

		SetWindowLong(hwndBalloon, GWL_USERDATA, (LONG)this);
		prev_wndproc = (WNDPROC)SetWindowLong(hwndBalloon, GWL_WNDPROC, (LONG)wndproc);

		debugPrint("win_balloon:win_balloon, setting timer. balloonTimeout=%i.",balloonTimeout);

		SetTimer(hwndBalloon, iBalloonTimerId, balloonTimeout, NULL);
		Post(NIN_BALLOONSHOW);
	}

private:
	~win_balloon()
	{
		finished = true;
		DestroyWindow(hwndBalloon);
	}

	void Post(LPARAM lParam)
	{
		PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
	}

	static LRESULT wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		win_balloon *p = (win_balloon *)GetWindowLong(hwnd, GWL_USERDATA);
		switch (message)
		{
			case WM_CREATE:
				debugPrint("wm_create: balloonTimeout=%i.",balloonTimeout);
				break;

			case WM_DESTROY:
				if (false == p->finished)
					p->Post(NIN_BALLOONHIDE);
				break;

			case WM_TIMER:
				debugPrint("wm_timer: balloonTimeout=%i.",balloonTimeout);
				if (iBalloonTimerId != (int)wParam) break;
				p->Post(NIN_BALLOONTIMEOUT);
				KillTimer(hwnd, iBalloonTimerId);
				debugPrint("wm_timer: timer was killed!");
				delete p;
				return 0;

			case WM_RBUTTONDOWN:
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
				p->Post(NIN_BALLOONUSERCLICK);
				delete p;
				return 0;

			default:
				break;
		}
		return CallWindowProc (p->prev_wndproc, hwnd, message, wParam, lParam);
	}
};

//===========================================================================
class msg_balloon
{
	systemTray icon;
	systemTrayBalloon balloon;

public:
	msg_balloon(plugin_info * mPI, systemTray *pIcon, RECT *r)
	{
		this->icon  = *pIcon;
		this->balloon = *pIcon->pBalloon;
		PostMessage(mPI->hwnd, NIN_BALLOONSHOW, 0, (LPARAM)this);
	}

	void show(void)
	{
		Post(NIN_BALLOONSHOW);
		char msg[1000];
		if (balloon.szInfoTitle[0]) sprintf(msg, "%s\n\n%s", balloon.szInfoTitle, balloon.szInfo);
		else sprintf(msg, "%s", balloon.szInfo);
		static int f [] =
		{
			0,
			MB_ICONINFORMATION,
			MB_ICONWARNING,
			MB_ICONERROR
		};
		if (IDOK == MessageBox(NULL, msg, "Balloon Tip (Beta)", f[iminmax(balloon.dwInfoFlags, 0, 3)]|MB_OKCANCEL|MB_TOPMOST|MB_SETFOREGROUND))
			Post(NIN_BALLOONUSERCLICK);
		else
			Post(NIN_BALLOONTIMEOUT);

		delete this;
	}

private:
	~msg_balloon()
	{
	}

	void Post(LPARAM lParam)
	{
		PostMessage(icon.hWnd, icon.uCallbackMessage, icon.uID, lParam);
	}

};

