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
*/

//#include "bbLeanBar.h"
//#include "bbLeanClasses.h"

// possible bar items
enum
{
	M_TASK = 1,
	M_TRAY,

	M_WSPL,
	M_CLCK,
	M_WINL,
	M_SPAC,
	M_NEWLINE,
	M_CUOB,     // CurrentOnlyButton
	M_TDPB,     // TaskDisplayButton
	M_WSPB_L,
	M_WSPB_R,

	M_WINB_L,
	M_WINB_R,

	M_EXTENDER = 128,

	M_BARLIST = 256,
	M_TASKLIST,
	M_TRAYLIST
};

enum
{
	ME_LEFT,
	ME_MID,
	ME_RIGHT,
	ME_DBL,

	NB_MOUSE_EVENT
};

ST const char *me_workspace[NB_MOUSE_EVENT];
ST const char *me_window[NB_MOUSE_EVENT];
ST const char *me_clock[NB_MOUSE_EVENT];
ST const char *me_curOnly[NB_MOUSE_EVENT];
ST const char *me_taskSty[NB_MOUSE_EVENT];
ST const char *me_wspb_l[NB_MOUSE_EVENT];
ST const char *me_wspb_r[NB_MOUSE_EVENT];
ST const char *me_wnd_l[NB_MOUSE_EVENT];
ST const char *me_wnd_r[NB_MOUSE_EVENT];

struct itemlist
{
	struct itemlist *next;
	class baritem *item;
};

void mouse_event_exec(int message, const char **events)
{
	switch (message)
	{
		case WM_LBUTTONUP:
			if (events[ME_LEFT]) PostMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)events[ME_LEFT]);
			break;

		case WM_MBUTTONUP:
			if (events[ME_MID]) PostMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)events[ME_MID]);
			break;

		case WM_RBUTTONUP:
			if (events[ME_RIGHT]) PostMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)events[ME_RIGHT]);
			break;

		case WM_LBUTTONDBLCLK:
			if (events[ME_DBL]) PostMessage(BBhwnd, BB_EXECUTEASYNC, 0, (LPARAM)events[ME_DBL]);
			break;
	}
}

//===========================================================================

//===========================================================================
// the base class for all items on the bar:

class baritem
{
public:
	barinfo *mPI;                // the pointer to the plugin_info structure
	int mtype;                  // the item's M_XXX ID
	RECT mr;                    // the rectangle of the item
	bool activate_bbhwnd;       // give focus to BB on click?
	bool active;

	//-----------------------------
	baritem(int type, barinfo *pi)
	{
		mPI = pi;
		mtype = type;
		activate_bbhwnd = false;
		active = false;
	}

	//-----------------------------
	virtual ~baritem()
	{
		if (mPI->capture_item == this)
		{
			ReleaseCapture();
			mPI->capture_item = NULL;
		}
	}

	//-----------------------------
	int set_location(int x, int y, int w, int h)
	{
		mr.left   = x;
		mr.top    = y;
		mr.right  = x + w;
		mr.bottom = y + h;
		return mr.right;
	}

	//-----------------------------
	// check the item for mouse-over
	bool mouse_over (int mx, int my)
	{
		RECT r = mr;
#if 1
		// extend clickable area to screen edge
		int b = (styleBorderWidth + styleBevelWidth);
		int bar_top = mPI->ypos - mPI->mon_rect.top;
		int bar_bottom = mPI->mon_rect.bottom - mPI->ypos;
		if (bar_top + r.top <= b) r.top = 0;
		if (bar_bottom - r.bottom <= b) r.bottom = bar_bottom;
#endif
		return mx >= r.left && mx < r.right && my >= r.top && my < r.bottom;
	}

	//-----------------------------
	virtual void mouse_event(int mx, int my, int event, unsigned flags)
	{
		// default: show bb-menu
		if (event == WM_RBUTTONUP)
			PostMessage(BBhwnd, BB_MENU, 0, 0);
	}

	//-----------------------------
	virtual void draw()
	{
	}

	//-----------------------------
	virtual void settip(void)
	{
	}

	//-----------------------------
	bool check_capture(int mx, int my, int message)
	{
		bool pa = active;
		bool ret = true;
		if (mPI->capture_item == this)
		{
			bool over = active = mouse_over(mx, my);
			if (message != WM_MOUSEMOVE)
			{
				ReleaseCapture();
				mPI->capture_item = NULL;
				active = false;
			}
			ret = over;
		}
		else
		if (message == WM_LBUTTONDOWN
		 || message == WM_RBUTTONDOWN
		 || message == WM_LBUTTONDBLCLK
		 || message == WM_RBUTTONDBLCLK
		 )
		{
			SetCapture(mPI->hwnd);
			mPI->capture_item = this;
			active = true;
		}
		if (active != pa)
			InvalidateRect(mPI->hwnd, &mr, FALSE);

		return ret;
	}

};

