/*===================================================

	SNAP WINDOWS HEADER - Copyright grischka

	- grischka@users.sourceforge.net -

===================================================*/

//Multiple definition prevention
#ifndef BBInterface_SnapWindow_h
#define BBInterface_SnapWindow_h

void snap_windows(WINDOWPOS *wp, bool sizing, int *content);

extern int plugin_snap_dist;
extern int plugin_snap_padding;
//extern bool plugin_snap_usegrid;
//extern int plugin_snap_gridsize;

void get_mon_rect(HMONITOR hMon, RECT *r);
HMONITOR get_monitor(HWND hw);

#endif

//*****************************************************************************
