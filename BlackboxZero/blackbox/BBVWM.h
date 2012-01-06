/* ==========================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004-2009 grischka

  http://bb4win.sourceforge.net/bblean
  http://developer.berlios.de/projects/bblean

  bbLean is free software, released under the GNU General Public License
  (GPL version 2). For details see:

  http://www.fsf.org/licenses/gpl.html

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

  ========================================================================== */

#ifndef _BBVWM_H_
#define _BBVWM_H_

struct winlist;

//=========================================================
// Init/exit

void vwm_init(void);
void vwm_reconfig(bool force);
void vwm_exit(void);

//=========================================================
// update the list

void vwm_update_winlist(void);
struct winlist* vwm_add_window(HWND hwnd);

//=========================================================
// set workspace

void vwm_switch(int newdesk);
void vwm_gather(void);

//=========================================================
// Set window properties

bool vwm_set_desk(HWND hwnd, int desk, bool switchto);
bool vwm_set_location(HWND hwnd, struct taskinfo *t, unsigned flags);
bool vwm_set_sticky(HWND hwnd, bool set);
bool vwm_lower_window(HWND hwnd);

// Workaround for BBPager:
bool vwm_set_workspace(HWND hwnd, int desk);

//=========================================================
// status infos about windows

int vwm_get_desk(HWND hwnd);
bool vwm_get_location(HWND hwnd, struct taskinfo *t);

bool vwm_get_status(HWND hwnd, int what);
// values for "what":
#define VWM_MOVED 1
#define VWM_HIDDEN 2
#define VWM_STICKY 3
#define VWM_ICONIC 4

//=========================================================
// required variables/functions from elswhere:

// total number of desktops
extern int nScreens;

// current desktop
extern int currentScreen;
extern int lastScreen;

// vwm options
extern bool Settings_altMethod, Settings_styleXPFix;

// from workspaces.cpp
extern bool check_sticky_name(HWND hwnd);
extern bool check_sticky_plugin(HWND hwnd);
extern void workspaces_set_desk(void);
extern void send_desk_refresh(void);
extern void send_task_refresh(void);

//=========================================================
#endif //def _BBVWM_H_