//===========================================================================

//===========================================================================
// a list class, for tasks and tray-icons, also for the entire bar

class baritemlist : public baritem
{
public:
	struct itemlist *items;

	//-----------------------------
	baritemlist(int type, barinfo *pi) : baritem(type, pi)
	{
		items = NULL;
	}

	//-----------------------------
	virtual ~baritemlist()
	{
		clear();
	}

	//-----------------------------
	void add(class baritem *entry)
	{
		append_node(&items, new_node(entry));
	}

	//-----------------------------
	void clear(void)
	{
		struct itemlist *i;
		dolist (i, items) delete i->item;
		freeall(&items);
	}

	//-----------------------------
	virtual void calc_sizes(void)
	{
	};

	//-----------------------------
	void draw()
	{
		calc_sizes();
		struct itemlist *p;
		dolist (p, items)
		{
			RECT rtmp;
			if (IntersectRect(&rtmp, &p->item->mr, mPI->p_rcPaint)
				|| (mPI->update_flag & 8)
				)
				p->item->draw();
		}
	}

	//-----------------------------
	virtual void mouse_event(int mx, int my, int msg, unsigned flags)
	{
		struct itemlist *p;
		dolist (p, items)
		{
			if (p->item->mouse_over(mx, my))
			{
				if (p->item->activate_bbhwnd)
				{
					if (msg == WM_LBUTTONDOWN
					 || msg == WM_RBUTTONDOWN
					 || msg == WM_MBUTTONDOWN
					 )
						SetActiveWindow(mPI->hwnd);
				}
				p->item->mouse_event(mx, my, msg, flags);
				return;
			}
		}
	}

	//-----------------------------
	void settip()
	{
		struct itemlist *p;
		dolist (p, items) p->item->settip();
	}

};

//===========================================================================

//===========================================================================
// One task entry, they are collected in the 'baritemlist'

class taskentry : public baritem
{
public:
	int m_index;
	bool m_showtip;

	//-----------------------------
	taskentry(int index, barinfo *pi) : baritem(M_TASK, pi)
	{
		m_index = index;
		m_showtip = false;
		activate_bbhwnd = true;
	}

	//-----------------------------
	~taskentry()
	{
	}

	//-----------------------------
	void draw()
	{
		m_showtip = false;

		struct tasklist *tl = mPI->GetTaskPtrEx(m_index);
		if (NULL==tl) return;
		bool lit = tl->active || tl->flashing;//|| active;

		StyleItem *S; COLORREF C;

		if (lit)
			S = ((StyleItem *)GetSettingPtr(SN_A)), C = Color_A;
		else
			S = ((StyleItem *)GetSettingPtr(SN_TOOLBAR)), C = Color_T;

		if (mPI->TaskStyle == 1)
			draw_icons(tl, lit, S);
		else
			draw_text(tl, lit, S, C);
	}

	//-----------------------------
	// Icon only mode
	void draw_icons(struct tasklist *tl, bool lit, StyleItem *S)
	{
		if (lit)
		{
			if (false == S->parentRelative)
				mPI->pBuff->MakeStyleGradient(mPI->hdcPaint,  &mr, S, false);
			else
				CreateBorder(mPI->hdcPaint, &mr, styleBorderColor, 1);
		}
		m_showtip = true;
		if (tl->icon)
			// drawIco (mr.left+1, mr.top+1, mPI->T_ICON_SIZE, (16 >= mPI->T_ICON_SIZE)? tl->icon: tl->icon_big, mPI->hdcPaint, false == lit, mPI->iconSaturationValue, mPI->iconHueIntensity);
			drawIco (mr.left+ (mr.right-mr.left-mPI->T_ICON_SIZE)/2, mr.top + (mr.bottom-mr.top-mPI->T_ICON_SIZE)/2, mPI->T_ICON_SIZE, (16 >= mPI->T_ICON_SIZE)? tl->icon: tl->icon_big, mPI->hdcPaint, false == lit, mPI->iconSaturationValue, mPI->iconHueIntensity);

	}

