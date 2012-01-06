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

#ifndef _BBWORKSPACES_H_
#define _BBWORKSPACES_H_

//===========================================================================
void Workspaces_Init(int nostartup);
void Workspaces_Exit(void);
void Workspaces_Reconfigure(void);
LRESULT Workspaces_Command(UINT msg, WPARAM wParam, LPARAM lParam);
void Workspaces_GatherWindows(void);
bool Workspaces_GetScreenMetrics(void);
void Workspaces_DeskSwitch(int);
void Workspaces_GetCaptions();
void CleanTasks(void);

void Workspaces_TaskProc(WPARAM, HWND);
void Workspaces_handletimer(void);

bool focus_top_window(void);
void SwitchToWindow(HWND hwnd);
void SwitchToBBWnd(void);
HWND GetActiveTaskWindow(void);
void ToggleWindowVisibility(HWND);

extern int nScreens;
extern int currentScreen;

//===========================================================================
#endif //ndef _BBWORKSPACES_H_
