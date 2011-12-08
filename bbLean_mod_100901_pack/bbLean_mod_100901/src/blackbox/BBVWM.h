/*
 ============================================================================

  This file is part of the bbLean source code
  Copyright © 2001-2003 The Blackbox for Windows Development Team
  Copyright © 2004 grischka

  http://bb4win.sourceforge.net/bblean
  http://sourceforge.net/projects/bb4win

 ============================================================================

  bbLean and bb4win are free software, released under the GNU General
  Public License (GPL version 2 or later), with an extension that allows
  linking of proprietary modules under a controlled interface. This means
  that plugins etc. are allowed to be released under any license the author
  wishes. For details see:

  http://www.fsf.org/licenses/gpl.html
  http://www.fsf.org/licenses/gpl-faq.html#LinkingOverControlledInterface

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.

 ============================================================================
*/

//=========================================================
//              Init/exit
//=========================================================

void vwm_init(bool alt_method, bool xpfix);
void vwm_reconfig(bool alt_method, bool xpfix);
void vwm_exit(void);

//=========================================================
//              update the list
//=========================================================

void vwm_update_winlist(void);

//=========================================================
//              set workspace
//=========================================================

void vwm_switch_desk(int newdesk);
void vwm_gather(void);

//=========================================================
//               Set window properties
//=========================================================

bool vwm_set_workspace(HWND hwnd, int desk);
bool vwm_set_desk(HWND hwnd, int desk, bool switchto);
bool vwm_set_location(HWND hwnd, struct taskinfo *t, UINT flags);
bool vwm_make_sticky(HWND hwnd, bool sticky);


//=========================================================
//              retrieve infos about windows
//=========================================================

int vwm_get_desk(HWND hwnd);
bool vwm_get_location(HWND hwnd, struct taskinfo *t);
int vwm_get_status(HWND hwnd); // returns:

#define VWM_KNOWN 1
#define VWM_MOVED 2
#define VWM_HIDDEN 4
#define VWM_ICONIC 8
#define VWM_STICKY 16


//=========================================================
// required variables/functions from elswhere:

// total number of desktops
extern int Settings_workspaces;

// current desktop
extern int currentScreen;

// does hwnd belong to a sticky application
extern bool check_sticky_name(HWND hwnd);