	//-----------------------------
	// Text (with icon) mode
	void draw_text(struct tasklist *tl, bool lit, StyleItem *S, COLORREF C)
	{
		StyleItem SI = *S;
		bool pr = ((StyleItem *)GetSettingPtr(SN_A))->parentRelative;

		if (pr && false == mPI->task_with_border)
			SI.parentRelative = true;

		if (mPI->task_with_border || (lit && SI.parentRelative))
		{
			if (false == SI.bordered)
			{
				SI.bordered = true;
				SI.borderWidth = 1;
				SI.borderColor = styleBorderColor;
			}
		}
		else
		if (false == mPI->task_with_border && false == lit)
			SI.bordered = false;

		mPI->pBuff->MakeStyleGradient(mPI->hdcPaint, &mr, &SI, false);


		HGDIOBJ oldfont = SelectObject(mPI->hdcPaint, mPI->hFont);
		SetBkMode(mPI->hdcPaint, TRANSPARENT);
		SetTextColor(mPI->hdcPaint, C);

		RECT ThisWin = mr;
		RECT s1 = {0,0,0,0};
		RECT s2 = {0,0,0,0};
		int n =strlen(tl->caption); if (n>8) n=8;
		DrawText(mPI->hdcPaint, tl->caption,  n, &s1, DT_CALCRECT|DT_NOPREFIX);
		DrawText(mPI->hdcPaint, tl->caption, -1, &s2, DT_CALCRECT|DT_NOPREFIX);

		int f = 0;
		if ((mPI->TaskStyle&2) && NULL != tl->icon)
			f = mPI->T_ICON_SIZE;

		ThisWin.left    += 3+f;
		ThisWin.right   -= 3;
		int s = ThisWin.right - ThisWin.left;

		BBDrawText(mPI->hdcPaint, tl->caption, -1, &ThisWin,
			(s>s1.right ? TBJustify : DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_NOPREFIX),
			S);

		SelectObject(mPI->hdcPaint, oldfont);

		if (s<s2.right)
			m_showtip = true;

		if (f)
			drawIco (mr.left+1, mr.top + (mr.bottom-mr.top-mPI->T_ICON_SIZE)/2, mPI->T_ICON_SIZE, (16 >= mPI->T_ICON_SIZE)? tl->icon: tl->icon_big, mPI->hdcPaint, false == lit, mPI->iconSaturationValue, mPI->iconHueIntensity);
	}

	void settip()
	{
		if (m_showtip)
		{
			struct tasklist *tl = mPI->GetTaskPtrEx(m_index);
			if (tl) SetToolTip(mPI->hwnd, &mr, tl->caption);
		}
	}

#if 0
	//-----------------------------
	void mouse_event(int mx, int my, int message, unsigned flags)
	{
		// everything on button-down
		struct tasklist *tl = GetTaskPtrEx(m_index);
		if (NULL == tl) return;
		HWND Window = tl->hwnd;
		bool shift_down = flags & MK_SHIFT;
		switch (message)
		{
			//====================
			// Restore and focus window

			case WM_LBUTTONDOWN:
			focuswindow:
				if (taskSysmenu && last_active_task == Window && !IsIconic(Window))
					goto minimize;

				if (shift_down)
					PostMessage(BBhwnd, BB_BRINGTOFRONT, BBBTF_CURRENT, (LPARAM)Window);
				else
					PostMessage(BBhwnd, BB_BRINGTOFRONT, 0,  (LPARAM)Window);
				break;

			//====================
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
				if (taskSysmenu)
					break;

				if (shift_down)
					PostMessage(BBhwnd, BB_WINDOWCLOSE, 1, (LPARAM)Window);
				else
	minimize:
					PostMessage(BBhwnd, BB_WINDOWMINIMIZE, 0, (LPARAM)Window);
				break;

			//====================
			// Move window to the next/previous workspace
			case WM_LBUTTONDBLCLK:
				if (taskSysmenu)
					goto focuswindow;

			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
				if (shift_down)
					PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWLEFT, (LPARAM)Window);
				else
					PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWRIGHT, (LPARAM)Window);
				break;

			//====================
			case WM_RBUTTONUP:
				if (taskSysmenu)
					ShowSysmenu(Window, PI->hwnd);
				break;

			//====================
			case BB_DRAGOVER:
				task_over_hwnd = Window;
				break;
		}
	}
