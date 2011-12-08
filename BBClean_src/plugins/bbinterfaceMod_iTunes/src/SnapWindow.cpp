/*===================================================

	SNAP WINDOWS CODE - Copyright 2004 grischka

	- grischka@users.sourceforge.net -

===================================================*/

// Global Include
#include "BBApi.h"
#include "SnapWindow.h"

/*===================================================*/
#if 0
void snap_windows(WINDOWPOS *wp, bool sizing, int *content)
{
	SIZE adjusted_content;
	if (content && sizing)
	{
		adjusted_content.cx = content[0] + 2*plugin_snap_padding;
		adjusted_content.cy = content[1] + 2*plugin_snap_padding;
		content = (int*)&adjusted_content;
	}
	SnapWindowToEdge(wp, (LPARAM)content, sizing ? SNAP_FULLSCREEN|SNAP_CONTENT|SNAP_SIZING : SNAP_FULLSCREEN|SNAP_CONTENT);
}

/*===================================================*/
#else
/*===================================================*/

// Structures
struct edges { int from1, from2, to1, to2, dmin, omin, d, o, def; };

struct snap_info {struct edges *h; struct edges *v;
	bool sizing; bool same_level; int pad; HWND self; HWND parent;
};

// Local fuctions
void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad);
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad);
BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam);

void get_mon_rect(HMONITOR hMon, RECT *r);
HMONITOR get_monitor(HWND hw);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//snap_windows
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void snap_windows(WINDOWPOS *wp, bool sizing, int *content)
{
	int snapdist    = plugin_snap_dist;
	//int grid        = plugin_snap_usegrid ? plugin_snap_gridsize : 0;
	int padding     = plugin_snap_padding;

	if (snapdist < 1) return;
	// ------------------------------------------------------

	HWND self   = wp->hwnd;
	HWND parent = GetParent(self);

	struct edges h;
	struct edges v;
	struct snap_info si = { &h, &v, sizing, true, padding, self, parent };

	h.dmin = v.dmin = h.def = v.def = snapdist;

	h.from1 = wp->x;
	h.from2 = h.from1 + wp->cx;
	v.from1 = wp->y;
	v.from2 = v.from1 + wp->cy;


	// ------------------------------------------------------
	// snap to grid
	/*
	if (grid > 1 && (parent || sizing))
	{
		snap_to_grid(&h, &v, sizing, grid, padding);
	}
	//else
	*/
	{
		// -----------------------------------------
		if (parent)
		{
			// snap to siblings
			EnumChildWindows(parent, SnapEnumProc, (LPARAM)&si);

			// snap to frame edges
			RECT r; GetClientRect(parent, &r);
			h.to1 = r.left;
			h.to2 = r.right;
			v.to1 = r.top;
			v.to2 = r.bottom;
			snap_to_edge(&h, &v, sizing, false, padding);
		}
		else
		{
			// snap to top level windows
			EnumThreadWindows(GetCurrentThreadId(), SnapEnumProc, (LPARAM)&si);

			// snap to screen edges
			RECT r; get_mon_rect(get_monitor(self), &r);
			h.to1 = r.left;
			h.to2 = r.right;
			v.to1 = r.top;
			v.to2 = r.bottom;
			snap_to_edge(&h, &v, sizing, false, 0);
		}

		// -----------------------------------------
		if (sizing)
		{
			// snap to button icons
			if (content)
			{
				// images have to be double padded, since they are centered
				h.to2 = (h.to1 = h.from1) + content[0];
				v.to2 = (v.to1 = v.from1) + content[1];
				snap_to_edge(&h, &v, sizing, false, -2*padding);
			}

			// snap frame to childs
			si.same_level = false;
			si.pad = -padding;
			si.self = NULL;
			si.parent = self;
			EnumChildWindows(self, SnapEnumProc, (LPARAM)&si);
		}
	}

	// -----------------------------------------
	// adjust the window-pos

	if (h.dmin < snapdist)
		if (sizing) wp->cx += h.omin; else wp->x += h.omin;

	if (v.dmin < snapdist)
		if (sizing) wp->cy += v.omin; else wp->y += v.omin;
}

//*****************************************************************************
BOOL CALLBACK SnapEnumProc(HWND hwnd, LPARAM lParam)
{
	struct snap_info *si = (struct snap_info *)lParam;
	if (hwnd != si->self && IsWindowVisible(hwnd))
	{
		HWND pw = GetParent(hwnd);
		if (pw == si->parent)
		{
			RECT r; GetWindowRect(hwnd, &r);
			r.right -= r.left;
			r.bottom -= r.top;
			if (pw) ScreenToClient(pw, (POINT*)&r.left);
			if (false == si->same_level)
			{
				r.left += si->h->from1;
				r.top  += si->v->from1;
			}
			si->h->to2 = (si->h->to1 = r.left) + r.right;
			si->v->to2 = (si->v->to1 = r.top)  + r.bottom;
			snap_to_edge(si->h, si->v, si->sizing, si->same_level, si->pad);
		}
	}
	return TRUE;
}   