#else
	//-----------------------------
	// everything on button-up
	void mouse_event(int mx, int my, int message, unsigned flags)
	{
		if (false == check_capture(mx, my, message))
			return;

		struct tasklist *tl = mPI->GetTaskPtrEx(m_index);
		if (NULL == tl) return;
		HWND Window = tl->hwnd;
		bool shift_down = flags & MK_SHIFT;

		static bool dblclk;
		switch (message)
		{
			//====================
			// Restore and focus window
			case WM_LBUTTONUP:
				if (dblclk) goto move_wnd;

				if (mPI->taskSysmenu && last_active_task == Window && !IsIconic(Window))
					goto minimize;

				if (shift_down)
					PostMessage(BBhwnd, BB_BRINGTOFRONT, BBBTF_CURRENT, (LPARAM)Window);
				else
					PostMessage(BBhwnd, BB_BRINGTOFRONT, 0,  (LPARAM)Window);

				break;

			//====================
			// Minimize (iconify) window

			case WM_RBUTTONUP:
				if (dblclk) break;
				if (mPI->taskSysmenu)
				{
					ShowSysmenu(Window, mPI->hwnd);
					break;
				}
				if (shift_down)
					PostMessage(BBhwnd, BB_WINDOWCLOSE, 0, (LPARAM)Window);
				else
			minimize:
					PostMessage(BBhwnd, BB_WINDOWMINIMIZE, 0, (LPARAM)Window);
					//PostMessage(Window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				break;

			//====================

			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDBLCLK:
				dblclk = true;
				break;

			case WM_RBUTTONDOWN:
			case WM_LBUTTONDOWN:
				dblclk = false;
				break;

			move_wnd:
				dblclk = false;

			// Move window to the next/previous workspace
			case WM_MBUTTONUP:
				if (shift_down)
					PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWLEFT, (LPARAM)Window);
				else
					PostMessage(BBhwnd, BB_WORKSPACE, BBWS_MOVEWINDOWRIGHT, (LPARAM)Window);
				break;

			//====================
			case BB_DRAGOVER:
				mPI->task_over_hwnd = Window;
				break;

			//====================
		}
	}
#endif
};

//===========================================================================

//===========================================================================
// one tray-icon

class trayentry : public baritem
{
public:
	int m_index;
	HICON m_icon;

	//-----------------------------
	trayentry(int index, barinfo *pi)  : baritem(M_TRAY, pi)
	{
		m_icon = NULL;
		m_index = index;
	}

	//-----------------------------
	~trayentry()
	{
	}

	//-----------------------------
	void draw()
	{
		systemTray* icon = mPI->GetTrayIcon(m_index);
		if (icon)
		{
			m_icon = icon->hIcon;
			drawIco(mr.left+1, mr.top+1, mPI->S_ICON_SIZE, m_icon, mPI->hdcPaint, true, mPI->traySaturationValue, mPI->trayHueIntensity);
		}
	}

	//-----------------------------
	void settip()
	{
		systemTray* icon = mPI->GetTrayIcon(m_index);
		if (icon)
		{
			SetToolTip(mPI->hwnd, &mr, icon->szTip);

			if (mPI->enable_balloons
			 && is_bblean
			 && 0 == icon->szTip[sizeof icon->szTip - 1]
			 && icon->pBalloon
			 && icon->pBalloon->uInfoTimeout
			 )
			{
				RECT r = mr;
				ClientToScreen(mPI->hwnd, (POINT*)&r.left);
				ClientToScreen(mPI->hwnd, (POINT*)&r.right);
				new bb_balloon(mPI, icon, r);
				icon->pBalloon->uInfoTimeout = 0;
			}
		}
	}

	//-----------------------------
	void mouse_event(int mx, int my, int message, unsigned flags)
	{
		systemTray* icon = mPI->GetTrayIcon(m_index);

		if (icon)
		{
			HWND iconWnd = icon->hWnd;
			if (IsWindow(iconWnd))
			{
				DWORD pid;
				if (WM_MOUSEMOVE != message
				 && pAllowSetForegroundWindow
				 && GetWindowThreadProcessId(iconWnd, &pid))
				{
					pAllowSetForegroundWindow(pid);
				}
				// Reroute the mouse message to the tray icon's host window...
				SendNotifyMessage(iconWnd, icon->uCallbackMessage, icon->uID, message);
			}
			else
			{
				PostMessage(BBhwnd, BB_CLEANTRAY, 0, 0);
			}
		}
	}
};

//===========================================================================

//===========================================================================
// common base class for clock, workspace-label, window-label

class barlabel : public baritem
{
public:
	int m_Style;
	const char *m_text;
	const char **m_events;

	//-----------------------------
	barlabel(int type, barinfo *pi, char *text, int S, const char **events) : baritem(type, pi)
	{
		m_Style = S;
		m_text = text;
		m_events = events;
	}

	//-----------------------------
	void draw()
	{
		StyleItem *pSI = (StyleItem*)GetSettingPtr(m_Style);
		mPI->pBuff->MakeStyleGradient(mPI->hdcPaint,  &mr, pSI, false);
		HGDIOBJ oldfont = SelectObject(mPI->hdcPaint, mPI->hFont);
		SetBkMode(mPI->hdcPaint, TRANSPARENT);
		SetTextColor(mPI->hdcPaint, pSI->TextColor);
		RECT r;
		r.left  = mr.left  + 3;
		r.right = mr.right - 3;
		r.top   = mr.top;
		r.bottom = mr.bottom;

		BBDrawText(mPI->hdcPaint, m_text, -1, &r, TBJustify, pSI);

		SelectObject(mPI->hdcPaint, oldfont);
	}

	//-----------------------------
	void mouse_event(int mx, int my, int message, unsigned flags)
	{
		mouse_event_exec(message, m_events);
	}
};

//===========================================================================

//===========================================================================
// workspace-label

class workspace_label : public barlabel
{
public:
	workspace_label(barinfo *pi) : barlabel(M_WSPL, pi, screenName, SN_TOOLBARLABEL, me_workspace)
	{
	}
};

//===========================================================================

//===========================================================================
// window-label

class window_label : public barlabel
{
public:
	window_label(barinfo *pi) : barlabel(M_WINL, pi, pi->windowlabel, SN_TOOLBARWINDOWLABEL, me_window)
	{
		activate_bbhwnd = true;
	}
};

//===========================================================================

//===========================================================================
// clock-label

class clock_displ : public barlabel
{
public:
	clock_displ(barinfo *pi) : barlabel(M_CLCK, pi, pi->clockTime, SN_TOOLBARCLOCK, me_clock)
	{
		activate_bbhwnd = true;
		mPI->set_clock_string();
	}

	//-----------------------------
	void settip()
	{
		SetToolTip(mPI->hwnd, &mr, mPI->clockTimeTips);
	}
};

//===========================================================================

//===========================================================================
// fill in a space or new line

class spacing : public baritem
{
public:
	spacing(int typ, barinfo *pi) : baritem(typ, pi)
	{
		activate_bbhwnd = true;
		ZeroMemory(&mr, sizeof(RECT));
	}
};

//===========================================================================

//===========================================================================
// buttons

class bar_button : public baritem
{
public:
	int dir; int mod;

	//-----------------------------
	bar_button(int m, barinfo *pi)  : baritem(m, pi)
	{
		dir = m==M_TDPB || m==M_CUOB ? 0 : m==M_WINB_L || m==M_WSPB_L ? -1 : 1;
		mod = m==M_TDPB || m==M_WINB_L || m==M_WINB_R ? 1 : 0;
	}

	//-----------------------------
	void draw()
	{
		StyleItem* S = (StyleItem *)GetSettingPtr(
			active||(dir>0 && mPI->force_button_pressed)
			? SN_TOOLBARBUTTONP : SN_TOOLBARBUTTON
			);

		mPI->pBuff->MakeStyleGradient(mPI->hdcPaint, &mr, S, false);

		HPEN Pen   = CreatePen(PS_SOLID, 1, S->picColor);
		HGDIOBJ other = SelectObject(mPI->hdcPaint, Pen);

		int w = (mr.right - mr.left) / 2;
		int x = mr.left + w;
		int y = mr.top  + w;
		w /= 3;

		if (0==dir)
		{
			bool on = mod ? false : false == mPI->currentOnly;
			Arc(mPI->hdcPaint, x-w, y-w, x+w, y+w, x,0,x,0);
			if (on)
			{
				do
				{
					Arc(mPI->hdcPaint, x-w, y-w, x+w-1, y+w, x,0,x,0);
					Arc(mPI->hdcPaint, x-w+1, y-w, x+w, y+w, x,0,x,0);
				}
				while(--w);
			}

		}
		else
		{
			int start, end, v = pSettings_arrowUnix ?(w/2):(w*2-1), z = y-v, z_end = y+v;
			if (dir < 0) start = x+w,   end = x-w+1;
			else         start = x-w+1, end = x+w;
			MoveToEx(mPI->hdcPaint, start, z, NULL);
			LineTo  (mPI->hdcPaint, start, z_end+1);
			for(; z <= z_end; z++)
			{
				MoveToEx(mPI->hdcPaint, end,   y, NULL);
				LineTo  (mPI->hdcPaint, start, z);
			}

		}

		DeleteObject(SelectObject(mPI->hdcPaint, other));
	}