//*****************************************************************************
/*
void snap_to_grid(struct edges *h, struct edges *v, bool sizing, int grid, int pad)
{
	for (struct edges *g = h;;g = v)
	{
		int o, d;
		if (sizing) o = g->from2 - g->from1 + pad; // relative to topleft
		else        o = g->from1 - pad; // absolute coords

		o = o % grid;
		if (o < 0) o += grid;

		if (o >= grid / 2)
			d = o = grid-o;
		else
			d = o, o = -o;

		if (d < g->dmin) g->dmin = d, g->omin = o;

		if (g == v) break;
	}
}
*/

//*****************************************************************************
void snap_to_edge(struct edges *h, struct edges *v, bool sizing, bool same_level, int pad)
{
	int o, d, n; struct edges *t;
	h->d = h->def; v->d = v->def;
	for (n = 2;;) // v- and h-edge
	{
		// see if there is any common edge, i.e if the lower top is above the upper bottom.
		if ((v->to2 < v->from2 ? v->to2 : v->from2) > (v->to1 > v->from1 ? v->to1 : v->from1))
		{
			if (same_level) // child to child
			{
				//snap to the opposite edge, with some padding between
				bool f = false;

				d = o = (h->to2 + pad) - h->from1;  // left edge
				if (d < 0) d = -d;
				if (d <= h->d)
				{
					if (false == sizing)
						if (d < h->d) h->d = d, h->o = o;
					if (d < h->def) f = true;
				}

				d = o = h->to1 - (h->from2 + pad); // right edge
				if (d < 0) d = -d;
				if (d <= h->d)
				{
					if (d < h->d) h->d = d, h->o = o;
					if (d < h->def) f = true;
				}

				if (f)
				{
					// if it's near, snap to the corner
					if (false == sizing)
					{
						d = o = v->to1 - v->from1;  // top corner
						if (d < 0) d = -d;
						if (d < v->d) v->d = d, v->o = o;
					}
					d = o = v->to2 - v->from2;  // bottom corner
					if (d < 0) d = -d;
					if (d < v->d) v->d = d, v->o = o;
				}
			}
			else // child to frame
			{
				//snap to the same edge, with some bevel between
				if (false == sizing)
				{
					d = o = h->to1 - (h->from1 - pad); // left edge
					if (d < 0) d = -d;
					if (d < h->d) h->d = d, h->o = o;
				}
				d = o = h->to2 - (h->from2 + pad); // right edge
				if (d < 0) d = -d;
				if (d < h->d) h->d = d, h->o = o;
			}
		}
		if (0 == --n) break;
		t = h; h = v, v = t;
	}

	if (false == sizing && false == same_level)
	{
		// snap to center
		for (n = 2;;) // v- and h-edge
		{
			if (v->d < v->dmin)
			{
				d = o = (h->to1 + h->to2)/2 - (h->from1 + h->from2)/2;
				if (d < 0) d = -d;
				if (d < h->d) h->d = d, h->o = o;
			}
			if (0 == --n) break;
			t = h; h = v, v = t;
		}
	}

	if (h->d < h->dmin) h->dmin = h->d, h->omin = h->o;
	if (v->d < v->dmin) v->dmin = v->d, v->omin = v->o;
}

//*****************************************************************************
#endif
//*****************************************************************************
//get_monitor - multimon api, win 9x compatible

static HMONITOR (WINAPI *pMonitorFromWindow)(HWND hwnd, DWORD dwFlags);
static BOOL     (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);

HMONITOR get_monitor(HWND hw)
{
	if (NULL == pMonitorFromWindow)
	{
		HMODULE hDll = GetModuleHandle("USER32");
		*(FARPROC*)&pMonitorFromWindow = GetProcAddress(hDll, "MonitorFromWindow" );
		*(FARPROC*)&pGetMonitorInfoA   = GetProcAddress(hDll, "GetMonitorInfoA"   );
		if (NULL == pMonitorFromWindow)
			*(int*)&pMonitorFromWindow = 1;
	}

	if (*(DWORD*)&pMonitorFromWindow <= 1)
		return NULL;

	return pMonitorFromWindow(hw, MONITOR_DEFAULTTOPRIMARY);
}

//*****************************************************************************
//get_mon_rect

void get_mon_rect(HMONITOR hMon, RECT *r)
{
	if (hMon)
	{
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if (pGetMonitorInfoA(hMon, &mi))
		{
			*r = mi.rcMonitor;
			//*r = mi.rcWork;
			return;
		}
	}
	//SystemParametersInfo(SPI_GETWORKAREA, 0, r, 0);
	r->top = r->left = 0;
	r->right = GetSystemMetrics(SM_CXSCREEN);
	r->bottom = GetSystemMetrics(SM_CYSCREEN);
}

//*****************************************************************************