	//-----------------------------
	// for the buttons, the mouse is captured on button-down
	void mouse_event(int mx, int my, int message, unsigned flags)
	{
		if (false == check_capture(mx, my, message))
			return;

		if (0 == dir)
		{
			if (message == WM_LBUTTONUP)
			{
				if (mod)
				{
					if (++mPI->TaskStyle==3) mPI->TaskStyle=0;
				}
				else
				{
					mPI->currentOnly = false==mPI->currentOnly;
					mPI->NewTasklist();
				}
				mPI->update_bar(2);
			}
			else
				if (mod)
					mouse_event_exec(message, me_taskSty);
				else
					mouse_event_exec(message, me_curOnly);
		}
		else
		if (mod)
		{
			if (dir > 0)
				mouse_event_exec(message, me_wnd_r);
			else
				mouse_event_exec(message, me_wnd_l);
		}
		else
		{
			if (dir > 0)
				mouse_event_exec(message, me_wspb_r);
			else
				mouse_event_exec(message, me_wspb_l);
		}
	}
};

//===========================================================================

//===========================================================================
// task zone

class taskitemlist : public baritemlist
{
public:
	taskitemlist(barinfo *pi) : baritemlist(M_TASKLIST, pi) { }

	//-----------------------------
	// This one assigns the individual locations and sizes for
	// the items in the task-list

	void calc_sizes(void)
	{
		if (false == (mPI->update_flag & 2))
			return;

		clear();
		int ts = mPI->GetTaskListSizeEx();
		if (0==ts) return;

		int b = styleBevelWidth;
		int w = mr.right - mr.left + b;
		int xpos = mr.left;
		int is = (mPI->T_ICON_SIZE + b + 2);
		bool icon_mode = 1 == mPI->TaskStyle;
		int min_width = is / 2;
		int max_width = mPI->maxTaskItemWidth;

		// Do some fugly math to get an estimation of how wide we can make the items
		int ideal_width = (w / ts);
		int task_width;
		if (max_width > 0) {
			task_width = (ideal_width > max_width)?max_width:ideal_width;
		} else {
			task_width = ideal_width;
		}
		if (task_width < min_width) task_width = min_width;


		int dyn_x = 0;
		int n = 0;
		do {
			int left, right;
			if (icon_mode)
			{
				left    = xpos + n * is;
				right   = left + is - b;
			}
			else
			{
				left    = xpos + dyn_x;
				right   = xpos + dyn_x + task_width - b;
				dyn_x = dyn_x + task_width;
			}
			if (right > mr.right)
				break;

			taskentry *gi = new taskentry(mPI->reverseTasks ? ts-n-1 : n, mPI);
			add(gi);
			gi->mr.top      = mr.top;
			gi->mr.bottom   = mr.bottom;
			gi->mr.left     = left;
			gi->mr.right    = right;
		} while (++n < ts);
	}

	//-----------------------------
	void mouse_event(int mx, int my, int msg, unsigned flags)
	{
		if (GetKeyState(VK_MENU) < 0)
		{
			switch (msg)
			{
				case WM_LBUTTONDOWN:
				case WM_LBUTTONDBLCLK:
					if (++mPI->TaskStyle==3) mPI->TaskStyle=0;
					break;

				case WM_RBUTTONDOWN:
				case WM_RBUTTONDBLCLK:
					mPI->currentOnly = false==mPI->currentOnly;
					mPI->NewTasklist();
					break;

				default:
					return;
			}
			mPI->update_bar(2);
			return;
		}
		baritemlist::mouse_event(mx, my, msg, flags);
	}
/*
	void draw()
	{
		if (TaskStyle == 1)
		{
			mPI->pBuff->MakeStyleGradient(hdcPaint,  &mr, TaskStyle_L, false);
		}
		baritemlist::draw();
	}
*/
};

//===========================================================================

//===========================================================================
// tray zone

class trayitemlist : public baritemlist
{
	int xpos, ypos;

public:
	trayitemlist(barinfo *pi) : baritemlist(M_TRAYLIST, pi) { xpos = ypos = 0; }

	//-----------------------------
	// This one assigns the individual locations and sizes for
	// the items in the tray-icon-list
	void calc_sizes(void)
	{
		int w = mr.right - mr.left;
		int h = mr.bottom - mr.top;

		int x0 = mr.left + mPI->xpos;
		int y0 = mr.top  + mPI->ypos;

		if (x0 != xpos || y0 != ypos)
		{
			xpos = x0, ypos = y0;
			if (tray_notify_wnd)
				SetWindowPos(tray_notify_wnd, NULL,
					x0, y0, w, h, SWP_NOACTIVATE|SWP_NOZORDER);

			if (tray_clock_wnd)
				SetWindowPos(tray_clock_wnd, NULL,
					0, 0, w, h, SWP_NOACTIVATE|SWP_NOZORDER);
		}

		if (false == (mPI->update_flag & 4))
			return;

		clear();

		int ts = mPI->GetTraySize();
		if (0==ts) return;

		int s = (h - mPI->S_ICON_SIZE)/2;
		int xpos = mr.left;
		int n = 0;

		do {
			int right = xpos + mPI->S_ICON_SIZE+2;
			if (right > mr.right)
				break;

			trayentry *gi = new trayentry(ts-n-1, mPI);
			add(gi);
			gi->mr.top      = mr.top + s - 1;
			gi->mr.bottom   = mr.top + s + mPI->S_ICON_SIZE + 1;
			gi->mr.left     = xpos;
			gi->mr.right    = xpos = right;
		} while (++n < ts);
	}
/*
	void draw()
	{
		mPI->pBuff->MakeStyleGradient(hdcPaint,  &mr, TaskStyle_L, false);
		baritemlist::draw();
	}
*/
};

//===========================================================================

//===========================================================================
// LeanBar - the main class

class LeanBar : public baritemlist
{
public:
	int max_clklabel_width, max_wsplabel_width;

	//-----------------------------
	LeanBar(barinfo *pi) : baritemlist(M_BARLIST, pi)
	{
	}

	//-----------------------------
	// search the an item and invalidate it's rect
	void invalidate_item(int type)
	{
		struct itemlist *p;
		dolist (p, items)
			if (type == p->item->mtype)
				InvalidateRect(mPI->hwnd, &p->item->mr, FALSE);
	}

	//-----------------------------
	// check for capture, otherwise dispatch the mouse event
	void mouse_event(int mx, int my, int msg, unsigned flags)
	{
		if (mPI->hwnd == GetCapture())
		{
			// on capture, the captured item gets the message
			if (mPI->capture_item)
				mPI->capture_item->mouse_event(mx, my, msg, flags);
			else
				ReleaseCapture();
			return;

		}
		mPI->capture_item = NULL;
		baritemlist::mouse_event(mx, my, msg, flags);
	}

	//-----------------------------
	// build everything from scratch
	void create_bar()
	{
		clear();
		max_clklabel_width = max_wsplabel_width = 0;

		char *item_ptr;
		for (item_ptr = mPI->item_string; *item_ptr;  item_ptr++)
		{
			switch (*item_ptr)
			{
			case M_TDPB:
			case M_CUOB:
			case M_WSPB_L:
			case M_WSPB_R:
			case M_WINB_L:
			case M_WINB_R:
				add( new bar_button(*item_ptr, mPI) );
				break;

			case M_NEWLINE:
			case M_SPAC:
				add( new spacing(*item_ptr, mPI) );
				break;

			case M_WSPL:
				add( new workspace_label(mPI) );
				break;

			case M_CLCK:
				add( new clock_displ(mPI) );
				break;

			case M_WINL:
				add( new window_label(mPI) );
				break;

			case M_TASK:
				add( new taskitemlist(mPI) );
				break;

			case M_TRAY:
				add( new trayitemlist(mPI) );
				break;

			}
		}
	}

	//-----------------------------
	int trayzone_width;

	void calc_sizes(void)
	{
		if (false == (mPI->update_flag & 15))
			return;

		if (mPI->update_flag & 8)
			create_bar();

		mPI->update_flag |= 6;

		trayzone_width = (mPI->S_ICON_SIZE + 2) * mPI->GetTraySize();

		int line = 0;
		int top = 0;

		HGDIOBJ oldfont = SelectObject(mPI->hdcPaint, mPI->hFont);

		// --- loop though lines ----------------------------------
		struct itemlist *p = items;
		while (p)
		{
			int height = mPI->bbLeanBarLineHeight[line];
			p = calc_line_size(p, top, top + height, height);
			top += height - styleBorderWidth;
			line ++;
		}

		SelectObject(mPI->hdcPaint, oldfont);
	}

	//-----------------------------
	// Here sizes are calculated in two passes: The first pass
	// gets all fixed sizes. Then the remaining space is assigned
	// to the variable ones (windowlabel/taskzone). The second pass
	// assigns the actual x-coords.

	struct itemlist *calc_line_size(struct itemlist *p0, int top, int bottom, int height)
	{
		int xpos;
		int clklabel_width, wsplabel_width, winlabel_width, taskzone_width;
		int clklabel_count, wsplabel_count;

		clklabel_count =
		wsplabel_count =
		clklabel_width =
		wsplabel_width =
		winlabel_width =
		taskzone_width = 0;

		struct itemlist *p;

		int b = styleBevelWidth;
		int bo = styleBorderWidth + b;
		int button_padding = (height - mPI->buttonH) / 2 - styleBorderWidth;

		int inner_margin = 0, prev_margin;

		prev_margin = b;
		xpos = styleBorderWidth;

		// --- 1st pass ----------------------------------
		dolist (p, p0)
		{
			char *cp; SIZE size;

			if (M_NEWLINE == p->item->mtype)
				break;

			int pm = prev_margin;
			xpos += pm;
			inner_margin = 1;
			prev_margin = b + inner_margin;

			switch (p->item->mtype)
			{
			case M_TDPB:
			case M_CUOB:
			case M_WSPB_L:
			case M_WSPB_R:
			case M_WINB_L:
			case M_WINB_R:
				xpos += imax(0, button_padding-pm) + mPI->buttonH;
				inner_margin = 0;
				prev_margin = button_padding;
				break;

			case M_WSPL:
				cp = screenName;
				GetTextExtentPoint32(mPI->hdcPaint, cp, strlen(cp), &size);
				if (size.cx > wsplabel_width)
					wsplabel_width = size.cx;

				wsplabel_count ++;
				break;

			case M_CLCK:
				cp = mPI->clockTime;
				GetTextExtentPoint32(mPI->hdcPaint, cp, strlen(cp), &size);
				if (size.cx > clklabel_width)
					clklabel_width = size.cx;

				clklabel_count ++;
				break;

			case M_WINL:
				winlabel_width = 1;
				break;

			case M_SPAC:
				xpos += styleBevelWidth+1;
				break;

			case M_TASKLIST:
				taskzone_width = 1;
				break;

			case M_TRAYLIST:
				xpos += trayzone_width;
				break;
			}
		}
		xpos += prev_margin - inner_margin;

		// --- assign variable widths ----------------------------------

		clklabel_width += 4 + 4; // label/clock left&right padding

		if (max_clklabel_width < clklabel_width)
			max_clklabel_width = clklabel_width;
		else
			clklabel_width = max_clklabel_width;

		wsplabel_width += 4 + 4; // label/workspace left&right padding

		if (max_wsplabel_width < wsplabel_width)
			max_wsplabel_width = wsplabel_width;
		else
			wsplabel_width = max_wsplabel_width;

		xpos += wsplabel_count * wsplabel_width + clklabel_count * clklabel_width + styleBorderWidth;

		int rest_width = imax(0, mPI->width - xpos);

		if (taskzone_width && winlabel_width)
		{
			if (1 == mPI->TaskStyle)
			{
				taskzone_width = imin(rest_width, (mPI->T_ICON_SIZE + b + 2) * mPI->GetTaskListSizeEx() - b);
				winlabel_width = rest_width - taskzone_width;
			}
			else
			{
				winlabel_width = rest_width / 2;
				if (winlabel_width > 200) winlabel_width = 200;
				if (winlabel_width < 32)  winlabel_width = 0;
				taskzone_width = rest_width - winlabel_width;
			}
		}
		else
		if (winlabel_width)
		{
			winlabel_width = rest_width;
		}
		else
		if (taskzone_width)
		{
			taskzone_width = rest_width;
		}

		// --- 2nd pass ----------------------------------

		prev_margin = b;
		xpos = styleBorderWidth;

		dolist (p, p0)
		{
			baritem *gi = p->item;

			if (M_NEWLINE == gi->mtype)
			{
				p = p->next;
				break;
			}

			int ypos = top + bo;
			int hs = height - bo - bo;
			int ws = 0;

			int pm = prev_margin;
			xpos += pm;
			inner_margin = 1;
			prev_margin = b + inner_margin;

			switch (gi->mtype)
			{

			case M_TDPB:
			case M_CUOB:
			case M_WSPB_L:
			case M_WSPB_R:
			case M_WINB_L:
			case M_WINB_R:
				xpos += imax(0, button_padding-pm);
				inner_margin = 0;
				prev_margin = button_padding;
				ypos = top + (height - (hs = ws = mPI->buttonH)) / 2;
				break;

			case M_SPAC:
				ws = b+1;
				break;

			case M_WSPL:
				ws = wsplabel_width;
				break;

			case M_CLCK:
				ws = clklabel_width;
				break;

			case M_WINL:
				ws = winlabel_width;
				break;

			case M_TASKLIST:
				ws = taskzone_width;
				break;

			case M_TRAYLIST:
				ws = trayzone_width;
				break;

			}
			xpos = gi->set_location(xpos, ypos, ws, hs);
		}
		return p;
	}
};

//===========================================================================
// Done with the C++ exercise
//===========================================================================


class bartender : public barlabel
{
public:
	bartender(barinfo *pi) : barlabel(M_WINL, pi, pi->windowlabel, SN_TOOLBARWINDOWLABEL, me_window)
	{

	}
};
